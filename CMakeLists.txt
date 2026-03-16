cmake_minimum_required(VERSION 3.10)
project(LangItCompiler VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(${PROJECT_NAME} src/main.cpp)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "compiler"
    PREFIX ""
)

target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:-g -DDEBUG>
    $<$<CONFIG:Release>:-O2 -DNDEBUG>
    -Wall -Wextra
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

enable_testing()
add_test(NAME help_test COMMAND ${PROJECT_NAME})
set_tests_properties(help_test PROPERTIES WILL_FAIL TRUE)