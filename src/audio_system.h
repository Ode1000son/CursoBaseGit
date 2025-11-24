#pragma once

// Sistema de áudio 3D baseado em MiniAudio
// Gerencia emissores espaciais, listener (câmera), volume global e carregamento
// de configuração via JSON. Suporta sons posicionais com atenuação por distância.

#include "miniaudio_config.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>
#include <miniaudio.h>

// Configuração inicial do sistema de áudio
struct AudioSystemConfig
{
    std::filesystem::path assetsRoot = "assets";
    std::filesystem::path configPath = "assets/scenes/audio_config.json";
    float globalVolume = 0.8f;
};

// Sistema de áudio 3D encapsulando o motor MiniAudio
// Gerencia múltiplos emissores espaciais, listener único e volume global.
// Carrega configuração de sons via JSON e permite atualização dinâmica de posições.
class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    AudioSystem(AudioSystem&&) = delete;
    AudioSystem& operator=(AudioSystem&&) = delete;

    // Inicializa o motor MiniAudio e carrega configuração de sons do JSON
    bool Initialize(const AudioSystemConfig& config);
    // Para todos os sons e libera recursos do MiniAudio
    void Shutdown();

    bool IsInitialized() const { return m_initialized; }

    // Atualiza posição e orientação do listener (normalmente a câmera)
    void UpdateListener(const glm::vec3& position, const glm::vec3& front, const glm::vec3& up);
    // Atualiza posição de um emissor espacial por ID
    void UpdateEmitterPosition(const std::string& id, const glm::vec3& position);
    // Reproduz um som one-shot (reinicia do início)
    void PlayOneShot(const std::string& id);

    // Define volume global (0.0 a 1.0) aplicado a todos os emissores
    void SetGlobalVolume(float volume);
    float GetGlobalVolume() const { return m_globalVolume; }

private:
    // Representa um emissor de áudio 3D com configuração espacial
    struct AudioEmitter
    {
        ma_sound sound{};
        glm::vec3 position{ 0.0f };
        float baseVolume = 1.0f;
        float minDistance = 1.0f;  // Distância mínima para atenuação
        float maxDistance = 25.0f; // Distância máxima (volume = 0 além disso)
        bool spatial = true;       // Se usa posicionamento 3D
    };

    // Carrega configuração de sons do arquivo JSON
    bool LoadConfiguration(const std::filesystem::path& configPath);
    // Cria um novo emissor de áudio com as propriedades especificadas
    bool CreateEmitter(const std::string& id,
                       const std::filesystem::path& relativePath,
                       const glm::vec3& position,
                       bool loop,
                       bool spatial,
                       bool playOnStart,
                       float baseVolume,
                       float minDistance,
                       float maxDistance);
    // Destrói todos os emissores e libera recursos
    void DestroyEmitters();
    // Resolve caminho relativo de asset para caminho absoluto
    std::filesystem::path ResolveAssetPath(const std::filesystem::path& relativePath) const;

    ma_engine* GetEngine() { return m_engine; }
    const ma_engine* GetEngine() const { return m_engine; }

    using EngineStorage = std::array<std::byte, sizeof(ma_engine)>;
    alignas(ma_engine) EngineStorage m_engineStorage{};
    ma_engine* m_engine = nullptr;
    bool m_initialized = false;
    float m_globalVolume = 1.0f;
    std::filesystem::path m_assetsRoot;
    std::unordered_map<std::string, std::unique_ptr<AudioEmitter>> m_emitters;
};


