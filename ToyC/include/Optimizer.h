#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "IR.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace ToyC {

class Optimizer {
public:
    explicit Optimizer(bool enableOpt = false) : enableOptimizations(enableOpt) {}

    void optimize(IRProgram* program);

    bool isEnabled() const { return enableOptimizations; }

private:
    bool enableOptimizations;

    // Optimization passes
    void constantFolding(IRProgram* program);
    void algebraicSimplification(IRProgram* program);
    void deadCodeElimination(IRProgram* program);
    void commonSubexpressionElimination(IRProgram* program);
    void copyPropagation(IRProgram* program);
    void strengthReduction(IRProgram* program);

    // Helper methods
    bool isConstant(const Operand& op);
    int getConstantValue(const Operand& op);
    bool canFold(Instruction* instr);
    int foldInstruction(Instruction* instr);

    // For CSE
    struct ExpressionKey {
        IROpcode opcode;
        std::vector<std::string> operands;

        bool operator==(const ExpressionKey& other) const {
            return opcode == other.opcode && operands == other.operands;
        }
    };

    struct ExpressionKeyHash {
        size_t operator()(const ExpressionKey& key) const {
            size_t h = static_cast<size_t>(key.opcode);
            for (const auto& op : key.operands) {
                h = h * 31 + std::hash<std::string>{}(op);
            }
            return h;
        }
    };

    std::unordered_map<ExpressionKey, Operand, ExpressionKeyHash> cseTable;
    std::unordered_set<std::string> modifiedGlobals;
};

} // namespace ToyC

#endif // OPTIMIZER_H
