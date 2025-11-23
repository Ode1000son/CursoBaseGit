# Aula 5.2 – Point Lights

Sequência direta da etapa de luzes direcionais. Mantivemos toda a base de materiais/cena e adicionamos um sistema completo de **luzes pontuais com atenuação, falloff por distância e limite de alcance configurável**. Cada luz possui posição, cores e coeficientes (constant/linear/quadratic + range) que definem o comportamento da intensidade.

## Conteúdo Abordado
- **Estruturas paralelas de iluminação**: `DirectionalLightManager` continua ativo e o novo `PointLightManager` sincroniza posições e cores das luzes pontuais.
- **Modelo de atenuação física**: shader usa o trio constant/linear/quadratic combinado com o fator de range para cortar a luz fora do raio útil.
- **Funções modulares**: `CalculateDirectionalLight` e `CalculatePointLight` permitem combinar múltiplos tipos de luz sem duplicar lógica.
- **Cena reutilizada**: mesmos materiais, câmera e modelos da aula anterior garantem foco exclusivo no comportamento das luzes.

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
├── main.cpp                 # Configuração do contexto, upload das luzes e render loop
├── camera.{h,cpp}           # Sistema de câmera FPS
├── texture.{h,cpp}          # Carregamento/gerenciamento de texturas (stb_image)
├── material.{h,cpp}         # Classe Material com cores/shine e binding de textura
├── light_manager.{h,cpp}    # Gerencia arrays de luzes direcionais e pontuais
├── model.{h,cpp}            # Carregamento Assimp + gerenciamento de meshes

assets/
├── models/
│   ├── scene.gltf / scene.bin        # Personagem principal
│   ├── cube.gltf / cube.bin          # Cubo usado como chão
│   ├── CubeTexture.jpg               # Textura aplicada ao chão
│   └── Vitalik_edit_2.png            # Textura usada pelo personagem
└── shaders/
    ├── vertex.glsl   # Normal matrix correta
    └── fragment.glsl # Phong com direcionais + pontuais e falloff por range
```

