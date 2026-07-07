#include "Lexer.h"
#include <cctype>
#include <stdexcept>

namespace ToyC {

Lexer::Lexer(const std::string& source)
    : source(source), current(0), start(0), line(1), column(1), hadError(false) {}

std::vector<Token> Lexer::tokenize() {
    tokens.clear();
    hadError = false;
    current = 0;
    start = 0;
    line = 1;
    column = 1;
    
    while (!isAtEnd()) {
        start = current;
        scanToken();
        if (hadError) {
            throw std::runtime_error("Lexer error: " + errorMessage);
        }
    }
    
    // Add end-of-file token
    makeToken(TokenType::END);
    return tokens;
}

void Lexer::scanToken() {
    char c = advance();
    
    switch (c) {
        // Single character tokens
        case '(': makeToken(TokenType::LPAREN); break;
        case ')': makeToken(TokenType::RPAREN); break;
        case '{': makeToken(TokenType::LBRACE); break;
        case '}': makeToken(TokenType::RBRACE); break;
        case ',': makeToken(TokenType::COMMA); break;
        case ';': makeToken(TokenType::SEMICOLON); break;
        case '+': makeToken(TokenType::PLUS); break;
        case '-': makeToken(TokenType::MINUS); break;
        case '*': makeToken(TokenType::MULTIPLY); break;
        
        // Two character tokens
        case '/':
            if (match('/')) {
                // Single-line comment
                while (peek() != '\n' && !isAtEnd()) advance();
            } else if (match('*')) {
                // Multi-line comment
                skipComment();
            } else {
                makeToken(TokenType::DIVIDE);
            }
            break;
        
        case '=':
            makeToken(match('=') ? TokenType::EQ : TokenType::ASSIGN);
            break;
        
        case '!':
            if (match('=')) {
                makeToken(TokenType::NE);
            } else {
                makeToken(TokenType::NOT);
            }
            break;
        
        case '<':
            makeToken(match('=') ? TokenType::LE : TokenType::LT);
            break;
        
        case '>':
            makeToken(match('=') ? TokenType::GE : TokenType::GT);
            break;
        
        case '&':
            if (match('&')) {
                makeToken(TokenType::AND);
            } else {
                error("Unexpected character '&'");
            }
            break;
        
        case '|':
            if (match('|')) {
                makeToken(TokenType::OR);
            } else {
                error("Unexpected character '|'");
            }
            break;
        
        case '%':
            makeToken(TokenType::MODULO);
            break;
        
        // Skip whitespace
        case ' ':
        case '\r':
        case '\t':
            skipWhitespace();
            break;
        
        case '\n':
            line++;
            column = 1;
            break;
        
        default:
            if (isDigit(c)) {
                makeNumber();
            } else if (isAlpha(c)) {
                makeIdentifier();
            } else {
                error("Unexpected character '" + std::string(1, c) + "'");
            }
            break;
    }
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && (peek() == ' ' || peek() == '\r' || peek() == '\t')) {
        advance();
    }
}

void Lexer::skipComment() {
    int nested = 1;
    while (!isAtEnd() && nested > 0) {
        if (peek() == '*' && peekNext() == '/') {
            advance(); advance();
            nested--;
        } else if (peek() == '/' && peekNext() == '*') {
            advance(); advance();
            nested++;
        } else {
            if (peek() == '\n') {
                line++;
                column = 1;
            }
            advance();
        }
    }
    if (nested > 0) {
        error("Unterminated multi-line comment");
    }
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source[current];
}

char Lexer::peekNext() const {
    if (current + 1 >= source.size()) return '\0';
    return source[current + 1];
}

char Lexer::advance() {
    char c = source[current];
    current++;
    column++;
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source[current] != expected) return false;
    current++;
    column++;
    return true;
}

bool Lexer::isAtEnd() const {
    return current >= source.size();
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

void Lexer::makeToken(TokenType type) {
    std::string lexeme = source.substr(start, current - start);
    tokens.emplace_back(type, lexeme, line, column - static_cast<int>(lexeme.length()));
}

void Lexer::makeToken(TokenType type, int value) {
    std::string lexeme = source.substr(start, current - start);
    tokens.emplace_back(type, lexeme, line, column - static_cast<int>(lexeme.length()), value);
}

void Lexer::makeNumber() {
    while (isDigit(peek())) {
        advance();
    }
    
    std::string numStr = source.substr(start, current - start);
    try {
        int value = std::stoi(numStr);
        makeToken(TokenType::NUMBER, value);
    } catch (...) {
        error("Invalid number: " + numStr);
    }
}

void Lexer::makeIdentifier() {
    while (isAlphaNumeric(peek())) {
        advance();
    }
    
    std::string text = source.substr(start, current - start);
    TokenType type = Token::keywordToToken(text);
    makeToken(type);
}

void Lexer::error(const std::string& message) {
    errorMessage = message + " at line " + std::to_string(line) + ", column " + std::to_string(column);
    hadError = true;
}

} // namespace ToyC
