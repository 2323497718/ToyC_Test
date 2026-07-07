#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>

namespace ToyC {

enum class ASTNodeType {
    // Literals and identifiers
    NumberLiteral,
    Identifier,

    // Expressions
    BinaryExpr,
    UnaryExpr,
    CallExpr,

    // Statements
    ReturnStmt,
    IfStmt,
    WhileStmt,
    BreakStmt,
    ContinueStmt,
    ExprStmt,
    AssignStmt,
    BlockStmt,

    // Declarations
    VarDecl,
    ConstDecl,

    // Function
    FuncDef,

    // Program
    CompUnit
};

class ASTNode {
public:
    explicit ASTNode(ASTNodeType type) : nodeType(type) {}
    virtual ~ASTNode() = default;

    ASTNodeType getNodeType() const { return nodeType; }

    virtual void print(int indent = 0) const = 0;

protected:
    static void printIndent(int indent);
    ASTNodeType nodeType;
};

// Forward declarations
class Expr;
class Stmt;
class Decl;
class FuncDef;
class CompUnit;

// ============== Expression Nodes ==============

class NumberLiteral : public ASTNode {
public:
    int value;
    explicit NumberLiteral(int val);

    void print(int indent = 0) const override;
};

class Identifier : public ASTNode {
public:
    std::string name;
    explicit Identifier(const std::string& n);

    void print(int indent = 0) const override;
};

class BinaryExpr : public ASTNode {
public:
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    BinaryExpr(const std::string& operation, std::unique_ptr<ASTNode> lhs, std::unique_ptr<ASTNode> rhs);

    void print(int indent = 0) const override;
};

class UnaryExpr : public ASTNode {
public:
    std::string op;
    std::unique_ptr<ASTNode> operand;

    UnaryExpr(const std::string& operation, std::unique_ptr<ASTNode> expr);

    void print(int indent = 0) const override;
};

class CallExpr : public ASTNode {
public:
    std::string callee;
    std::vector<std::unique_ptr<ASTNode>> arguments;

    CallExpr(const std::string& func, std::vector<std::unique_ptr<ASTNode>> args);

    void print(int indent = 0) const override;
};

// ============== Statement Nodes ==============

class ReturnStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> value;  // null for void returns

    explicit ReturnStmt(std::unique_ptr<ASTNode> val);
    explicit ReturnStmt();

    void print(int indent = 0) const override;
};

class IfStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenBranch;
    std::unique_ptr<ASTNode> elseBranch;  // null if no else

    IfStmt(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> thenStmt,
           std::unique_ptr<ASTNode> elseStmt);

    void print(int indent = 0) const override;
};

class WhileStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;

    WhileStmt(std::unique_ptr<ASTNode> cond, std::unique_ptr<ASTNode> loopBody);

    void print(int indent = 0) const override;
};

class BreakStmt : public ASTNode {
public:
    BreakStmt();

    void print(int indent = 0) const override;
};

class ContinueStmt : public ASTNode {
public:
    ContinueStmt();

    void print(int indent = 0) const override;
};

class ExprStmt : public ASTNode {
public:
    std::unique_ptr<ASTNode> expr;

    explicit ExprStmt(std::unique_ptr<ASTNode> e);

    void print(int indent = 0) const override;
};

class AssignStmt : public ASTNode {
public:
    std::string target;
    std::unique_ptr<ASTNode> value;

    AssignStmt(const std::string& var, std::unique_ptr<ASTNode> val);

    void print(int indent = 0) const override;
};

class BlockStmt : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;

    BlockStmt();
    void addStatement(std::unique_ptr<ASTNode> stmt);

    void print(int indent = 0) const override;
};

// ============== Declaration Nodes ==============

class VarDecl : public ASTNode {
public:
    std::string name;
    std::unique_ptr<ASTNode> initializer;
    bool isGlobal;

    VarDecl(const std::string& n, std::unique_ptr<ASTNode> init, bool global = false);

    void print(int indent = 0) const override;
};

class ConstDecl : public ASTNode {
public:
    std::string name;
    std::unique_ptr<ASTNode> value;
    int constValue;  // Compile-time constant value

    ConstDecl(const std::string& n, std::unique_ptr<ASTNode> val, int constVal);

    void print(int indent = 0) const override;
};

// ============== Function Definition ==============

class FuncDef : public ASTNode {
public:
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<ASTNode> body;
    bool isVoid;

    FuncDef(const std::string& n, const std::vector<std::string>& p,
            std::unique_ptr<ASTNode> b, bool voidReturn);

    void print(int indent = 0) const override;
};

// ============== Compilation Unit ==============

class CompUnit : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> declarations;

    CompUnit();
    void addDeclaration(std::unique_ptr<ASTNode> decl);

    void print(int indent = 0) const override;
};

} // namespace ToyC

#endif // AST_H
