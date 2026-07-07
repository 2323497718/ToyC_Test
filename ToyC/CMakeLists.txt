cmake_minimum_required(VERSION 3.20)
project(ToyC CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
set(SOURCES
    src/main.cpp
    src/Lexer/Token.cpp
    src/Lexer/Lexer.cpp
    src/AST/AST.cpp
    src/Parser/Parser.cpp
    src/SymbolTable/SymbolTable.cpp
    src/Semantic/SemanticAnalyzer.cpp
    src/IR/IR.cpp
    src/IR/IRGenerator.cpp
    src/Optimizer/Optimizer.cpp
    src/Codegen/CodeGenerator.cpp
)

# Header files
set(HEADERS
    include/Token.h
    include/Lexer.h
    include/AST.h
    include/Parser.h
    include/SemanticAnalyzer.h
    include/IR.h
    include/IRGenerator.h
    include/Optimizer.h
    include/CodeGenerator.h
    include/SymbolTable.h
)

add_executable(toyc ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(toyc PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(toyc PRIVATE -O2 -Wall -Wextra)
else()
    target_compile_options(toyc PRIVATE -Wall -Wextra)
endif()
