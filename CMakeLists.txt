cmake_minimum_required(VERSION 3.20)
project(Synera)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 强制锁死你实际在用的MinGW编译器
set(CMAKE_C_COMPILER "C:/mingw64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/mingw64/bin/g++.exe")

# 强制锁死MinGW生成器，彻底禁止CMake调用VS编译器
set(CMAKE_GENERATOR "MinGW Makefiles" CACHE INTERNAL "" FORCE)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
qt_standard_project_setup()

qt_add_executable(Synera
    main.cpp
    synera.h
    synera.cpp
    synera.ui
    unit.h
    unit.cpp
    hero.h
    hero.cpp
    enemy.h
    enemy.cpp
    weapon.h
    weapon.cpp
    board.h
    board.cpp
)

target_link_libraries(Synera PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)
