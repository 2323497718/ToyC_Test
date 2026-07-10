package com.toycc.util;

/**
 * 致命错误工具类
 */
public class Fatal {
    public static void error(String message) {
        System.err.println("toycc: " + message);
        System.exit(1);
    }

    public static void error(String message, Throwable cause) {
        System.err.println("toycc: " + message);
        cause.printStackTrace();
        System.exit(1);
    }
}
