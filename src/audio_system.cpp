// Implementação do sistema de áudio 3D usando MiniAudio
// Carrega configuração JSON, gerencia emissores espaciais e atualiza listener.

#include "audio_system.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

namespace
{
glm::vec3 ParseVec3(const nlohmann::json& node, const glm::vec3& fallback)
{
    if (!node.is_object())
    {
        return fallback;
    }
    glm::vec3 result = fallback;
    result.x = node.value("x", fallback.x);
    result.y = node.value("y", fallback.y);
    result.z = node.value("z", fallback.z);
    return result;
}

std::filesystem::path MakeAbsolute(const std::filesystem::path& path)
{
    if (path.is_absolute())
    {
        return path;
    }
    return std::filesystem::current_path() / path;
}
} // namespace

using json = nlohmann::json;

AudioSystem::AudioSystem() = default;

AudioSystem::~AudioSystem()
{
    Shutdown();
}

bool AudioSystem::Initialize(const AudioSystemConfig& config)
{
    if (m_initialized)
    {
        SetGlobalVolume(config.globalVolume);
        return true;
    }

    m_assetsRoot = config.assetsRoot;

    if (m_engine == nullptr)
    {
        m_engine = reinterpret_cast<ma_engine*>(m_engineStorage.data());
    }

    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.listenerCount = 1;

    if (ma_engine_init(&engineConfig, m_engine) != MA_SUCCESS)
    {
        std::cerr << "[AudioSystem] Falha ao inicializar o motor de áudio MiniAudio." << std::endl;
        return false;
    }

    m_initialized = true;
    SetGlobalVolume(config.globalVolume);

    const std::filesystem::path resolvedConfig = MakeAbsolute(config.configPath);
    if (!LoadConfiguration(resolvedConfig))
    {
        std::cerr << "[AudioSystem] Falha ao carregar '" << resolvedConfig.string() << "'." << std::endl;
        Shutdown();
        return false;
    }

    return true;
}

void AudioSystem::Shutdown()
{
    if (!m_initialized)
    {
        DestroyEmitters();
        return;
    }

    ma_engine_stop(m_engine);
    DestroyEmitters();
    ma_engine_uninit(m_engine);
    m_engine = nullptr;
    m_initialized = false;
}

