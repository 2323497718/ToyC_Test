package com.toycc.sema;

import com.toycc.ast.*;
import com.toycc.util.Fatal;

import java.util.*;

/**
 * 语义分析器
 */
public class SemanticAnalyzer {

    private static class FunctionInfo {
        FuncDef.ReturnType returnType;
        int arity;
        int order;

        FunctionInfo(FuncDef.ReturnType returnType, int arity, int order) {
            this.returnType = returnType;
            this.arity = arity;
            this.order = order;
        }
    }

    private static class ValueInfo {
        boolean isConst;

        ValueInfo(boolean isConst) {
            this.isConst = isConst;
        }
    }

    private final CompUnit program;
    private final Map<String, FunctionInfo> functions = new HashMap<>();
    private final List<Map<String, ValueInfo>> scopes = new ArrayList<>();
    private FuncDef.ReturnType currentReturnType = FuncDef.ReturnType.INT;
    private String currentFunctionName = "";
    private int currentFunctionOrder = 0;
    private int nextFunctionOrder = 0;
    private int loopDepth = 0;

    public SemanticAnalyzer(CompUnit program) {
        this.program = program;
    }

    public void analyze() {
        collectGlobalsAndFunctions();
        requireMain();
        for (Node decl : program.decls) {
            if (decl instanceof FuncDef func) {
                analyzeFunction(func);
            }
        }
    }

    private void collectGlobalsAndFunctions() {
        pushScope();
        for (Node decl : program.decls) {
            if (decl instanceof ConstDecl constant) {
                analyzeConstExpr(constant.initializer);
                declareValue(constant.name, true);
            } else if (decl instanceof VarDecl var) {
                analyzeConstExpr(var.initializer);
                declareValue(var.name, false);
            } else if (decl instanceof FuncDef func) {
                if (lookupValueLocal(func.name) != null) {
                    Fatal.error("function conflicts with global value: " + func.name);
                }
                if (!functions.putIfAbsent(func.name,
                        new FunctionInfo(func.returnType, func.params.size(), nextFunctionOrder++)) == null) {
                    // successfully inserted
                } else {
                    Fatal.error("duplicate function: " + func.name);
                }
            }
        }
    }

    private void requireMain() {
        FunctionInfo main = functions.get("main");
        if (main == null ||
            main.returnType != FuncDef.ReturnType.INT ||
            main.arity != 0) {
            Fatal.error("missing valid int main()");
        }
    }

    private void analyzeFunction(FuncDef func) {
        currentReturnType = func.returnType;
        currentFunctionName = func.name;
        currentFunctionOrder = functions.get(func.name).order;
        loopDepth = 0;
        pushScope();
        for (Param param : func.params) {
            declareValue(param.name, false);
        }
        analyzeStmt(func.body);
        popScope();
        if (func.returnType == FuncDef.ReturnType.INT && !mustReturn(func.body)) {
            Fatal.error("int function may not return on all paths: " + func.name);
        }
    }

    private void analyzeStmt(Stmt stmt) {
        if (stmt instanceof EmptyStmt) {
            return;
        }

        if (stmt instanceof BlockStmt block) {
            pushScope();
            for (Stmt s : block.statements) {
                analyzeStmt(s);
            }
            popScope();
            return;
        }

        if (stmt instanceof ExprStmt exprStmt) {
            analyzeExpr(exprStmt.expr, false);
            return;
        }

        if (stmt instanceof AssignStmt assign) {
            ValueInfo value = lookupValue(assign.name);
            if (value.isConst) {
                Fatal.error("assignment to constant: " + assign.name);
            }
            analyzeExpr(assign.value, true);
            return;
        }

        if (stmt instanceof DeclStmt declStmt) {
            analyzeDecl(declStmt.decl);
            return;
        }

        if (stmt instanceof IfStmt ifStmt) {
            analyzeExpr(ifStmt.condition, true);
            analyzeStmt(ifStmt.thenBranch);
            if (ifStmt.elseBranch != null) {
                analyzeStmt(ifStmt.elseBranch);
            }
            return;
        }

        if (stmt instanceof WhileStmt whileStmt) {
            analyzeExpr(whileStmt.condition, true);
            loopDepth++;
            analyzeStmt(whileStmt.body);
            loopDepth--;
            return;
        }

        if (stmt instanceof BreakStmt) {
            if (loopDepth == 0) {
                Fatal.error("break outside loop");
            }
            return;
        }

        if (stmt instanceof ContinueStmt) {
            if (loopDepth == 0) {
                Fatal.error("continue outside loop");
            }
            return;
        }

        if (stmt instanceof ReturnStmt ret) {
            if (currentReturnType == FuncDef.ReturnType.VOID) {
                if (ret.value != null) {
                    Fatal.error("void function returns a value");
                }
            } else {
                if (ret.value == null) {
                    Fatal.error("int function returns without a value");
                }
                analyzeExpr(ret.value, true);
            }
            return;
        }

        Fatal.error("unsupported statement in semantic analysis");
    }

