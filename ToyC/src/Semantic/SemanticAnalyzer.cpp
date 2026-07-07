#include "SemanticAnalyzer.h"
#include <iostream>

namespace ToyC {

SemanticAnalyzer::SemanticAnalyzer() 
    : loopDepth(0), currentFunction(""), currentFunctionReturnsValue(false), 
      functionHasReturn(false), hadError(false) {}

bool SemanticAnalyzer::analyze(CompUnit* root) {
    hadError = false;
    loopDepth = 0;
    
    // First pass: collect all function declarations
    for (auto& decl : root->declarations) {
        if (decl->getNodeType() == ASTNodeType::FuncDef) {
            auto func = static_cast<FuncDef*>(decl.get());
            
            // Check for main function
            if (func->name == "main") {
                if (func->isVoid) {
                    reportError("main function must return int", decl.get());
                    return false;
                }
                if (!func->params.empty()) {
                    reportError("main function must have no parameters", decl.get());
                    return false;
                }
            }
            
            // Register function
            if (!symbolTable.declare(func->name, SymbolKind::Function, 
                                     func->isVoid ? ValueType::Void : ValueType::Int)) {
                reportError("Function '" + func->name + "' already declared", decl.get());
                return false;
            }
        }
    }
    
    // Second pass: analyze declarations
    for (auto& decl : root->declarations) {
        visitStmt(decl.get());
    }
    
    return !hadError;
}

void SemanticAnalyzer::reportError(const std::string& message, int line, int col) {
    hadError = true;
    errorMessage = message;
    if (line > 0) {
        errorMessage += " at line " + std::to_string(line);
    }
}

void SemanticAnalyzer::reportError(const std::string& message, ASTNode* node) {
    hadError = true;
    errorMessage = message;
}

void SemanticAnalyzer::visitCompUnit(CompUnit* node) {
    for (auto& decl : node->declarations) {
        visitStmt(decl.get());
    }
}

void SemanticAnalyzer::visitFuncDef(FuncDef* node) {
    currentFunction = node->name;
    currentFunctionReturnsValue = !node->isVoid;
    functionHasReturn = false;
    
    // Enter function scope
    symbolTable.pushScope();
    
    // Declare parameters
    symbolTable.resetParamCount();
    for (const auto& param : node->params) {
        symbolTable.declare(param, SymbolKind::Parameter, ValueType::Int);
    }
    symbolTable.setParamCount(static_cast<int>(node->params.size()));
    
    // Visit body
    visitBlock(static_cast<BlockStmt*>(node->body.get()));
    
    // Check for return in non-void functions
    if (currentFunctionReturnsValue && !functionHasReturn) {
        reportError("Function '" + currentFunction + "' must return a value on all paths", node);
    }
    
    // Exit function scope
    symbolTable.popScope();
    currentFunction = "";
}

void SemanticAnalyzer::visitBlock(BlockStmt* node) {
    symbolTable.pushScope();
    
    for (auto& stmt : node->statements) {
        visitStmt(stmt.get());
    }
    
    symbolTable.popScope();
}

void SemanticAnalyzer::visitStmt(ASTNode* node) {
    if (!node) return;
    
    switch (node->getNodeType()) {
        case ASTNodeType::FuncDef:
            visitFuncDef(static_cast<FuncDef*>(node));
            break;
        case ASTNodeType::BlockStmt:
            visitBlock(static_cast<BlockStmt*>(node));
            break;
        case ASTNodeType::ReturnStmt:
            visitReturnStmt(static_cast<ReturnStmt*>(node));
            break;
        case ASTNodeType::IfStmt:
            visitIfStmt(static_cast<IfStmt*>(node));
            break;
        case ASTNodeType::WhileStmt:
            visitWhileStmt(static_cast<WhileStmt*>(node));
            break;
        case ASTNodeType::BreakStmt:
            visitBreakStmt(static_cast<BreakStmt*>(node));
            break;
        case ASTNodeType::ContinueStmt:
            visitContinueStmt(static_cast<ContinueStmt*>(node));
            break;
        case ASTNodeType::VarDecl:
            visitVarDecl(static_cast<VarDecl*>(node));
            break;
        case ASTNodeType::ConstDecl:
            visitConstDecl(static_cast<ConstDecl*>(node));
            break;
        case ASTNodeType::AssignStmt:
            visitAssignStmt(static_cast<AssignStmt*>(node));
            break;
        case ASTNodeType::ExprStmt:
            visitExprStmt(static_cast<ExprStmt*>(node));
            break;
        default:
            break;
    }
}

void SemanticAnalyzer::visitReturnStmt(ReturnStmt* node) {
    functionHasReturn = true;
    
    if (node->value) {
        if (!currentFunctionReturnsValue) {
            reportError("void function should not return a value", node);
        }
        getExprNode(node->value.get());
    } else {
        if (currentFunctionReturnsValue) {
            reportError("non-void function must return a value", node);
        }
    }
}

void SemanticAnalyzer::visitIfStmt(IfStmt* node) {
    getExprNode(node->condition.get());
    visitStmt(node->thenBranch.get());
    if (node->elseBranch) {
        visitStmt(node->elseBranch.get());
    }
}

void SemanticAnalyzer::visitWhileStmt(WhileStmt* node) {
    getExprNode(node->condition.get());
    loopDepth++;
    visitStmt(node->body.get());
    loopDepth--;
}

void SemanticAnalyzer::visitBreakStmt(BreakStmt* node) {
    if (loopDepth == 0) {
        reportError("'break' must be inside a loop", node);
    }
}

void SemanticAnalyzer::visitContinueStmt(ContinueStmt* node) {
    if (loopDepth == 0) {
        reportError("'continue' must be inside a loop", node);
    }
}

void SemanticAnalyzer::visitVarDecl(VarDecl* node) {
    // Check if already declared
    auto existing = symbolTable.lookupCurrentScope(node->name);
    if (existing) {
        reportError("Variable '" + node->name + "' already declared", node);
        return;
    }
    
    // Evaluate initializer
    getExprNode(node->initializer.get());
    
    // Declare variable
    bool isGlobal = symbolTable.getCurrentScopeLevel() == 0;
    symbolTable.declare(node->name, SymbolKind::Variable, ValueType::Int);
    
    // Update isGlobal in AST
    node->isGlobal = isGlobal;
}

void SemanticAnalyzer::visitConstDecl(ConstDecl* node) {
    // Check if already declared
    auto existing = symbolTable.lookupCurrentScope(node->name);
    if (existing) {
        reportError("Constant '" + node->name + "' already declared", node);
        return;
    }
    
    // Evaluate constant value
    int value = evaluateConstantExpr(node->value.get());
    node->constValue = value;
    
    // Declare constant
    symbolTable.declare(node->name, SymbolKind::Constant, ValueType::Int, true, value);
}

void SemanticAnalyzer::visitAssignStmt(AssignStmt* node) {
    // Check if variable exists
    auto sym = symbolTable.lookup(node->target);
    if (!sym) {
        reportError("Undefined variable '" + node->target + "'", node);
        return;
    }
    
    if ((*sym)->isConst) {
        reportError("Cannot assign to constant '" + node->target + "'", node);
        return;
    }
    
    // Evaluate value
    getExprNode(node->value.get());
}

void SemanticAnalyzer::visitExprStmt(ExprStmt* node) {
    if (node->expr) {
        getExprNode(node->expr.get());
    }
}

int SemanticAnalyzer::evaluateConstantExpr(ASTNode* node) {
    if (!node) return 0;
    
    switch (node->getNodeType()) {
        case ASTNodeType::NumberLiteral:
            return evaluateNumberLiteral(static_cast<NumberLiteral*>(node));
        case ASTNodeType::BinaryExpr:
            return evaluateBinaryExpr(static_cast<BinaryExpr*>(node));
        case ASTNodeType::UnaryExpr:
            return evaluateUnaryExpr(static_cast<UnaryExpr*>(node));
        case ASTNodeType::Identifier:
            return evaluateIdentifier(static_cast<Identifier*>(node));
        default:
            reportError("Invalid constant expression", node);
            return 0;
    }
}

int SemanticAnalyzer::evaluateBinaryExpr(BinaryExpr* node) {
    int left = evaluateConstantExpr(node->left.get());
    int right = evaluateConstantExpr(node->right.get());
    
    if (node->op == "+") return left + right;
    if (node->op == "-") return left - right;
    if (node->op == "*") return left * right;
    if (node->op == "/") return right != 0 ? left / right : 0;
    if (node->op == "%") return right != 0 ? left % right : 0;
    
    return 0;
}

int SemanticAnalyzer::evaluateUnaryExpr(UnaryExpr* node) {
    int operand = evaluateConstantExpr(node->operand.get());
    
    if (node->op == "-") return -operand;
    if (node->op == "+") return operand;
    if (node->op == "!") return !operand;
    
    return 0;
}

int SemanticAnalyzer::evaluateNumberLiteral(NumberLiteral* node) {
    return node->value;
}

int SemanticAnalyzer::evaluateIdentifier(Identifier* node) {
    auto sym = symbolTable.lookup(node->name);
    if (!sym) {
        reportError("Undefined identifier '" + node->name + "'", node);
        return 0;
    }
    
    if (!(*sym)->isConst) {
        reportError("Constant expression expected, but '" + node->name + "' is not constant", node);
        return 0;
    }
    
    return (*sym)->constValue.value_or(0);
}

int SemanticAnalyzer::evaluateCallExpr(CallExpr* node) {
    // For now, function calls in constant expressions are not supported
    reportError("Function calls in constant expressions are not supported", node);
    return 0;
}

ASTNode* SemanticAnalyzer::getExprNode(ASTNode* node) {
    if (!node) return nullptr;
    
    switch (node->getNodeType()) {
        case ASTNodeType::NumberLiteral:
        case ASTNodeType::Identifier:
        case ASTNodeType::BinaryExpr:
        case ASTNodeType::UnaryExpr:
        case ASTNodeType::CallExpr:
            return node;
        default:
            break;
    }
    
    return nullptr;
}

} // namespace ToyC
