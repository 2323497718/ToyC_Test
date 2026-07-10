package com.toycc.ast;

/**
 * AST 节点基类
 */
public abstract class Node {
    // 用于访问者模式
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
