# Aula 8.4 – Application Host e Input Layer

Quarta aula do módulo de renderização avançada. Depois de separar `Scene` e `Renderer`, agora extraímos tudo o que ainda restava no `main.cpp` para uma camada de **Application Host**: inicialização/loop da engine, gerenciamento de entrada e controles de debug viraram módulos dedicados. O entry point ficou reduzido a criar `Application` e chamar `Run()`, exatamente como em uma engine profissional.

## Objetivos
- **Application host**: classe `Application` responsável por inicializar GLFW/GLAD, criar a janela, coordenar `Scene`, `Renderer` e gerenciar o loop principal.
- **InputController**: componente independente que trata teclado/mouse, captura FPS quando o botão direito está pressionado e expõe apenas `ProcessInput`.
- **RendererController**: controla atalhos de depuração (`1/2/3`) desacoplados do renderer.
- **Main mínimo**: `main.cpp` apenas instancia `Application` com uma configuração e chama `Run()`.

## Conteúdo Abordado
- **`application.{h,cpp}`**: encapsula todo o ciclo de vida (Initialize, Run, Shutdown), além de manter instâncias de `Camera`, `Scene`, `Renderer`, `InputController` e `RendererController`.
- **`input_controller.{h,cpp}`**: registra callbacks no GLFW, lida com estado do mouse e traduz eventos em chamadas da câmera.
- **`renderer_controller.{h,cpp}`**: monitora teclas `1/2/3` e chama `Renderer::SetOverrideMode`.
- **`main.cpp`**: reduzido a 7 linhas, mantendo o entry point puro.
- **Demais módulos** (`scene`, `renderer`, `camera`, etc.) seguem reaproveitados da Aula 8.3 sem duplicação de lógica.

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
├── application.{h,cpp}        # Host do loop + inicialização/encerramento
├── input_controller.{h,cpp}   # Teclado/mouse desacoplados
├── renderer_controller.{h,cpp}# Atalhos de depuração do renderer
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
