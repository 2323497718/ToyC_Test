package com.toycc.ast;

/**
 * return 语句节点
 * 语法: return ; | return expr ;
 */
public class ReturnStmt extends Stmt {
    public final Expr value;  // null for void return

    public ReturnStmt(Expr value) {
        this.value = value;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
