#include "IR.h"
#include <sstream>
#include <stdexcept>

namespace ToyC {

// ============== Operand ==============

Operand Operand::constant(int val) {
    Operand op;
    op.kind = Kind::Constant;
    op.constValue = val;
    op.value = std::to_string(val);
    return op;
}

Operand Operand::temp(const std::string& name) {
    Operand op;
    op.kind = Kind::Temp;
    op.value = name;
    return op;
}

Operand Operand::variable(const std::string& name) {
    Operand op;
    op.kind = Kind::Variable;
    op.value = name;
    return op;
}

Operand Operand::label(const std::string& name) {
    Operand op;
    op.kind = Kind::Label;
    op.value = name;
    op.constValue = 0;
    return op;
}

Operand Operand::function(const std::string& name) {
    Operand op;
    op.kind = Kind::Function;
    op.value = name;
    return op;
}

std::string Operand::toString() const {
    switch (kind) {
        case Kind::Constant: return std::to_string(constValue);
        case Kind::Temp: return "%" + value;
        case Kind::Variable: return "%" + value;
        case Kind::Label: return value + ":";
        case Kind::Function: return "@" + value;
    }
    return value;
}

// ============== Instruction ==============

const char* Instruction::opcodeToString(IROpcode op) {
    switch (op) {
        case IROpcode::ADD: return "add";
        case IROpcode::SUB: return "sub";
        case IROpcode::MUL: return "mul";
        case IROpcode::DIV: return "div";
        case IROpcode::MOD: return "mod";
        case IROpcode::NEG: return "neg";
        case IROpcode::AND: return "and";
        case IROpcode::OR: return "or";
        case IROpcode::NOT: return "not";
        case IROpcode::EQ: return "eq";
        case IROpcode::NE: return "ne";
        case IROpcode::LT: return "lt";
        case IROpcode::GT: return "gt";
        case IROpcode::LE: return "le";
        case IROpcode::GE: return "ge";
        case IROpcode::LOAD: return "load";
        case IROpcode::STORE: return "store";
        case IROpcode::LOADG: return "loadg";
        case IROpcode::STOREG: return "storeg";
        case IROpcode::LOADARG: return "loadarg";
        case IROpcode::LABEL: return "label";
        case IROpcode::JUMP: return "jump";
        case IROpcode::JUMPF: return "jumpf";
        case IROpcode::JUMPT: return "jumpt";
        case IROpcode::CALL: return "call";
        case IROpcode::RETURN: return "ret";
        case IROpcode::RETURNVOID: return "retvoid";
        case IROpcode::FUNC_BEGIN: return "func_begin";
        case IROpcode::FUNC_END: return "func_end";
        case IROpcode::PARAM: return "param";
        case IROpcode::PUSH: return "push";
        case IROpcode::MOVE: return "move";
        case IROpcode::PHI: return "phi";
        case IROpcode::ALLOC: return "alloc";
    }
    return "unknown";
}

std::string Instruction::toString() const {
    std::ostringstream oss;
    oss << opcodeToString(opcode) << " ";
    
    if (result.kind != Operand::Kind::Constant || result.value != "0") {
        oss << result.toString() << " = ";
    } else {
        oss << "  ";
    }
    
    for (size_t i = 0; i < operands.size(); i++) {
        if (i > 0) oss << ", ";
        oss << operands[i].toString();
    }
    
    return oss.str();
}

// ============== BasicBlock ==============

void BasicBlock::addInstruction(std::unique_ptr<Instruction> instr) {
    instructions.push_back(std::move(instr));
}

void BasicBlock::addSuccessor(BasicBlock* block) {
    successors.insert(block);
}

// ============== Function ==============

void Function::addBlock(std::unique_ptr<BasicBlock> block) {
    if (!entryBlock) {
        entryBlock = block.get();
    }
    blocks.push_back(std::move(block));
}

BasicBlock* Function::createBlock(const std::string& name) {
    auto block = std::make_unique<BasicBlock>(name);
    BasicBlock* ptr = block.get();
    addBlock(std::move(block));
    return ptr;
}

// ============== IRProgram ==============

std::string IRProgram::newTemp() {
    return "t" + std::to_string(tempCounter++);
}

std::string IRProgram::newLabel(const std::string& prefix) {
    return prefix + std::to_string(labelCounter++);
}

void IRProgram::addFunction(std::unique_ptr<Function> func) {
    functions.push_back(std::move(func));
}

Function* IRProgram::getFunction(const std::string& name) {
    for (auto& func : functions) {
        if (func->name == name) {
            return func.get();
        }
    }
    return nullptr;
}

void IRProgram::addGlobal(std::unique_ptr<GlobalVariable> gvar) {
    globals.push_back(std::move(gvar));
}

void IRProgram::print(std::ostream& os) const {
    // Print globals
    for (auto& gvar : globals) {
        os << ".global " << gvar->name;
        if (gvar->hasInit) {
            os << " = " << gvar->initValue;
        }
        os << "\n";
    }
    os << "\n";
    
    // Print functions
    for (auto& func : functions) {
        os << "function " << func->name << " (";
        for (size_t i = 0; i < func->blocks.size(); i++) {
            if (i > 0) os << ", ";
            os << func->blocks[i]->name;
        }
        os << ")\n";
        
        for (auto& block : func->blocks) {
            os << block->name << ":\n";
            for (auto& instr : block->instructions) {
                os << "  " << instr->toString() << "\n";
            }
            os << "\n";
        }
    }
}

} // namespace ToyC
