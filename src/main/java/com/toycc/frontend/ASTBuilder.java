package com.toycc.frontend;

import com.toycc.ast.*;
import org.antlr.v4.runtime.tree.TerminalNode;

import java.util.List;

/**
 * 将 ANTLR 解析树转换为 AST
 */
public class ASTBuilder extends ToyCBaseVisitor<Node> {

    @Override
    public Node visitCompUnit(ToyCParser.CompUnitContext ctx) {
        CompUnit compUnit = new CompUnit();
        for (var child : ctx.children) {
            if (child instanceof ToyCParser.DeclContext || child instanceof ToyCParser.FuncDefContext) {
                compUnit.addDecl(visit(child));
            }
        }
        return compUnit;
    }

    @Override
    public Node visitDecl(ToyCParser.DeclContext ctx) {
        if (ctx.constDecl() != null) {
            return visitConstDecl(ctx.constDecl());
        } else {
            return visitVarDecl(ctx.varDecl());
        }
    }

    @Override
    public Node visitConstDecl(ToyCParser.ConstDeclContext ctx) {
        String name = ctx.ID().getText();
        Expr initializer = (Expr) visit(ctx.expr());
        return new ConstDecl(name, initializer);
    }

    @Override
    public Node visitVarDecl(ToyCParser.VarDeclContext ctx) {
        String name = ctx.ID().getText();
        Expr initializer = (Expr) visit(ctx.expr());
        return new VarDecl(name, initializer);
    }

    @Override
    public Node visitFuncDef(ToyCParser.FuncDefContext ctx) {
        FuncDef.ReturnType returnType;
        if (ctx.VOID() != null) {
            returnType = FuncDef.ReturnType.VOID;
        } else {
            returnType = FuncDef.ReturnType.INT;
        }

        String name = ctx.ID().getText();

        List<Param> params;
        if (ctx.paramListOpt() != null && ctx.paramListOpt().paramList() != null) {
            params = visitParamList(ctx.paramListOpt().paramList());
        } else {
            params = new java.util.ArrayList<>();
        }

        BlockStmt body = (BlockStmt) visitBlock(ctx.block());

        return new FuncDef(returnType, name, params, body);
    }

    private List<Param> visitParamList(ToyCParser.ParamListContext ctx) {
        List<Param> params = new java.util.ArrayList<>();
        for (TerminalNode id : ctx.ID()) {
            params.add(new Param(id.getText()));
        }
        return params;
    }

    @Override
    public Node visitBlock(ToyCParser.BlockContext ctx) {
        BlockStmt block = new BlockStmt();
        for (var stmtCtx : ctx.stmt()) {
            block.addStatement((Stmt) visit(stmtCtx));
        }
        return block;
    }

    @Override
    public Node visitStmt(ToyCParser.StmtContext ctx) {
        // 空语句
        if (ctx.getChild(0) instanceof TerminalNode && ctx.getChild(0).getText().equals(";")) {
            return new EmptyStmt();
        }

        // 块语句
        if (ctx.block() != null) {
            return visitBlock(ctx.block());
        }

        // if 语句
        if (ctx.IF() != null) {
            Expr condition = (Expr) visit(ctx.expr(0));
            Stmt thenBranch = (Stmt) visit(ctx.stmt(0));
            Stmt elseBranch = null;
            if (ctx.ELSE() != null) {
                elseBranch = (Stmt) visit(ctx.stmt(1));
            }
            return new IfStmt(condition, thenBranch, elseBranch);
        }

        // while 语句
        if (ctx.WHILE() != null) {
            Expr condition = (Expr) visit(ctx.expr(0));
            Stmt body = (Stmt) visit(ctx.stmt(0));
            return new WhileStmt(condition, body);
        }

        // break 语句
        if (ctx.BREAK() != null) {
            return new BreakStmt();
        }

        // continue 语句
        if (ctx.CONTINUE() != null) {
            return new ContinueStmt();
        }

        // return 语句
        if (ctx.RETURN() != null) {
            if (ctx.expr().isEmpty()) {
                return new ReturnStmt(null);
            } else {
                return new ReturnStmt((Expr) visit(ctx.expr(0)));
            }
        }

        // 赋值语句 ID = expr
        if (ctx.ID() != null && ctx.getChildCount() >= 3 && ctx.getChild(1).getText().equals("=")) {
            String name = ctx.ID().getText();
            Expr value = (Expr) visit(ctx.expr(0));
            return new AssignStmt(name, value);
        }

        // 声明语句
        if (ctx.decl() != null) {
            Node declNode = visit(ctx.decl());
            if (declNode instanceof Decl) {
                return new DeclStmt((Decl) declNode);
            }
        }

        // 表达式语句
        if (!ctx.expr().isEmpty()) {
            Expr expr = (Expr) visit(ctx.expr(0));
            return new ExprStmt(expr);
        }

        return new EmptyStmt();
    }

    @Override
    public Node visitExpr(ToyCParser.ExprContext ctx) {
        return visit(ctx.lorExpr(0));
    }

