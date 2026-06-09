add_executable(kem_test_lexer
    lexer/test_tokens.cpp
)

target_link_libraries(kem_test_lexer PRIVATE libkem)

add_dependencies(kem_test_lexer copy_langs)

# Registrar con CTest para correr con "ctest" o "cmake --build . --target test"
enable_testing()
add_test(
    NAME lexer_tokens
    COMMAND kem_test_lexer
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Placeholders para fases futuras — se descomientan cuando se implementen
# add_executable(kem_test_parser     parser/test_expresiones.cpp)
# add_executable(kem_test_semantic   semantic/test_tipos.cpp)
# add_executable(kem_test_codegen    codegen/test_ir.cpp)
# add_executable(kem_test_integration integration/test_ejemplos.cpp)