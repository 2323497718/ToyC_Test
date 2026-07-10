package com.toycc.ir;

import com.toycc.ast.*;
import com.toycc.util.Fatal;

import java.util.*;

/**
 * IR 生成器 - 将 AST 转换为三地址 IR
 */
public class IRBuilder {

    private static class Binding {
        enum Kind { SLOT, GLOBAL, CONSTANT }

        Kind kind = Kind.SLOT;
        int slot = -1;
        int value = 0;
        String label = "";

        Binding(Kind kind, int slot, int value, String label) {
            this.kind = kind;
            this.slot = slot;
            this.value = value;
            this.label = label;
        }
    }

    private static class LoopLabels {
        String breakLabel;
        String continueLabel;

        LoopLabels(String breakLabel, String continueLabel) {
            this.breakLabel = breakLabel;
            this.continueLabel = continueLabel;
        }
    }

    private final CompUnit ast;
    private final Program program = new Program();
    private Function currentFunc = null;
    private final List<Map<String, Binding>> scopes = new ArrayList<>();
    private final List<LoopLabels> loopStack = new ArrayList<>();
    private int nextLabel = 0;

    public IRBuilder(CompUnit ast) {
        this.ast = ast;
    }

    public Program build() {
        collectGlobals();
        for (Node decl : ast.decls) {
            if (decl instanceof FuncDef func) {
                program.functions.add(buildFunction(func));
            }
        }
        return program;
    }

    private void collectGlobals() {
        pushScope();
        for (Node decl : ast.decls) {
            if (decl instanceof ConstDecl constant) {
                int value = evalConst(constant.initializer);
                scopes.get(scopes.size() - 1).put(constant.name,
                        new Binding(Binding.Kind.CONSTANT, -1, value, ""));
            } else if (decl instanceof VarDecl var) {
                int value = evalConst(var.initializer);
                String label = globalLabel(var.name);
                scopes.get(scopes.size() - 1).put(var.name,
                        new Binding(Binding.Kind.GLOBAL, -1, 0, label));
                program.globals.add(new Global(var.name, label, value));
            }
        }
    }

    private Function buildFunction(FuncDef func) {
        currentFunc = new Function();
        currentFunc.name = func.name;
        nextLabel = 0;
        loopStack.clear();
        pushScope();

        // 分配形参槽位
        for (Param param : func.params) {
            int slot = currentFunc.newSlot();
            currentFunc.paramSlots.add(slot);
            currentFunc.namedSlots.add(slot);
            scopes.get(scopes.size() - 1).put(param.name,
                    new Binding(Binding.Kind.SLOT, slot, 0, ""));
        }

        emitStmt(func.body);

        if (func.returnType == FuncDef.ReturnType.VOID) {
            currentFunc.addInstruction(Instruction.retVoid());
        }

        popScope();
        Function result = currentFunc;
        currentFunc = null;
        return result;
    }

    private int evalConst(Expr expr) {
        if (expr instanceof NumberExpr number) {
            return number.value;
        }

        if (expr instanceof NameExpr name) {
            Binding binding = lookup(name.name);
            if (binding.kind != Binding.Kind.CONSTANT) {
                Fatal.error("non-constant in constant expression");
            }
            return binding.value;
        }

        if (expr instanceof UnaryExpr unary) {
            int v = evalConst(unary.operand);
            return switch (unary.op) {
                case PLUS -> v;
                case MINUS -> -v;
                case NOT -> (v != 0) ? 0 : 1;
            };
        }

        if (expr instanceof BinaryExpr binary) {
            if (binary.op == BinaryExpr.Op.LOGICAL_AND) {
                int left = evalConst(binary.left);
                return (left != 0) ? truthy(evalConst(binary.right)) : 0;
            }
            if (binary.op == BinaryExpr.Op.LOGICAL_OR) {
                int left = evalConst(binary.left);
                return (left != 0) ? 1 : truthy(evalConst(binary.right));
            }
            int left = evalConst(binary.left);
            int right = evalConst(binary.right);
            return switch (binary.op) {
                case LESS -> left < right ? 1 : 0;
                case GREATER -> left > right ? 1 : 0;
                case LESS_EQUAL -> left <= right ? 1 : 0;
                case GREATER_EQUAL -> left >= right ? 1 : 0;
                case EQUAL -> left == right ? 1 : 0;
                case NOT_EQUAL -> left != right ? 1 : 0;
                case ADD -> left + right;
                case SUB -> left - right;
                case MUL -> left * right;
                case DIV -> left / right;
                case MOD -> left % right;
                default -> 0;
            };
        }

        Fatal.error("unsupported constant expression");
        return 0; // unreachable
    }

