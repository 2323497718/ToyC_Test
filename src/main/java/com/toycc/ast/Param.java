package com.toycc.ast;

/**
 * 函数形参节点
 * 语法: int ID
 */
public class Param extends Node {
    public final String name;

    public Param(String name) {
        this.name = name;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
