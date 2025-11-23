# Aula 8.2 – Sistema de Renderização Modular

Segunda aula do módulo de renderização avançada. Partimos da base da Aula 8.1, mas reorganizamos toda a render pipeline dentro de um **renderer modular** inspirado diretamente na arquitetura da engine **ultraMini**. O foco aqui é mostrar como encapsular inicialização, ciclo de renderização e desligamento em uma única classe plug-and-play (`renderer.cpp`), mantendo o restante da aplicação (entrada, câmera, janela) totalmente desacoplado.

## Objetivos
- **Arquitetura de renderer dedicada**: expor claramente responsabilidades e ciclos de vida.
- **Inicialização/Shutdown determinísticos**: criação e destruição de FBOs, shaders e assets em um único ponto.
- **Scene rendering pipeline**: passes de sombras, renderização off-screen com MRT e pós-processamento organizados como etapas privadas do renderer.
- **Integração com controles**: teclado/mouse ficam no `main.cpp`, que apenas injeta eventos e aciona `Renderer::RenderFrame`.

## Conteúdo Abordado
- **Classe `Renderer`** (`src/renderer.{h,cpp}`): mantém estado completo (modelos, luzes, framebuffers, shaders), aplica os princípios de separação engine vs renderer e expõe uma API mínima (`Initialize`, `RenderFrame`, `Shutdown`, `SetOverrideMode`).
- **Scene graph interno**: objetos da cena (chão, personagem, carro) e animações são geridos pelo renderer; o aplicativo cliente só precisa controlar a câmera.
- **Pipeline em três estágios**:
  1. **Sombras** – geração de depth maps direcionais e cúbicos.
  2. **Renderização off-screen** – MRT HDR reutilizando a infraestrutura da Aula 8.1.
  3. **Pós-processamento** – tone mapping + bloom em um quad de tela cheia.
- **Atalhos plug-and-play**: o `main.cpp` chama `renderer.SetOverrideMode(...)` para alternar materiais com as teclas `1/2/3`, validando a integração entre camadas.

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
├── main.cpp                   # Loop da aplicação + entrada + câmera
├── renderer.{h,cpp}           # Renderer modular (inicialização, passes e shutdown)
├── camera.{h,cpp}             # Câmera FPS compartilhada
├── light_manager.{h,cpp}      # Upload automático de direcional/pontual
├── model.{h,cpp}              # Importador + overrides
├── texture.{h,cpp}, material.{h,cpp}

assets/
├── models/scene.gltf, car.glb, cube.gltf, etc.
└── shaders/
    ├── vertex.glsl / fragment.glsl           # MRT
    ├── postprocess_vertex/fragment.glsl      # Tone mapping + bloom
    ├── depth_vertex/fragment/geometry.glsl   # Point shadows
    ├── directional_depth_vertex/fragment.glsl
```

