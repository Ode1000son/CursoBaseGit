# Aula 9.1 – Performance (Instancing, Culling e LOD)

Primeira aula do módulo de otimização. Mantendo toda a arquitetura da engine criada nos módulos anteriores, adicionamos ferramentas de **renderização eficiente**: culling por frustum, níveis de detalhe (LOD) e instancing em GPU. Também trocamos o personagem por um peixe (`Fish.glb`) que já vem com 6 LODs distintos.

## Objetivos
- **LOD real**: carregar o mesmo `Fish.glb` filtrando nós `Fish_LOD0..5` e selecionar o nível automaticamente conforme a distância até a câmera.
- **Frustum culling**: cada `SceneObject` tem uma bounding sphere em world space e só é enviado à GPU quando realmente aparece na câmera.
- **Instancing**: dezenas de pilares/corais são desenhados com um único draw usando atributos de matriz (`mat4`) e um VBO dinâmico.
- **Shaders cientes de instancing**: vertex e depth shaders recebem o atributo `aInstanceModel` e podem alternar entre uniform `model` e matriz por instância.

## Conteúdo Abordado
- **`model.{h,cpp}`**: suporta filtro de nós (para carregar apenas um LOD do glTF) e calcula bounding boxes/esferas reutilizadas pela cena.
- **`scene.{h,cpp}`**: cria os objetos com `SceneObjectLOD`, calcula bounds, gera lotes instanciados (`SceneInstancedBatch`) e substitui o personagem pelo peixe animado.
- **`renderer.{h,cpp}`**: extrai frustum (`projection * view`), seleciona LODs por distância, aplica culling por esfera e gerencia um `instanceVBO` compartilhado para todos os batches.
- **Shaders** (`vertex`, `directional_depth_vertex`, `depth_vertex`): adicionam `uUseInstanceTransform` e `aInstanceModel`.

## Controles
- `W A S D`: movimentação no plano.
- `Q / E`: movimento vertical.
- `Botão direito + mouse`: look-around (cursor capturado enquanto pressionado).
- `1`: restaura texturas originais.
- `2`: força textura checker.
- `3`: textura metálica de destaque.
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
├── scene.{h,cpp}              # Scene graph + LOD + batches instanciados
├── renderer.{h,cpp}           # Sombras, MRT, post + culling + instancing
├── camera.{h,cpp}
├── light_manager.{h,cpp}
├── material.{h,cpp}
├── model.{h,cpp}              # LOD filtering + bounding volumes
└── texture.{h,cpp}

assets/
├── models/Fish.glb, car.glb, cube.gltf, CubeTexture.jpg
└── shaders/
    ├── vertex/fragment.glsl                # MRT principal + instancing flag
    ├── postprocess_vertex/fragment.glsl    # Tone mapping + bloom
    ├── depth_vertex/fragment/geometry.glsl # Sombras omnidirecionais
    ├── directional_depth_vertex/fragment.glsl
```
