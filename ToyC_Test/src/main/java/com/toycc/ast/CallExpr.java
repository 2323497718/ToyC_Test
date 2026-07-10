package com.toycc.ast;

import java.util.List;
import java.util.ArrayList;

/**
 * 函数调用节点
 * 语法: ID '(' argListOpt ')'
 */
public class CallExpr extends Expr {
    public final String callee;
    public final List<Expr> args = new ArrayList<>();

    public CallExpr(String callee, List<Expr> args) {
        this.callee = callee;
        this.args.addAll(args);
    }

    @Override
    public <R> R accept(NodeVisitor<R> visitor) {
        return visitor.visit(this);
    }
}
