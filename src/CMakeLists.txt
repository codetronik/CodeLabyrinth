cmake_minimum_required(VERSION 3.13.5)

set(COMPILER ${CMAKE_C_COMPILER} CACHE STRING "")
set(CMAKE_C_COMPILER ${COMPILER})
set(CMAKE_CXX_COMPILER ${COMPILER})
message("Clang path : ${CMAKE_CXX_COMPILER}")

project(CodeLabyrinth)

find_package(LLVM PATHS "${LLVM_DIR}" REQUIRED CONFIG NO_DEFAULT_PATH)

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
link_directories(${LLVM_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

add_library(CodeLabyrinth SHARED RegisterPass.cpp)

target_include_directories(CodeLabyrinth PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(CodeLabyrinth PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

llvm_map_components_to_libnames(LLVM_LIBRARIES Core Support TransformUtils)
target_link_libraries(CodeLabyrinth PRIVATE ${LLVM_LIBRARIES})

add_subdirectory(Common)
add_subdirectory(FakeInstructionInserter)
add_subdirectory(DynamicCallConverter)
add_subdirectory(BranchAddressEncryptor)
add_subdirectory(StringEncryptor)