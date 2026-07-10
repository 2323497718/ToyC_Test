package com.toycc.ast;

/**
 * 数字字面量节点
 * 语法: NUMBER
 */
public class NumberExpr extends Expr {
    public final int value;

    public NumberExpr(int value) {
        this.value = value;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
