# Aula 5.3 – Spot Lights

Terceira etapa do módulo avançado. Além das luzes direcionais e pontuais já existentes, introduzimos **luzes cônicas** com controle total de cone (inner/outer angles), falloff angular suave e alcance limitado. Também adicionamos uma lanterna ligada à câmera para demonstrar spot lights dinâmicas.

## Conteúdo Abordado
- **SpotLightManager dedicado**: gerencia arrays de spots com posições, direções, cones e coeficientes de atenuação.
- **Cone-based lighting**: shader calcula a intensidade angular com base nos cortes interno/externo para evitar transições bruscas.
- **Falloff composto**: combinação do falloff angular com o mesmo modelo físico de distância (constant/linear/quadratic + range).
- **Lanterna em primeira pessoa**: um dos spots segue posição/direção da câmera a cada frame, simulando uma flashlight.

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
├── main.cpp                 # Configura janela, materiais e sincroniza as luzes
├── camera.{h,cpp}           # Sistema de câmera FPS (base para a lanterna)
├── texture.{h,cpp}          # Carregamento/gerenciamento de texturas (stb_image)
├── material.{h,cpp}         # Materiais reaproveitados
├── light_manager.{h,cpp}    # Directional, Point e Spot light managers
├── model.{h,cpp}            # Carregamento Assimp + gerenciamento de meshes

assets/
├── models/
│   ├── scene.gltf / scene.bin        # Personagem principal
│   ├── cube.gltf / cube.bin          # Cubo usado como chão
│   ├── CubeTexture.jpg               # Textura aplicada ao chão
│   └── Vitalik_edit_2.png            # Textura usada pelo personagem
└── shaders/
    ├── vertex.glsl   # Normal matrix correta
    └── fragment.glsl # Phong com direcionais + pontuais + spots (cone + falloff angular)
```

