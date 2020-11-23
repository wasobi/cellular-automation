#!/bin/bash
if [ ! -d build ]; then
  mkdir -p build;
fi
cd build
# g++ -Wall -std=c++17 -stdlib=libc++ ../../Version01/main.cpp ../../Version01/gl_frontEnd.cpp -lm -framework OpenGL -framework GLUT -o cell_v1
# ./cell_v1
g++ -Wall -std=c++17 -stdlib=libc++ ../../Version02/main.cpp ../../Version02/gl_frontEnd.cpp -lm -framework OpenGL -framework GLUT -o cell_v2
./cell_v2
