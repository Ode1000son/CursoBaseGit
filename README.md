# Aula 8.3 – Separação de Cena e Entry Point

Terceira aula do módulo de renderização avançada. Evoluímos a arquitetura modular da Aula 8.2 criando uma camada explícita de **Scene management** e transformando o `main.cpp` em um entry point puro que apenas inicializa o ambiente, direciona a entrada e chama o renderer. Agora o código segue ainda mais de perto os princípios da engine **ultraMini**: cada subsistema conhece somente a sua responsabilidade e expõe interfaces plug-and-play.

## Objetivos
- **Scene class dedicada**: toda a lógica de carregamento de modelos, criação dos objetos da cena e animações vive em `scene.h/.cpp`.
- **Renderer desacoplado**: `Renderer::Initialize(Scene*)` recebe uma cena já preparada e foca exclusivamente em recursos de GPU, pós-processamento e iluminação.
- **Main enxuto**: `main.cpp` apenas configura GLFW/GLAD, cria `Scene`, `Renderer` e executa o loop chamando `RenderFrame`.
- **Encapsulamento total**: overrides de textura, animações, modelos e luzes ficam escondidos atrás de APIs simples, permitindo trocar a aplicação sem mexer na engine.

## Conteúdo Abordado
- **`scene.{h,cpp}`**: define `SceneObject`, `SceneObjectTransform` e `Scene`, cuidando de:
  - Carregar modelos e texturas.
  - Construir o scene graph (chão, personagem, carro).
  - Atualizar animações de cada objeto em `Scene::Update`.
  - Expor apenas ponteiros para os modelos necessários ao renderer.
- **`renderer.{h,cpp}`**: passou a depender apenas da interface da cena. Toda a montagem de framebuffers, MRT e passes de sombra permanece igual, mas agora recebe o estado da cena por parâmetro.
- **`main.cpp` refatorado**: virou entry point puro, responsável por:
  - Inicializar GLFW/GLAD.
  - Criar `Scene` + `Renderer`.
  - Encaminhar entrada (teclado/mouse) e atalhos de override para o renderer.

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
├── main.cpp                   # Entry point puro + roteamento de entrada
├── scene.{h,cpp}              # Scene graph, assets e animações
├── renderer.{h,cpp}           # Renderer plug-and-play (sombras + MRT + pós)
├── camera.{h,cpp}
├── light_manager.{h,cpp}
├── material.{h,cpp}
├── model.{h,cpp}
└── texture.{h,cpp}

assets/
├── models/scene.gltf, car.glb, cube.gltf, etc.
└── shaders/
    ├── vertex/fragment.glsl                # Passo principal (MRT)
    ├── postprocess_vertex/fragment.glsl    # Tone mapping + bloom
    ├── depth_vertex/fragment/geometry.glsl # Sombras omnidirecionais
    ├── directional_depth_vertex/fragment.glsl
```
