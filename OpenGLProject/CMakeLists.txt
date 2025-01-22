# Setze die minimale CMake-Version
cmake_minimum_required(VERSION 3.15)

# Definiere das Projekt
project(MyPongGame)

# C++ Standard festlegen
set(CMAKE_CXX_STANDARD 17)

# Setze den vcpkg-Toolchain-File-Pfad
set(CMAKE_TOOLCHAIN_FILE "C:/Pfad/zu/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Füge Quellcode-Dateien hinzu
file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

# Erstelle das Executable
add_executable(MyPongGame ${SOURCES} ${HEADERS})

# Finde die installierten vcpkg-Pakete und linke sie
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glew REQUIRED)

# Verlinke die gefundenen Bibliotheken
target_link_libraries(MyPongGame PRIVATE glm::glm glfw3 GLEW::GLEW)
