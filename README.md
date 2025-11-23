# Aula 7.1 – Materiais e Texturas

Primeira aula do módulo de carregamento avançado. Mantemos o pipeline completo de iluminação/sombras da aula anterior, mas focamos em **materiais importados via Assimp**, **múltiplas texturas por modelo** e um **texture override system** para depuração em tempo real.

## Conteúdo Abordado
- **Carregamento de materiais**: cada `aiMaterial` vira um `Material` completo (ambient/diffuse/specular/shininess) enviado antes de cada draw call.
- **Múltiplas texturas por mesh**: o `Model` armazena um material por mesh, carrega difusa individual e usa fallback branco 1x1 quando preciso.
- **Texture override system**: atalhos `1/2/3` limpam ou aplicam texturas globais (checker/metálica) usando `Model::OverrideAllTextures`.
- **Integração com sombras**: o pipeline de shadow mapping direcional + omnidirecional permanece igual, garantindo testes em cenários reais.

## Controles
- `W A S D`: movimentação no plano.
- `Q / E`: movimento vertical.
- `Botão direito + mouse`: look-around (cursor capturado enquanto pressionado).
- `1`: usa apenas as texturas importadas dos materiais.
- `2`: força a textura checker em todos os meshes.
- `3`: aplica a textura metálica/destaque globalmente.
- `ESC`: encerra a aplicação.

## Como executar
```bash
build.bat    # gera a solução e compila (Premake + MSBuild)
run.bat      # executa o binário gerado em build/bin/Debug
```

## Estrutura Principal
```
src/
├── main.cpp                 # Pipeline de sombras + sistemas de materiais/override
├── camera.{h,cpp}           # Câmera FPS usada nos dois passes
├── texture.{h,cpp}          # Carregamento/gerenciamento de texturas (stb_image)
├── material.{h,cpp}         # Materiais Phong com suporte a overrides
├── light_manager.{h,cpp}    # Gerencia as luzes direcionais, pontuais e spots
├── model.{h,cpp}            # Importador Assimp + materiais por mesh + overrides

assets/
├── models/
│   ├── scene.gltf / scene.bin        # Personagem principal
│   ├── cube.gltf / cube.bin          # Cubo usado como chão
│   ├── CubeTexture.jpg               # Textura aplicada ao chão e aos overrides
│   └── Vitalik_edit_2.png            # Textura original do personagem
└── shaders/
    ├── vertex.glsl                       # MVP + dados para shadow mapping
    ├── fragment.glsl                     # Phong + sombras direcional/omni + materiais
    ├── directional_depth_vertex.glsl     # Passe de profundidade direcional
    ├── directional_depth_fragment.glsl   # Fragment shader (depth only)
    ├── depth_vertex.glsl                 # Vertex para depth cubemap
    ├── depth_geometry.glsl               # Geometry com 6 matrizes lookAt
    └── depth_fragment.glsl               # Normaliza a distância no cubemap
```

