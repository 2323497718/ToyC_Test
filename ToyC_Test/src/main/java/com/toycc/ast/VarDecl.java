package com.toycc.ast;

/**
 * 变量声明节点
 * 语法: int ID = expr;
 */
public class VarDecl extends Node {
    public final String name;
    public final Expr initializer;

    public VarDecl(String name, Expr initializer) {
        this.name = name;
        this.initializer = initializer;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
