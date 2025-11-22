# Aula 2.2 – Sistema de Câmera

Terceira entrega prática do curso. Implementa um sistema de câmera interativa com controles WASD e mouse para navegação 3D.

## Conteúdo Abordado
- **Classe Camera**: Sistema de câmera FPS-style
- **Movimentação WASD**: Controles de teclado para navegação
- **Controle por Mouse**: Look around com captura do cursor
- **Matriz LookAt**: Cálculo automático da matriz de visão
- **Delta Time**: Movimento suave independente da taxa de frames

## Controles
- **WASD**: Movimentação horizontal (Forward/Back/Left/Right)
- **QE**: Movimentação vertical (Up/Down)
- **Botão Direito do Mouse + Movimento**: Rotação da câmera (look around)
- **ESC**: Sair da aplicação

## Como executar
```bash
build.bat    # gera solução e compila
run.bat      # executa a aplicação
```

## Arquivos da Implementação
```
src/
├── main.cpp       # Loop principal e configuração
├── camera.h       # Interface da classe Camera
└── camera.cpp     # Implementação da câmera
```

Scripts reutilizam dependências do projeto `ultraMini`.

## Nota
O triângulo permanece estático para focar nos controles da câmera. A rotação foi comentada no código para demonstração pura do sistema de navegação 3D.