package com.toycc;

import com.toycc.ast.CompUnit;
import com.toycc.frontend.ToyCParser;
import com.toycc.ir.IRBuilder;
import com.toycc.ir.Program;
import com.toycc.optim.Optimizer;
import com.toycc.backend.RiscVEmitter;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

/**
 * ToyC 编译器主程序
 * 
 * 用法: java -jar toycc.jar [-opt] < input.tc > output.s
 */
public class Main {

    public static void main(String[] args) {
        boolean optimize = args.length > 0 && args[0].equals("-opt");

        try {
            // 读取输入
            String source = readStdin();
            
            // 1. 词法/语法分析
            CompUnit ast = ToyCParser.parse(source);
            
            // 2. 语义分析
            com.toycc.sema.SemanticAnalyzer analyzer = new com.toycc.sema.SemanticAnalyzer(ast);
            analyzer.analyze();
            
            // 3. 生成 IR
            Program ir = IRBuilder.build(ast);
            
            // 4. 优化
            Optimizer.optimize(ir);
            
            // 5. 生成 RISC-V 汇编
            RiscVEmitter emitter = new RiscVEmitter(ir);
            String assembly = emitter.emit();
            
            // 输出
            System.out.print(assembly);
            
        } catch (Exception e) {
            System.err.println("toycc: " + e.getMessage());
            System.exit(1);
        }
    }

    private static String readStdin() throws IOException {
        StringBuilder sb = new StringBuilder();
        try (BufferedReader reader = new BufferedReader(
                new InputStreamReader(System.in, StandardCharsets.UTF_8))) {
            String line;
            while ((line = reader.readLine()) != null) {
                sb.append(line).append('\n');
            }
        }
        return sb.toString();
    }
}
