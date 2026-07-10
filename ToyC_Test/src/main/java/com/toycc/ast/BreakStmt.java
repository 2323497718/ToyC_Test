package com.toycc.ast;

/**
 * break 语句节点
 * 语法: break ;
 */
public class BreakStmt extends Stmt {
    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
