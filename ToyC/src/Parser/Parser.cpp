#include "Parser.h"
#include <stdexcept>

namespace ToyC {

Parser::Parser(const std::vector<Token>& tokens)
    : tokens(tokens), current(0), hadError(false), errorLine(0), errorColumn(0) {}

std::unique_ptr<CompUnit> Parser::parse() {
    auto unit = std::make_unique<CompUnit>();
    
    while (!isAtEnd()) {
        auto decl = parseDecl();
        if (decl) {
            unit->addDeclaration(std::move(decl));
        } else {
            break;
        }
    }
    
    return unit;
}

// ============== Helper Methods ==============

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return const_cast<Token&>(tokens[current - 1]);
}

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::previous() const {
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END;
}

void Parser::error(const std::string& message) {
    hadError = true;
    errorMessage = message + " at line " + std::to_string(peek().line) + 
                   ", column " + std::to_string(peek().column);
    throw std::runtime_error(errorMessage);
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == TokenType::SEMICOLON) return;
        switch (peek().type) {
            case TokenType::INT:
            case TokenType::VOID:
            case TokenType::CONST:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::RETURN:
            case TokenType::BREAK:
            case TokenType::CONTINUE:
            case TokenType::LBRACE:
                return;
            default:
                advance();
        }
    }
}

// ============== Parse Methods ==============

std::unique_ptr<ASTNode> Parser::parseCompUnit() {
    return parseDecl();
}

std::unique_ptr<ASTNode> Parser::parseDecl() {
    // Try to parse const declaration
    if (match(TokenType::CONST)) {
        return parseConstDecl();
    }
    
    // Try to parse variable declaration
    if (match(TokenType::INT)) {
        if (check(TokenType::ID) && peekNext().type == TokenType::LPAREN) {
            // This is a function definition (int func ...)
            current--;
            return parseFuncDef();
        } else if (check(TokenType::ID)) {
            return parseVarDecl();
        } else {
            error("Expected identifier after 'int'");
            return nullptr;
        }
    }
    
    // Try to parse void function definition
    if (match(TokenType::VOID)) {
        if (!check(TokenType::ID)) {
            error("Expected identifier after 'void'");
            return nullptr;
        }
        current--;
        return parseFuncDef();
    }
    
    // Check for function definition without type (shouldn't happen in valid ToyC)
    if (check(TokenType::ID) && peekNext().type == TokenType::LPAREN) {
        error("Function must have return type");
        return nullptr;
    }
    
    return nullptr;
}

std::unique_ptr<ASTNode> Parser::parseConstDecl() {
    // Expect "int"
    if (!match(TokenType::INT)) {
        error("Expected 'int' in constant declaration");
        return nullptr;
    }
    
    // Expect identifier
    if (!check(TokenType::ID)) {
        error("Expected identifier in constant declaration");
        return nullptr;
    }
    std::string name = peek().lexeme;
    advance();
    
    // Expect "="
    if (!match(TokenType::ASSIGN)) {
        error("Expected '=' in constant declaration");
        return nullptr;
    }
    
    // Parse initializer expression
    auto value = parseExpr();
    if (!value) {
        error("Expected expression in constant declaration");
        return nullptr;
    }
    
    // Expect ";"
    if (!match(TokenType::SEMICOLON)) {
        error("Expected ';' after constant declaration");
        return nullptr;
    }
    
    // Evaluate constant at compile time
    int constValue = 0;
    try {
        if (value->getNodeType() == ASTNodeType::NumberLiteral) {
            constValue = static_cast<NumberLiteral*>(value.get())->value;
        } else {
            // For non-constant expressions, we'll evaluate in semantic analyzer
            constValue = 0;
        }
    } catch (...) {
        constValue = 0;
    }
    
    return std::make_unique<ConstDecl>(name, std::move(value), constValue);
}

