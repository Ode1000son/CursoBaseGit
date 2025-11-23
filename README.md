# Aula 9.2 – Debugging e Profiling (OpenGL Debug Output, métricas de frame e GPU timers)

Esta segunda aula do módulo de otimização usa a base completa de renderização construída até a Aula 9.1 para habilitar **ferramentas de desenvolvimento** diretamente na engine. O foco é identificar gargalos sem alterar o comportamento visual do projeto.

## Objetivos
- **OpenGL debug output**: registrar mensagens de driver e GL_KHR_debug filtradas por severidade, mantendo histórico e facilitando inspeção sem pausar o jogo.
- **Frame time measurement**: capturar tempo de CPU do frame (delta real) e estatísticas como média/máximo/mínimo em janelas deslizantes.
- **GPU profiling**: medir seções críticas com `glQueryCounter` e atualizar continuamente os tempos de shadow pass, geometria e pós-processamento.

## Conteúdo Abordado
- **`application.{h,cpp}`**: força contextos com debug flag, liga `glDebugMessageCallback` e encaminha mensagens para o renderer.
- **`renderer_controller.{h,cpp}`**: expõe atalhos para alternar métricas (F1), limpar mensagens (F2) e continuar controlando overrides de textura.
- **`renderer.{h,cpp}`**: mantém histórico de frame time, cria timer queries para cada passe e atualiza o título da janela com FPS contínuo + overlay de métricas.
- **`scene.{h,cpp}`**: reaproveitado da Aula 9.1 para fornecer objetos/instâncias que alimentam o profiling.
- **Shaders em `assets/shaders`**: reutilizados sem alterações funcionais, garantindo que a instrumentação não impacte a saída visual.

## Controles
- `W A S D`: movimentação padrão.
- `Q / E`: deslocamento vertical.
- `Botão direito + mouse`: look-around.
- `F1`: alterna overlay de métricas (frame time + GPU timers).
- `F2`: limpa mensagens capturadas do OpenGL debug output.
- `ESC`: encerra a aplicação.

## Como executar
```bash
build.bat    # Premake + MSBuild (gera Aula09_DebugProfiling.sln)
run.bat      # Executa build/bin/Debug/Aula09_DebugProfiling.exe
```

## Estrutura Principal
```
src/
├── application.{h,cpp}        # Contexto + debug output
├── input_controller.{h,cpp}
├── renderer_controller.{h,cpp}# Toggles de métricas
├── renderer.{h,cpp}           # GPU timers + pipeline
├── scene.{h,cpp}              # Estatísticas de visibilidade
├── camera.{h,cpp}
├── light_manager.{h,cpp}
├── material.{h,cpp}
├── model.{h,cpp}
└── texture.{h,cpp}

assets/
├── models/ (Fish.glb, etc.)
└── shaders/
    ├── vertex/fragment.glsl
    ├── postprocess_vertex/fragment.glsl
    ├── depth_vertex/fragment/geometry.glsl
    ├── directional_depth_vertex/fragment.glsl
```
