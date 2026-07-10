package com.toycc.ir;

/**
 * IR 指令操作码
 */
public enum InstOp {
    LABEL,
    GOTO,
    BRANCH,
    CONST,
    MOVE,
    LOAD_GLOBAL,
    STORE_GLOBAL,
    UNARY,
    BINARY,
    CALL,
    RETURN,
    RETURN_VOID
}