std::unique_ptr<ASTNode> Parser::parseVarDecl() {
    // Already matched "int"
    
    // Expect identifier
    if (!check(TokenType::ID)) {
        error("Expected identifier in variable declaration");
        return nullptr;
    }
    std::string name = peek().lexeme;
    advance();
    
    // Expect "="
    if (!match(TokenType::ASSIGN)) {
        error("Expected '=' in variable declaration");
        return nullptr;
    }
    
    // Parse initializer expression
    auto initializer = parseExpr();
    if (!initializer) {
        error("Expected expression in variable declaration");
        return nullptr;
    }
    
    // Expect ";"
    if (!match(TokenType::SEMICOLON)) {
        error("Expected ';' after variable declaration");
        return nullptr;
    }
    
    return std::make_unique<VarDecl>(name, std::move(initializer));
}

const Token& Parser::peekNext() const {
    if (current + 1 >= tokens.size()) {
        static Token endToken(TokenType::END, "", 0, 0);
        return endToken;
    }
    return tokens[current + 1];
}

std::unique_ptr<ASTNode> Parser::parseFuncDef() {
    bool isVoid = false;
    
    // Parse return type
    if (match(TokenType::INT)) {
        isVoid = false;
    } else if (match(TokenType::VOID)) {
        isVoid = true;
    } else {
        error("Expected 'int' or 'void' in function definition");
        return nullptr;
    }
    
    // Expect identifier
    if (!check(TokenType::ID)) {
        error("Expected function name");
        return nullptr;
    }
    std::string name = peek().lexeme;
    advance();
    
    // Expect "("
    if (!match(TokenType::LPAREN)) {
        error("Expected '(' after function name");
        return nullptr;
    }
    
    // Parse parameters
    std::vector<std::string> params;
    if (!check(TokenType::RPAREN)) {
        do {
            // Expect "int"
            if (!match(TokenType::INT)) {
                error("Expected 'int' in parameter list");
                return nullptr;
            }
            
            // Expect identifier
            if (!check(TokenType::ID)) {
                error("Expected parameter name");
                return nullptr;
            }
            params.push_back(peek().lexeme);
            advance();
        } while (match(TokenType::COMMA));
    }
    
    // Expect ")"
    if (!match(TokenType::RPAREN)) {
        error("Expected ')' after parameter list");
        return nullptr;
    }
    
    // Parse function body
    auto body = parseBlock();
    if (!body) {
        error("Expected function body");
        return nullptr;
    }
    
    return std::make_unique<FuncDef>(name, params, std::move(body), isVoid);
}

std::unique_ptr<ASTNode> Parser::parseParam() {
    // Expect "int"
    if (!match(TokenType::INT)) {
        error("Expected 'int' in parameter");
        return nullptr;
    }
    
    // Expect identifier
    if (!check(TokenType::ID)) {
        error("Expected parameter name");
        return nullptr;
    }
    std::string name = peek().lexeme;
    advance();
    
    return std::make_unique<Identifier>(name);
}

