enable_testing()

# ── Lexer tests ───────────────────────────────
add_executable(kem_test_lexer lexer/test_tokens.cpp)
target_link_libraries(kem_test_lexer PRIVATE libkem)
add_dependencies(kem_test_lexer copy_langs)
add_test(NAME lexer_tokens COMMAND kem_test_lexer
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lexer)

# ── Parser tests ──────────────────────────────
add_executable(kem_test_parser parser/test_parser.cpp)
target_link_libraries(kem_test_parser PRIVATE libkem)
add_dependencies(kem_test_parser copy_langs)
add_test(NAME parser_ast COMMAND kem_test_parser
         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/parser)

# add_executable(kem_test_semantic semantic/test_tipos.cpp)
# add_executable(kem_test_codegen  codegen/test_ir.cpp)
# add_executable(kem_test_integration integration/test_ejemplos.cpp)

add_custom_command(TARGET kem_test_parser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_BINARY_DIR}/langs
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/langs
        ${CMAKE_CURRENT_BINARY_DIR}/langs
    COMMENT "Copiando langs/ para test_parser"
)