void AudioSystem::UpdateListener(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up)
{
    if (!m_initialized)
    {
        return;
    }

    ma_engine_listener_set_position(m_engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(m_engine, 0, front.x, front.y, front.z);
    ma_engine_listener_set_world_up(m_engine, 0, up.x, up.y, up.z);
}

void AudioSystem::UpdateEmitterPosition(const std::string& id, const glm::vec3& position)
{
    if (!m_initialized)
    {
        return;
    }

    auto it = m_emitters.find(id);
    if (it == m_emitters.end() || it->second == nullptr)
    {
        return;
    }

    AudioEmitter* emitter = it->second.get();
    emitter->position = position;
    ma_sound_set_position(&emitter->sound, position.x, position.y, position.z);
}

void AudioSystem::PlayOneShot(const std::string& id)
{
    if (!m_initialized)
    {
        return;
    }

    auto it = m_emitters.find(id);
    if (it == m_emitters.end() || it->second == nullptr)
    {
        return;
    }

    AudioEmitter* emitter = it->second.get();
    ma_sound_stop(&emitter->sound);
    ma_sound_seek_to_pcm_frame(&emitter->sound, 0);
    ma_sound_start(&emitter->sound);
}

void AudioSystem::SetGlobalVolume(float volume)
{
    m_globalVolume = std::clamp(volume, 0.0f, 1.0f);

    if (!m_initialized)
    {
        return;
    }

    for (auto& [_, emitter] : m_emitters)
    {
        if (emitter != nullptr)
        {
            ma_sound_set_volume(&emitter->sound, emitter->baseVolume * m_globalVolume);
        }
    }
}

bool AudioSystem::LoadConfiguration(const std::filesystem::path& configPath)
{
    DestroyEmitters();

    std::ifstream stream(configPath);
    if (!stream.is_open())
    {
        std::cerr << "[AudioSystem] Não foi possível abrir '" << configPath.string() << "'." << std::endl;
        return false;
    }

    json document;
    try
    {
        stream >> document;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[AudioSystem] Erro ao ler JSON de áudio: " << ex.what() << std::endl;
        return false;
    }

    if (document.contains("globalVolume"))
    {
        SetGlobalVolume(document.value("globalVolume", m_globalVolume));
    }

    const auto soundsIt = document.find("sounds");
    if (soundsIt == document.end() || !soundsIt->is_array())
    {
        std::cerr << "[AudioSystem] Estrutura JSON inválida: array 'sounds' inexistente." << std::endl;
        return false;
    }

    bool success = true;
    for (const auto& node : *soundsIt)
    {
        const std::string id = node.value("id", "");
        const std::string file = node.value("file", "");
        if (id.empty() || file.empty())
        {
            std::cerr << "[AudioSystem] Som sem 'id' ou 'file' ignorado." << std::endl;
            success = false;
            continue;
        }

        const glm::vec3 position = ParseVec3(node.value("position", json::object()), glm::vec3(0.0f));
        const bool loop = node.value("loop", false);
        const bool spatial = node.value("spatial", true);
        const bool playOnStart = node.value("playOnStart", false);
        const float baseVolume = node.value("volume", 1.0f);
        const float minDistance = node.value("minDistance", 1.0f);
        const float maxDistance = node.value("maxDistance", 25.0f);

        if (!CreateEmitter(id, file, position, loop, spatial, playOnStart, baseVolume, minDistance, maxDistance))
        {
            success = false;
        }
    }

    return success;
}

bool AudioSystem::CreateEmitter(const std::string& id,
                                const std::filesystem::path& relativePath,
                                const glm::vec3& position,
                                bool loop,
                                bool spatial,
                                bool playOnStart,
                                float baseVolume,
                                float minDistance,
                                float maxDistance)
{
    if (!m_initialized)
    {
        return false;
    }

    auto [it, inserted] = m_emitters.try_emplace(id, std::make_unique<AudioEmitter>());
    if (!inserted && it->second != nullptr)
    {
        std::cerr << "[AudioSystem] Já existe um emissor com id '" << id << "'." << std::endl;
        return false;
    }
    if (!inserted)
    {
        it->second = std::make_unique<AudioEmitter>();
    }

    AudioEmitter* emitter = it->second.get();
    emitter->position = position;
    emitter->baseVolume = baseVolume;
    emitter->minDistance = std::max(0.01f, minDistance);
    emitter->maxDistance = std::max(emitter->minDistance, maxDistance);
    emitter->spatial = spatial;

    const std::filesystem::path resolved = ResolveAssetPath(relativePath);
    const std::string filePath = resolved.string();
    const ma_uint32 flags = loop ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;

    if (ma_sound_init_from_file(m_engine, filePath.c_str(), flags, nullptr, nullptr, &emitter->sound) != MA_SUCCESS)
    {
        std::cerr << "[AudioSystem] Falha ao carregar som '" << filePath << "'." << std::endl;
        m_emitters.erase(it);
        return false;
    }

    ma_sound_set_spatialization_enabled(&emitter->sound, spatial ? MA_TRUE : MA_FALSE);
    ma_sound_set_position(&emitter->sound, position.x, position.y, position.z);
    ma_sound_set_min_gain(&emitter->sound, 0.0f);
    ma_sound_set_max_gain(&emitter->sound, 1.0f);
    ma_sound_set_min_distance(&emitter->sound, emitter->minDistance);
    ma_sound_set_max_distance(&emitter->sound, emitter->maxDistance);
    ma_sound_set_attenuation_model(&emitter->sound,
                                   spatial ? ma_attenuation_model_inverse : ma_attenuation_model_none);
    ma_sound_set_volume(&emitter->sound, emitter->baseVolume * m_globalVolume);
    ma_sound_set_looping(&emitter->sound, loop ? MA_TRUE : MA_FALSE);

    if (playOnStart)
    {
        ma_sound_start(&emitter->sound);
    }
    return true;
}

void AudioSystem::DestroyEmitters()
{
    for (auto& [_, emitter] : m_emitters)
    {
        if (emitter != nullptr)
        {
            ma_sound_stop(&emitter->sound);
            ma_sound_uninit(&emitter->sound);
        }
    }
    m_emitters.clear();
}

std::filesystem::path AudioSystem::ResolveAssetPath(const std::filesystem::path& relativePath) const
{
    if (relativePath.is_absolute())
    {
        return relativePath;
    }

    std::filesystem::path root = m_assetsRoot;
    if (!root.empty() && root.is_relative())
    {
        root = std::filesystem::current_path() / root;
    }
    if (root.empty())
    {
        root = std::filesystem::current_path();
    }
    return root / relativePath;
}


