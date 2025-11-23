# Aula 5.1 – Directional Lights

Primeira aula do módulo de iluminação avançada. Mantivemos a base da Aula 4.2 (materiais e texturas), mas agora passamos a controlar **múltiplas luzes direcionais** via arrays de uniforms. Cada luz possui direção, cores (ambient/diffuse/specular) e comportamento próprio (algumas animadas) e todas contribuem para o resultado final no fragment shader.

## Conteúdo Abordado
- **Arrays de Luzes Direcionais**: Struct `DirectionalLight` no shader e upload dinâmico no C++ (até 4 luzes).
- **Gerenciador de Luzes**: Classe `DirectionalLightManager` centraliza configuração/animação das luzes e sincroniza com o shader.
- **Shader Modular**: Função `CalculateDirectionalLight` soma a contribuição de cada luz.
- **Materiais Reutilizáveis**: Classe `Material` continua encapsulando parâmetros + texturas (personagem x chão).
- **Câmera FPS + Cena Completa**: Mesmo setup de navegação/Assimp, agora com iluminação multi-fonte.

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
├── main.cpp                 # Configuração do contexto, shaders e render loop (modelo 3D)
├── camera.{h,cpp}           # Sistema de câmera FPS
├── texture.{h,cpp}          # Carregamento e gerenciamento de texturas (stb_image)
├── material.{h,cpp}         # Classe Material com cores/shine e binding de textura
├── directional_light_manager.{h,cpp} # Gerenciamento e upload das luzes direcionais
├── model.{h,cpp}            # Carregamento Assimp + gerenciamento de meshes

assets/
├── models/
│   ├── scene.gltf          # Modelo glTF importado
│   ├── scene.bin           # Buffers do glTF
│   ├── cube.gltf / cube.bin          # Cubo usado como chão
│   ├── CubeTexture.jpg               # Textura aplicada ao chão (recebe cor adicional do material)
│   └── Vitalik_edit_2.png            # Textura usada pelo personagem
└── shaders/
    ├── vertex.glsl   # Atributos completos + normal matrix normalizada
    └── fragment.glsl # Implementação do modelo de iluminação Phong com materiais
```

