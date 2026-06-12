#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "kem/LangConfig.hpp"
#include "kem/Lexer.hpp"
#include "kem/Parser.hpp"
#include "kem/ErrorHandler.hpp"
#include "kem/Token.hpp"

struct CliOptions {
    std::string source_file;
    std::string lang_file  = "langs/espanol.json";
    bool emit_tokens       = false;
    bool emit_ast          = false;   // reservado para fase futura
    bool help              = false;
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
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h")       opts.help = true;
        else if (arg == "--emit-tokens")           opts.emit_tokens = true;
        else if (arg.substr(0, 7) == "--lang=")   opts.lang_file = arg.substr(7);
        else if (!arg.empty() && arg[0] != '-')   opts.source_file = arg;
        else {
            std::cerr << "Opción desconocida: '" << arg << "'\n";
            std::exit(1);
        }
    }
    return opts;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw kem::KemError(kem::Phase::CLI, "No se puede abrir el archivo: '" + path + "'");
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void printTokens(const std::vector<kem::Token>& tokens) {
    std::cout << "\n";
    std::cout << "┌──────────┬──────────────────────┬──────────────────────────┐\n";
    std::cout << "│  Línea   │       Tipo           │        Lexema            │\n";
    std::cout << "├──────────┼──────────────────────┼──────────────────────────┤\n";

    for (const auto& tok : tokens) {
        std::string pos    = std::to_string(tok.line) + ":" + std::to_string(tok.col);
        std::string type   = kem::tokenTypeName(tok.type);
        std::string lexeme = tok.lexeme == "\n" ? "↵" : tok.lexeme;

        auto pad = [](std::string s, int w) {
            while ((int)s.size() < w) s += ' ';
            if ((int)s.size() > w) s = s.substr(0, w - 1) + "…";
            return s;
        };

        std::cout << "│ " << pad(pos, 8)
                  << " │ " << pad(type, 20)
                  << " │ " << pad(lexeme, 24) << " │\n";
    }
    std::cout << "└──────────┴──────────────────────┴──────────────────────────┘\n";
    std::cout << "  Total: " << tokens.size() << " tokens\n\n";
}

int main(int argc, char* argv[]) {
    CliOptions opts = parseArgs(argc, argv);

    if (opts.help || opts.source_file.empty()) {
        printUsage(argv[0]);
        return opts.help ? 0 : 1;
    }

    try {
        // ── Fase 0: Cargar idioma ──────────────────
        kem::LangConfig config(opts.lang_file);
        std::cout << "Idioma: " << config.langName() << "\n";

        // ── Fase 1: Leer archivo ───────────────────
        std::string source = readFile(opts.source_file);

        // ── Fase 2: Tokenizar ──────────────────────
        kem::Lexer lexer(source, config);
        auto tokens = lexer.tokenize();

        if (opts.emit_tokens) {
            printTokens(tokens);
            return 0;
        }

        // ── Fase 3: Parsear ────────────────────────
        kem::Parser parser(std::move(tokens));
        auto program = parser.parse();

        std::cout << "Parser OK\n";
        std::cout << "  Declaraciones de nivel superior: " << program->decls.size() << "\n";
        std::cout << "  Bloque inicio: " << (program->main_block ? "presente" : "ausente") << "\n";

        // Fases 4+ (Semántico, Codegen, JIT) se agregan aquí

    } catch (const kem::KemError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error interno: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
