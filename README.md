# Aula 4.1 – Phong Lighting

Primeira entrega do módulo de iluminação. Evoluímos o projeto da Aula 3.2 para demonstrar o modelo de iluminação Phong completo (ambiente + difusa + especular) aplicado sobre o personagem importado via Assimp. A textura `Vitalik_edit_2.png` continua funcionando como fallback caso o material do glTF não possua textura difusa própria.

## Conteúdo Abordado
- **Shader Phong Completo**: Structs `Light` e `Material` com parâmetros independentes (direção, cores, shininess).
- **Variação Dinâmica do Vetor de Luz**: A direção da luz gira suavemente, evidenciando o efeito especular conforme a câmera se move.
- **Transformação Correta de Normais**: Vertex shader normaliza o resultado de `mat3(transpose(inverse(model))) * normal`.
- **Assimp + Texturas**: Reaproveitamos a infraestrutura de `Model/Mesh` + textura fallback criada na Aula 3.2.
- **Plano de Referência**: Um cubo importado (`cube.gltf`) é escalado e texturizado (`CubeTexture.jpg`) para servir como chão, ajudando a perceber sombras e especularidade.
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
├── main.cpp        # Configuração do contexto, shaders e render loop (modelo 3D)
├── camera.{h,cpp}  # Sistema de câmera FPS
├── texture.{h,cpp} # Carregamento e gerenciamento de texturas (stb_image)
├── model.{h,cpp}   # Carregamento Assimp + gerenciamento de meshes

assets/
├── models/
│   ├── scene.gltf          # Modelo glTF importado
│   ├── scene.bin           # Buffers do glTF
│   ├── cube.gltf / cube.bin          # Cubo usado como chão
│   ├── CubeTexture.jpg               # Textura aplicada ao chão
│   └── Vitalik_edit_2.png            # Textura usada pelo personagem/fallback
└── shaders/
    ├── vertex.glsl   # Atributos completos + normal matrix normalizada
    └── fragment.glsl # Implementação do modelo de iluminação Phong
```

