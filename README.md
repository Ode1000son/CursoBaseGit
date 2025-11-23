# Aula 8.1 – Framebuffers e MRT

Primeira aula do módulo de renderização avançada. Evoluímos a base da engine **ultraMini** e da Aula 7.2 para introduzir um pipeline completo de **renderização off-screen** com **Multiple Render Targets (MRT)** e um estágio dedicado de **pós-processamento**.

## Objetivos
- **Renderização off-screen**: desenhar toda a cena em um framebuffer HDR dedicado.
- **Multiple render targets**: armazenar simultaneamente a cena iluminada e um buffer de realces para efeitos.
- **Pós-processamento plug-and-play**: aplicar tone mapping + bloom leve antes de apresentar o frame final.

## Conteúdo Abordado
- **Framebuffer HDR com MRT**: dois attachments (`cor` e `highlights`) redimensionados dinamicamente pelo tamanho da janela.
- **Pós-processamento**: quad de tela cheia com shaders dedicados (`postprocess_vertex/fragment.glsl`) que misturam os buffers e aplicam exposição configurável.
- **Integração com sombras**: o pipeline mantém o shadow mapping direcional e pontual já existentes, reaproveitando os recursos do módulo anterior.
- **Cena animada completa**: três modelos (`scene.gltf`, `car.glb`, `cube.gltf`) com overrides de textura e animações suaves.
- **Atalhos de depuração**: mesmas teclas `1/2/3` para alternar materiais e visualizar rapidamente o efeito do pós-processamento.

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
├── main.cpp                   # Loop principal + MRT + pós-processamento
├── camera.{h,cpp}             # Câmera FPS compartilhada em todos os passes
├── texture.{h,cpp}            # Carregamento/gerenciamento de texturas (stb_image)
├── material.{h,cpp}           # Materiais Phong com suporte a overrides
├── light_manager.{h,cpp}      # Direcionais + pontuais com upload automático
├── model.{h,cpp}              # Importador Assimp + materiais por mesh + overrides

assets/
├── models/
│   ├── scene.gltf / scene.bin        # Personagem principal
│   ├── cube.gltf / cube.bin          # Chão/base da cena
│   ├── car.glb                       # Veículo animado
│   ├── CubeTexture.jpg               # Textura aplicada ao chão e como fallback
│   └── Vitalik_edit_2.png            # Textura original do personagem
└── shaders/
    ├── vertex.glsl
    ├── fragment.glsl                 # Escreve em dois attachments (cor + highlight)
    ├── postprocess_vertex.glsl       # Quad de tela cheia
    ├── postprocess_fragment.glsl     # Tone mapping + acentuação de highlights
    ├── directional_depth_vertex.glsl
    ├── directional_depth_fragment.glsl
    ├── depth_vertex.glsl
    ├── depth_geometry.glsl
    └── depth_fragment.glsl
```

