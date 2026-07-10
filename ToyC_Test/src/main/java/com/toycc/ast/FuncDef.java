package com.toycc.ast;

/**
 * 函数定义节点
 * 语法: (int | void) ID '(' paramListOpt ')' block
 */
public class FuncDef extends Node {
    public enum ReturnType { INT, VOID }

    public final ReturnType returnType;
    public final String name;
    public final List<Param> params;
    public final BlockStmt body;

    public FuncDef(ReturnType returnType, String name, List<Param> params, BlockStmt body) {
        this.returnType = returnType;
        this.name = name;
        this.params = params;
        this.body = body;
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
