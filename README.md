# Aula 3.1 – Carregamento de Texturas

Quarta entrega prática do curso. Demonstra o carregamento e aplicação de texturas usando stb_image e samplers OpenGL.

## Conteúdo Abordado
- **Classe Texture**: Sistema completo de carregamento de texturas
- **stb_image**: Biblioteca para carregamento de imagens (PNG, JPG, etc.)
- **Coordenadas UV**: Mapeamento de texturas nos vértices
- **Sampler Uniforms**: Passagem de texturas para shaders
- **Atributos de Vértice**: Posição + Cor + UV coordinates

## Controles
- **WASD**: Movimentação horizontal (Forward/Back/Left/Right)
- **QE**: Movimentação vertical (Up/Down)
- **Botão Direito do Mouse + Movimento**: Rotação da câmera (look around)
- **ESC**: Sair da aplicação

## Como executar
```bash
build.bat    # gera solução e compila
run.bat      # executa a aplicação
```

## Arquivos da Implementação
```
src/
├── main.cpp       # Loop principal e configuração
├── camera.h       # Interface da classe Camera
├── camera.cpp     # Implementação da câmera
├── texture.h      # Interface da classe Texture
└── texture.cpp    # Implementação do carregamento de texturas

assets/
├── texture.png    # Textura aplicada ao triângulo
└── shaders/
    ├── vertex.glsl     # Shader com suporte a UV coordinates
    └── fragment.glsl   # Shader com sampler de textura
```

## Formato dos Dados do Vértice
```
Posição (x,y,z) + Cor (r,g,b) + UV (u,v) = 8 floats por vértice
```

Scripts reutilizam dependências do projeto `ultraMini`.

## Nota
O triângulo permanece estático para focar nos controles da câmera. A rotação foi comentada no código para demonstração pura do sistema de navegação 3D.