std::unique_ptr<ASTNode> Parser::parseStmt() {
    // Block statement
    if (check(TokenType::LBRACE)) {
        return parseBlock();
    }
    
    // Empty statement
    if (match(TokenType::SEMICOLON)) {
        return std::make_unique<ExprStmt>(nullptr);
    }
    
    // Return statement
    if (match(TokenType::RETURN)) {
        if (match(TokenType::SEMICOLON)) {
            return std::make_unique<ReturnStmt>();
        }
        auto value = parseExpr();
        if (!match(TokenType::SEMICOLON)) {
            error("Expected ';' after return value");
        }
        return std::make_unique<ReturnStmt>(std::move(value));
    }
    
    // If statement
    if (match(TokenType::IF)) {
        if (!match(TokenType::LPAREN)) {
            error("Expected '(' after 'if'");
        }
        auto condition = parseExpr();
        if (!condition) {
            error("Expected condition in 'if' statement");
        }
        if (!match(TokenType::RPAREN)) {
            error("Expected ')' after condition");
        }
        
        auto thenBranch = parseStmt();
        if (!thenBranch) {
            error("Expected statement in 'if' branch");
        }
        
        std::unique_ptr<ASTNode> elseBranch;
        if (match(TokenType::ELSE)) {
            elseBranch = parseStmt();
            if (!elseBranch) {
                error("Expected statement in 'else' branch");
            }
        }
        
        return std::make_unique<IfStmt>(std::move(condition), 
                                        std::move(thenBranch),
                                        std::move(elseBranch));
    }
    
    // While statement
    if (match(TokenType::WHILE)) {
        if (!match(TokenType::LPAREN)) {
            error("Expected '(' after 'while'");
        }
        auto condition = parseExpr();
        if (!condition) {
            error("Expected condition in 'while' statement");
        }
        if (!match(TokenType::RPAREN)) {
            error("Expected ')' after condition");
        }
        
        auto body = parseStmt();
        if (!body) {
            error("Expected loop body");
        }
        
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }
    
    // Break statement
    if (match(TokenType::BREAK)) {
        if (!match(TokenType::SEMICOLON)) {
            error("Expected ';' after 'break'");
        }
        return std::make_unique<BreakStmt>();
    }
    
    // Continue statement
    if (match(TokenType::CONTINUE)) {
        if (!match(TokenType::SEMICOLON)) {
            error("Expected ';' after 'continue'");
        }
        return std::make_unique<ContinueStmt>();
    }
    
    // Const declaration
    if (match(TokenType::CONST)) {
        return parseConstDecl();
    }
    
    // Int declaration (variable declaration)
    if (match(TokenType::INT)) {
        if (!check(TokenType::ID)) {
            error("Expected identifier after 'int'");
            return nullptr;
        }
        current--;
        return parseVarDecl();
    }
    
    // Assignment statement
    if (check(TokenType::ID)) {
        std::string varName = peek().lexeme;
        if (peekNext().type == TokenType::ASSIGN) {
            advance(); advance();
            auto value = parseExpr();
            if (!match(TokenType::SEMICOLON)) {
                error("Expected ';' after assignment");
            }
            return std::make_unique<AssignStmt>(varName, std::move(value));
        } else if (peekNext().type == TokenType::LPAREN) {
            // Function call
            auto call = parseCallExpr();
            if (!match(TokenType::SEMICOLON)) {
                error("Expected ';' after expression statement");
            }
            return std::make_unique<ExprStmt>(std::move(call));
        }
    }
    
    // Expression statement
    auto expr = parseExpr();
    if (!match(TokenType::SEMICOLON)) {
        error("Expected ';' after expression");
    }
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<ASTNode> Parser::parseCallExpr() {
    if (!check(TokenType::ID)) {
        error("Expected function name");
        return nullptr;
    }
    std::string callee = peek().lexeme;
    advance();
    
    if (!match(TokenType::LPAREN)) {
        error("Expected '(' in function call");
        return nullptr;
    }
    
    std::vector<std::unique_ptr<ASTNode>> args;
    if (!check(TokenType::RPAREN)) {
        do {
            auto arg = parseExpr();
            if (!arg) {
                error("Expected argument expression");
                break;
            }
            args.push_back(std::move(arg));
        } while (match(TokenType::COMMA));
    }
    
    if (!match(TokenType::RPAREN)) {
        error("Expected ')' after arguments");
    }
    
    return std::make_unique<CallExpr>(callee, std::move(args));
}

std::unique_ptr<ASTNode> Parser::parseBlock() {
    if (!match(TokenType::LBRACE)) {
        error("Expected '{'");
        return nullptr;
    }
    
    auto block = std::make_unique<BlockStmt>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        auto stmt = parseDecl();
        if (stmt) {
            block->addStatement(std::move(stmt));
        } else {
            stmt = parseStmt();
            if (stmt) {
                block->addStatement(std::move(stmt));
            }
        }
    }
    
    if (!match(TokenType::RBRACE)) {
        error("Expected '}'");
    }
    
    return std::move(block);
}

std::unique_ptr<ASTNode> Parser::parseExpr() {
    return parseLOrExpr();
}

