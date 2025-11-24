# Aula 11.2 – Estruturas de Física e Parsing da Cena

Segunda etapa do módulo de PhysX. Saimos do puro setup e passamos a descrever
informações físicas diretamente no JSON da cena, espelhando exatamente o que
já existe no `AppTestFisica`. Toda a sincronização acontece dentro de
`SceneObject`, mantendo a engine fiel aos **Princípios da Nova Arquitetura**.

## Objetivos
- **Modelos de dados**: introduzir `PhysicsShapeType`, `PhysicsBodyMode` e
  `SceneObjectPhysics`, além dos helpers `GetScaledHalfExtents`,
  `GetLocalBoundsCenter` e `ApplyPhysicsPose`.
- **Parsing completo**: interpretar o bloco `physics` do JSON, com suporte a
  auto-radius/half extents, alinhamento opcional ao bounds e modo container.
- **Ciclo de recarregamento**: rastrear `m_lastScenePath` e expor `Scene::Reload`
  para permitir hot-reload da cena sem reconstruir a engine.
- **Scripts/documentação**: atualizar nomes para `Aula11_PhysicsScene` nos
  scripts de build/run e alinhar o README com o novo escopo da aula.

## Passo a passo implementado
1. **Estruturas do SceneObject** – Copiamos de `AppTestFisica` toda a
   definição física, incluindo enums, struct e métodos auxiliares para leitura
   e aplicação de poses vindas do PhysX.
2. **Parsing dedicado** – Foram adicionados os helpers `ParsePhysicsDefinition`,
   `ComputeAutoRadius` e `ComputeAutoHalfExtents`, garantindo consistência com
   o projeto de referência e mantendo o JSON como fonte da verdade.
3. **Reload seguro da cena** – `Scene::LoadSceneDefinition` registra o caminho,
   `Scene::Reload` reconstrói objetos/batches on-demand e o resto da engine
   continua plug-and-play.
4. **Infra atualizada** – `premake5.lua`, `build.bat` e `run.bat` agora usam o
   nome `Aula11_PhysicsScene`, mantendo o pipeline idêntico ao da aula anterior.

## Como executar
```powershell
# Build/execução rápidos da aula
$env:NOPAUSE=1; .\build.bat
$env:NOPAUSE=1; .\run.bat


## Arquivos relevantes
- `src/scene.h` – definições das estruturas físicas e novas APIs do SceneObject.
- `src/scene.cpp` – parsing do JSON, helpers de física e `Scene::Reload`.
- `premake5.lua` – workspace `Aula11_PhysicsScene` com mesmas dependências da aula 11.1.
- `build.bat` / `run.bat` – scripts oficiais apontando para o novo executável.

## Observações finais
Todas as mudanças mantêm máximo reaproveitamento, módulos plug-and-play,
separação engine/renderer/UI, tolerância zero a alteração de funcionalidade e
modularidade extrema. A cena continua sendo descrita apenas via JSON e nenhum
código fora da camada apropriada tem acesso direto a detalhes específicos do
backend de física.
