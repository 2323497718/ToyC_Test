#include "CodeGenerator.h"

namespace ToyC {

CodeGenerator::CodeGenerator(bool enableOpt) 
    : enableOptimizations(enableOpt), output(nullptr), indentLevel(0), 
      tempRegCounter(0), currentFuncName(""), nextLocalOffset(-8), hasReturned(false) {}

void CodeGenerator::generate(IRProgram* ir, std::ostream& out) {
    output = &out;
    
    // Generate assembly header
    emitLine(".intel_syntax noprefix");
    emitLine(".text");
    
    // Generate global variables
    for (size_t i = 0; i < ir->globals.size(); i++) {
        auto& gvar = ir->globals[i];
        emitLine(".globl " + gvar->name);
        emitLine(".data");
        emitLine(".align 4");
        emitLine(gvar->name + ":");
        emitLine("    .long " + std::to_string(gvar->initValue));
        emitLine("");
    }
    
    // Generate functions
    for (size_t i = 0; i < ir->functions.size(); i++) {
        generateFunction(ir->functions[i].get());
    }
}

void CodeGenerator::emitLine(const std::string& line) {
    output->write(line.c_str(), line.size());
    output->put('\n');
}

void CodeGenerator::generateFunction(Function* func) {
    currentFuncName = func->name;
    nextLocalOffset = -8;
    localVars.clear();
    hasReturned = false;
    
    // Function header
    emitLine("");
    emitLine(".globl " + func->name);
    emitLine(func->name + ":");
    emitLine("    push rbp");
    emitLine("    mov rbp, rsp");
    emitLine("    sub rsp, 32");
    
    // Save callee-saved registers
    emitLine("    push rbx");
    emitLine("    push r12");
    emitLine("    push r13");
    emitLine("    push r14");
    emitLine("    push r15");
    
    // Handle parameters
    for (int i = 0; i < func->paramCount && i < 6; i++) {
        int offset = nextLocalOffset;
        localVars["param" + std::to_string(i)] = offset;
        nextLocalOffset -= 4;
        emitLine("    mov DWORD PTR [rbp" + std::to_string(offset) + "], " + getParamReg(i));
    }
    
    // Generate function body
    for (size_t i = 0; i < func->blocks.size(); i++) {
        generateBasicBlock(func->blocks[i].get());
    }
    
    // Epilogue
    emitLine("");
    emitLine(func->name + ".epilog:");
    emitLine("    pop r15");
    emitLine("    pop r14");
    emitLine("    pop r13");
    emitLine("    pop r12");
    emitLine("    pop rbx");
    emitLine("    leave");
    emitLine("    ret");
}

std::string CodeGenerator::getParamReg(int idx) {
    static const char* regs[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
    if (idx >= 0 && idx < 6) return regs[idx];
    return "edi";
}

void CodeGenerator::generateBasicBlock(BasicBlock* block) {
    emitLine("");
    emitLine(block->name + ":");
    
    for (size_t i = 0; i < block->instructions.size(); i++) {
        // After return, only emit labels; skip all other instructions
        if (hasReturned && block->instructions[i]->opcode != IROpcode::LABEL) {
            continue;
        }
        generateInstruction(block->instructions[i].get());
    }
}

void CodeGenerator::generateInstruction(Instruction* instr) {
    switch (instr->opcode) {
        case IROpcode::FUNC_BEGIN:
        case IROpcode::FUNC_END:
        case IROpcode::PARAM:
        case IROpcode::LOADARG: {
            // Load parameter from register
            if (instr->operands.size() >= 2) {
                int paramIdx = instr->operands[0].constValue;
                std::string varName = instr->operands[1].value;
                int offset = getVarOffset(varName);
                emitLine("    mov DWORD PTR [rbp" + std::to_string(offset) + "], " + getParamReg(paramIdx));
            }
            break;
        }
        
        case IROpcode::LABEL:
            emitLine(instr->result.value + ":");
            break;
        
        case IROpcode::JUMPF:
            if (!instr->result.value.empty()) {
                emitLine("    cmp eax, 0");
            }
            if (instr->result.kind == Operand::Kind::Label) {
                std::string label = instr->result.value;
                if (!label.empty() && label.back() == ':') {
                    label = label.substr(0, label.size() - 1);
                }
                emitLine("    je " + label);
            }
            break;
        
        case IROpcode::JUMPT:
            if (!instr->result.value.empty()) {
                emitLine("    cmp eax, 0");
                std::string label = instr->result.value;
                if (!label.empty() && label.back() == ':') {
                    label = label.substr(0, label.size() - 1);
                }
                emitLine("    jne " + label);
            }
            break;
        
        case IROpcode::JUMP:
            if (!instr->result.value.empty()) {
                std::string label = instr->result.value;
                if (!label.empty() && label.back() == ':') {
                    label = label.substr(0, label.size() - 1);
                }
                emitLine("    jmp " + label);
            }
            break;
        
        case IROpcode::ADD:
        case IROpcode::SUB:
        case IROpcode::MUL:
        case IROpcode::AND:
        case IROpcode::OR:
            genBinaryOp(instr);
            break;
        
        case IROpcode::DIV:
        case IROpcode::MOD:
            genDivMod(instr);
            break;
        
        case IROpcode::NEG:
            genNeg(instr);
            break;
        
        case IROpcode::NOT:
            genNot(instr);
            break;
        
        case IROpcode::EQ:
        case IROpcode::NE:
        case IROpcode::LT:
        case IROpcode::GT:
        case IROpcode::LE:
        case IROpcode::GE:
            genCompare(instr);
            break;
        
        case IROpcode::LOAD:
            genLoad(instr);
            break;
        
        case IROpcode::LOADG:
            genLoadGlobal(instr);
            break;
        
        case IROpcode::STORE:
            genStore(instr);
            break;
        
        case IROpcode::STOREG:
            genStoreGlobal(instr);
            break;
        
        case IROpcode::MOVE:
            genMove(instr);
            break;
        
        case IROpcode::CALL:
            genCall(instr);
            break;
        
        case IROpcode::PUSH:
            if (!instr->operands.empty()) {
                emitLine("    push " + getOperandValue(instr->operands[0]));
            }
            break;
        
        case IROpcode::RETURN:
            if (!instr->operands.empty()) {
                emitLine("    mov eax, " + getOperandValue(instr->operands[0]));
            }
            emitLine("    jmp " + currentFuncName + ".epilog");
            hasReturned = true;
            break;
        
        case IROpcode::RETURNVOID:
            emitLine("    jmp " + currentFuncName + ".epilog");
            break;
        
        default:
            break;
    }
}

std::string CodeGenerator::getLabelName(const Operand& op) {
    if (op.kind == Operand::Kind::Label) {
        return op.value;  // Keep the colon
    }
    return op.value + ":";
}

std::string CodeGenerator::getOperandValue(const Operand& op) {
    switch (op.kind) {
        case Operand::Kind::Constant:
            return getImmediate(op);
        case Operand::Kind::Temp:
        case Operand::Kind::Variable:
            return getValue(op.value);
        default:
            return "0";
    }
}

std::string CodeGenerator::getValue(const std::string& name) {
    // Check if it's in a register (tracked)
    // For now, always load from memory
    int offset = getVarOffset(name);
    return "[rbp" + std::to_string(offset) + "]";
}

std::string CodeGenerator::getImmediate(const Operand& op) {
    if (op.kind == Operand::Kind::Constant) {
        return std::to_string(op.constValue);
    }
    return "0";
}

int CodeGenerator::getVarOffset(const std::string& var) {
    auto it = localVars.find(var);
    if (it != localVars.end()) {
        return it->second;
    }
    int offset = nextLocalOffset;
    localVars[var] = offset;
    nextLocalOffset -= 4;
    return offset;
}

void CodeGenerator::genBinaryOp(Instruction* instr) {
    // Load left operand
    if (instr->operands[0].kind == Operand::Kind::Constant) {
        emitLine("    mov eax, " + getImmediate(instr->operands[0]));
    } else {
        int leftOffset = getVarOffset(instr->operands[0].value);
        emitLine("    mov eax, [rbp" + std::to_string(leftOffset) + "]");
    }
    
    // Save left to stack
    emitLine("    push rax");
    
    // Load right operand
    if (instr->operands[1].kind == Operand::Kind::Constant) {
        emitLine("    mov eax, " + getImmediate(instr->operands[1]));
    } else {
        int rightOffset = getVarOffset(instr->operands[1].value);
        emitLine("    mov eax, [rbp" + std::to_string(rightOffset) + "]");
    }
    
    // Pop left and perform operation
    std::string op;
    switch (instr->opcode) {
        case IROpcode::ADD: op = "add"; break;
        case IROpcode::SUB: op = "sub"; break;
        case IROpcode::MUL: op = "imul"; break;
        case IROpcode::AND: op = "and"; break;
        case IROpcode::OR: op = "or"; break;
        default: return;
    }
    
    emitLine("    pop rdx");
    emitLine("    " + op + " eax, edx");
    
    // Store result
    int resultOffset = getVarOffset(instr->result.value);
    emitLine("    mov [rbp" + std::to_string(resultOffset) + "], eax");
}

void CodeGenerator::genDivMod(Instruction* instr) {
    // Load left operand
    if (instr->operands[0].kind == Operand::Kind::Constant) {
        emitLine("    mov eax, " + getImmediate(instr->operands[0]));
    } else {
        int leftOffset = getVarOffset(instr->operands[0].value);
        emitLine("    mov eax, [rbp" + std::to_string(leftOffset) + "]");
    }
    
    // Load right operand
    if (instr->operands[1].kind == Operand::Kind::Constant) {
        emitLine("    mov ecx, " + getImmediate(instr->operands[1]));
    } else {
        int rightOffset = getVarOffset(instr->operands[1].value);
        emitLine("    mov ecx, [rbp" + std::to_string(rightOffset) + "]");
    }
    
    emitLine("    cdq");
    emitLine("    idiv ecx");
    
    std::string resultReg = instr->opcode == IROpcode::MOD ? "edx" : "eax";
    int resultOffset = getVarOffset(instr->result.value);
    emitLine("    mov [rbp" + std::to_string(resultOffset) + "], " + resultReg);
}

void CodeGenerator::genNeg(Instruction* instr) {
    if (instr->operands[0].kind == Operand::Kind::Constant) {
        emitLine("    mov eax, " + getImmediate(instr->operands[0]));
    } else {
        int offset = getVarOffset(instr->operands[0].value);
        emitLine("    mov eax, [rbp" + std::to_string(offset) + "]");
    }
    
    emitLine("    neg eax");
    
    int resultOffset = getVarOffset(instr->result.value);
    emitLine("    mov [rbp" + std::to_string(resultOffset) + "], eax");
}

void CodeGenerator::genNot(Instruction* instr) {
    if (instr->operands[0].kind == Operand::Kind::Constant) {
        emitLine("    mov eax, " + getImmediate(instr->operands[0]));
    } else {
        int offset = getVarOffset(instr->operands[0].value);
        emitLine("    mov eax, [rbp" + std::to_string(offset) + "]");
    }
    
    emitLine("    xor eax, 1");
    
    int resultOffset = getVarOffset(instr->result.value);
    emitLine("    mov [rbp" + std::to_string(resultOffset) + "], eax");
}

void CodeGenerator::genCompare(Instruction* instr) {
    // Load left operand
    if (instr->operands[0].kind == Operand::Kind::Constant) {
        emitLine("    mov eax, " + getImmediate(instr->operands[0]));
    } else {
        int leftOffset = getVarOffset(instr->operands[0].value);
        emitLine("    mov eax, [rbp" + std::to_string(leftOffset) + "]");
    }
    
    // Load right operand
    if (instr->operands[1].kind == Operand::Kind::Constant) {
        emitLine("    mov ecx, " + getImmediate(instr->operands[1]));
    } else {
        int rightOffset = getVarOffset(instr->operands[1].value);
        emitLine("    mov ecx, [rbp" + std::to_string(rightOffset) + "]");
    }
    
    emitLine("    cmp eax, ecx");
    
    std::string setOp;
    switch (instr->opcode) {
        case IROpcode::EQ: setOp = "sete"; break;
        case IROpcode::NE: setOp = "setne"; break;
        case IROpcode::LT: setOp = "setl"; break;
        case IROpcode::GT: setOp = "setg"; break;
        case IROpcode::LE: setOp = "setle"; break;
        case IROpcode::GE: setOp = "setge"; break;
        default: return;
    }
    
    emitLine("    " + setOp + " al");
    emitLine("    movzx eax, al");
    
    int resultOffset = getVarOffset(instr->result.value);
    emitLine("    mov [rbp" + std::to_string(resultOffset) + "], eax");
}

void CodeGenerator::genLoad(Instruction* instr) {
    if (instr->operands.empty()) return;
    
    int srcOffset = getVarOffset(instr->operands[0].value);
    emitLine("    mov eax, [rbp" + std::to_string(srcOffset) + "]");
    
    int dstOffset = getVarOffset(instr->result.value);
    emitLine("    mov [rbp" + std::to_string(dstOffset) + "], eax");
}

void CodeGenerator::genStore(Instruction* instr) {
    if (instr->result.value.empty() || instr->operands.empty()) return;
    
    int dstOffset = getVarOffset(instr->result.value);
    
    if (instr->operands[0].kind == Operand::Kind::Constant) {
        emitLine("    mov DWORD PTR [rbp" + std::to_string(dstOffset) + "], " + getImmediate(instr->operands[0]));
    } else {
        int srcOffset = getVarOffset(instr->operands[0].value);
        emitLine("    mov eax, [rbp" + std::to_string(srcOffset) + "]");
        emitLine("    mov DWORD PTR [rbp" + std::to_string(dstOffset) + "], eax");
    }
}

void CodeGenerator::genLoadGlobal(Instruction* instr) {
    if (instr->operands.empty()) return;
    
    emitLine("    mov eax, [" + instr->operands[0].value + "]");
    
    int dstOffset = getVarOffset(instr->result.value);
    emitLine("    mov [rbp" + std::to_string(dstOffset) + "], eax");
}

void CodeGenerator::genStoreGlobal(Instruction* instr) {
    if (instr->operands.empty()) return;
    
    std::string val = getOperandValue(instr->operands[0]);
    emitLine("    mov [" + instr->result.value + "], " + val);
}

void CodeGenerator::genMove(Instruction* instr) {
    if (instr->operands.empty()) return;
    
    int dstOffset = getVarOffset(instr->result.value);
    
    if (instr->operands[0].kind == Operand::Kind::Constant) {
        emitLine("    mov DWORD PTR [rbp" + std::to_string(dstOffset) + "], " + getImmediate(instr->operands[0]));
    } else {
        int srcOffset = getVarOffset(instr->operands[0].value);
        emitLine("    mov eax, [rbp" + std::to_string(srcOffset) + "]");
        emitLine("    mov DWORD PTR [rbp" + std::to_string(dstOffset) + "], eax");
    }
}

void CodeGenerator::genCall(Instruction* instr) {
    if (instr->operands.empty()) return;
    
    emitLine("    call " + instr->operands[0].value);
    
    // Clean up pushed arguments: add esp, N*4
    int argCount = 0;
    if (instr->operands.size() >= 2) {
        argCount = instr->operands[1].constValue;
    }
    if (argCount > 0) {
        emitLine("    add esp, " + std::to_string(argCount * 4));
    }
    
    int resultOffset = getVarOffset(instr->result.value);
    emitLine("    mov [rbp" + std::to_string(resultOffset) + "], eax");
}

} // namespace ToyC
