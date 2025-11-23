#include "texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <vector>
#include <cstring>

/// @brief Construtor padrão - inicializa membros
Texture::Texture()
    : m_textureID(0)
    , m_width(0)
    , m_height(0)
    , m_channels(0)
{
}

/// @brief Destrutor - libera recursos OpenGL
Texture::~Texture()
{
    Cleanup();
}

/// @brief Carrega uma textura de arquivo usando stb_image
/// @param filePath Caminho para o arquivo de imagem (PNG, JPG, etc.)
/// @return true se carregou com sucesso, false caso contrário
bool Texture::LoadFromFile(const std::string& filePath)
{
    // Libera textura anterior se existir
    Cleanup();

    // Configura stb_image para carregar com orientação vertical invertida (convenção OpenGL)
    stbi_set_flip_vertically_on_load(true);

    // Carrega a imagem usando stb_image
    unsigned char* imageData = stbi_load(filePath.c_str(), &m_width, &m_height, &m_channels, 0);

    if (!imageData) {
        std::cerr << "Erro ao carregar textura: " << filePath << std::endl;
        std::cerr << "stb_image error: " << stbi_failure_reason() << std::endl;
        return false;
    }

    bool result = UploadToGPU(imageData, m_width, m_height, m_channels);

    stbi_image_free(imageData);

    if (result) {
        std::cout << "Textura carregada com sucesso: " << filePath << std::endl;
        std::cout << "  Dimensões: " << m_width << "x" << m_height << std::endl;
        std::cout << "  Canais: " << m_channels << std::endl;
    }

    return result;
}

bool Texture::LoadFromMemory(const unsigned char* data, std::size_t size, bool flipVertically)
{
    if (!data || size == 0) {
        return false;
    }

    Cleanup();
    stbi_set_flip_vertically_on_load(flipVertically);

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* imageData = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 0);
    if (!imageData) {
        std::cerr << "Erro ao carregar textura da memória: " << stbi_failure_reason() << std::endl;
        return false;
    }

    bool result = UploadToGPU(imageData, width, height, channels);
    stbi_image_free(imageData);
    return result;
}

bool Texture::LoadFromRawData(const unsigned char* data, int width, int height, int channels, bool flipVertically)
{
    if (!data || width <= 0 || height <= 0 || channels <= 0) {
        return false;
    }

    Cleanup();

    const std::size_t rowSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
    const std::size_t totalSize = rowSize * static_cast<std::size_t>(height);
    const unsigned char* sourceData = data;
    std::vector<unsigned char> flipped;
    if (flipVertically) {
        flipped.resize(totalSize);
        for (int y = 0; y < height; ++y) {
            const unsigned char* srcRow = data + (static_cast<std::size_t>(height - 1 - y) * rowSize);
            unsigned char* dstRow = flipped.data() + static_cast<std::size_t>(y) * rowSize;
            std::memcpy(dstRow, srcRow, rowSize);
        }
        sourceData = flipped.data();
    }

    return UploadToGPU(sourceData, width, height, channels);
}

/// @brief Vincula a textura para uso no shader
/// @param textureUnit Unidade de textura (GL_TEXTURE0, GL_TEXTURE1, etc.)
void Texture::Bind(GLenum textureUnit) const
{
    if (m_textureID == 0) {
        std::cerr << "Erro: Tentativa de vincular textura não carregada" << std::endl;
        return;
    }

    // Ativa a unidade de textura
    glActiveTexture(textureUnit);

    // Vincula a textura
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

/// @brief Desvincula a textura
void Texture::Unbind() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

/// @brief Define os parâmetros de wrapping (repetição) da textura
/// @param s Modo de wrapping no eixo S (horizontal)
/// @param t Modo de wrapping no eixo T (vertical)
void Texture::SetWrapping(GLenum s, GLenum t)
{
    if (m_textureID == 0) {
        std::cerr << "Erro: Tentativa de configurar wrapping sem textura válida" << std::endl;
        return;
    }

    // Assume que a textura já está vinculada antes da chamada
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t);
}

/// @brief Define os parâmetros de filtragem da textura
/// @param minFilter Filtro usado quando a textura é minificada (objeto pequeno)
/// @param magFilter Filtro usado quando a textura é magnificada (objeto grande)
void Texture::SetFiltering(GLenum minFilter, GLenum magFilter)
{
    if (m_textureID == 0) {
        std::cerr << "Erro: Tentativa de configurar filtragem sem textura válida" << std::endl;
        return;
    }

    // Assume que a textura já está vinculada antes da chamada
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

/// @brief Libera a textura OpenGL se ela foi carregada
void Texture::Cleanup()
{
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
        m_width = 0;
        m_height = 0;
        m_channels = 0;
    }
}

bool Texture::UploadToGPU(const unsigned char* data, int width, int height, int channels)
{
    if (!data) {
        return false;
    }

    GLenum format;
    switch (channels) {
        case 1: format = GL_RED; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default:
            std::cerr << "Formato de textura não suportado: " << channels << " canais" << std::endl;
            return false;
    }

    m_width = width;
    m_height = height;
    m_channels = channels;

    glGenTextures(1, &m_textureID);
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    SetWrapping(GL_REPEAT, GL_REPEAT);
    SetFiltering(GL_LINEAR, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}
