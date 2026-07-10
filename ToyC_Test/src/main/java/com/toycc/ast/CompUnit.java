package com.toycc.ast;

import java.util.List;
import java.util.ArrayList;

/**
 * 编译单元 - ToyC 程序的顶层结构
 */
public class CompUnit extends Node {
    public final List<Node> decls = new ArrayList<>();

    public void addDecl(Node decl) {
        decls.add(decl);
    }
}
