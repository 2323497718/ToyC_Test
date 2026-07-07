#include "IRGenerator.h"
#include <stdexcept>

namespace ToyC {

IRGenerator::IRGenerator() 
    : program(std::make_unique<IRProgram>()), currentFunction(nullptr), 
      currentBlock(nullptr), hadError(false), functionReturnsValue(false), hasReturned(false) {}

std::unique_ptr<IRProgram> IRGenerator::generate(CompUnit* ast) {
    program = std::make_unique<IRProgram>();
    hadError = false;
    
    generateCompUnit(ast);
    
    if (hadError) {
        throw std::runtime_error(errorMessage);
    }
    
    return std::move(program);
}

Operand IRGenerator::emit(IROpcode op, const Operand& result, const std::vector<Operand>& ops) {
    if (!currentBlock) {
        return result;
    }
    
    auto instr = std::make_unique<Instruction>(op, result, ops);
    currentBlock->addInstruction(std::move(instr));
    return result;
}

Operand IRGenerator::emitLabel(const std::string& name) {
    if (!currentBlock) {
        return Operand::label(name);
    }
    
    auto instr = std::make_unique<Instruction>(IROpcode::LABEL, Operand::label(name));
    currentBlock->addInstruction(std::move(instr));
    return Operand::label(name);
}

void IRGenerator::generateCompUnit(CompUnit* node) {
    // First, collect global variables and functions
    for (auto& decl : node->declarations) {
        if (decl->getNodeType() == ASTNodeType::VarDecl) {
            auto varDecl = static_cast<VarDecl*>(decl.get());
            auto gvar = std::make_unique<GlobalVariable>(varDecl->name);
            
            // Evaluate initializer
            if (varDecl->initializer) {
                try {
                    gvar->initValue = evaluateConstantExpr(varDecl->initializer.get());
                    gvar->hasInit = true;
                } catch (...) {
                    gvar->initValue = 0;
                }
            }
            
            program->addGlobal(std::move(gvar));
        } else if (decl->getNodeType() == ASTNodeType::ConstDecl) {
            auto constDecl = static_cast<ConstDecl*>(decl.get());
            auto gvar = std::make_unique<GlobalVariable>(constDecl->name);
            gvar->initValue = constDecl->constValue;
            gvar->hasInit = true;
            gvar->isConst = true;
            program->addGlobal(std::move(gvar));
        }
    }
    
    // Then generate IR for each function
    for (auto& decl : node->declarations) {
        if (decl->getNodeType() == ASTNodeType::FuncDef) {
            generateFuncDef(static_cast<FuncDef*>(decl.get()));
        }
    }
}

void IRGenerator::generateFuncDef(FuncDef* node) {
    auto func = std::make_unique<Function>(node->name);
    func->isVoid = node->isVoid;
    func->paramCount = static_cast<int>(node->params.size());
    
    currentFunction = func.get();
    program->currentFunction = func.get();
    currentFunctionName = node->name;
    functionReturnsValue = !node->isVoid;
    hasReturned = false;
    
    // Create entry block
    auto entryBlock = std::make_unique<BasicBlock>(program->newLabel("entry"));
    currentBlock = entryBlock.get();
    func->addBlock(std::move(entryBlock));
    
    // Add function begin marker
    emit(IROpcode::FUNC_BEGIN, Operand::function(node->name));
    
    // Generate body
    symbolTable.pushScope();
    symbolTable.resetParamCount();
    
    // Declare parameters (they are passed in registers: rdi, rsi, rdx, rcx, r8, r9)
    static const std::vector<std::string> paramRegs = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
    for (size_t i = 0; i < node->params.size(); i++) {
        symbolTable.declare(node->params[i], SymbolKind::Parameter, ValueType::Int);
        
        // Emit loadarg for parameter
        if (i < paramRegs.size()) {
            std::string temp = program->newTemp();
            emit(IROpcode::LOADARG, Operand::temp(temp), 
                 {Operand::constant(static_cast<int>(i)), Operand::variable(node->params[i])});
        }
    }
    symbolTable.setParamCount(static_cast<int>(node->params.size()));
    
    generateBlock(static_cast<BlockStmt*>(node->body.get()));
    
    // If no return statement was generated (void function or missing return)
    if (functionReturnsValue && !hasReturned) {
        // Add implicit return 0 if needed
        emit(IROpcode::RETURN, Operand::constant(0));
    }
    
    emit(IROpcode::FUNC_END);
    
    symbolTable.popScope();
    
    // Add function to program
    functions[node->name] = func.get();
    program->addFunction(std::move(func));
    
    currentFunction = nullptr;
    currentBlock = nullptr;
}

void IRGenerator::generateBlock(BlockStmt* node) {
    for (auto& stmt : node->statements) {
        generateStmt(stmt.get());
    }
}

void IRGenerator::generateStmt(ASTNode* node) {
    if (!node || hasReturned) return;
    
    switch (node->getNodeType()) {
        case ASTNodeType::ReturnStmt:
            generateReturnStmt(static_cast<ReturnStmt*>(node));
            hasReturned = true;
            break;
        case ASTNodeType::IfStmt:
            generateIfStmt(static_cast<IfStmt*>(node));
            break;
        case ASTNodeType::WhileStmt:
            generateWhileStmt(static_cast<WhileStmt*>(node));
            break;
        case ASTNodeType::BreakStmt:
            generateBreakStmt(static_cast<BreakStmt*>(node));
            break;
        case ASTNodeType::ContinueStmt:
            generateContinueStmt(static_cast<ContinueStmt*>(node));
            break;
        case ASTNodeType::VarDecl:
            generateVarDecl(static_cast<VarDecl*>(node));
            break;
        case ASTNodeType::ConstDecl:
            generateConstDecl(static_cast<ConstDecl*>(node));
            break;
        case ASTNodeType::AssignStmt:
            generateAssignStmt(static_cast<AssignStmt*>(node));
            break;
        case ASTNodeType::ExprStmt:
            generateExprStmt(static_cast<ExprStmt*>(node));
            break;
        case ASTNodeType::BlockStmt:
            symbolTable.pushScope();
            generateBlock(static_cast<BlockStmt*>(node));
            symbolTable.popScope();
            break;
        default:
            break;
    }
}

void IRGenerator::generateReturnStmt(ReturnStmt* node) {
    if (node->value) {
        Operand val = generateExpr(node->value.get());
        emit(IROpcode::RETURN, val);
    } else {
        emit(IROpcode::RETURNVOID);
    }
    // Note: No jump to epilog needed - RETURN handles the control flow
}

void IRGenerator::generateIfStmt(IfStmt* node) {
    Operand cond = generateExpr(node->condition.get());
    
    std::string elseLabel = program->newLabel("else");
    std::string endLabel = program->newLabel("endif");
    
    // Jump to else if condition is false
    emit(IROpcode::JUMPF, Operand::label(elseLabel), {cond});
    
    // Generate then branch
    bool hadReturnBeforeThen = hasReturned;
    generateStmt(node->thenBranch.get());
    bool thenReturned = hasReturned;
    hasReturned = hadReturnBeforeThen;
    
    // Jump to end after then branch (unless it ends with return)
    if (!thenReturned) {
        emit(IROpcode::JUMP, Operand::label(endLabel), {});
    }
    
    // Else label
    emitLabel(elseLabel);
    
    // Generate else branch if exists, otherwise fall through to end
    if (node->elseBranch) {
        generateStmt(node->elseBranch.get());
    }
    
    // End label
    emitLabel(endLabel);
}

void IRGenerator::generateWhileStmt(WhileStmt* node) {
    std::string loopLabel = program->newLabel("loop");
    std::string bodyLabel = program->newLabel("body");
    std::string endLabel = program->newLabel("endloop");
    
    // Push loop context for break/continue
    LoopContext ctx;
    ctx.breakBlock = nullptr;  // Will be handled by label
    ctx.continueBlock = nullptr;
    loopStack.push(ctx);
    
    // Jump to condition
    emit(IROpcode::JUMP, Operand::label(loopLabel), {});
    
    // Loop body label
    emitLabel(bodyLabel);
    generateStmt(node->body.get());
    
    // Loop condition label
    emitLabel(loopLabel);
    Operand cond = generateExpr(node->condition.get());
    emit(IROpcode::JUMPT, Operand::label(bodyLabel), {cond});
    
    // End loop label
    emitLabel(endLabel);
    
    loopStack.pop();
}

void IRGenerator::generateBreakStmt(BreakStmt* node) {
    if (!loopStack.empty()) {
        std::string endLabel = program->newLabel("endloop");
        emit(IROpcode::JUMP, Operand::label(endLabel));
    }
}

void IRGenerator::generateContinueStmt(ContinueStmt* node) {
    if (!loopStack.empty()) {
        std::string loopLabel = program->newLabel("loop");
        emit(IROpcode::JUMP, Operand::label(loopLabel));
    }
}

void IRGenerator::generateVarDecl(VarDecl* node) {
    // Declare variable
    symbolTable.declare(node->name, SymbolKind::Variable, ValueType::Int);
    
    // Evaluate initializer and store to variable
    Operand val = generateExpr(node->initializer.get());
    
    // Get symbol info
    auto sym = symbolTable.lookup(node->name);
    if (sym && (*sym)->isGlobal) {
        emit(IROpcode::STOREG, Operand::variable(node->name), {val});
    } else {
        emit(IROpcode::STORE, Operand::variable(node->name), {val});
    }
}

void IRGenerator::generateConstDecl(ConstDecl* node) {
    // Evaluate constant value
    int constVal = evaluateConstantExpr(node->value.get());
    
    // Declare constant
    symbolTable.declare(node->name, SymbolKind::Constant, ValueType::Int, true, constVal);
}

void IRGenerator::generateAssignStmt(AssignStmt* node) {
    Operand val = generateExpr(node->value.get());
    
    // Get symbol info
    auto sym = symbolTable.lookup(node->target);
    if (sym && (*sym)->isGlobal) {
        emit(IROpcode::STOREG, Operand::variable(node->target), {val});
    } else {
        emit(IROpcode::STORE, Operand::variable(node->target), {val});
    }
}

void IRGenerator::generateExprStmt(ExprStmt* node) {
    if (node->expr) {
        generateExpr(node->expr.get());
    }
}

Operand IRGenerator::generateExpr(ASTNode* node) {
    if (!node) return Operand::constant(0);
    
    switch (node->getNodeType()) {
        case ASTNodeType::NumberLiteral: {
            auto lit = static_cast<NumberLiteral*>(node);
            return Operand::constant(lit->value);
        }
        
        case ASTNodeType::Identifier: {
            auto id = static_cast<Identifier*>(node);
            auto sym = symbolTable.lookup(id->name);
            
            if (sym && (*sym)->isGlobal) {
                std::string temp = program->newTemp();
                emit(IROpcode::LOADG, Operand::temp(temp), {Operand::variable(id->name)});
                return Operand::temp(temp);
            } else {
                std::string temp = program->newTemp();
                emit(IROpcode::LOAD, Operand::temp(temp), {Operand::variable(id->name)});
                return Operand::temp(temp);
            }
        }
        
        case ASTNodeType::BinaryExpr: {
            auto bin = static_cast<BinaryExpr*>(node);
            Operand left = generateExpr(bin->left.get());
            Operand right = generateExpr(bin->right.get());
            std::string result = program->newTemp();
            
            IROpcode op;
            if (bin->op == "+") op = IROpcode::ADD;
            else if (bin->op == "-") op = IROpcode::SUB;
            else if (bin->op == "*") op = IROpcode::MUL;
            else if (bin->op == "/") op = IROpcode::DIV;
            else if (bin->op == "%") op = IROpcode::MOD;
            else if (bin->op == "==") op = IROpcode::EQ;
            else if (bin->op == "!=") op = IROpcode::NE;
            else if (bin->op == "<") op = IROpcode::LT;
            else if (bin->op == ">") op = IROpcode::GT;
            else if (bin->op == "<=") op = IROpcode::LE;
            else if (bin->op == ">=") op = IROpcode::GE;
            else if (bin->op == "&&") op = IROpcode::AND;
            else if (bin->op == "||") op = IROpcode::OR;
            else {
                return Operand::constant(0);
            }
            
            emit(op, Operand::temp(result), {left, right});
            return Operand::temp(result);
        }
        
        case ASTNodeType::UnaryExpr: {
            auto unary = static_cast<UnaryExpr*>(node);
            Operand operand = generateExpr(unary->operand.get());
            std::string result = program->newTemp();
            
            IROpcode op;
            if (unary->op == "-") op = IROpcode::NEG;
            else if (unary->op == "!") op = IROpcode::NOT;
            else {
                return operand;
            }
            
            emit(op, Operand::temp(result), {operand});
            return Operand::temp(result);
        }
        
        case ASTNodeType::CallExpr: {
            return generateCall(static_cast<CallExpr*>(node));
        }
        
        default:
            return Operand::constant(0);
    }
}

Operand IRGenerator::generateCall(CallExpr* node) {
    // Push arguments
    for (auto& arg : node->arguments) {
        Operand val = generateExpr(arg.get());
        emit(IROpcode::PUSH, val);
    }
    
    // Call function
    std::string result = program->newTemp();
    emit(IROpcode::CALL, Operand::temp(result), {Operand::function(node->callee), Operand::constant(static_cast<int>(node->arguments.size()))});
    
    // Check if function returns value (we assume int return for now)
    // For void functions, result is ignored
    return Operand::temp(result);
}

int IRGenerator::evaluateConstantExpr(ASTNode* node) {
    if (!node) return 0;
    
    switch (node->getNodeType()) {
        case ASTNodeType::NumberLiteral:
            return static_cast<NumberLiteral*>(node)->value;
        
        case ASTNodeType::UnaryExpr: {
            auto unary = static_cast<UnaryExpr*>(node);
            int val = evaluateConstantExpr(unary->operand.get());
            if (unary->op == "-") return -val;
            if (unary->op == "+") return val;
            if (unary->op == "!") return !val;
            return 0;
        }
        
        case ASTNodeType::BinaryExpr: {
            auto bin = static_cast<BinaryExpr*>(node);
            int left = evaluateConstantExpr(bin->left.get());
            int right = evaluateConstantExpr(bin->right.get());
            if (bin->op == "+") return left + right;
            if (bin->op == "-") return left - right;
            if (bin->op == "*") return left * right;
            if (bin->op == "/") return right != 0 ? left / right : 0;
            if (bin->op == "%") return right != 0 ? left % right : 0;
            return 0;
        }
        
        case ASTNodeType::Identifier: {
            auto id = static_cast<Identifier*>(node);
            auto sym = symbolTable.lookup(id->name);
            if (sym && (*sym)->constValue.has_value()) {
                return (*sym)->constValue.value();
            }
            return 0;
        }
        
        default:
            return 0;
    }
}

} // namespace ToyC
