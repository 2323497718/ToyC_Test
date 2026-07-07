#ifndef IR_H
#define IR_H

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <unordered_map>
#include <unordered_set>

namespace ToyC {

enum class IROpcode {
    // Arithmetic
    ADD, SUB, MUL, DIV, MOD,
    NEG,        // Unary minus

    // Logical
    AND, OR, NOT,

    // Comparison
    EQ, NE, LT, GT, LE, GE,

    // Memory
    LOAD, STORE,
    LOADG, STOREG,  // Global variables
    LOADARG,         // Load function argument

    // Control flow
    LABEL,
    JUMP,
    JUMPF,          // Jump if false
    JUMPT,          // Jump if true
    CALL,
    RETURN,
    RETURNVOID,

    // Function
    FUNC_BEGIN,
    FUNC_END,
    PARAM,          // Mark function parameter
    PUSH,           // Push argument for call

    // Moves
    MOVE,

    // Phi function for SSA
    PHI,

    // Allocation (for temporaries)
    ALLOC
};

struct Operand {
    enum class Kind { Constant, Temp, Variable, Label, Function } kind;
    std::string value;
    int constValue;  // For constants

    Operand() : kind(Kind::Constant), value(""), constValue(0) {}

    static Operand constant(int val);
    static Operand temp(const std::string& name);
    static Operand variable(const std::string& name);
    static Operand label(const std::string& name);
    static Operand function(const std::string& name);

    std::string toString() const;
};

struct Instruction {
    IROpcode opcode;
    Operand result;
    std::vector<Operand> operands;

    Instruction(IROpcode op) : opcode(op) {}
    Instruction(IROpcode op, const Operand& res) : opcode(op), result(res) {}
    Instruction(IROpcode op, const Operand& res, const std::vector<Operand>& ops)
        : opcode(op), result(res), operands(ops) {}

    std::string toString() const;
    static const char* opcodeToString(IROpcode op);
};

struct BasicBlock {
    std::string name;
    std::vector<std::unique_ptr<Instruction>> instructions;
    BasicBlock* pred1;
    BasicBlock* pred2;
    std::unordered_set<BasicBlock*> successors;

    explicit BasicBlock(const std::string& n)
        : name(n), pred1(nullptr), pred2(nullptr) {}

    void addInstruction(std::unique_ptr<Instruction> instr);
    void addSuccessor(BasicBlock* block);
};

struct Function {
    std::string name;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    BasicBlock* entryBlock;
    int paramCount;
    bool isVoid;

    explicit Function(const std::string& n)
        : name(n), entryBlock(nullptr), paramCount(0), isVoid(false) {}

    void addBlock(std::unique_ptr<BasicBlock> block);
    BasicBlock* createBlock(const std::string& name);
};

struct GlobalVariable {
    std::string name;
    int initValue;
    bool isConst;
    bool hasInit;

    explicit GlobalVariable(const std::string& n)
        : name(n), initValue(0), isConst(false), hasInit(false) {}
};

struct IRProgram {
    std::vector<std::unique_ptr<Function>> functions;
    std::vector<std::unique_ptr<GlobalVariable>> globals;
    Function* currentFunction;
    int tempCounter;
    int labelCounter;

    IRProgram() : currentFunction(nullptr), tempCounter(0), labelCounter(0) {}

    std::string newTemp();
    std::string newLabel(const std::string& prefix = "L");
    void addFunction(std::unique_ptr<Function> func);
    Function* getFunction(const std::string& name);
    void addGlobal(std::unique_ptr<GlobalVariable> gvar);

    void print(std::ostream& os) const;
};

} // namespace ToyC

#endif // IR_H
