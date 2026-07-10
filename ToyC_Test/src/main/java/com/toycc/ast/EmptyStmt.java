package com.toycc.ast;

/**
 * 空语句节点
 * 语法: ;
 */
public class EmptyStmt extends Stmt {
    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
