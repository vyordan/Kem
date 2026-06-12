#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

#include "kem/Token.hpp"
#include "kem/LangConfig.hpp"
#include "kem/Lexer.hpp"
#include "kem/Parser.hpp"
#include "kem/AST.hpp"
#include "kem/ErrorHandler.hpp"

// ─────────────────────────────────────────────
//  Mini framework de tests
// ─────────────────────────────────────────────
static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    void name(); \
    struct _reg_##name { _reg_##name() { \
        ++tests_run; \
        try { name(); ++tests_passed; std::cout << "  \xE2\x9C\x93 " #name "\n"; } \
        catch (const std::exception& e) { \
            ++tests_failed; \
            std::cout << "  \xE2\x9C\x97 " #name ": " << e.what() << "\n"; \
        } \
    }} _inst_##name; \
    void name()

#define ASSERT_TRUE(expr) \
    if (!(expr)) throw std::runtime_error("ASSERT_TRUE falló: " #expr)

#define ASSERT_FALSE(expr) \
    if ((expr)) throw std::runtime_error("ASSERT_FALSE falló: " #expr)

#define ASSERT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a != _b) { \
            std::ostringstream _ss; \
            _ss << "ASSERT_EQ falló: " << _a << " != " << _b; \
            throw std::runtime_error(_ss.str()); \
        } \
    } while(0)

#define ASSERT_THROWS(expr) \
    { bool _threw = false; \
      try { expr; } catch (...) { _threw = true; } \
      if (!_threw) throw std::runtime_error("ASSERT_THROWS falló: se esperaba excepción"); }

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
static kem::LangConfig& cfg() {
    static kem::LangConfig c("langs/espanol.json");
    return c;
}

// Lexea + parsea un string completo y devuelve el Program
static std::unique_ptr<kem::Program> parse(const std::string& src) {
    kem::Lexer lexer(src, cfg());
    auto tokens = lexer.tokenize();
    kem::Parser parser(std::move(tokens));
    return parser.parse();
}

// Castea un ASTNode* al tipo T, lanza si falla
template<typename T>
T* as(kem::ASTNode* node) {
    auto* p = dynamic_cast<T*>(node);
    if (!p) throw std::runtime_error(
        std::string("Cast fallido a ") + typeid(T).name());
    return p;
}

// ─────────────────────────────────────────────
//  Tests — Programa mínimo
// ─────────────────────────────────────────────

TEST(test_programa_vacio_con_inicio) {
    auto prog = parse("inicio { }");
    ASSERT_TRUE(prog != nullptr);
    ASSERT_TRUE(prog->main_block != nullptr);
    ASSERT_EQ((int)prog->decls.size(), 0);
}

TEST(test_programa_sin_inicio_falla) {
    // Un programa sin inicio{} no tiene bloque principal
    // pero no lanza — simplemente main_block es nullptr
    auto prog = parse("funcion suma(entero a, entero b) entero { devolver a + b }");
    ASSERT_TRUE(prog->main_block == nullptr);
    ASSERT_EQ((int)prog->decls.size(), 1);
}

// ─────────────────────────────────────────────
//  Tests — Literales
// ─────────────────────────────────────────────

