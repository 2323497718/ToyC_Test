package com.toycc.ast;

/**
 * 声明语句节点
 * 语法: decl
 */
public class DeclStmt extends Stmt {
    public final Decl decl;

    public DeclStmt(Decl decl) {
        this.decl = decl;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
