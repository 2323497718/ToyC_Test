package com.toycc.ast;

/**
 * 标识符引用节点
 * 语法: ID
 */
public class NameExpr extends Expr {
    public final String name;

    public NameExpr(String name) {
        this.name = name;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
