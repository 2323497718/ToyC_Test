#include "Optimizer.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace ToyC {

void Optimizer::optimize(IRProgram* program) {
    if (!enableOptimizations) return;
    
    // Run optimization passes
    constantFolding(program);
    algebraicSimplification(program);
    deadCodeElimination(program);
    commonSubexpressionElimination(program);
    copyPropagation(program);
    strengthReduction(program);
}

bool Optimizer::isConstant(const Operand& op) {
    return op.kind == Operand::Kind::Constant;
}

int Optimizer::getConstantValue(const Operand& op) {
    if (op.kind == Operand::Kind::Constant) {
        return op.constValue;
    }
    return 0;
}

bool Optimizer::canFold(Instruction* instr) {
    // Check if all operands are constants
    for (const auto& op : instr->operands) {
        if (!isConstant(op)) return false;
    }
    return true;
}

int Optimizer::foldInstruction(Instruction* instr) {
    int a = instr->operands.size() > 0 ? getConstantValue(instr->operands[0]) : 0;
    int b = instr->operands.size() > 1 ? getConstantValue(instr->operands[1]) : 0;
    
    switch (instr->opcode) {
        case IROpcode::ADD: return a + b;
        case IROpcode::SUB: return a - b;
        case IROpcode::MUL: return a * b;
        case IROpcode::DIV: return b != 0 ? a / b : 0;
        case IROpcode::MOD: return b != 0 ? a % b : 0;
        case IROpcode::NEG: return -a;
        case IROpcode::EQ: return a == b;
        case IROpcode::NE: return a != b;
        case IROpcode::LT: return a < b;
        case IROpcode::GT: return a > b;
        case IROpcode::LE: return a <= b;
        case IROpcode::GE: return a >= b;
        case IROpcode::AND: return a && b;
        case IROpcode::OR: return a || b;
        case IROpcode::NOT: return !a;
        default: return 0;
    }
}

void Optimizer::constantFolding(IRProgram* program) {
    for (auto& func : program->functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                if (canFold(instr.get())) {
                    int result = foldInstruction(instr.get());
                    instr->result = Operand::constant(result);
                    instr->operands.clear();
                }
            }
        }
    }
}

void Optimizer::algebraicSimplification(IRProgram* program) {
    for (auto& func : program->functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                // x + 0 = x
                if (instr->opcode == IROpcode::ADD) {
                    if (instr->operands.size() >= 2) {
                        if (isConstant(instr->operands[1]) && getConstantValue(instr->operands[1]) == 0) {
                            instr->opcode = IROpcode::MOVE;
                            instr->operands = {instr->operands[0]};
                        }
                    }
                }
                
                // x - 0 = x
                if (instr->opcode == IROpcode::SUB) {
                    if (instr->operands.size() >= 2) {
                        if (isConstant(instr->operands[1]) && getConstantValue(instr->operands[1]) == 0) {
                            instr->opcode = IROpcode::MOVE;
                            instr->operands = {instr->operands[0]};
                        }
                    }
                }
                
                // x * 1 = x
                if (instr->opcode == IROpcode::MUL) {
                    if (instr->operands.size() >= 2) {
                        if (isConstant(instr->operands[1]) && getConstantValue(instr->operands[1]) == 1) {
                            instr->opcode = IROpcode::MOVE;
                            instr->operands = {instr->operands[0]};
                        }
                    }
                }
                
                // x * 0 = 0
                if (instr->opcode == IROpcode::MUL) {
                    if (instr->operands.size() >= 2) {
                        if ((isConstant(instr->operands[0]) && getConstantValue(instr->operands[0]) == 0) ||
                            (isConstant(instr->operands[1]) && getConstantValue(instr->operands[1]) == 0)) {
                            instr->result = Operand::constant(0);
                            instr->operands.clear();
                        }
                    }
                }
                
                // x / 1 = x
                if (instr->opcode == IROpcode::DIV) {
                    if (instr->operands.size() >= 2) {
                        if (isConstant(instr->operands[1]) && getConstantValue(instr->operands[1]) == 1) {
                            instr->opcode = IROpcode::MOVE;
                            instr->operands = {instr->operands[0]};
                        }
                    }
                }
                
                // x & 0 = 0
                if (instr->opcode == IROpcode::AND) {
                    if (instr->operands.size() >= 2) {
                        if (isConstant(instr->operands[1]) && getConstantValue(instr->operands[1]) == 0) {
                            instr->result = Operand::constant(0);
                            instr->operands.clear();
                        }
                    }
                }
                
                // x | 0 = x
                if (instr->opcode == IROpcode::OR) {
                    if (instr->operands.size() >= 2) {
                        if (isConstant(instr->operands[1]) && getConstantValue(instr->operands[1]) == 0) {
                            instr->opcode = IROpcode::MOVE;
                            instr->operands = {instr->operands[0]};
                        }
                    }
                }
            }
        }
    }
}

