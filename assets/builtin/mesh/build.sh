python3 -m boilermaker ./boma.hu
g++ -std=c++17 -g -O0 -Igen-cpp/overground/inc gen-cpp/overground/src/*.cpp main.cpp -obuild/bin/mesh.debug -lhumon
