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
        src/util/file-io/textfile.c
        src/util/file-io/textfile.h
        src/parser.c
        src/parser.h
        src/debug/parser.c
        src/debug/parser.h
        src/interpret/interpreter.c
        src/interpret/interpreter.h
        src/debug/debugInfos.c
        src/debug/debugInfos.h
        src/interpret/value.c
        src/interpret/value.h
)

target_compile_options(grtcmp
        PUBLIC -Wall
        PUBLIC -Wextra
        #PUBLIC -Werror
        #PUBLIC -Ofast
)
