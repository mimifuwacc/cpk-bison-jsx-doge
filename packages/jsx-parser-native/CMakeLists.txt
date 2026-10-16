find_package(BISON 3.8 REQUIRED)

BISON_TARGET(
  JsxGrammar
  src/parser.y
  ${CMAKE_CURRENT_BINARY_DIR}/generated/parser.cpp
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/generated/parser.hpp
)

add_library(jsx_parser
  src/ast.cpp
  src/driver.cpp
  src/lexer.cpp
  ${BISON_JsxGrammar_OUTPUTS}
)
set_target_properties(jsx_parser PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(jsx_parser
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_compile_options(jsx_parser PRIVATE -Wall -Wextra -Wpedantic)

find_program(NODE_EXECUTABLE node REQUIRED)
execute_process(
  COMMAND ${NODE_EXECUTABLE} -p
    "require('node:path').resolve(process.execPath, '../../include/node')"
  OUTPUT_VARIABLE NODE_API_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)

add_library(jsx_parser_node MODULE src/node_addon.cpp)
target_link_libraries(jsx_parser_node PRIVATE jsx_parser)
target_include_directories(jsx_parser_node PRIVATE ${NODE_API_INCLUDE_DIR})
target_compile_definitions(jsx_parser_node PRIVATE NAPI_VERSION=10)
set_target_properties(jsx_parser_node PROPERTIES
  PREFIX ""
  SUFFIX ".node"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/node-addon"
)
if(APPLE)
  target_link_options(jsx_parser_node PRIVATE "-undefined" "dynamic_lookup")
endif()

add_executable(jsx-parser src/main.cpp)
target_link_libraries(jsx-parser PRIVATE jsx_parser)
set_target_properties(jsx-parser PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

add_executable(jsx_parser_tests tests/parser_test.cpp)
target_link_libraries(jsx_parser_tests PRIVATE jsx_parser)
add_test(NAME jsx_parser_tests COMMAND jsx_parser_tests)
