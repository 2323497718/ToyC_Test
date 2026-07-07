#ifndef PARSER_H
#define PARSER_H

#include "Token.h"
#include "AST.h"
#include "SymbolTable.h"
#include <vector>
#include <memory>
#include <string>
#include <optional>

namespace ToyC {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    std::unique_ptr<CompUnit> parse();

    bool hasError() const { return hadError; }
    const std::string& getErrorMessage() const { return errorMessage; }

private:
    // Helper methods
    bool check(TokenType type) const;
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token& advance();
    const Token& peek() const;
    const Token& previous() const;
    bool isAtEnd() const;
    const Token& peekNext() const;

    // Error handling
    void error(const std::string& message);
    void synchronize();

    // Parsing methods
    std::unique_ptr<ASTNode> parseCompUnit();
    std::unique_ptr<ASTNode> parseDecl();
    std::unique_ptr<ASTNode> parseConstDecl();
    std::unique_ptr<ASTNode> parseVarDecl();
    std::unique_ptr<ASTNode> parseFuncDef();
    std::unique_ptr<ASTNode> parseParam();
    std::unique_ptr<ASTNode> parseStmt();
    std::unique_ptr<ASTNode> parseBlock();
    std::unique_ptr<ASTNode> parseExpr();
    std::unique_ptr<ASTNode> parseLOrExpr();
    std::unique_ptr<ASTNode> parseLAndExpr();
    std::unique_ptr<ASTNode> parseRelExpr();
    std::unique_ptr<ASTNode> parseAddExpr();
    std::unique_ptr<ASTNode> parseMulExpr();
    std::unique_ptr<ASTNode> parseUnaryExpr();
    std::unique_ptr<ASTNode> parsePrimaryExpr();
    std::unique_ptr<ASTNode> parseCallExpr();

    const std::vector<Token>& tokens;
    size_t current;
    bool hadError;
    std::string errorMessage;
    int errorLine;
    int errorColumn;
};

} // namespace ToyC

#endif // PARSER_H
