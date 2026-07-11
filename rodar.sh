#!/bin/bash

echo -e "\e[1;34m[1/3] Compilando o Java...\e[0m"
javac -h java/headers -d java/build java/src/main/*.java

echo -e "\e[1;33m[2/3] Compilando o C++ (Gerando libmotor.so)...\e[0m"
# -ljawt: biblioteca do JAWT (Java AWT Native Interface), usada para pegar
# a Window X11 por tras do Canvas Swing e reparentar a janela GLFW nela
# (ver embedIntoCanvas() em motor.cpp). libjawt.so mora em $JAVA_HOME/lib.
# -lX11: XReparentWindow/XResizeWindow/XMapWindow usados no mesmo lugar.
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

echo -e "\e[1;32m[3/3] Iniciando o Simulador...\e[0m"
java --enable-native-access=ALL-UNNAMED -Djava.library.path=java/lib -cp java/build main.Main