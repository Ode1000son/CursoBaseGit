# Aula 3.2 – Modelos 3D Simples

Quinta entrega prática do curso. Evoluímos o pipeline da aula anterior para carregar um modelo glTF via Assimp, configurar atributos completos (posição, normal, UV) e renderizar com iluminação básica. O projeto também demonstra o uso de uma textura fallback (`Vitalik_edit_2.png`) aplicada automaticamente quando o material do modelo não fornece um arquivo válido.

## Conteúdo Abordado
- **Integração com Assimp**: Importação de `scene.gltf` e processamento recursivo de nós/meshes.
- **Estrutura `Model/Mesh`**: Encapsula VBO/VAO/EBO com atributos (posição, normal, UV).
- **Textura Fallback**: `Texture` padrão utilizada se o material não carregar uma textura difusa.
- **Iluminação Básica**: Vertex shader envia normais corretas; fragment shader calcula componentes ambiente/difusa/especular para destacar o volume do personagem.
- **Controles FPS**: Mesmo sistema de câmera da aula 2.2 para inspeção em 3D.

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
├── main.cpp        # Configuração do contexto, shaders e render loop (modelo 3D)
├── camera.{h,cpp}  # Sistema de câmera FPS
├── texture.{h,cpp} # Carregamento e gerenciamento de texturas (stb_image)
├── model.{h,cpp}   # Carregamento Assimp + gerenciamento de meshes

assets/
├── models/
│   ├── scene.gltf          # Modelo glTF importado
│   ├── scene.bin           # Buffers do glTF
│   └── Vitalik_edit_2.png  # Textura usada pelo personagem/fallback
└── shaders/
    ├── vertex.glsl   # Atributos completos + transformação de normais
    └── fragment.glsl # Iluminação simples + amostragem de textura
```

