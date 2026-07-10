package com.toycc.ast;

/**
 * continue 语句节点
 * 语法: continue ;
 */
public class ContinueStmt extends Stmt {
    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
