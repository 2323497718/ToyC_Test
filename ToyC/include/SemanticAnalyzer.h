#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "AST.h"
#include "SymbolTable.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <optional>

namespace ToyC {

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer();

    bool analyze(CompUnit* root);

    bool hasError() const { return hadError; }
    const std::string& getErrorMessage() const { return errorMessage; }

private:
    // Symbol table management
    SymbolTable symbolTable;
    int loopDepth;  // Track nested loops for break/continue

    // Function tracking
    std::string currentFunction;
    bool currentFunctionReturnsValue;
    bool functionHasReturn;

    // Error reporting
    void reportError(const std::string& message, int line = 0, int col = 0);
    void reportError(const std::string& message, ASTNode* node);

    // AST traversal methods
    void visitCompUnit(CompUnit* node);
    void visitFuncDef(FuncDef* node);
    void visitBlock(BlockStmt* node);
    void visitStmt(ASTNode* node);
    void visitReturnStmt(ReturnStmt* node);
    void visitIfStmt(IfStmt* node);
    void visitWhileStmt(WhileStmt* node);
    void visitBreakStmt(BreakStmt* node);
    void visitContinueStmt(ContinueStmt* node);
    void visitVarDecl(VarDecl* node);
    void visitConstDecl(ConstDecl* node);
    void visitAssignStmt(AssignStmt* node);
    void visitExprStmt(ExprStmt* node);

    // Expression analysis
    int evaluateConstantExpr(ASTNode* node);
    int evaluateBinaryExpr(BinaryExpr* node);
    int evaluateUnaryExpr(UnaryExpr* node);
    int evaluateNumberLiteral(NumberLiteral* node);
    int evaluateIdentifier(Identifier* node);
    int evaluateCallExpr(CallExpr* node);

    ASTNode* getExprNode(ASTNode* node);

    bool hadError;
    std::string errorMessage;
    std::unordered_set<std::string> functionsWithReturn;
};

} // namespace ToyC

#endif // SEMANTIC_ANALYZER_H