std::unique_ptr<ASTNode> Parser::parseLOrExpr() {
    auto left = parseLAndExpr();
    if (!left) return nullptr;
    
    while (match(TokenType::OR)) {
        auto right = parseLAndExpr();
        if (!right) {
            error("Expected expression after '||'");
            return nullptr;
        }
        left = std::make_unique<BinaryExpr>("||", std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseLAndExpr() {
    auto left = parseRelExpr();
    if (!left) return nullptr;
    
    while (match(TokenType::AND)) {
        auto right = parseRelExpr();
        if (!right) {
            error("Expected expression after '&&'");
            return nullptr;
        }
        left = std::make_unique<BinaryExpr>("&&", std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseRelExpr() {
    auto left = parseAddExpr();
    if (!left) return nullptr;
    
    static const std::vector<TokenType> relOps = {
        TokenType::LT, TokenType::GT, TokenType::LE, TokenType::GE, 
        TokenType::EQ, TokenType::NE
    };
    
    while (match(relOps)) {
        std::string op;
        switch (previous().type) {
            case TokenType::LT: op = "<"; break;
            case TokenType::GT: op = ">"; break;
            case TokenType::LE: op = "<="; break;
            case TokenType::GE: op = ">="; break;
            case TokenType::EQ: op = "=="; break;
            case TokenType::NE: op = "!="; break;
            default: op = ""; break;
        }
        
        auto right = parseAddExpr();
        if (!right) {
            error("Expected expression after relational operator");
            return nullptr;
        }
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseAddExpr() {
    auto left = parseMulExpr();
    if (!left) return nullptr;
    
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        std::string op = (previous().type == TokenType::PLUS) ? "+" : "-";
        auto right = parseMulExpr();
        if (!right) {
            error("Expected expression after additive operator");
            return nullptr;
        }
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseMulExpr() {
    auto left = parseUnaryExpr();
    if (!left) return nullptr;
    
    while (match({TokenType::MULTIPLY, TokenType::DIVIDE, TokenType::MODULO})) {
        std::string op;
        switch (previous().type) {
            case TokenType::MULTIPLY: op = "*"; break;
            case TokenType::DIVIDE: op = "/"; break;
            case TokenType::MODULO: op = "%"; break;
            default: op = ""; break;
        }
        
        auto right = parseUnaryExpr();
        if (!right) {
            error("Expected expression after multiplicative operator");
            return nullptr;
        }
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseUnaryExpr() {
    if (match({TokenType::PLUS, TokenType::MINUS, TokenType::NOT})) {
        std::string op;
        switch (previous().type) {
            case TokenType::PLUS: op = "+"; break;
            case TokenType::MINUS: op = "-"; break;
            case TokenType::NOT: op = "!"; break;
            default: op = ""; break;
        }
        
        auto operand = parseUnaryExpr();
        if (!operand) {
            error("Expected expression after unary operator");
            return nullptr;
        }
        return std::make_unique<UnaryExpr>(op, std::move(operand));
    }
    
    return parsePrimaryExpr();
}

std::unique_ptr<ASTNode> Parser::parsePrimaryExpr() {
    // Number literal
    if (match(TokenType::NUMBER)) {
        return std::make_unique<NumberLiteral>(previous().value);
    }
    
    // Identifier or function call
    if (match(TokenType::ID)) {
        std::string name = previous().lexeme;
        
        // Check if it's a function call
        if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<ASTNode>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    auto arg = parseExpr();
                    if (!arg) {
                        error("Expected argument expression");
                        break;
                    }
                    args.push_back(std::move(arg));
                } while (match(TokenType::COMMA));
            }
            
            if (!match(TokenType::RPAREN)) {
                error("Expected ')' after arguments");
            }
            
            return std::make_unique<CallExpr>(name, std::move(args));
        }
        
        return std::make_unique<Identifier>(name);
    }
    
    // Parenthesized expression
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpr();
        if (!expr) {
            error("Expected expression");
            return nullptr;
        }
        if (!match(TokenType::RPAREN)) {
            error("Expected ')' after expression");
        }
        return expr;
    }
    
    error("Unexpected token in expression");
    return nullptr;
}

} // namespace ToyC
