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
* **[3] A* (A-Star):** Utiliza a heurística de Distância de Manhattan para encontrar a saída de forma rápida e otimizada.

---

## Tecnologias Utilizadas

* **C++17** (Lógica Orientada a Objetos, Algoritmos e Estrutura de Dados)
* **OpenGL 3.3 Core Profile** (Pipeline gráfico)
* **GLFW** (Gerenciamento de janelas e inputs do sistema operacional)
* **GLAD** (Carregador de ponteiros do OpenGL)
* **GLM** (Matemática para Computação Gráfica)
* **CMake** (Sistema de automação de build)

---

## Como Compilar e Executar (Linux)

### 1. Dependências do Sistema
Antes de compilar, instale os pacotes de desenvolvimento do GLFW e do OpenGL no seu sistema (baseado em Debian/Ubuntu):

```bash
sudo apt update
sudo apt install libglfw3-dev libgl1-mesa-dev xorg-dev
```

### 2. Dependências extras (Java Swing + JNI)

Além das dependências gráficas acima, esta versão do projeto integra **Java Swing** com o motor em C++ via **JNI (Java Native Interface)**. Para isso é necessário também:

```bash
sudo apt install openjdk-21-jdk
sudo apt install libdecor-0-plugin-1-gtk
```

### 3. Compilando o projeto

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
    -o java/lib/libmotor.so \
    -lglfw -lGL -ldl
```

### 4. Executando

```bash
java --enable-native-access=ALL-UNNAMED -Djava.library.path=java/lib -cp java/build main.Main
```