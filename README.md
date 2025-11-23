# Aula 4.2 – Materiais e Propriedades

Segunda entrega do módulo de iluminação. Evoluímos o projeto anterior para tratar um **sistema de materiais**: cada objeto define suas próprias componentes ambiente/difusa/especular e fator de brilho (shininess). Com isso, o personagem mantém a aparência realista enquanto o chão recebe uma coloração personalizada sobre a textura `CubeTexture.jpg`.

## Conteúdo Abordado
- **Shader Phong com Materiais**: Struct `Material` agora possui ambient/diffuse/specular independentes, modulando o resultado junto à textura.
- **Materiais Múltiplos**: Personagem e chão aplicam conjuntos distintos de cores/shine para evidenciar superfícies diferentes.
- **Variação Dinâmica do Vetor de Luz**: A direção da luz gira suavemente, evidenciando o efeito especular conforme a câmera se move.
- **Transformação Correta de Normais**: Vertex shader normaliza o resultado de `mat3(transpose(inverse(model))) * normal`.
- **Assimp + Texturas**: Reaproveitamos a infraestrutura de `Model/Mesh` + texturas já carregadas.
- **Plano de Referência Colorido**: O cubo importado (`cube.gltf`) é escalado/texturizado e recebe um material em tons quentes para destacar especularidade do piso.
- **Controles FPS**: Mesmo sistema de navegação da Aula 2.2 para inspecionar o personagem em 3D.

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
├── main.cpp          # Configuração do contexto, shaders e render loop (modelo 3D)
├── camera.{h,cpp}    # Sistema de câmera FPS
├── texture.{h,cpp}   # Carregamento e gerenciamento de texturas (stb_image)
├── material.{h,cpp}  # Classe Material com cores/shine e binding de textura
├── model.{h,cpp}     # Carregamento Assimp + gerenciamento de meshes

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

