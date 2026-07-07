#include "Token.h"
#include <unordered_map>

namespace ToyC {

TokenType Token::keywordToToken(const std::string& kw) {
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"int", TokenType::INT},
        {"void", TokenType::VOID},
        {"const", TokenType::CONST},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"while", TokenType::WHILE},
        {"break", TokenType::BREAK},
        {"continue", TokenType::CONTINUE},
        {"return", TokenType::RETURN}
    };
    
    auto it = keywords.find(kw);
    if (it != keywords.end()) {
        return it->second;
    }
    return TokenType::ID;
}

std::ostream& operator<<(std::ostream& os, const Token& tok) {
    os << "Token(";
    switch (tok.type) {
        case TokenType::END: os << "END"; break;
        case TokenType::ID: os << "ID"; break;
        case TokenType::NUMBER: os << "NUMBER"; break;
        case TokenType::INT: os << "INT"; break;
        case TokenType::VOID: os << "VOID"; break;
        case TokenType::CONST: os << "CONST"; break;
        case TokenType::IF: os << "IF"; break;
        case TokenType::ELSE: os << "ELSE"; break;
        case TokenType::WHILE: os << "WHILE"; break;
        case TokenType::BREAK: os << "BREAK"; break;
        case TokenType::CONTINUE: os << "CONTINUE"; break;
        case TokenType::RETURN: os << "RETURN"; break;
        case TokenType::PLUS: os << "PLUS"; break;
        case TokenType::MINUS: os << "MINUS"; break;
        case TokenType::MULTIPLY: os << "MULTIPLY"; break;
        case TokenType::DIVIDE: os << "DIVIDE"; break;
        case TokenType::MODULO: os << "MODULO"; break;
        case TokenType::ASSIGN: os << "ASSIGN"; break;
        case TokenType::EQ: os << "EQ"; break;
        case TokenType::NE: os << "NE"; break;
        case TokenType::LT: os << "LT"; break;
        case TokenType::GT: os << "GT"; break;
        case TokenType::LE: os << "LE"; break;
        case TokenType::GE: os << "GE"; break;
        case TokenType::NOT: os << "NOT"; break;
        case TokenType::AND: os << "AND"; break;
        case TokenType::OR: os << "OR"; break;
        case TokenType::LPAREN: os << "LPAREN"; break;
        case TokenType::RPAREN: os << "RPAREN"; break;
        case TokenType::LBRACE: os << "LBRACE"; break;
        case TokenType::RBRACE: os << "RBRACE"; break;
        case TokenType::COMMA: os << "COMMA"; break;
        case TokenType::SEMICOLON: os << "SEMICOLON"; break;
        case TokenType::ERROR: os << "ERROR"; break;
    }
    os << ", \"" << tok.lexeme << "\", " << tok.line << ":" << tok.column;
    if (tok.type == TokenType::NUMBER) {
        os << ", value=" << tok.value;
    }
    os << ")";
    return os;
}

} // namespace ToyC
