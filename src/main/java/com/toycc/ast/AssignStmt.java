package com.toycc.ast;

/**
 * 赋值语句节点
 * 语法: ID = expr ;
 */
public class AssignStmt extends Stmt {
    public final String name;
    public final Expr value;

    public AssignStmt(String name, Expr value) {
        this.name = name;
        this.value = value;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
