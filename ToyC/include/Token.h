#ifndef TOKEN_H
#define TOKEN_H

#include <string>
#include <variant>
#include <ostream>

namespace ToyC {

enum class TokenType {
    // End of file
    END,

    // Identifiers and literals
    ID,
    NUMBER,

    // Keywords
    INT,
    VOID,
    CONST,
    IF,
    ELSE,
    WHILE,
    BREAK,
    CONTINUE,
    RETURN,

    // Operators
    PLUS,       // +
    MINUS,      // -
    MULTIPLY,   // *
    DIVIDE,     // /
    MODULO,     // %
    ASSIGN,     // =
    EQ,         // ==
    NE,         // !=
    LT,         // <
    GT,         // >
    LE,         // <=
    GE,         // >=
    NOT,        // !
    AND,        // &&
    OR,         // ||

    // Delimiters
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    COMMA,      // ,
    SEMICOLON,  // ;

    // Error
    ERROR
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    int value;  // For NUMBER tokens

    Token(TokenType t, const std::string& l, int ln, int col, int val = 0)
        : type(t), lexeme(l), line(ln), column(col), value(val) {}

    bool isKeyword() const {
        return type >= TokenType::INT && type <= TokenType::RETURN;
    }

    static TokenType keywordToToken(const std::string& kw);
};

std::ostream& operator<<(std::ostream& os, const Token& tok);

} // namespace ToyC

#endif // TOKEN_H
