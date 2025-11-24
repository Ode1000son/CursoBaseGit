# CursoBaseGit – Base com PhysX integrado

Este diretório é a fundação limpa que usamos em todas as aulas. Agora ele
também está preparado para o **PhysX 5**, garantindo que qualquer aula possa
ativar física avançada sem tocar nos projetos gerados pelo Visual Studio.

## Objetivos desta configuração
- **SDK oficial dentro de `vendor/physx`** – copiamos o conteúdo diretamente do
  `AppTestFisica`, preservando `install/vc17win64-cpu-only/PhysX`.
- **Premake consciente do PhysX** – `premake5.lua` adiciona include/lib dirs,
  ativa `staticruntime "On"` e linka todas as libs necessárias
  (`PhysX_64`, `PhysXCommon_64`, `PhysXExtensions_static_64`, etc.).
- **Cópia automática de DLLs** – pós-build garante que `PhysX_64.dll` e
  demais dependências sempre acompanhem o executável.
- **Zero atrito para assets** – `xcopy` continua replicando `assets/` para o
  diretório de saída.

## Estrutura relevante
- `src/` – código da base comum.
- `assets/` – modelos, shaders e cenas compartilhadas.
- `vendor/`  
  - `physx/` – SDK completo (include/bin/pvdruntime).  
  - `glfw-3.4`, `assimp`, `glm`, `miniaudio`, etc.
- `premake5.lua` – script único que descreve a solução `CursoOpenGL`.
- `build.bat` / `run.bat` – wrappers oficiais (Premake + MSBuild e execução).

## Como validar
```powershell
# Gerar solução e compilar (sempre com a flag preferida pelo usuário)
$env:NOPAUSE=1; .\build.bat

# Executar o binário recém-gerado
$env:NOPAUSE=1; .\run.bat
```

Ambos os scripts devem ser executados em primeiro plano para acompanhar os
logs. Se qualquer etapa falhar, corrija antes de prosseguir – não seguimos em
frente com o build quebrado.

## Observações
- `premake5.lua` centraliza toda a integração: inclui do PhysX, `libdirs`
  específicos por configuração e cópias pós-build das DLLs.
- O SDK permanece encapsulado no `vendor/physx`; código cliente continua
  consumindo apenas as interfaces expostas pelo projeto.
- Mantemos os **Princípios da Nova Arquitetura**: máximo reaproveitamento de
  código, plug-and-play total, separação engine/renderer/UI, tolerância zero a
  alteração de funcionalidade e modularidade extrema.
