# Aula 7.2 – Cena Completa

Segunda aula do módulo de carregamento avançado. Evoluímos a base da Aula 7.1 para construir uma **cena completa** com múltiplos modelos, cada um com transformações próprias e sistema de gerenciamento de cena plug‑and‑play.

## Objetivos
- **Gerenciamento de cena**: Implementar sistema de organização e renderização de múltiplos objetos 3D

## Conteúdo Abordado
- **Múltiplos modelos**: Personagem (`scene.gltf`), carro (`car.glb`) e chão (`cube.gltf`) em uma mesma cena
- **Transformações individuais**: Cada objeto possui posição, rotação e escala independentes
- **Scene management**: Classes `Scene`, `SceneObject` e `SceneObjectTransform` para gerenciar a cena
- **Sistema de animação**: Animações suaves para personagem e carro demonstrando o sistema de transformações
- **Texture override system**: Atalhos `1/2/3` para depuração rápida das texturas com overrides globais
- **Integração com iluminação avançada**: Pipeline completo de sombras direcionais e omnidirecionais

## Controles
- `W A S D`: movimentação no plano.
- `Q / E`: movimento vertical.
- `Botão direito + mouse`: look-around (cursor capturado enquanto pressionado).
- `1`: restaura apenas as texturas importadas dos materiais.
- `2`: força a textura checker globalmente.
- `3`: aplica a textura de destaque metálica.
- `ESC`: encerra a aplicação.

## Como executar
```bash
build.bat    # gera a solução e compila (Premake + MSBuild)
run.bat      # executa o binário gerado em build/bin/Debug
```

## Estrutura Principal
```
src/
├── main.cpp                 # Loop principal + scene management + sombras
├── camera.{h,cpp}           # Câmera FPS usada nos três passes
├── texture.{h,cpp}          # Carregamento/gerenciamento de texturas (stb_image)
├── material.{h,cpp}         # Materiais Phong com suporte a overrides
├── light_manager.{h,cpp}    # Gerencia luzes direcionais e pontuais
├── model.{h,cpp}            # Importador Assimp + materiais por mesh + overrides

assets/
├── models/
│   ├── scene.gltf / scene.bin        # Personagem principal
│   ├── cube.gltf / cube.bin          # Chão/base da cena
│   ├── car.glb                       # Novo veículo utilizado na aula
│   ├── CubeTexture.jpg               # Textura aplicada ao chão e como fallback
│   └── Vitalik_edit_2.png            # Textura original do personagem
└── shaders/
    ├── vertex.glsl
    ├── fragment.glsl
    ├── directional_depth_vertex.glsl
    ├── directional_depth_fragment.glsl
    ├── depth_vertex.glsl
    ├── depth_geometry.glsl
    └── depth_fragment.glsl
```

