# Gerador de Labirinto | CG + POO

Um simulador interativo de geração e resolução de labirintos desenvolvido em **C++** puro com renderização gráfica utilizando **OpenGL Moderno**. 

Este é um projeto integrador desenvolvido em conjunto pelas turmas de **Computação Gráfica (CG)** e **Programação Orientada a Objetos (POO)**.

---

## Sobre o Projeto

O objetivo do simulador é demonstrar, de forma visual e em tempo real, como diferentes algoritmos de busca (Pathfinding) operam dentro de uma estrutura de grafos (representada por um labirinto bidimensional). 

O projeto aplica conceitos de POO para separar a lógica matemática (dados, matrizes, nós) da lógica de renderização gráfica (Shaders, Buffers, Matrizes de Projeção).

### Algoritmos de Busca Implementados:
* **[1] BFS (Busca em Largura):** Explora o mapa em ondas. Garante o caminho mais curto, mas visita muitos nós.
* **[2] DFS (Busca em Profundidade):** Mergulha por um corredor até achar um beco sem saída e realiza *backtracking*.
* **[3] A* (A-Star):** Combina o custo já percorrido (g) com a heurística de Distância de Manhattan (h) para encontrar a saída de forma rápida e otimizada.
* **[4] Dijkstra:** Busca de custo uniforme (sem heurística) — expande sempre o nó de menor custo acumulado da fila de prioridade. Equivalente a uma UCS (*Uniform Cost Search*); como todas as arestas do labirinto têm o mesmo custo, garante o caminho mais curto, mas explora mais nós que o A*.
* **[5] Gulosa / GBS (Greedy Best-First Search):** Usa apenas a heurística de Distância de Manhattan até o objetivo (ignora o custo já percorrido). É a mais rápida "correndo na direção certa", mas não garante o caminho mais curto.

---

## Tecnologias Utilizadas

* **C++17** (Lógica Orientada a Objetos, Algoritmos e Estrutura de Dados)
* **OpenGL 3.3 Core Profile** (Pipeline gráfico)
* **GLFW** (Gerenciamento de janelas e inputs do sistema operacional)
* **GLAD** (Carregador de ponteiros do OpenGL)
* **GLM** (Matemática para Computação Gráfica)
* **CMake** (Sistema de automação de build)

---

## Como Compilar e Executar

### Modo rápido (recomendado)

```bash
chmod +x instalar_dependencias.sh rodar.sh
./instalar_dependencias.sh   # instala todas as dependências do sistema
./rodar.sh                   # compila (Java + C++) e executa
```

O `instalar_dependencias.sh` cobre Debian, Ubuntu e WSL (Ubuntu/Debian). **Este projeto só funciona em Linux** — se você usa Windows, veja a seção [Windows](#windows)

### Modo manual (Linux/WSL)

#### 1. Dependências do sistema

```bash
sudo apt update
sudo apt install build-essential libglfw3-dev libgl1-mesa-dev xorg-dev libglm-dev
```

> **`libglm-dev` é obrigatório**: `Renderer.cpp`, `Renderer.hpp` e `labirinto.cpp` incluem `<glm/glm.hpp>` e outros headers do GLM, que **não** vêm empacotados no repositório.

#### 2. Dependências extras (Java Swing + JNI)

```bash
sudo apt install openjdk-21-jdk
sudo apt install libdecor-0-plugin-1-gtk
```
#### 3. Compilando o projeto

**Passo 1 — Compilar o Java e gerar os headers JNI:**

```bash
javac -h java/headers -d java/build java/src/main/*.java
```

**Passo 2 — Compilar a biblioteca nativa (C++):**

```bash
g++ -std=c++17 -shared -fPIC \
    java/native/motor.cpp \
    src/Maze.cpp src/Renderer.cpp src/Solver.cpp \
    external/glad/src/glad.c \
    -Iinclude -Iexternal/glad/include -Ijava/headers \
    -I/usr/lib/jvm/java-21-openjdk-amd64/include \
    -I/usr/lib/jvm/java-21-openjdk-amd64/include/linux \
    -L/usr/lib/jvm/java-21-openjdk-amd64/lib \
    -o java/lib/libmotor.so \
    -lglfw -lGL -ldl -ljawt -lX11 \
    -Wl,-rpath,/usr/lib/jvm/java-21-openjdk-amd64/lib
```
#### 4. Executando

```bash
java --enable-native-access=ALL-UNNAMED -Djava.library.path=java/lib -cp java/build main.Main
```

Os passos 3 e 4 acima são exatamente o que o `rodar.sh` só que automatizado

---

## Windows

**Este projeto só funciona em Linux.** O motor nativo (`java/native/motor.cpp`) usa X11/Xlib diretamente para embutir a janela do OpenGL dentro do Canvas Swing, e essa API não existe no Windows — por isso não há (e não haverá, sem reescrever essa parte do código) uma forma de compilar ou instalar as dependências nativamente pelo PowerShell/CMD.

Se você usa Windows, instale o **WSL** (Windows Subsystem for Linux) com uma distro Ubuntu ou Debian:

```powershell
wsl --install -d Ubuntu
```

Reinicie o Windows se for a primeira instalação, abra o app **Ubuntu** pelo menu Iniciar, crie seu usuário/senha Linux e copie o projeto para dentro do filesystem do Linux (ex.: `~/labirinto` — evite deixar em `/mnt/c/...`, o desempenho de I/O é pior). Depois disso, dentro do terminal Ubuntu, é só seguir **exatamente os mesmos comandos** de como compilar e executar acima.