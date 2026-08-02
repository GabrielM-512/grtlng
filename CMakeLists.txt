cmake_minimum_required(VERSION 4.1)
project(grtlng C)

include(CTest)

set(CMAKE_C_STANDARD 23)

add_executable(grtlng
        src/main.c
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
        src/parser/parser.c
        src/parser/parser.h
        src/debug/parser.c
        src/debug/parser.h
        src/interpret/interpreter.c
        src/interpret/interpreter.h
        src/debug/debugInfos.c
        src/debug/debugInfos.h
        src/interpret/value.c
        src/interpret/value.h
        src/util/HashMap.c
        src/util/HashMap.h
        src/parser/globalScope.c
        src/parser/globalScope.h
        src/parser/parseUtils.c
        src/parser/parseUtils.h
        src/parser/expressions.c
        src/parser/expressions.h
        src/parser/scoping.c
        src/parser/scoping.h
        src/parser/statements.c
        src/parser/statements.h
        src/parser/resolving.c
        src/parser/resolving.h
)

target_compile_options(grtlng
        PUBLIC -Wall
        PUBLIC -Wextra
        #PUBLIC -Werror
        #PUBLIC -Ofast
)

enable_testing()

add_test(NAME End-To-End COMMAND python ../testing/end-to-end.py ../testing/config.json multi)