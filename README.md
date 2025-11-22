# Aula 1.2 – Primeiro Triângulo

Segunda entrega prática do curso. Demonstra a renderização de geometria básica usando VAO, VBO, shaders GLSL e o pipeline de renderização OpenGL.

## Conteúdo Abordado
- **VAO (Vertex Array Object)**: Organiza atributos de vértice
- **VBO (Vertex Buffer Object)**: Armazena dados de geometria na GPU
- **Shaders GLSL**: Vertex shader e fragment shader básicos
- **Pipeline de Renderização**: Do vértice ao fragmento

## Como executar
```bash
build.bat    # gera solução e compila
run.bat      # executa a aplicação
```

## Estrutura dos Shaders
```
assets/shaders/
├── vertex.glsl     # Processa vértices e atributos
└── fragment.glsl   # Define cor final dos pixels
```

Scripts reutilizam dependências do projeto `ultraMini`.

## Estrutura
```
Aula 1.1/
├── build.bat
├── premake5.lua
├── run.bat
└── src/
    └── main.cpp
```

## Documentação complementar
- `TUTORIAL_Aula1.2.md`: guia completo com todos os arquivos (código, shaders e scripts).
- `ROTEIRO_VIDEO_Aula1.2.md`: roteiro sugerido para a gravação da aula em vídeo.
- `../DIFF_Aulas1.1-1.2.md`: resumo das diferenças entre as duas entregas.


