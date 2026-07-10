package com.toycc.util;

/**
 * 编译器错误异常
 */
public class CompilerError extends RuntimeException {
    public CompilerError(String message) {
        super(message);
    }

    public CompilerError(String message, Throwable cause) {
        super(message, cause);
    }
}
