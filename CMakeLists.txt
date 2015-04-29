project(mos6502)
cmake_minimum_required(VERSION 2.8)

if(MSVC)

else()
    add_definitions(-Wall -Wextra)
endif()

# COPY FILES TO BUILD DIR
set(ASM_DIR "asm_samples")
file(GLOB COPY_FILES RELATIVE ${PROJECT_SOURCE_DIR}
    "${ASM_DIR}/*.6502")

foreach(f ${COPY_FILES})
    message("COPYING ${f}\n")
    # configure_file(<in> <out> COPYONLY) where <in> is relative to the
    # project source directory and <out> is relative to the project build
    # directory. If you use absolute paths here, this won't work. Ensure
    # all your paths are relative to the source dir.
    configure_file(${f} ${f} COPYONLY)
endforeach(f)

########################
# NORMAL EXECUTABLE
########################

set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/includes)
set(LIB_DIR ${PROJECT_SOURCE_DIR}/libs)

set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(SRC_LIST
    ${SRC_DIR}/nullstream.h
    ${SRC_DIR}/cpu.h ${SRC_DIR}/cpu.cpp
    ${SRC_DIR}/reg.h
    ${SRC_DIR}/mem.h
    ${SRC_DIR}/opcodes.h ${SRC_DIR}/opcodes.cpp
    ${SRC_DIR}/assembler.h ${SRC_DIR}/assembler.cpp)
include_directories(${INCLUDE_DIR} ${SRC_DIR})

add_executable(${PROJECT_NAME} ${SRC_DIR}/main.cpp ${SRC_LIST})

########################
# ASSEMBLER EXECUTABLE
########################
set(ASSEMBLER_EXECUTABLE_NAME asm6502)
add_executable(${ASSEMBLER_EXECUTABLE_NAME} ${SRC_LIST} ${SRC_DIR}/assembler_main.cpp)

########################
# BUILD TEST EXECUTABLE
########################
set(TEST_SRC_DIR ${PROJECT_SOURCE_DIR}/tests)
set(TEST_SRC_LIST
    ${TEST_SRC_DIR}/assembler_fixtures.h ${TEST_SRC_DIR}/assembler_fixtures.cpp
    ${TEST_SRC_DIR}/test_assembler.cpp
    ${TEST_SRC_DIR}/test_main.cpp
    ${TEST_SRC_DIR}/test_cpu.cpp)
set(TEST_MAIN_NAME "${PROJECT_NAME}_test")
include_directories(${TEST_SRC_DIR})

add_executable(${TEST_MAIN_NAME} ${SRC_LIST} ${TEST_SRC_LIST} ${TEST_SRC_LIST})

# find libgtest.a or similar
find_library(GTEST
    NAMES gtest
    PATHS ${LIB_DIR})
# find libgtest_main.a or similar
find_library(GTEST_MAIN
    NAMES gtest_main
    PATHS ${LIB_DIR})
target_link_libraries(${TEST_MAIN_NAME} ${GTEST} ${GTEST_MAIN})


