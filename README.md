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