void Optimizer::deadCodeElimination(IRProgram* program) {
    for (auto& func : program->functions) {
        std::set<std::string> usedTemps;
        
        // Find all used temporaries
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                // Check operands
                for (const auto& op : instr->operands) {
                    if (op.kind == Operand::Kind::Temp) {
                        usedTemps.insert(op.value);
                    }
                }
                
                // Check result
                if (instr->result.kind == Operand::Kind::Temp) {
                    // If this result is never used, mark it
                    // We'll remove unused instructions later
                }
            }
        }
        
        // Remove instructions whose results are never used
        for (auto& block : func->blocks) {
            block->instructions.erase(
                std::remove_if(block->instructions.begin(), block->instructions.end(),
                    [&usedTemps](const std::unique_ptr<Instruction>& instr) {
                        // Keep instructions with side effects
                        switch (instr->opcode) {
                            case IROpcode::CALL:
                            case IROpcode::STORE:
                            case IROpcode::STOREG:
                            case IROpcode::JUMP:
                            case IROpcode::JUMPF:
                            case IROpcode::JUMPT:
                            case IROpcode::RETURN:
                            case IROpcode::RETURNVOID:
                            case IROpcode::LABEL:
                            case IROpcode::FUNC_BEGIN:
                            case IROpcode::FUNC_END:
                            case IROpcode::PUSH:
                                return false;
                            default:
                                break;
                        }
                        
                        // Remove if result is never used
                        if (instr->result.kind == Operand::Kind::Temp) {
                            return usedTemps.find(instr->result.value) == usedTemps.end();
                        }
                        return false;
                    }),
                block->instructions.end()
            );
        }
    }
}

void Optimizer::commonSubexpressionElimination(IRProgram* program) {
    for (auto& func : program->functions) {
        cseTable.clear();
        
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                // Only CSE binary operations
                switch (instr->opcode) {
                    case IROpcode::ADD:
                    case IROpcode::SUB:
                    case IROpcode::MUL:
                    case IROpcode::DIV:
                    case IROpcode::EQ:
                    case IROpcode::NE:
                    case IROpcode::LT:
                    case IROpcode::GT:
                    case IROpcode::LE:
                    case IROpcode::GE:
                    case IROpcode::AND:
                    case IROpcode::OR: {
                        // Check if we can reuse a previous computation
                        std::vector<std::string> opValues;
                        for (const auto& op : instr->operands) {
                            opValues.push_back(op.toString());
                        }
                        
                        ExpressionKey key{instr->opcode, opValues};
                        auto it = cseTable.find(key);
                        if (it != cseTable.end()) {
                            // Replace with the previous result
                            instr->opcode = IROpcode::MOVE;
                            instr->result = it->second;
                            instr->operands.clear();
                        } else {
                            // Store this computation for future use
                            if (instr->result.kind == Operand::Kind::Temp) {
                                cseTable[key] = instr->result;
                            }
                        }
                        break;
                    }
                    default:
                        // Reset CSE table for operations that may have side effects
                        cseTable.clear();
                        break;
                }
            }
        }
    }
}

void Optimizer::copyPropagation(IRProgram* program) {
    for (auto& func : program->functions) {
        std::unordered_map<std::string, Operand> copies;
        
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                // x = y -> remember copy
                if (instr->opcode == IROpcode::MOVE && instr->operands.size() == 1) {
                    if (instr->result.kind == Operand::Kind::Temp && 
                        instr->operands[0].kind == Operand::Kind::Temp) {
                        copies[instr->result.value] = instr->operands[0];
                    }
                }
                
                // Replace operands with known copies
                for (auto& op : instr->operands) {
                    if (op.kind == Operand::Kind::Temp) {
                        auto it = copies.find(op.value);
                        if (it != copies.end()) {
                            op = it->second;
                        }
                    }
                }
                
                // If instruction writes to a variable, clear copies
                if (instr->opcode == IROpcode::STORE || 
                    instr->opcode == IROpcode::STOREG ||
                    instr->opcode == IROpcode::CALL) {
                    copies.clear();
                }
            }
        }
    }
}

void Optimizer::strengthReduction(IRProgram* program) {
    // Simple strength reduction: replace multiply by power of 2 with shift
    for (auto& func : program->functions) {
        for (auto& block : func->blocks) {
            for (auto& instr : block->instructions) {
                if (instr->opcode == IROpcode::MUL && instr->operands.size() >= 2) {
                    int multiplier = 0;
                    if (isConstant(instr->operands[1])) {
                        multiplier = getConstantValue(instr->operands[1]);
                    } else if (isConstant(instr->operands[0])) {
                        multiplier = getConstantValue(instr->operands[0]);
                    }
                    
                    // Check if multiplier is power of 2
                    if (multiplier > 0 && (multiplier & (multiplier - 1)) == 0) {
                        int shift = 0;
                        int m = multiplier;
                        while (m > 1) {
                            m >>= 1;
                            shift++;
                        }
                        
                        // Emit shift left instead of multiply
                        Operand shiftAmt = Operand::constant(shift);
                        instr->operands.clear();
                        if (isConstant(instr->operands[0])) {
                            instr->operands.push_back(instr->operands[1]);
                        } else {
                            instr->operands.push_back(instr->operands[0]);
                        }
                        instr->operands.push_back(shiftAmt);
                        // Note: True strength reduction would need a SHL instruction
                        // For now, we just identify the pattern
                    }
                }
            }
        }
    }
}

} // namespace ToyC
