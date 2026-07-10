package com.toycc.ast;

/**
 * 访问者接口
 */
public interface NodeVisitor<R> {
    R visit(CompUnit node);
    R visit(ConstDecl node);
    R visit(VarDecl node);
    R visit(FuncDef node);
    R visit(Param node);
    R visit(BlockStmt node);
    R visit(EmptyStmt node);
    R visit(ExprStmt node);
    R visit(AssignStmt node);
    R visit(DeclStmt node);
    R visit(IfStmt node);
    R visit(WhileStmt node);
    R visit(BreakStmt node);
    R visit(ContinueStmt node);
    R visit(ReturnStmt node);
    R visit(NumberExpr node);
    R visit(NameExpr node);
    R visit(CallExpr node);
    R visit(UnaryExpr node);
    R visit(BinaryExpr node);
}
