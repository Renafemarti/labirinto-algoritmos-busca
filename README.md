# Gerador de Labirinto | CG + POO

Um simulador interativo de geração e resolução de labirintos, com o motor gráfico em **C++** puro (renderização em **OpenGL Moderno**) e a interface de controle em **Java Swing**, ligados via **JNI (Java Native Interface)**.

Este é um projeto integrador desenvolvido em conjunto pelas turmas de **Computação Gráfica (CG)** e **Programação Orientada a Objetos (POO)**.

---

## Sobre o Projeto

O objetivo do simulador é demonstrar, de forma visual e em tempo real, como diferentes algoritmos de busca (Pathfinding) operam dentro de uma estrutura de grafos (representada por um labirinto bidimensional).

O projeto aplica conceitos de POO para separar a lógica matemática (dados, matrizes, nós) da lógica de renderização gráfica (Shaders, Buffers, Matrizes de Projeção), e a UI de controle/estatísticas (Java Swing) do motor gráfico (C++/OpenGL).

### Algoritmos de Busca Implementados:
* **[1] BFS (Busca em Largura):** Explora o mapa em ondas. Garante o caminho mais curto, mas visita muitos nós.
* **[2] DFS (Busca em Profundidade):** Mergulha por um corredor até achar um beco sem saída e realiza *backtracking*.
* **[3] A* (A-Star):** Utiliza a heurística de Distância de Manhattan para encontrar a saída de forma rápida e otimizada.

---

## Arquitetura

O projeto é dividido em duas partes que se comunicam via JNI:

* **Motor gráfico (C++)** — `src/*.cpp` + `external/glad`: gera o labirinto, roda os algoritmos de busca e desenha tudo com OpenGL/GLFW/GLM. É compilado como uma biblioteca nativa (`java/lib/libmotor.so`), não como um executável separado.
* **Interface de controle (Java Swing)** — `java/src/main/*.java`: janela com os controles (gerar labirinto, escolher algoritmo, estatísticas em gráfico de barras etc.).
* **Ponte JNI** — `java/native/motor.cpp` + `java/headers/main_MotorGrafico.h`: expõe funções do motor C++ para o Java chamar.

A janela OpenGL **não abre separada**: ela é embutida dentro de um `Canvas` do Swing, formando uma única janela com barra de título do SO. Isso é feito com **JAWT** (para pegar a `Window` X11 nativa por trás do `Canvas`) e chamadas X11 diretas (`XReparentWindow`/`XResizeWindow`/`XMapWindow`) para "encaixar" a janela GLFW dentro dela — ver `embedIntoCanvas()` em `java/native/motor.cpp`. É por isso que o motor nativo depende de **JAWT** e **X11**, e não só de OpenGL/GLFW.

---

## Tecnologias Utilizadas

* **C++17** (lógica de POO, algoritmos e estrutura de dados do labirinto)
* **OpenGL 3.3 Core Profile** (pipeline gráfico)
* **GLFW** (janela/contexto OpenGL e input, embutida via X11 dentro do Swing)
* **GLAD** (carregador de ponteiros do OpenGL — já incluído em `external/glad`)
* **GLM** (matemática para computação gráfica — vetores/matrizes)
* **stb_truetype** (renderização de texto na tela — já incluído em `include/stb_truetype.h`)
* **Java 21 + Swing** (interface de controle, gráficos de estatísticas, tela de transformações)
* **JNI / JAWT** (ponte entre o Java e o motor nativo C++, e embedding da janela GLFW no Canvas Swing)

> Existe um `CMakeLists.txt` no repositório, mas ele é resquício de uma versão antiga do projeto (um executável C++ único, sem a integração com Java/JNI). O fluxo atual de build **não usa CMake** — a compilação é feita diretamente via `g++`, como mostrado abaixo.

---

## Como Compilar e Executar

### Modo rápido (recomendado)

```bash
chmod +x instalar_dependencias.sh rodar.sh
./instalar_dependencias.sh   # instala todas as dependências do sistema
./rodar.sh                   # compila (Java + C++) e executa
```

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

> `-ljawt` e `-lX11` são necessários porque a janela do labirinto é
> **embutida** dentro do `Canvas` Swing da tela de controles (uma única
> janela, com barra de título do SO) em vez de ser uma janela separada. O
> lado nativo usa **JAWT** para pegar a Window X11 por trás do `Canvas` e
> faz a janela GLFW nascer reparentada dentro dela via `XReparentWindow`
> (ver `embedIntoCanvas()` em `java/native/motor.cpp`).

#### 4. Executando

```bash
java --enable-native-access=ALL-UNNAMED -Djava.library.path=java/lib -cp java/build main.Main
```

Os passos 3 e 4 acima são exatamente o que o `rodar.sh` já automatiza — use-o no dia a dia em vez de rodar cada comando manualmente.

---

## Windows / WSL

O motor nativo (`java/native/motor.cpp`) usa **X11/Xlib diretamente** (`XReparentWindow`, `glfwGetX11Window`, e força `GLFW_PLATFORM_X11` na inicialização) para embutir a janela do OpenGL dentro do Canvas Swing. Isso é uma API exclusiva do Linux — **não existe compilação nativa disso no Windows** (MSVC/MinGW) sem reescrever essa parte usando a API do Win32 (JAWT no modo Win32 + `SetParent`, em vez de Xlib).

Por isso, no Windows o caminho recomendado é o **WSL** (Windows Subsystem for Linux), que roda um Linux de verdade dentro do Windows:

1. Rode `instalar_dependencias_windows.ps1` — ele instala o WSL com Ubuntu (`wsl --install -d Ubuntu`).
2. Reinicie o Windows se for a primeira instalação do WSL, abra o app **Ubuntu** e crie seu usuário/senha Linux.
3. Copie o projeto para dentro do filesystem do Linux (ex.: `~/labirinto` — **não** deixe em `/mnt/c/...`, o desempenho de I/O é muito pior).
4. Dentro do terminal Ubuntu, rode os dois comandos normais:
   ```bash
   ./instalar_dependencias.sh
   ./rodar.sh
   ```

No Windows 10 (build 19044+) e Windows 11, o **WSLg** já vem incluso e dá suporte gráfico automático — a janela do simulador abre normalmente na sua área de trabalho, sem configuração extra. Em versões mais antigas do WSL2 sem WSLg, é necessário instalar um servidor X no Windows (ex.: VcXsrv) e exportar a variável `DISPLAY` antes de rodar `./rodar.sh`.