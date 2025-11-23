# Aula 6.1 – Shadow Mapping Direcional

Primeira aula do módulo de sombras. Partimos da cena completa com materiais e luzes direcionais e implementamos o pipeline clássico de **shadow mapping direcional**: passamos a gerar um depth map dedicado, calculamos a light space matrix e integramos comparação com bias + PCF no shader principal.

## Conteúdo Abordado
- **Depth-only rendering**: novo shader (`directional_depth_vertex/fragment`) e framebuffer configurado com textura de profundidade 2D.
- **Light space matrix**: construção de projeção ortográfica + view da luz para cobrir a cena inteira controlando range/near/far.
- **Shadow comparison + PCF**: shader principal usa o depth map para descartar fragmentos em sombra com bias dinâmico e filtro 3x3.
- **Integração modular**: o render loop executa dois passes (shadow + forward) reaproveitando materiais, modelos e gerenciador de luzes.

## Controles
- `W A S D`: movimentação no plano.
- `Q / E`: movimento vertical.
- `Botão direito + mouse`: look-around (cursor é capturado enquanto pressionado).
- `ESC`: encerra a aplicação.

## Como executar
```bash
build.bat    # gera a solução e compila (Premake + MSBuild)
run.bat      # executa o binário gerado em build/bin/Debug
```

## Estrutura Principal
```
src/
├── main.cpp                 # Configura janela, materiais e pipeline de shadow mapping
├── camera.{h,cpp}           # Sistema de câmera FPS usado nos dois passes
├── texture.{h,cpp}          # Carregamento/gerenciamento de texturas (stb_image)
├── material.{h,cpp}         # Materiais reutilizados (personagem + chão)
├── light_manager.{h,cpp}    # Gerencia as luzes direcionais
├── model.{h,cpp}            # Carregamento Assimp + gerenciamento de meshes

assets/
├── models/
│   ├── scene.gltf / scene.bin        # Personagem principal
│   ├── cube.gltf / cube.bin          # Cubo usado como chão
│   ├── CubeTexture.jpg               # Textura aplicada ao chão
│   └── Vitalik_edit_2.png            # Textura usada pelo personagem
└── shaders/
    ├── vertex.glsl                     # MVP + lightSpaceMatrix
    ├── fragment.glsl                   # Phong + cálculo de sombra direcional (bias + PCF)
    ├── directional_depth_vertex.glsl   # Depth-only para o passe da luz
    └── directional_depth_fragment.glsl # Fragment shader vazio (depth only)
```

