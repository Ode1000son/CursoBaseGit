#pragma once

#include <glad/glad.h>
#include <string>

/// @brief Classe para gerenciamento de texturas OpenGL
/// Esta classe encapsula o carregamento e gerenciamento de texturas usando stb_image
class Texture {
public:
    /// @brief Construtor padrão
    Texture();

    /// @brief Destrutor - libera recursos OpenGL
    ~Texture();

    /// @brief Carrega uma textura de arquivo
    /// @param filePath Caminho para o arquivo de imagem
    /// @return true se carregou com sucesso, false caso contrário
    bool LoadFromFile(const std::string& filePath);

    /// @brief Vincula a textura para uso no shader
    /// @param textureUnit Unidade de textura (GL_TEXTURE0, GL_TEXTURE1, etc.)
    void Bind(GLenum textureUnit = GL_TEXTURE0) const;

    /// @brief Desvincula a textura
    void Unbind() const;

    /// @brief Define os parâmetros de wrapping da textura
    /// @param s Modo de wrapping no eixo S (GL_REPEAT, GL_CLAMP_TO_EDGE, etc.)
    /// @param t Modo de wrapping no eixo T (GL_REPEAT, GL_CLAMP_TO_EDGE, etc.)
    void SetWrapping(GLenum s, GLenum t);

    /// @brief Define os parâmetros de filtragem da textura
    /// @param minFilter Filtro para minificação (GL_NEAREST, GL_LINEAR, etc.)
    /// @param magFilter Filtro para magnificação (GL_NEAREST, GL_LINEAR)
    void SetFiltering(GLenum minFilter, GLenum magFilter);

    /// @brief Obtém o ID OpenGL da textura
    /// @return ID da textura
    GLuint GetID() const { return m_textureID; }

    /// @brief Obtém a largura da textura
    /// @return Largura em pixels
    int GetWidth() const { return m_width; }

    /// @brief Obtém a altura da textura
    /// @return Altura em pixels
    int GetHeight() const { return m_height; }

private:
    GLuint m_textureID;  ///< ID OpenGL da textura
    int m_width;         ///< Largura da textura em pixels
    int m_height;        ///< Altura da textura em pixels
    int m_channels;      ///< Número de canais de cor (RGB=3, RGBA=4)

    /// @brief Libera a textura se ela foi carregada
    void Cleanup();
};
