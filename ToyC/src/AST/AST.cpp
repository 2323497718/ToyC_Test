#include "AST.h"
#include <iostream>
#include <sstream>

namespace ToyC {

void ASTNode::printIndent(int indent) {
    for (int i = 0; i < indent; i++) {
        std::cout << "  ";
    }
}

// ============== Number Literal ==============

NumberLiteral::NumberLiteral(int val) : ASTNode(ASTNodeType::NumberLiteral), value(val) {}

void NumberLiteral::print(int indent) const {
    printIndent(indent);
    std::cout << "NumberLiteral: " << value << std::endl;
}

// ============== Identifier ==============

Identifier::Identifier(const std::string& n) : ASTNode(ASTNodeType::Identifier), name(n) {}

void Identifier::print(int indent) const {
    printIndent(indent);
    std::cout << "Identifier: " << name << std::endl;
}

// ============== Binary Expression ==============

BinaryExpr::BinaryExpr(const std::string& operation, std::unique_ptr<ASTNode> lhs, 
                       std::unique_ptr<ASTNode> rhs)
    : ASTNode(ASTNodeType::BinaryExpr), op(operation), left(std::move(lhs)), right(std::move(rhs)) {}

void BinaryExpr::print(int indent) const {
    printIndent(indent);
    std::cout << "BinaryExpr: " << op << std::endl;
    left->print(indent + 1);
    right->print(indent + 1);
}

// ============== Unary Expression ==============

UnaryExpr::UnaryExpr(const std::string& operation, std::unique_ptr<ASTNode> expr)
    : ASTNode(ASTNodeType::UnaryExpr), op(operation), operand(std::move(expr)) {}

void UnaryExpr::print(int indent) const {
    printIndent(indent);
    std::cout << "UnaryExpr: " << op << std::endl;
    operand->print(indent + 1);
}

// ============== Call Expression ==============

CallExpr::CallExpr(const std::string& func, std::vector<std::unique_ptr<ASTNode>> args)
    : ASTNode(ASTNodeType::CallExpr), callee(func), arguments(std::move(args)) {}

void CallExpr::print(int indent) const {
    printIndent(indent);
    std::cout << "CallExpr: " << callee << std::endl;
    printIndent(indent + 1);
    std::cout << "Arguments:" << std::endl;
    for (auto& arg : arguments) {
        arg->print(indent + 2);
    }
}

// ============== Return Statement ==============

ReturnStmt::ReturnStmt(std::unique_ptr<ASTNode> val) 
    : ASTNode(ASTNodeType::ReturnStmt), value(std::move(val)) {}

ReturnStmt::ReturnStmt() : ASTNode(ASTNodeType::ReturnStmt), value(nullptr) {}

void ReturnStmt::print(int indent) const {
    printIndent(indent);
    std::cout << "ReturnStmt" << std::endl;
    if (value) {
        value->print(indent + 1);
    }
}

// ============== If Statement ==============

IfStmt::IfStmt(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> thenStmt,
               std::unique_ptr<ASTNode> elseStmt)
    : ASTNode(ASTNodeType::IfStmt), condition(std::move(cond)), 
      thenBranch(std::move(thenStmt)), elseBranch(std::move(elseStmt)) {}

void IfStmt::print(int indent) const {
    printIndent(indent);
    std::cout << "IfStmt" << std::endl;
    printIndent(indent + 1);
    std::cout << "Condition:" << std::endl;
    condition->print(indent + 2);
    printIndent(indent + 1);
    std::cout << "Then:" << std::endl;
    thenBranch->print(indent + 2);
    if (elseBranch) {
        printIndent(indent + 1);
        std::cout << "Else:" << std::endl;
        elseBranch->print(indent + 2);
    }
}

// ============== While Statement ==============

WhileStmt::WhileStmt(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> loopBody)
    : ASTNode(ASTNodeType::WhileStmt), condition(std::move(cond)), body(std::move(loopBody)) {}

void WhileStmt::print(int indent) const {
    printIndent(indent);
    std::cout << "WhileStmt" << std::endl;
    printIndent(indent + 1);
    std::cout << "Condition:" << std::endl;
    condition->print(indent + 2);
    printIndent(indent + 1);
    std::cout << "Body:" << std::endl;
    body->print(indent + 2);
}

// ============== Break Statement ==============

BreakStmt::BreakStmt() : ASTNode(ASTNodeType::BreakStmt) {}

void BreakStmt::print(int indent) const {
    printIndent(indent);
    std::cout << "BreakStmt" << std::endl;
}

// ============== Continue Statement ==============

ContinueStmt::ContinueStmt() : ASTNode(ASTNodeType::ContinueStmt) {}

void ContinueStmt::print(int indent) const {
    printIndent(indent);
    std::cout << "ContinueStmt" << std::endl;
}

// ============== Expression Statement ==============

ExprStmt::ExprStmt(std::unique_ptr<ASTNode> e) 
    : ASTNode(ASTNodeType::ExprStmt), expr(std::move(e)) {}

void ExprStmt::print(int indent) const {
    printIndent(indent);
    std::cout << "ExprStmt" << std::endl;
    if (expr) {
        expr->print(indent + 1);
    }
}

// ============== Assignment Statement ==============

AssignStmt::AssignStmt(const std::string& var, std::unique_ptr<ASTNode> val)
    : ASTNode(ASTNodeType::AssignStmt), target(var), value(std::move(val)) {}

void AssignStmt::print(int indent) const {
    printIndent(indent);
    std::cout << "AssignStmt: " << target << std::endl;
    value->print(indent + 1);
}

// ============== Block Statement ==============

BlockStmt::BlockStmt() : ASTNode(ASTNodeType::BlockStmt) {}

void BlockStmt::addStatement(std::unique_ptr<ASTNode> stmt) {
    statements.push_back(std::move(stmt));
}

void BlockStmt::print(int indent) const {
    printIndent(indent);
    std::cout << "BlockStmt" << std::endl;
    for (auto& stmt : statements) {
        stmt->print(indent + 1);
    }
}

// ============== Variable Declaration ==============

VarDecl::VarDecl(const std::string& n, std::unique_ptr<ASTNode> init, bool global)
    : ASTNode(ASTNodeType::VarDecl), name(n), initializer(std::move(init)), isGlobal(global) {}

void VarDecl::print(int indent) const {
    printIndent(indent);
    std::cout << "VarDecl: " << name << (isGlobal ? " (global)" : "") << std::endl;
    initializer->print(indent + 1);
}

// ============== Constant Declaration ==============

ConstDecl::ConstDecl(const std::string& n, std::unique_ptr<ASTNode> val, int constVal)
    : ASTNode(ASTNodeType::ConstDecl), name(n), value(std::move(val)), constValue(constVal) {}

void ConstDecl::print(int indent) const {
    printIndent(indent);
    std::cout << "ConstDecl: " << name << " = " << constValue << std::endl;
    value->print(indent + 1);
}

// ============== Function Definition ==============

FuncDef::FuncDef(const std::string& n, const std::vector<std::string>& p,
                 std::unique_ptr<ASTNode> b, bool voidReturn)
    : ASTNode(ASTNodeType::FuncDef), name(n), params(p), body(std::move(b)), isVoid(voidReturn) {}

void FuncDef::print(int indent) const {
    printIndent(indent);
    std::cout << "FuncDef: " << name << " (";
    if (isVoid) {
        std::cout << "void";
    } else {
        std::cout << "int";
    }
    std::cout << ")(";
    for (size_t i = 0; i < params.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << "int " << params[i];
    }
    std::cout << ")" << std::endl;
    body->print(indent + 1);
}

// ============== Compilation Unit ==============

CompUnit::CompUnit() : ASTNode(ASTNodeType::CompUnit) {}

void CompUnit::addDeclaration(std::unique_ptr<ASTNode> decl) {
    declarations.push_back(std::move(decl));
}

void CompUnit::print(int indent) const {
    printIndent(indent);
    std::cout << "CompUnit" << std::endl;
    for (auto& decl : declarations) {
        decl->print(indent + 1);
    }
}

} // namespace ToyC
