cmake_minimum_required(VERSION 3.20)
project(Synera)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 优先使用 Qt 自带的 MinGW 工具链（保证与 Qt 二进制 ABI 一致），不存在时回退 PATH 中的编译器
if(EXISTS "C:/Qt/Tools/mingw1310_64/bin/g++.exe")
    set(CMAKE_C_COMPILER "C:/Qt/Tools/mingw1310_64/bin/gcc.exe")
    set(CMAKE_CXX_COMPILER "C:/Qt/Tools/mingw1310_64/bin/g++.exe")
endif()

# 强制锁死MinGW生成器，彻底禁止CMake调用VS编译器
set(CMAKE_GENERATOR "MinGW Makefiles" CACHE INTERNAL "" FORCE)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)   # 编译 resources.qrc（装备图标）进可执行文件

qt_add_executable(Synera
    resources.qrc
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
    equipsynthwindow.h
    equipsynthwindow.cpp
)

target_link_libraries(Synera PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)

# 拷贝单位图片资源到构建目录，运行时按相对路径 src/unit/ 加载（Boss 立绘等）
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/src/unit/ DESTINATION ${CMAKE_BINARY_DIR}/src/unit)
