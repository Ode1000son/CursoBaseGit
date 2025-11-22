@echo off
..\ultraMini\vendor\premake\premake5.exe vs2022
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\CursoOpenGL.sln /p:Configuration=Debug /p:Platform=x64

