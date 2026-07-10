package com.toycc.ast;

/**
 * 二元运算表达式节点
 * 语法: expr op expr
 */
public class BinaryExpr extends Expr {
    public enum Op {
        // Logical
        LOGICAL_OR,
        LOGICAL_AND,
        // Relational
        LESS,
        GREATER,
        LESS_EQUAL,
        GREATER_EQUAL,
        EQUAL,
        NOT_EQUAL,
        // Arithmetic
        ADD,
        SUB,
        MUL,
        DIV,
        MOD
    }

    public final Op op;
    public final Expr left;
    public final Expr right;

    public BinaryExpr(Op op, Expr left, Expr right) {
        this.op = op;
        this.left = left;
        this.right = right;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