    private void analyzeDecl(Decl decl) {
        if (decl instanceof ConstDecl constant) {
            analyzeConstExpr(constant.initializer);
            declareValue(constant.name, true);
            return;
        }

        if (decl instanceof VarDecl var) {
            analyzeExpr(var.initializer, true);
            declareValue(var.name, false);
            return;
        }

        Fatal.error("function declaration inside block");
    }

    private void analyzeExpr(Expr expr, boolean requireValue) {
        if (expr instanceof NumberExpr) {
            return;
        }

        if (expr instanceof NameExpr name) {
            lookupValue(name.name);
            return;
        }

        if (expr instanceof UnaryExpr unary) {
            analyzeExpr(unary.operand, true);
            return;
        }

        if (expr instanceof BinaryExpr binary) {
            analyzeExpr(binary.left, true);
            analyzeExpr(binary.right, true);
            return;
        }

        if (expr instanceof CallExpr call) {
            FunctionInfo called = functions.get(call.callee);
            if (called == null) {
                Fatal.error("call to unknown function: " + call.callee);
            }
            if (!currentFunctionName.equals(call.callee) && called.order >= currentFunctionOrder) {
                Fatal.error("call to function before declaration: " + call.callee);
            }
            if (called.arity != call.args.size()) {
                Fatal.error("wrong argument count: " + call.callee);
            }
            if (requireValue && called.returnType == FuncDef.ReturnType.VOID) {
                Fatal.error("void function used as value: " + call.callee);
            }
            for (Expr arg : call.args) {
                analyzeExpr(arg, true);
            }
            return;
        }

        Fatal.error("unsupported expression in semantic analysis");
    }

    private void analyzeConstExpr(Expr expr) {
        if (expr instanceof NumberExpr) {
            return;
        }

        if (expr instanceof NameExpr name) {
            ValueInfo value = lookupValue(name.name);
            if (!value.isConst) {
                Fatal.error("non-constant in constant initializer: " + name.name);
            }
            return;
        }

        if (expr instanceof UnaryExpr unary) {
            analyzeConstExpr(unary.operand);
            return;
        }

        if (expr instanceof BinaryExpr binary) {
            analyzeConstExpr(binary.left);
            analyzeConstExpr(binary.right);
            return;
        }

        Fatal.error("call in constant initializer");
    }

    private boolean mustReturn(Stmt stmt) {
        if (stmt instanceof ReturnStmt) {
            return true;
        }

        if (stmt instanceof BlockStmt block) {
            for (Stmt s : block.statements) {
                if (mustReturn(s)) {
                    return true;
                }
            }
            return false;
        }

        if (stmt instanceof IfStmt ifStmt) {
            return ifStmt.elseBranch != null &&
                   mustReturn(ifStmt.thenBranch) &&
                   mustReturn(ifStmt.elseBranch);
        }

        return false;
    }

    private void pushScope() {
        scopes.add(new HashMap<>());
    }

    private void popScope() {
        scopes.remove(scopes.size() - 1);
    }

    private void declareValue(String name, boolean isConst) {
        Map<String, ValueInfo> scope = scopes.get(scopes.size() - 1);
        ValueInfo existing = scope.put(name, new ValueInfo(isConst));
        if (existing != null) {
            Fatal.error("duplicate name in scope: " + name);
        }
    }

    private ValueInfo lookupValueLocal(String name) {
        if (scopes.isEmpty()) {
            return null;
        }
        Map<String, ValueInfo> scope = scopes.get(scopes.size() - 1);
        return scope.get(name);
    }

    private ValueInfo lookupValue(String name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            ValueInfo info = scopes.get(i).get(name);
            if (info != null) {
                return info;
            }
        }
        Fatal.error("unknown value: " + name);
        return null; // unreachable
    }
}