TEST(test_literal_entero) {
    auto prog = parse("inicio { entero x = 42 }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* lit   = as<kem::NumberLiteral>(decl->init.get());
    ASSERT_EQ(lit->value, 42LL);
}

TEST(test_literal_decimal) {
    auto prog = parse("inicio { decimal x = 3.14 }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* lit   = as<kem::FloatLiteral>(decl->init.get());
    ASSERT_TRUE(lit->value > 3.13 && lit->value < 3.15);
}

TEST(test_literal_texto) {
    auto prog = parse("inicio { texto s = \"hola\" }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* lit   = as<kem::StringLiteral>(decl->init.get());
    ASSERT_EQ(lit->value, std::string("hola"));
}

TEST(test_literal_booleano_verdadero) {
    auto prog = parse("inicio { booleano b = verdadero }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* lit   = as<kem::BoolLiteral>(decl->init.get());
    ASSERT_TRUE(lit->value);
}

TEST(test_literal_booleano_falso) {
    auto prog = parse("inicio { booleano b = falso }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* lit   = as<kem::BoolLiteral>(decl->init.get());
    ASSERT_FALSE(lit->value);
}

// ─────────────────────────────────────────────
//  Tests — Declaraciones de variables
// ─────────────────────────────────────────────

TEST(test_decl_sin_inicializador) {
    auto prog = parse("inicio { entero x }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    ASSERT_EQ(decl->name, std::string("x"));
    ASSERT_TRUE(decl->init == nullptr);
    ASSERT_EQ((int)decl->type.kind, (int)kem::TypeKind::ENTERO);
}

TEST(test_decl_arreglo) {
    auto prog = parse("inicio { entero nums[3] = [1, 2, 3] }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::ArrayDecl>(block->stmts[0].get());
    ASSERT_EQ(decl->name, std::string("nums"));
    ASSERT_TRUE(decl->type.is_array);
    ASSERT_EQ(decl->type.array_size, 3);
    ASSERT_EQ((int)decl->init.size(), 3);
}

TEST(test_decl_arreglo_sin_init) {
    auto prog = parse("inicio { entero nums[5] }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::ArrayDecl>(block->stmts[0].get());
    ASSERT_EQ(decl->type.array_size, 5);
    ASSERT_EQ((int)decl->init.size(), 0);
}

// ─────────────────────────────────────────────
//  Tests — Expresiones y precedencia
// ─────────────────────────────────────────────

TEST(test_expr_binaria_suma) {
    auto prog = parse("inicio { entero x = 2 + 3 }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* bin   = as<kem::BinaryExpr>(decl->init.get());
    ASSERT_EQ(bin->op, std::string("+"));
    as<kem::NumberLiteral>(bin->left.get());
    as<kem::NumberLiteral>(bin->right.get());
}

TEST(test_precedencia_mul_sobre_suma) {
    // 2 + 3 * 4  →  raíz es '+', hijo derecho es '*'
    auto prog = parse("inicio { entero x = 2 + 3 * 4 }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* bin   = as<kem::BinaryExpr>(decl->init.get());
    ASSERT_EQ(bin->op, std::string("+"));
    auto* right = as<kem::BinaryExpr>(bin->right.get());
    ASSERT_EQ(right->op, std::string("*"));
}

TEST(test_parentesis_cambia_precedencia) {
    // (2 + 3) * 4  →  raíz es '*'
    auto prog = parse("inicio { entero x = (2 + 3) * 4 }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* bin   = as<kem::BinaryExpr>(decl->init.get());
    ASSERT_EQ(bin->op, std::string("*"));
}

TEST(test_expr_unaria_negacion) {
    auto prog = parse("inicio { entero x = -5 }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* un    = as<kem::UnaryExpr>(decl->init.get());
    ASSERT_EQ(un->op, std::string("-"));
}

TEST(test_expr_unaria_no) {
    auto prog = parse("inicio { booleano b = no verdadero }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* un    = as<kem::UnaryExpr>(decl->init.get());
    ASSERT_EQ(un->op, std::string("no"));
}

TEST(test_expr_logica_y) {
    auto prog = parse("inicio { booleano b = verdadero y falso }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* bin   = as<kem::BinaryExpr>(decl->init.get());
    ASSERT_EQ(bin->op, std::string("y"));
}

TEST(test_expr_relacional) {
    auto prog = parse("inicio { booleano b = 3 >= 2 }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* bin   = as<kem::BinaryExpr>(decl->init.get());
    ASSERT_EQ(bin->op, std::string(">="));
}

// ─────────────────────────────────────────────
//  Tests — Asignación
// ─────────────────────────────────────────────

TEST(test_asignacion_simple) {
    auto prog = parse("inicio { entero x\n x = 10 }");
    auto* block  = as<kem::Block>(prog->main_block.get());
    auto* assign = as<kem::AssignStmt>(block->stmts[1].get());
    auto* target = as<kem::IdentExpr>(assign->target.get());
    ASSERT_EQ(target->name, std::string("x"));
}

TEST(test_asignacion_arreglo) {
    auto prog = parse("inicio { entero nums[3]\n nums[0] = 42 }");
    auto* block  = as<kem::Block>(prog->main_block.get());
    auto* assign = as<kem::AssignStmt>(block->stmts[1].get());
    as<kem::IndexExpr>(assign->target.get());
}

TEST(test_asignacion_miembro) {
    auto prog = parse("inicio { persona.edad = 25 }");
    auto* block  = as<kem::Block>(prog->main_block.get());
    auto* assign = as<kem::AssignStmt>(block->stmts[0].get());
    auto* member = as<kem::MemberExpr>(assign->target.get());
    ASSERT_EQ(member->field, std::string("edad"));
}

// ─────────────────────────────────────────────
//  Tests — Flujo de control
// ─────────────────────────────────────────────

TEST(test_si_simple) {
    auto prog = parse("inicio { si x > 0 { devolver } }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* si    = as<kem::IfStmt>(block->stmts[0].get());
    ASSERT_TRUE(si->condition != nullptr);
    ASSERT_TRUE(si->then_block != nullptr);
    ASSERT_TRUE(si->else_block == nullptr);
}

TEST(test_si_sino) {
    auto prog = parse("inicio {\n si x > 0 {\n devolver\n } sino {\n devolver\n }\n }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* si    = as<kem::IfStmt>(block->stmts[0].get());
    ASSERT_TRUE(si->else_block != nullptr);
}

TEST(test_sino_si_encadenado) {
    std::string src = R"(
inicio {
    si x > 10 {
        devolver
    } sino si x > 5 {
        devolver
    } sino {
        devolver
    }
}
)";
    auto prog  = parse(src);
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* si1  = as<kem::IfStmt>(block->stmts[0].get());
    auto* si2  = as<kem::IfStmt>(si1->else_block.get()); // sino si
    ASSERT_TRUE(si2->else_block != nullptr);              // el sino final
}

TEST(test_mientras) {
    auto prog = parse("inicio { mientras x < 10 { x = x + 1 } }");
    auto* block  = as<kem::Block>(prog->main_block.get());
    auto* w      = as<kem::WhileStmt>(block->stmts[0].get());
    ASSERT_TRUE(w->condition != nullptr);
    ASSERT_TRUE(w->body != nullptr);
}

TEST(test_hasta_sin_paso) {
    auto prog = parse("inicio { entero i\n i = 0 hasta 10 { } }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* f     = as<kem::ForStmt>(block->stmts[1].get());
    ASSERT_EQ(f->iter_var, std::string("i"));
    ASSERT_TRUE(f->step == nullptr); // paso implícito de 1
}

TEST(test_hasta_con_paso) {
    auto prog = parse("inicio { entero i\n i = 0 hasta 100 paso 5 { } }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* f     = as<kem::ForStmt>(block->stmts[1].get());
    ASSERT_TRUE(f->step != nullptr);
    auto* step = as<kem::NumberLiteral>(f->step.get());
    ASSERT_EQ(step->value, 5LL);
}

// ─────────────────────────────────────────────
//  Tests — Funciones y procedimientos
// ─────────────────────────────────────────────

TEST(test_funcion_simple) {
    auto prog = parse("funcion suma(entero a, entero b) entero { devolver a + b }\ninicio{}");
    auto* fn = as<kem::FuncDef>(prog->decls[0].get());
    ASSERT_EQ(fn->name, std::string("suma"));
    ASSERT_EQ((int)fn->params.size(), 2);
    ASSERT_EQ((int)fn->return_type.kind, (int)kem::TypeKind::ENTERO);
}

TEST(test_funcion_sin_params) {
    auto prog = parse("funcion getValor() entero { devolver 42 }\ninicio{}");
    auto* fn = as<kem::FuncDef>(prog->decls[0].get());
    ASSERT_EQ((int)fn->params.size(), 0);
}

TEST(test_procedimiento) {
    auto prog = parse("procedimiento saludar(texto nombre) { }\ninicio{}");
    auto* proc = as<kem::ProcDef>(prog->decls[0].get());
    ASSERT_EQ(proc->name, std::string("saludar"));
    ASSERT_EQ((int)proc->params.size(), 1);
}

TEST(test_parametro_por_referencia) {
    auto prog = parse("procedimiento swap(referencia entero a, referencia entero b) { }\ninicio{}");
    auto* proc = as<kem::ProcDef>(prog->decls[0].get());
    ASSERT_TRUE(proc->params[0].is_ref);
    ASSERT_TRUE(proc->params[1].is_ref);
}

TEST(test_llamada_funcion_en_expr) {
    auto prog = parse("funcion f(entero x) entero { devolver x }\ninicio { entero r = f(42) }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* call  = as<kem::CallExpr>(decl->init.get());
    ASSERT_EQ(call->callee, std::string("f"));
    ASSERT_EQ((int)call->args.size(), 1);
}

TEST(test_llamada_funcion_como_stmt) {
    auto prog = parse("inicio { imprimir(42) }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* call  = as<kem::CallExpr>(block->stmts[0].get());
    ASSERT_EQ(call->callee, std::string("imprimir"));
}

// ─────────────────────────────────────────────
//  Tests — Estructura
// ─────────────────────────────────────────────

TEST(test_estructura) {
    std::string src = R"(
estructura Punto {
    decimal x
    decimal y
}
inicio {}
)";
    auto prog = parse(src);
    auto* st  = as<kem::StructDef>(prog->decls[0].get());
    ASSERT_EQ(st->name, std::string("Punto"));
    ASSERT_EQ((int)st->fields.size(), 2);
    ASSERT_EQ(st->fields[0].name, std::string("x"));
    ASSERT_EQ(st->fields[1].name, std::string("y"));
}

// ─────────────────────────────────────────────
//  Tests — Enlazar
// ─────────────────────────────────────────────

TEST(test_enlazar) {
    auto prog = parse("enlazar vacio imprimir(texto)\ninicio{}");
    auto* lnk = as<kem::LinkDecl>(prog->decls[0].get());
    ASSERT_EQ(lnk->name, std::string("imprimir"));
    ASSERT_EQ((int)lnk->return_type.kind, (int)kem::TypeKind::VOID);
    ASSERT_EQ((int)lnk->param_types.size(), 1);
}

// ─────────────────────────────────────────────
//  Tests — Acceso a miembros e índices en expr
// ─────────────────────────────────────────────

TEST(test_acceso_miembro_en_expr) {
    auto prog = parse("inicio { entero e = persona.edad }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    auto* mem   = as<kem::MemberExpr>(decl->init.get());
    ASSERT_EQ(mem->field, std::string("edad"));
}

TEST(test_acceso_indice_en_expr) {
    auto prog = parse("inicio { entero v = nums[2] }");
    auto* block = as<kem::Block>(prog->main_block.get());
    auto* decl  = as<kem::VarDecl>(block->stmts[0].get());
    as<kem::IndexExpr>(decl->init.get());
}

// ─────────────────────────────────────────────
//  Tests — Recuperación de errores
// ─────────────────────────────────────────────

TEST(test_error_sintaxis_lanza) {
    // Una sentencia completamente inválida debe lanzar KemError
    ASSERT_THROWS(parse("inicio { @@@@ }"));
}

TEST(test_programa_completo_fibonacci) {
    std::string src = R"(
funcion fibonacci(entero n) entero {
    si n < 2 {
        devolver n
    }
    devolver fibonacci(n - 1) + fibonacci(n - 2)
}

inicio {
    entero resultado = fibonacci(10)
}
)";
    auto prog = parse(src);
    ASSERT_TRUE(prog != nullptr);
    ASSERT_EQ((int)prog->decls.size(), 1);
    ASSERT_TRUE(prog->main_block != nullptr);

    auto* fn = as<kem::FuncDef>(prog->decls[0].get());
    ASSERT_EQ(fn->name, std::string("fibonacci"));
}

TEST(test_programa_completo_estructuras) {
    std::string src = R"(
estructura Punto {
    decimal x
    decimal y
}

procedimiento mover(referencia Punto p, decimal dx, decimal dy) {
    p.x = p.x + dx
    p.y = p.y + dy
}

inicio {
    entero i
    entero suma = 0
    entero nums[5] = [10, 20, 30, 40, 50]
    i = 0 hasta 5 {
        suma = suma + nums[i]
    }
}
)";
    // Nota: Punto como tipo de parámetro es un IDENT — el semántico
    // lo resolverá. El parser lo acepta como IDENT.
    auto prog = parse(src);
    ASSERT_TRUE(prog != nullptr);
    ASSERT_EQ((int)prog->decls.size(), 2);
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main() {
    std::cout << "\n\xE2\x94\x80\xE2\x94\x80 Tests del Parser de KEM "
                 "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\n\n";

    std::cout << "\n\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"
                 "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"
                 "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\n";
    std::cout << "  Total:   " << tests_run    << "\n";
    std::cout << "  Passed:  " << tests_passed << "\n";
    std::cout << "  Failed:  " << tests_failed << "\n\n";
    return tests_failed > 0 ? 1 : 0;
}
