package com.toycc.ir;

import java.util.*;

/**
 * IR 指令
 * 三地址代码格式
 */
public class Instruction {
    public InstOp op = InstOp.LABEL;
    public int dest = -1;     // 目标槽号
    public int lhs = -1;      // 左操作数槽号
    public int rhs = -1;      // 右操作数槽号
    public int value = 0;     // 常量值
    public UnaryOp unary = UnaryOp.PLUS;
    public BinaryOp binary = BinaryOp.ADD;
    public String label = "";         // 跳转目标标签
    public String falseLabel = "";    // 条件为假时的跳转标签
    public String name = "";          // 全局变量名或函数名
    public List<Integer> args = new ArrayList<>();  // 函数调用参数

    public Instruction() {}

    public static Instruction label(String label) {
        Instruction inst = new Instruction();
        inst.op = InstOp.LABEL;
        inst.label = label;
        return inst;
    }

    public static Instruction goto_(String label) {
        Instruction inst = new Instruction();
        inst.op = InstOp.GOTO;
        inst.label = label;
        return inst;
    }

    public static Instruction branch(int cond, String trueLabel, String falseLabel) {
        Instruction inst = new Instruction();
        inst.op = InstOp.BRANCH;
        inst.lhs = cond;
        inst.label = trueLabel;
        inst.falseLabel = falseLabel;
        return inst;
    }

    public static Instruction const_(int dest, int value) {
        Instruction inst = new Instruction();
        inst.op = InstOp.CONST;
        inst.dest = dest;
        inst.value = value;
        return inst;
    }

    public static Instruction move(int dest, int src) {
        Instruction inst = new Instruction();
        inst.op = InstOp.MOVE;
        inst.dest = dest;
        inst.lhs = src;
        return inst;
    }

    public static Instruction loadGlobal(int dest, String name) {
        Instruction inst = new Instruction();
        inst.op = InstOp.LOAD_GLOBAL;
        inst.dest = dest;
        inst.name = name;
        return inst;
    }

    public static Instruction storeGlobal(int src, String name) {
        Instruction inst = new Instruction();
        inst.op = InstOp.STORE_GLOBAL;
        inst.lhs = src;
        inst.name = name;
        return inst;
    }

    public static Instruction unary(int dest, UnaryOp op, int operand) {
        Instruction inst = new Instruction();
        inst.op = InstOp.UNARY;
        inst.dest = dest;
        inst.unary = op;
        inst.lhs = operand;
        return inst;
    }

    public static Instruction binary(int dest, BinaryOp op, int lhs, int rhs) {
        Instruction inst = new Instruction();
        inst.op = InstOp.BINARY;
        inst.dest = dest;
        inst.binary = op;
        inst.lhs = lhs;
        inst.rhs = rhs;
        return inst;
    }

    public static Instruction call(String callee, int dest, List<Integer> args) {
        Instruction inst = new Instruction();
        inst.op = InstOp.CALL;
        inst.name = callee;
        inst.dest = dest;
        inst.args.addAll(args);
        return inst;
    }

    public static Instruction ret(int value) {
        Instruction inst = new Instruction();
        inst.op = InstOp.RETURN;
        inst.lhs = value;
        return inst;
    }

    public static Instruction retVoid() {
        Instruction inst = new Instruction();
        inst.op = InstOp.RETURN_VOID;
        return inst;
    }
}
