# Aula 10.1 – Engine Completa (Scene Loading + Advanced Lighting)

Primeira aula do **Projeto Final**. A engine passa a carregar uma cena completa
a partir de um arquivo JSON (`assets/scenes/final_scene.json`), reaproveitando
toda a arquitetura modular construída nos módulos anteriores: render pipeline
completo, instancing, LOD, debug overlay, métricas de CPU/GPU e overrides de
textura.

## Objetivos
- **Integração total**: Application → Scene → Renderer → UI/controller trabalham
  como módulos plug-and-play, mantendo o entry point limpo.
- **Scene loading**: objetos, LODs, lotes instanciados, câmera e luzes são
  definidos em JSON, permitindo reorganizar a cena sem recompilar.
- **Advanced lighting setup**: direção, cor, animação e luzes que projetam
  sombras são configuradas via dados, habilitando pipelines híbridos (directional
  + point shadows).

## Conteúdo Abordado
- **`scene.{h,cpp}`**: adiciona parser JSON usando nlohmann, registra modelos,
  resolve LODs por chave e gera batches instanciados conforme parâmetros da cena.
- **`application.{h,cpp}` + `camera.{h,cpp}`**: câmera inicial vem do arquivo de
  cena (posição, yaw/pitch, velocidade, sensibilidade).
- **`renderer.{h,cpp}` + `light_manager.{h,cpp}`**: luzes direcionais e pontuais
  são preenchidas dinamicamente; orbit/point shadow parameters passam a ser
  dados da cena.
- **`renderer_controller.{h,cpp}`**: continua expondo os toggles de textura e o
  overlay de métricas herdado da Aula 9.2.
- **Assets**: novo diretório `assets/scenes/` com a definição da cena final.

## Controles
- `W A S D`: movimentação padrão.
- `Q / E`: deslocamento vertical.
- `Botão direito + mouse`: look-around com captura de cursor.
- `1 / 2 / 3`: overrides de textura (importada, checker, highlight metálico).
- `F1`: alterna overlay de métricas (CPU + GPU timers).
- `F2`: limpa mensagens capturadas pelo OpenGL debug output.
- `ESC`: encerra a aplicação.

## Como executar
```bash
build.bat    # Premake + MSBuild (gera Aula10_EngineCompleta.sln)
run.bat      # Executa build/bin/Debug/Aula10_EngineCompleta.exe
```

## Estrutura Principal
```
src/
├── application.{h,cpp}        # Host do loop + debug output
├── camera.{h,cpp}             # Câmera FPS configurável
├── input_controller.{h,cpp}
├── renderer_controller.{h,cpp}
├── renderer.{h,cpp}           # Pipeline completo + métricas
├── scene.{h,cpp}              # Parser JSON + objetos + instancing
├── light_manager.{h,cpp}
├── material.{h,cpp}
├── model.{h,cpp}
└── texture.{h,cpp}

assets/
├── models/ (Fish.glb, car.glb, cube.gltf, etc.)
├── shaders/ (MRT, depth, post-process)
└── scenes/final_scene.json    # Layout completo da engine
```
