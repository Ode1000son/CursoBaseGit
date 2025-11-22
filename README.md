# Aula 2.1 – Matrizes de Transformação

Primeira aula do Módulo 2. Demonstra o uso das matrizes MVP (Model, View, Projection) para transformar objetos 3D usando GLM.

## Conteúdo Abordado
- **Matriz Model**: Transforma objeto do espaço local para o mundo
- **Matriz View**: Transforma do mundo para o espaço da câmera
- **Matriz Projection**: Projeta para coordenadas de recorte (perspectiva)
- **GLM**: Biblioteca matemática para operações com matrizes e vetores
- **Uniforms**: Passagem de dados para shaders via variáveis uniform

## Como executar
```bash
build.bat    # gera solução e compila
run.bat      # executa a aplicação
```

## Transformações Demonstradas
- **Rotação**: Triângulo gira ao redor do eixo Z
- **Perspectiva**: Projeção 3D com campo de visão de 45 graus
- **MVP Pipeline**: Sequência completa de transformações 3D

## Estrutura dos Shaders
```
assets/shaders/
├── vertex.glsl     # Aplica transformações MVP + processa atributos
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