    private int truthy(int value) {
        return value != 0 ? 1 : 0;
    }

    private void pushScope() {
        scopes.add(new HashMap<>());
    }

    private void popScope() {
        scopes.remove(scopes.size() - 1);
    }

    private Binding lookup(String name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            Binding binding = scopes.get(i).get(name);
            if (binding != null) {
                return binding;
            }
        }
        Fatal.error("unknown IR binding: " + name);
        return null; // unreachable
    }

    private String newLabel(String prefix) {
        return ".L_" + currentFunc.name + "_" + prefix + "_" + (nextLabel++);
    }

    private void emitLabel(String label) {
        currentFunc.addInstruction(Instruction.label(label));
    }

    private void emitGoto(String label) {
        currentFunc.addInstruction(Instruction.goto_(label));
    }

    private void emitStmt(Stmt stmt) {
        if (stmt instanceof EmptyStmt) {
            return;
        }

        if (stmt instanceof BlockStmt block) {
            pushScope();
            for (Stmt s : block.statements) {
                emitStmt(s);
            }
            popScope();
            return;
        }

        if (stmt instanceof ExprStmt exprStmt) {
            emitExpr(exprStmt.expr);
            return;
        }

        if (stmt instanceof AssignStmt assign) {
            int value = emitExpr(assign.value);
            storeName(assign.name, value);
            return;
        }

        if (stmt instanceof DeclStmt declStmt) {
            emitDecl(declStmt.decl);
            return;
        }

        if (stmt instanceof IfStmt ifStmt) {
            String elseLabel = newLabel("else");
            String endLabel = newLabel("ifend");
            int cond = emitExpr(ifStmt.condition);
            emitBranch(cond, ifStmt.elseBranch != null ? elseLabel : endLabel);
            emitStmt(ifStmt.thenBranch);
            emitGoto(endLabel);
            if (ifStmt.elseBranch != null) {
                emitLabel(elseLabel);
                emitStmt(ifStmt.elseBranch);
            }
            emitLabel(endLabel);
            return;
        }

        if (stmt instanceof WhileStmt whileStmt) {
            String condLabel = newLabel("while_cond");
            String bodyLabel = newLabel("while_body");
            String endLabel = newLabel("while_end");
            loopStack.add(new LoopLabels(endLabel, condLabel));
            emitLabel(condLabel);
            int cond = emitExpr(whileStmt.condition);
            emitBranch(cond, endLabel);
            emitLabel(bodyLabel);
            emitStmt(whileStmt.body);
            emitGoto(condLabel);
            emitLabel(endLabel);
            loopStack.remove(loopStack.size() - 1);
            return;
        }

        if (stmt instanceof BreakStmt) {
            emitGoto(loopStack.get(loopStack.size() - 1).breakLabel);
            return;
        }

        if (stmt instanceof ContinueStmt) {
            emitGoto(loopStack.get(loopStack.size() - 1).continueLabel);
            return;
        }

        if (stmt instanceof ReturnStmt ret) {
            if (ret.value != null) {
                int value = emitExpr(ret.value);
                currentFunc.addInstruction(Instruction.ret(value));
            } else {
                currentFunc.addInstruction(Instruction.retVoid());
            }
            return;
        }

        Fatal.error("unsupported statement in IR builder");
    }

    private void emitDecl(Decl decl) {
        if (decl instanceof VarDecl var) {
            int slot = currentFunc.newSlot();
            currentFunc.namedSlots.add(slot);
            scopes.get(scopes.size() - 1).put(var.name,
                    new Binding(Binding.Kind.SLOT, slot, 0, ""));
            int value = emitExpr(var.initializer);
            currentFunc.addInstruction(Instruction.move(slot, value));
            return;
        }

        if (decl instanceof ConstDecl constant) {
            int value = evalConst(constant.initializer);
            scopes.get(scopes.size() - 1).put(constant.name,
                    new Binding(Binding.Kind.CONSTANT, -1, value, ""));
            return;
        }

        Fatal.error("unexpected declaration in function body");
    }

    private int emitExpr(Expr expr) {
        if (expr instanceof NumberExpr number) {
            int dest = currentFunc.newTemp();
            currentFunc.addInstruction(Instruction.const_(dest, number.value));
            return dest;
        }

        if (expr instanceof NameExpr name) {
            Binding binding = lookup(name.name);
            if (binding.kind == Binding.Kind.SLOT) {
                return binding.slot;
            }
            int dest = currentFunc.newTemp();
            if (binding.kind == Binding.Kind.CONSTANT) {
                currentFunc.addInstruction(Instruction.const_(dest, binding.value));
            } else {
                currentFunc.addInstruction(Instruction.loadGlobal(dest, binding.label));
            }
            return dest;
        }

        if (expr instanceof UnaryExpr unary) {
            int operand = emitExpr(unary.operand);
            int dest = currentFunc.newTemp();
            UnaryOp op = switch (unary.op) {
                case PLUS -> UnaryOp.PLUS;
                case MINUS -> UnaryOp.MINUS;
                case NOT -> UnaryOp.NOT;
            };
            currentFunc.addInstruction(Instruction.unary(dest, op, operand));
            return dest;
        }

        if (expr instanceof BinaryExpr binary) {
            return emitBinary(binary);
        }

        if (expr instanceof CallExpr call) {
            List<Integer> args = new ArrayList<>();
            for (Expr arg : call.args) {
                args.add(emitExpr(arg));
            }
            int dest = currentFunc.newTemp();
            currentFunc.addInstruction(Instruction.call(call.callee, dest, args));
            return dest;
        }

        Fatal.error("unsupported expression in IR builder");
        return -1; // unreachable
    }

    private int emitBinary(BinaryExpr binary) {
        if (binary.op == BinaryExpr.Op.LOGICAL_AND ||
            binary.op == BinaryExpr.Op.LOGICAL_OR) {
            int dest = currentFunc.newTemp();
            String shortLabel = newLabel(binary.op == BinaryExpr.Op.LOGICAL_AND ? "and_false" : "or_true");
            String endLabel = newLabel("logic_end");

            int left = emitExpr(binary.left);
            String trueBranchLabel = binary.op == BinaryExpr.Op.LOGICAL_AND ? shortLabel : endLabel;
            String falseBranchLabel = binary.op == BinaryExpr.Op.LOGICAL_AND ? endLabel : shortLabel;
            emitBranch(left, trueBranchLabel, falseBranchLabel);

            int right = emitExpr(binary.right);
            // Apply NOT twice to get truth value
            int not1 = currentFunc.newTemp();
            int not2 = currentFunc.newTemp();
            currentFunc.addInstruction(Instruction.unary(not1, UnaryOp.NOT, right));
            currentFunc.addInstruction(Instruction.unary(not2, UnaryOp.NOT, not1));
            currentFunc.addInstruction(Instruction.move(dest, not2));
            emitGoto(endLabel);

            emitLabel(shortLabel);
            currentFunc.addInstruction(Instruction.const_(dest, binary.op == BinaryExpr.Op.LOGICAL_AND ? 0 : 1));

            emitLabel(endLabel);
            return dest;
        }

        int left = emitExpr(binary.left);
        int right = emitExpr(binary.right);
        int dest = currentFunc.newTemp();

        BinaryOp op = switch (binary.op) {
            case LESS -> BinaryOp.LESS;
            case GREATER -> BinaryOp.GREATER;
            case LESS_EQUAL -> BinaryOp.LESS_EQUAL;
            case GREATER_EQUAL -> BinaryOp.GREATER_EQUAL;
            case EQUAL -> BinaryOp.EQUAL;
            case NOT_EQUAL -> BinaryOp.NOT_EQUAL;
            case ADD -> BinaryOp.ADD;
            case SUB -> BinaryOp.SUB;
            case MUL -> BinaryOp.MUL;
            case DIV -> BinaryOp.DIV;
            case MOD -> BinaryOp.MOD;
            default -> BinaryOp.ADD;
        };

        currentFunc.addInstruction(Instruction.binary(dest, op, left, right));
        return dest;
    }

    private void emitBranch(int cond, String trueLabel, String falseLabel) {
        currentFunc.addInstruction(Instruction.branch(cond, trueLabel, falseLabel));
    }

    private void storeName(String name, int value) {
        Binding binding = lookup(name);
        if (binding.kind == Binding.Kind.GLOBAL) {
            currentFunc.addInstruction(Instruction.storeGlobal(value, binding.label));
            return;
        }
        currentFunc.addInstruction(Instruction.move(binding.slot, value));
    }

    private String globalLabel(String name) {
        return ".G_" + name;
    }
}
