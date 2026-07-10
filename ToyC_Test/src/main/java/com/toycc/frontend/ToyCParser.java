package com.toycc.frontend;

import com.toycc.ast.*;
import com.toycc.util.Fatal;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.tree.*;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

/**
 * ToyC 编译器前端 - 解析器
 */
public class ToyCParser {

    /**
     * 解析 ToyC 源代码，返回 AST
     */
    public static CompUnit parse(String source) {
        ANTLRInputStream input = new ANTLRInputStream(source);
        ToyCLexer lexer = new ToyCLexer(input);
        lexer.removeErrorListeners();
        lexer.addErrorListener(new LexerErrorListener());

        CommonTokenStream tokens = new CommonTokenStream(lexer);
        ToyCParser parser = new ToyCParser(tokens);
        parser.removeErrorListeners();
        parser.addErrorListener(new ParserErrorListener());

        ParseTree tree = parser.compUnit();
        ASTBuilder builder = new ASTBuilder();
        CompUnit ast = builder.visitCompUnit(tree);
        return ast;
    }

    /**
     * 从输入流解析
     */
    public static CompUnit parse(InputStream in) throws IOException {
        String source = new String(in.readAllBytes(), StandardCharsets.UTF_8);
        return parse(source);
    }

    // ===== 词法错误监听器 =====
    private static class LexerErrorListener implements ANTLRErrorListener {
        @Override
        public void syntaxError(Recognizer<?, ?> recognizer, Object offendingSymbol,
                                int line, int charPositionInLine, String msg,
                                RecognitionException e) {
            Fatal.error("lexer error at line " + line + ": " + msg);
        }

        @Override
        public void reportAmbiguity(Parser recognizer, DFA dfa, int startIndex,
                                    int stopIndex, boolean exact, java.util.BitSet ambigAlts,
                                    ATNConfigSet configs) {}

        @Override
        public void reportAttemptingFullContext(Parser recognizer, DFA dfa,
                                                 int startIndex, int stopIndex,
                                                 java.util.BitSet conflictingAlts,
                                                 ATNConfigSet configs) {}

        @Override
        public void reportContextSensitivity(Parser recognizer, DFA dfa,
                                              int startIndex, int stopIndex,
                                              int decision, ATNConfigSet configs) {}
    }

    // ===== 语法错误监听器 =====
    private static class ParserErrorListener implements ANTLRErrorListener {
        @Override
        public void syntaxError(Recognizer<?, ?> recognizer, Object offendingSymbol,
                                int line, int charPositionInLine, String msg,
                                RecognitionException e) {
            Fatal.error("syntax error at line " + line + ": " + msg);
        }

        @Override
        public void reportAmbiguity(Parser recognizer, DFA dfa, int startIndex,
                                    int stopIndex, boolean exact, java.util.BitSet ambigAlts,
                                    ATNConfigSet configs) {}

        @Override
        public void reportAttemptingFullContext(Parser recognizer, DFA dfa,
                                                 int startIndex, int stopIndex,
                                                 java.util.BitSet conflictingAlts,
                                                 ATNConfigSet configs) {}

        @Override
        public void reportContextSensitivity(Parser recognizer, DFA dfa,
                                              int startIndex, int stopIndex,
                                              int decision, ATNConfigSet configs) {}
    }
}
