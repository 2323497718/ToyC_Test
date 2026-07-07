#ifndef CODE_GENERATOR_H
#define CODE_GENERATOR_H

#include "IR.h"
#include <string>
#include <unordered_map>
#include <ostream>

namespace ToyC {

class CodeGenerator {
public:
    explicit CodeGenerator(bool enableOpt = false);
    
    void generate(IRProgram* ir, std::ostream& out);

private:
    bool enableOptimizations;
    std::ostream* output;
    int indentLevel;
    int tempRegCounter;
    std::string currentFuncName;
    int nextLocalOffset;
    bool hasReturned;  // Track if current function has returned
    
    // Local variable offsets
    std::unordered_map<std::string, int> localVars;
    
    // Helper methods
    void emitLine(const std::string& line);
    std::string getOperandValue(const Operand& op);
    std::string getImmediate(const Operand& op);
    std::string getLabelName(const Operand& op);
    std::string getParamReg(int idx);
    std::string getValue(const std::string& name);
    int getVarOffset(const std::string& var);
    
    // Code generation
    void generateFunction(Function* func);
    void generateBasicBlock(BasicBlock* block);
    void generateInstruction(Instruction* instr);
    
    // X86-64 specific generation
    void genBinaryOp(Instruction* instr);
    void genDivMod(Instruction* instr);
    void genNeg(Instruction* instr);
    void genNot(Instruction* instr);
    void genCompare(Instruction* instr);
    void genLoad(Instruction* instr);
    void genStore(Instruction* instr);
    void genLoadGlobal(Instruction* instr);
    void genStoreGlobal(Instruction* instr);
    void genMove(Instruction* instr);
    void genCall(Instruction* instr);
};

} // namespace ToyC

#endif // CODE_GENERATOR_H