    @Override
    public Node visitLorExpr(ToyCParser.LorExprContext ctx) {
        if (ctx.landExpr().size() == 1) {
            return visit(ctx.landExpr(0));
        }

        Expr left = (Expr) visit(ctx.landExpr(0));
        for (int i = 1; i < ctx.landExpr().size(); i++) {
            Expr right = (Expr) visit(ctx.landExpr(i));
            left = new BinaryExpr(BinaryExpr.Op.LOGICAL_OR, left, right);
        }
        return left;
    }

    @Override
    public Node visitLandExpr(ToyCParser.LandExprContext ctx) {
        if (ctx.relExpr().size() == 1) {
            return visit(ctx.relExpr(0));
        }

        Expr left = (Expr) visit(ctx.relExpr(0));
        for (int i = 1; i < ctx.relExpr().size(); i++) {
            Expr right = (Expr) visit(ctx.relExpr(i));
            left = new BinaryExpr(BinaryExpr.Op.LOGICAL_AND, left, right);
        }
        return left;
    }

    @Override
    public Node visitRelExpr(ToyCParser.RelExprContext ctx) {
        if (ctx.addExpr().size() == 1) {
            return visit(ctx.addExpr(0));
        }

        Expr left = (Expr) visit(ctx.addExpr(0));
        for (int i = 0; i < ctx.relOp().size(); i++) {
            BinaryExpr.Op op = convertRelOp(ctx.relOp(i).getText());
            Expr right = (Expr) visit(ctx.addExpr(i + 1));
            left = new BinaryExpr(op, left, right);
        }
        return left;
    }

    @Override
    public Node visitAddExpr(ToyCParser.AddExprContext ctx) {
        if (ctx.mulExpr().size() == 1) {
            return visit(ctx.mulExpr(0));
        }

        Expr left = (Expr) visit(ctx.mulExpr(0));
        for (int i = 0; i < ctx.addOp().size(); i++) {
            BinaryExpr.Op op = ctx.addOp(i).getText().equals("+") ?
                    BinaryExpr.Op.ADD : BinaryExpr.Op.SUB;
            Expr right = (Expr) visit(ctx.mulExpr(i + 1));
            left = new BinaryExpr(op, left, right);
        }
        return left;
    }

    @Override
    public Node visitMulExpr(ToyCParser.MulExprContext ctx) {
        if (ctx.unaryExpr().size() == 1) {
            return visit(ctx.unaryExpr(0));
        }

        Expr left = (Expr) visit(ctx.unaryExpr(0));
        for (int i = 0; i < ctx.mulOp().size(); i++) {
            BinaryExpr.Op op = convertMulOp(ctx.mulOp(i).getText());
            Expr right = (Expr) visit(ctx.unaryExpr(i + 1));
            left = new BinaryExpr(op, left, right);
        }
        return left;
    }

    @Override
    public Node visitUnaryExpr(ToyCParser.UnaryExprContext ctx) {
        if (ctx.unaryExpr() == null) {
            return visit(ctx.primaryExpr());
        }

        UnaryExpr.Op op;
        String opText = ctx.getChild(0).getText();
        switch (opText) {
            case "+" -> op = UnaryExpr.Op.PLUS;
            case "-" -> op = UnaryExpr.Op.MINUS;
            case "!" -> op = UnaryExpr.Op.NOT;
            default -> throw new RuntimeException("Unknown unary operator: " + opText);
        }

        Expr operand = (Expr) visit(ctx.unaryExpr());
        return new UnaryExpr(op, operand);
    }

    @Override
    public Node visitPrimaryExpr(ToyCParser.PrimaryExprContext ctx) {
        // 数字
        if (ctx.NUMBER() != null) {
            String numText = ctx.NUMBER().getText();
            int value = Integer.parseInt(numText);
            return new NumberExpr(value);
        }

        // 括号表达式
        if (ctx.expr() != null) {
            return visit(ctx.expr());
        }

        // 函数调用
        if (ctx.ID() != null && ctx.argListOpt() != null) {
            String callee = ctx.ID().getText();
            List<Expr> args = new java.util.ArrayList<>();
            if (ctx.argListOpt().argList() != null) {
                for (ToyCParser.ExprContext argCtx : ctx.argListOpt().argList().expr()) {
                    args.add((Expr) visit(argCtx));
                }
            }
            return new CallExpr(callee, args);
        }

        // 标识符
        if (ctx.ID() != null) {
            return new NameExpr(ctx.ID().getText());
        }

        throw new RuntimeException("Unknown primary expression");
    }

    private BinaryExpr.Op convertRelOp(String op) {
        return switch (op) {
            case "<" -> BinaryExpr.Op.LESS;
            case ">" -> BinaryExpr.Op.GREATER;
            case "<=" -> BinaryExpr.Op.LESS_EQUAL;
            case ">=" -> BinaryExpr.Op.GREATER_EQUAL;
            case "==" -> BinaryExpr.Op.EQUAL;
            case "!=" -> BinaryExpr.Op.NOT_EQUAL;
            default -> throw new RuntimeException("Unknown relational operator: " + op);
        };
    }

    private BinaryExpr.Op convertMulOp(String op) {
        return switch (op) {
            case "*" -> BinaryExpr.Op.MUL;
            case "/" -> BinaryExpr.Op.DIV;
            case "%" -> BinaryExpr.Op.MOD;
            default -> throw new RuntimeException("Unknown multiplicative operator: " + op);
        };
    }
}
