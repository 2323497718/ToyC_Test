package com.toycc.ast;

/**
 * 常量声明节点
 * 语法: const int ID = expr;
 */
public class ConstDecl extends Node {
    public final String name;
    public final Expr initializer;

    public ConstDecl(String name, Expr initializer) {
        this.name = name;
        this.initializer = initializer;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
