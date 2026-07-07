#ifndef LEXER_H
#define LEXER_H

#include "Token.h"
#include <vector>
#include <memory>

namespace ToyC {

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    void scanToken();
    void skipWhitespace();
    void skipComment();
    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool isAtEnd() const;
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;

    void makeToken(TokenType type);
    void makeToken(TokenType type, int value);
    void makeNumber();
    void makeIdentifier();

    void error(const std::string& message);

    const std::string& source;
    size_t current;
    size_t start;
    int line;
    int column;
    std::vector<Token> tokens;
    bool hadError;
    std::string errorMessage;
};

} // namespace ToyC

#endif // LEXER_H
