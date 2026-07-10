package com.toycc.ast;

/**
 * 一元运算表达式节点
 * 语法: (+ | - | !) unaryExpr
 */
public class UnaryExpr extends Expr {
    public enum Op { PLUS, MINUS, NOT }

    public final Op op;
    public final Expr operand;

    public UnaryExpr(Op op, Expr operand) {
        this.op = op;
        this.operand = operand;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
