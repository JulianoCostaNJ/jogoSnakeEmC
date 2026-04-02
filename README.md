# 🐍 Snake Inteligente (C + BFS)

Um jogo clássico da cobrinha desenvolvido em **C**, rodando no **console do Windows**, com suporte a **modo automático usando algoritmo BFS (Breadth-First Search)**.

---

## 🚀 Funcionalidades

- 🎮 Modo manual (controle pelo teclado)
- 🤖 Modo automático com IA (BFS encontra o caminho até a fruta)
- 🍎 Sistema de frutas com raridade (comum, rara, épica)
- 🧱 Obstáculos gerados aleatoriamente conforme a dificuldade
- 📈 Sistema de pontuação e níveis
- 🏆 Ranking persistente
- 🎨 Interface com cores e emojis (UTF-8)
- ⏸️ Sistema de pausa
- 💀 Tela de Game Over com animação

---

## 🧠 Tecnologias e Conceitos

- Linguagem C (C11)
- Programação modular
- Máquina de estados
- Algoritmo BFS (pathfinding)
- Manipulação de console (Windows API)
- Estruturas de dados (arrays, structs)

---

## 📁 Estrutura do Projeto

```

snakeV2/
│
├── include/        # Arquivos .h (interfaces)
│   ├── game.h
│   ├── input.h
│   ├── render.h
│   ├── console.h
│   ├── bfs.h
│   └── ...
│
├── src/            # Implementações (.c)
│   ├── main.c
│   ├── game.c
│   ├── input.c
│   ├── render.c
│   ├── bfs.c
│   └── ...
│
└── README.md

````

---

## ⚙️ Como Compilar

### 🔧 Requisitos
- GCC (recomendado: TDM-GCC ou MinGW)
- Windows (uso de `windows.h`)

### ▶️ Comando no terminal 

```bash
gcc src/*.c -o snake.exe -std=c11
````

---

## 🎮 Como Jogar

### Controles

| Tecla   | Ação          |
| ------- | ------------- |
| ↑ ↓ ← → | Movimentar    |
| P       | Pausar        |
| Q       | Voltar / Sair |
| Enter   | Confirmar     |

---

## 🤖 Modo Automático

O jogo utiliza **BFS (Breadth-First Search)** para:

* Encontrar o caminho mais curto até a fruta
* Evitar obstáculos
* Simular comportamento inteligente

---

## 📊 Complexidade

* Movimento da cobra: `O(n)`
* Verificação de colisão: `O(n)`
* BFS: `O(W × H)`

---

## 🎯 Objetivo

Comer o máximo de frutas possível, aumentar a pontuação e sobreviver o maior tempo possível sem colidir com:

* Paredes
* Obstáculos
* O próprio corpo

---

## 🛠️ Melhorias Futuras

* [ ] Refatoração para reduzir variáveis globais
* [✅] Menu de seleção de dificuldade de jogo
* [ ] Otimização do BFS (evitar recalcular a cada frame)
* [ ] Sistema de save/load mais robusto
* [ ] Melhorias na IA (prevenção de becos sem saída)
* [ ] Interface mais dinâmica (animações)

---

## 📜 Licença

Este projeto é livre para uso e estudo.

