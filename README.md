# Aula 6.2 – Point Light Shadows

Segunda aula do módulo de sombras. Expandimos o pipeline direcional da 6.1 para suportar **sombras omnidirecionais de luzes pontuais**, reaproveitando a mesma cena/materials e adicionando um passe extra que renderiza a profundidade em um cubemap com vertex, geometry e fragment shader dedicados.

## Conteúdo Abordado
- **Cubemap depth pass**: criação de um framebuffer dedicado e textura cube map com os shaders `depth_vertex/geometry/fragment`.
- **Shadow matrices para 6 faces**: geração das seis view-projection (lookAt) usando o geometry shader para renderizar todas as faces em um único draw.
- **Shadow comparison com bias + PCF esférico**: o fragment shader principal calcula a intensidade angular/distância e aplica PCF 3D sobre o cubemap.
- **Integração com luzes existentes**: o loop final combina luz direcional (com shadow map 2D legado) e os point lights agora com fator de sombra omnidirecional.

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
├── main.cpp                 # Configura janela, materiais e pipeline de shadow mapping direcional + point
├── camera.{h,cpp}           # Sistema de câmera FPS usado nos dois passes
├── texture.{h,cpp}          # Carregamento/gerenciamento de texturas (stb_image)
├── material.{h,cpp}         # Materiais reutilizados (personagem + chão)
├── light_manager.{h,cpp}    # Gerencia as luzes direcionais, pontuais e spots
├── model.{h,cpp}            # Carregamento Assimp + gerenciamento de meshes

assets/
├── models/
│   ├── scene.gltf / scene.bin        # Personagem principal
│   ├── cube.gltf / cube.bin          # Cubo usado como chão
│   ├── CubeTexture.jpg               # Textura aplicada ao chão
│   └── Vitalik_edit_2.png            # Textura usada pelo personagem
└── shaders/
    ├── vertex.glsl                       # MVP + dados para os dois tipos de sombra
    ├── fragment.glsl                     # Phong + sombras direcionais e omnidirecionais
    ├── directional_depth_vertex.glsl     # Depth-only do passe direcional
    ├── directional_depth_fragment.glsl   # Fragment shader vazio (depth only)
    ├── depth_vertex.glsl                 # Vertex do cubemap depth
    ├── depth_geometry.glsl               # Geometry com 6 shadow matrices
    └── depth_fragment.glsl               # Grava distância normalizada no cubemap
```

