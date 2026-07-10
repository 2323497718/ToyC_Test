package com.toycc.ast;

import java.util.List;
import java.util.ArrayList;

/**
 * 语句块节点
 * 语法: { stmt* }
 */
public class BlockStmt extends Node {
    public final List<Stmt> statements = new ArrayList<>();

    public void addStatement(Stmt stmt) {
        statements.add(stmt);
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
