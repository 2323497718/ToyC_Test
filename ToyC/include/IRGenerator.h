#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include "AST.h"
#include "IR.h"
#include "SymbolTable.h"
#include <string>
#include <unordered_map>
#include <stack>

namespace ToyC {

class IRGenerator {
public:
    explicit IRGenerator();

    std::unique_ptr<IRProgram> generate(CompUnit* ast);

    bool hasError() const { return hadError; }
    const std::string& getErrorMessage() const { return errorMessage; }

private:
    std::unique_ptr<IRProgram> program;
    SymbolTable symbolTable;
    std::unordered_map<std::string, Function*> functions;
    Function* currentFunction;
    BasicBlock* currentBlock;

    // Break/continue stack for loops
    struct LoopContext {
        BasicBlock* breakBlock;
        BasicBlock* continueBlock;
    };
    std::stack<LoopContext> loopStack;

    std::string currentFunctionName;
    bool functionReturnsValue;
    bool hasReturned;  // Track if current function has returned

    // Helper methods
    Operand emit(IROpcode op, const Operand& result, const std::vector<Operand>& ops = {});
    Operand emit(IROpcode op, const std::vector<Operand>& ops = {});
    Operand emitLabel(const std::string& name);

    // Expression generation
    Operand generateExpr(ASTNode* node);

    // Statement generation
    void generateStmt(ASTNode* node);
    void generateCompUnit(CompUnit* node);
    void generateFuncDef(FuncDef* node);
    void generateBlock(BlockStmt* node);
    void generateReturnStmt(ReturnStmt* node);
    void generateIfStmt(IfStmt* node);
    void generateWhileStmt(WhileStmt* node);
    void generateBreakStmt(BreakStmt* node);
    void generateContinueStmt(ContinueStmt* node);
    void generateVarDecl(VarDecl* node);
    void generateConstDecl(ConstDecl* node);
    void generateAssignStmt(AssignStmt* node);
    void generateExprStmt(ExprStmt* node);

    // Helper for function calls
    Operand generateCall(CallExpr* node);
    int evaluateConstantExpr(ASTNode* node);

    bool hadError;
    std::string errorMessage;
};

} // namespace ToyC

#endif // IR_GENERATOR_H
