#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

#include "kem/LangConfig.hpp"
#include "kem/Lexer.hpp"
#include "kem/ErrorHandler.hpp"
#include "kem/Token.hpp"

//  Opciones del CLI
struct CliOptions {
    std::string source_file;
    std::string lang_file   = "langs/espanol.json"; // idioma por defecto
    bool emit_tokens        = false; // --emit-tokens: imprime los tokens y sale
    bool help               = false;
};

void printUsage(const char* program) {
    std::cerr
        << "Uso: " << program << " [opciones] archivo.kem\n\n"
        << "Opciones:\n"
        << "  --lang=<archivo.json>   Archivo de idioma (por defecto: langs/espanol.json)\n"
        << "  --emit-tokens           Imprime los tokens y termina (debug)\n"
        << "  --help                  Muestra esta ayuda\n\n"
        << "Ejemplos:\n"
        << "  " << program << " programa.kem\n"
        << "  " << program << " --lang=langs/english.json programa.kem\n"
        << "  " << program << " --emit-tokens programa.kem\n";
}

CliOptions parseArgs(int argc, char* argv[]) {
    CliOptions opts;
    std::vector<std::string> args(argv + 1, argv + argc);

    for (const auto& arg : args) {
        if (arg == "--help" || arg == "-h") {
            opts.help = true;
        } else if (arg == "--emit-tokens") {
            opts.emit_tokens = true;
        } else if (arg.substr(0, 7) == "--lang=") {
            opts.lang_file = arg.substr(7);
        } else if (!arg.empty() && arg[0] != '-') {
            opts.source_file = arg;
        } else {
            std::cerr << "Opción desconocida: '" << arg << "'\n";
            std::cerr << "Usa --help para ver las opciones disponibles.\n";
            std::exit(1);
        }
    }

    return opts;
}

//  Leer archivo completo a string
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw kem::KemError(kem::Phase::CLI,
            "No se puede abrir el archivo: '" + path + "'");
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

//  Imprimir tabla de tokens (modo --emit-tokens)
void printTokens(const std::vector<kem::Token>& tokens) {
    std::cout << "\n";
    std::cout << "┌──────────┬──────────────────────┬──────────────────────────┐\n";
    std::cout << "│  Línea   │       Tipo           │        Lexema            │\n";
    std::cout << "├──────────┼──────────────────────┼──────────────────────────┤\n";

    for (const auto& tok : tokens) {
        // Formatear línea:col
        std::string pos = std::to_string(tok.line) + ":" + std::to_string(tok.col);
        // Formatear tipo
        std::string type = kem::tokenTypeName(tok.type);
        // Lexema (escapar newlines para visualización)
        std::string lexeme = tok.lexeme;
        if (lexeme == "\n") lexeme = "↵";

        // Padding manual para alinear columnas
        auto pad = [](std::string s, int width) {
            while ((int)s.size() < width) s += ' ';
            if ((int)s.size() > width) s = s.substr(0, width - 1) + "…";
            return s;
        };

        std::cout << "│ " << pad(pos, 8)
                  << " │ " << pad(type, 20)
                  << " │ " << pad(lexeme, 24)
                  << " │\n";
    }

    std::cout << "└──────────┴──────────────────────┴──────────────────────────┘\n";
    std::cout << "  Total: " << tokens.size() << " tokens\n\n";
}

int main(int argc, char* argv[]) {
    CliOptions opts = parseArgs(argc, argv);

    if (opts.help || (opts.source_file.empty() && !opts.help)) {
        printUsage(argv[0]);
        return opts.help ? 0 : 1;
    }

    try {
        // ── Fase 0: Cargar idioma ──────────────────
        kem::LangConfig config(opts.lang_file);
        std::cout << "Idioma: " << config.langName() << "\n";

        // ── Fase 1: Leer archivo fuente ────────────
        std::string source = readFile(opts.source_file);

        // ── Fase 2: Tokenizar ──────────────────────
        kem::Lexer lexer(source, config);
        std::vector<kem::Token> tokens = lexer.tokenize();

        // ── Modo --emit-tokens ─────────────────────
        if (opts.emit_tokens) {
            printTokens(tokens);
            return 0;
        }

        // En fases posteriores, aquí vendrán:
        // Parser → SemanticAnalyzer → IRGenerator → JITEngine
        std::cout << "Lexer OK — " << tokens.size() << " tokens generados.\n";
        std::cout << "(El Parser se implementa en la Fase 2)\n";

    } catch (const kem::KemError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error interno: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
