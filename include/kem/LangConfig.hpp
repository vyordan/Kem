#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "kem/Token.hpp"

namespace kem {
//  Se instancia una vez al arrancar el compilador.
//  Lee el archivo JSON del idioma y construye la tabla de keywords.
//  El Lexer consulta resolve() para saber si un identificador es una keyword del idioma o un nombre de variable/función.
//  Todo el resto del compilador maneja solo TokenType nunca sabe qué palabra concreta usó el programador.
class LangConfig {
public:

    // Carga y valida el archivo JSON de idioma.
    // Lanza std::runtime_error si:
    //   - el archivo no existe o no se puede leer
    //   - el JSON está malformado
    //   - falta alguna keyword obligatoria
    explicit LangConfig(const std::string& json_path);

    // Resuelve un identificador.
    // Retorna el TokenType correspondiente si es una keyword del idioma,
    // o TokenType::IDENT si es un nombre de usuario.
    TokenType resolve(const std::string& word) const;

    // Retorna la ruta del archivo cargado (para mensajes de error)
    const std::string& path() const { return path_; }

    // Retorna el nombre del idioma declarado en el JSON ("español", "english"…)
    const std::string& langName() const { return lang_name_; }

private:
    std::string path_;
    std::string lang_name_;

    // Mapa: palabra en el idioma → TokenType interno
    std::unordered_map<std::string, TokenType> keyword_map_;

    // Parsea el JSON a mano 
    void loadFromFile(const std::string& json_path);

    // Verifica que estén presentes todas las keywords obligatorias.
    void validate() const;

    // Mapea el string del JSON al TokenType correspondiente.
    static TokenType tokenTypeFromString(const std::string& name);

    // Keywords obligatorias — cualquier archivo de idioma debe definirlas
    static const std::vector<std::string>& requiredKeywords();
};

} // namespace kem