# Aula 10.2 – Sistema de Áudio com MiniAudio

Segunda aula do **Projeto Final**. Mantemos a renderização completa da aula
anterior e adicionamos um subsistema de áudio 3D baseado no **MiniAudio**, com
carregamento declarativo, controle de volume global e integração direta com a
câmera e com objetos principais da cena.

## Objetivos
- **MiniAudio plug-and-play**: integrar o header único `miniaudio.h` via Premake
  (sem alterar outras camadas da engine).
- **AudioSystem modular**: classe dedicada (`audio_system.{h,cpp}`) cuida de
  inicialização do dispositivo, carregamento de sons, posicionamento 3D e
  cleanup.
- **Configuração por dados**: `assets/scenes/audio_config.json` descreve cada
  emissor (arquivo, loop, distância, posição inicial).
- **Integração com Application/Camera**: listener segue a câmera, fontes ligadas
  aos objetos `HeroFish` e `Car`, e controles de runtime para volume e eventos.

## Conteúdo Abordado
- **`audio_system.{h,cpp}`**: encapsula o `ma_engine`, registra emissores,
  expõe atualização de listener, um-shot sounds e volume global.
- **`application.{h,cpp}`**: instancia o `AudioSystem`, sincroniza posições dos
  objetos da cena, adiciona atalhos (`[ ]` e `Space`) e envia feedback pelo
  debug overlay.
- **`camera.h`**: expõe `GetUp()` para alimentar a orientação 3D do MiniAudio.
- **`premake5.lua`**: inclui `vendor/miniaudio` e linka `ole32` (backend
  WASAPI).
- **Assets**: `audio_config.json` organiza trilha ambiente, motor do carro,
  beacon do herói e o efeito one-shot de coleta.

## Controles
- `W A S D / Q / E`: movimentação e deslocamento vertical.
- `Botão direito + mouse`: look-around com captura de cursor.
- `1 / 2 / 3`: overrides de textura (renderer controller).
- `F1`: alterna overlay de métricas da cena.
- `F2`: limpa o log de debug do OpenGL.
- `[` / `]`: diminui/aumenta o volume global do áudio em steps de 5%.
- `Space`: reforça o ping sonoro do herói (reinicia o loop `hero_beacon`).
- `ESC`: encerra a aplicação.

## Como executar
```powershell
build.bat    # Premake + MSBuild (gera Aula10_SistemaAudio.sln)
run.bat      # Executa build/bin/Debug/Aula10_SistemaAudio.exe
```

## Arquivos Relevantes
```
src/
├── audio_system.{h,cpp}        # MiniAudio engine + emissores 3D
├── miniaudio_config.h          # Config global de decoders/defines do MiniAudio
├── miniaudio_impl.cpp          # TU dedicado com MINIAUDIO_IMPLEMENTATION
├── application.{h,cpp}         # Loop principal + integração de áudio
├── camera.{h,cpp}              # GetUp() para listener
├── input_controller.{h,cpp}
├── renderer.{h,cpp}
├── renderer_controller.{h,cpp}
├── scene.{h,cpp}
└── ...

assets/
├── audio/                      # mp3 de ambient, motor e efeitos
├── scenes/final_scene.json     # Layout visual
└── scenes/audio_config.json    # Layout sonoro (MiniAudio)
```

Mantivemos todos os **Princípios da Nova Arquitetura**: máximo
reaproveitamento, componentes plug-and-play e zero alteração da funcionalidade
visual existente. O áudio é uma camada adicional acoplada apenas via
interfaces (`Application` ↔ `AudioSystem`), preservando a separação engine vs.
renderer vs. UI.
