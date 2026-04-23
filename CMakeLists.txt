cmake_minimum_required(VERSION 4.1)
project(language C)

set(CMAKE_C_STANDARD 23)

add_executable(grtcmp src/main.c
        src/lexer.c
        src/lexer.h
        src/util/ArrayList.c
        src/util/ArrayList.h
        src/util/ArenaAllocator.c
        src/util/ArenaAllocator.h
        src/global.h
        src/main.c
        src/debug/lexer.c
        src/debug/lexer.h
        src/error.c
        src/error.h
)
