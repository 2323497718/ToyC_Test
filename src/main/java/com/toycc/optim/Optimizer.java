package com.toycc.optim;

import com.toycc.ir.*;
import com.toycc.util.Fatal;

import java.util.*;

/**
 * IR 优化器 - 包含多种优化 pass
 */
public class Optimizer {

    public static void optimize(Program program) {
        for (Function func : program.functions) {
            boolean changed = true;
            while (changed) {
                changed = false;
                // 顺序执行各个优化 pass
                if (constFoldPass(func)) changed = true;
                if (copyPropPass(func)) changed = true;
                if (csePass(func)) changed = true;
                if (branchFoldPass(func)) changed = true;
                if (gotoCleanupPass(func)) changed = true;
                if (dcePass(func)) changed = true;
                if (deadBlockPass(func)) changed = true;
            }
        }
    }

    // ===== 辅助方法 =====

    private static List<Integer> readsOf(Instruction inst) {
        List<Integer> result = new ArrayList<>();
        switch (inst.op) {
            case MOVE, STORE_GLOBAL, UNARY, BRANCH, RETURN -> {
                if (inst.lhs != -1) result.add(inst.lhs);
            }
            case BINARY -> {
                if (inst.lhs != -1) result.add(inst.lhs);
                if (inst.rhs != -1) result.add(inst.rhs);
            }
            case CALL -> {
                for (int arg : inst.args) {
                    if (arg != -1) result.add(arg);
                }
            }
            default -> {}
        }
        return result;
    }

    private static boolean definesSlot(Instruction inst) {
        return switch (inst.op) {
            case CONST, MOVE, LOAD_GLOBAL, UNARY, BINARY, CALL -> true;
            default -> false;
        };
    }

    private static boolean isControlBoundary(Instruction inst) {
        return switch (inst.op) {
            case LABEL, GOTO, BRANCH, RETURN, RETURN_VOID -> true;
            default -> false;
        };
    }

    private static int foldConstUnary(UnaryOp op, int v) {
        return switch (op) {
            case PLUS -> v;
            case MINUS -> -v;
            case NOT -> (v != 0) ? 0 : 1;
        };
    }

    private static int foldConstBinary(BinaryOp op, int a, int b) {
        return switch (op) {
            case LESS -> a < b ? 1 : 0;
            case GREATER -> a > b ? 1 : 0;
            case LESS_EQUAL -> a <= b ? 1 : 0;
            case GREATER_EQUAL -> a >= b ? 1 : 0;
            case EQUAL -> a == b ? 1 : 0;
            case NOT_EQUAL -> a != b ? 1 : 0;
            case ADD -> a + b;
            case SUB -> a - b;
            case MUL -> a * b;
            case DIV -> b != 0 ? a / b : 0;
            case MOD -> b != 0 ? a % b : 0;
        };
    }

    private static Map<Integer, Integer> computeUseCount(Function fn) {
        Map<Integer, Integer> uses = new HashMap<>();
        for (Instruction inst : fn.instructions) {
            for (int s : readsOf(inst)) {
                uses.merge(s, 1, Integer::sum);
            }
        }
        return uses;
    }

    // ===== 常量折叠 pass =====
    private static boolean constFoldPass(Function fn) {
        boolean any = false;
        Map<Integer, Integer> known = new HashMap<>();
        List<Instruction> kept = new ArrayList<>();

        for (int i = 0; i < fn.instructions.size(); i++) {
            Instruction inst = fn.instructions.get(i);

            if (inst.op == InstOp.LABEL) {
                known.clear();
                kept.add(inst);
                continue;
            }

            // 处理常量折叠
            switch (inst.op) {
                case CONST -> {
                    known.put(inst.dest, inst.value);
                    kept.add(inst);
                }
                case MOVE -> {
                    Integer val = known.get(inst.lhs);
                    if (val != null) {
                        // 折叠为常量
                        Instruction folded = Instruction.const_(inst.dest, val);
                        folded.dest = inst.dest;
                        known.put(inst.dest, val);
                        kept.add(folded);
                        any = true;
                    } else {
                        known.remove(inst.dest);
                        kept.add(inst);
                    }
                }
                case UNARY -> {
                    Integer val = known.get(inst.lhs);
                    if (val != null) {
                        int result = foldConstUnary(inst.unary, val);
                        Instruction folded = Instruction.const_(inst.dest, result);
                        known.put(inst.dest, result);
                        kept.add(folded);
                        any = true;
                    } else {
                        known.remove(inst.dest);
                        kept.add(inst);
                    }
                }
                case BINARY -> {
                    Integer lv = known.get(inst.lhs);
                    Integer rv = known.get(inst.rhs);
                    if (lv != null && rv != null) {
                        int result = foldConstBinary(inst.binary, lv, rv);
                        Instruction folded = Instruction.const_(inst.dest, result);
                        known.put(inst.dest, result);
                        kept.add(folded);
                        any = true;
                    } else {
                        // 代数简化
                        boolean simplified = false;
                        if (inst.binary == BinaryOp.ADD) {
                            if (lv != null && lv == 0) {
                                Instruction s = Instruction.move(inst.dest, inst.rhs);
                                kept.add(s);
                                known.remove(inst.dest);
                                simplified = true;
                                any = true;
                            } else if (rv != null && rv == 0) {
                                Instruction s = Instruction.move(inst.dest, inst.lhs);
                                kept.add(s);
                                known.remove(inst.dest);
                                simplified = true;
                                any = true;
                            }
                        } else if (inst.binary == BinaryOp.SUB && rv != null && rv == 0) {
                            Instruction s = Instruction.move(inst.dest, inst.lhs);
                            kept.add(s);
                            known.remove(inst.dest);
                            simplified = true;
                            any = true;
                        } else if (inst.binary == BinaryOp.MUL) {
                            if ((lv != null && lv == 0) || (rv != null && rv == 0)) {
                                Instruction s = Instruction.const_(inst.dest, 0);
                                kept.add(s);
                                known.put(inst.dest, 0);
                                simplified = true;
                                any = true;
                            } else if ((lv != null && lv == 1) || (rv != null && rv == 1)) {
                                int src = (lv != null && lv == 1) ? inst.rhs : inst.lhs;
                                Instruction s = Instruction.move(inst.dest, src);
                                kept.add(s);
                                known.remove(inst.dest);
                                simplified = true;
                                any = true;
                            }
                        }
                        if (!simplified) {
                            known.remove(inst.dest);
                            kept.add(inst);
                        }
                    }
                }
                case LOAD_GLOBAL, CALL -> {
                    known.remove(inst.dest);
                    kept.add(inst);
                }
                case GOTO, BRANCH, RETURN, RETURN_VOID -> {
                    known.clear();
                    kept.add(inst);
                }
                default -> kept.add(inst);
            }
        }

        if (any) {
            fn.instructions = kept;
        }
        return any;
    }

    // ===== 复制传播 pass =====
    private static boolean copyPropPass(Function fn) {
        boolean any = false;
        Map<Integer, Integer> alias = new HashMap<>();
        List<Instruction> kept = new ArrayList<>();

        for (Instruction inst : fn.instructions) {
            if (inst.op == InstOp.LABEL) {
                alias.clear();
                kept.add(inst);
                continue;
            }

            switch (inst.op) {
                case MOVE -> {
                    int src = subst(inst.lhs, alias);
                    int dest = inst.dest;
                    alias.remove(dest);
                    if (dest != src) {
                        alias.put(dest, src);
                    }
                    kept.add(Instruction.move(dest, src));
                    any = true;
                }
                case UNARY -> {
                    int src = subst(inst.lhs, alias);
                    alias.remove(inst.dest);
                    kept.add(Instruction.unary(inst.dest, inst.unary, src));
                    any = true;
                }
                case BINARY -> {
                    int lhs = subst(inst.lhs, alias);
                    int rhs = subst(inst.rhs, alias);
                    alias.remove(inst.dest);
                    if (lhs != inst.lhs || rhs != inst.rhs) {
                        any = true;
                    }
                    kept.add(Instruction.binary(inst.dest, inst.binary, lhs, rhs));
                }
                case CONST, LOAD_GLOBAL, CALL -> {
                    alias.remove(inst.dest);
                    kept.add(inst);
                }
                case STORE_GLOBAL, BRANCH, RETURN -> {
                    int lhs = subst(inst.lhs, alias);
                    if (lhs != inst.lhs) {
                        any = true;
                    }
                    if (inst.op == InstOp.BRANCH || inst.op == InstOp.RETURN) {
                        alias.clear();
                    }
                    kept.add(cloneWithLhs(inst, lhs));
                }
                case GOTO, RETURN_VOID -> {
                    alias.clear();
                    kept.add(inst);
                }
                default -> kept.add(inst);
            }
        }

        if (any) {
            fn.instructions = kept;
        }
        return any;
    }

    private static int subst(int slot, Map<Integer, Integer> alias) {
        int cur = slot;
        Set<Integer> seen = new HashSet<>();
        while (alias.containsKey(cur) && !seen.contains(cur)) {
            seen.add(cur);
            cur = alias.get(cur);
        }
        return cur;
    }

    private static Instruction cloneWithLhs(Instruction inst, int lhs) {
        return switch (inst.op) {
            case BRANCH -> Instruction.branch(lhs, inst.label, inst.falseLabel);
            case RETURN -> Instruction.ret(lhs);
            case STORE_GLOBAL -> Instruction.storeGlobal(lhs, inst.name);
            default -> inst;
        };
    }

    // ===== 公共子表达式消除 pass =====
    private static boolean csePass(Function fn) {
        boolean any = false;
        Map<Integer, Integer> known = new HashMap<>();
        Map<Long, Integer> seen = new HashMap<>();

        for (int i = 0; i < fn.instructions.size(); i++) {
            Instruction inst = fn.instructions.get(i);

            if (inst.op == InstOp.LABEL) {
                known.clear();
                seen.clear();
                continue;
            }

            if (isControlBoundary(inst)) {
                known.clear();
                seen.clear();
                continue;
            }

            if (inst.op == InstOp.CONST && inst.dest != -1) {
                known.put(inst.dest, inst.value);
            }

            if (inst.op == InstOp.BINARY && inst.dest != -1) {
                // 构建 key
                int a = inst.lhs;
                int b = inst.rhs;
                // 规范化可交换运算
                if (inst.binary == BinaryOp.ADD || inst.binary == BinaryOp.MUL) {
                    if (a > b) {
                        int tmp = a;
                        a = b;
                        b = tmp;
                    }
                }
                int opCode = inst.binary.ordinal();
                long key = ((long) opCode << 48) | ((long) a << 32) | (b & 0xFFFFFFFFL);

                Integer prev = seen.get(key);
                if (prev != null) {
                    // 发现公共子表达式，复用之前的结果
                    Instruction reused = Instruction.move(inst.dest, prev);
                    fn.instructions.set(i, reused);
                    any = true;
                } else {
                    seen.put(key, inst.dest);
                }
                known.remove(inst.dest);
            } else if (definesSlot(inst) && inst.dest != -1) {
                known.remove(inst.dest);
            }
        }

        return any;
    }

    // ===== 分支折叠 pass =====
    private static boolean branchFoldPass(Function fn) {
        boolean any = false;
        Map<Integer, Integer> known = new HashMap<>();
        List<Instruction> kept = new ArrayList<>();

        for (int i = 0; i < fn.instructions.size(); i++) {
            Instruction inst = fn.instructions.get(i);

            if (inst.op == InstOp.LABEL) {
                known.clear();
                kept.add(inst);
                continue;
            }

            if (inst.op == InstOp.BRANCH && inst.lhs != -1) {
                Integer val = known.get(inst.lhs);
                if (val != null) {
                    String target = (val == 0) ? inst.falseLabel : inst.label;
                    if (!target.isEmpty()) {
                        kept.add(Instruction.goto_(target));
                    }
                    any = true;
                    known.clear();
                    continue;
                }
                known.clear();
                kept.add(inst);
                continue;
            }

            // 更新已知值
            switch (inst.op) {
                case CONST -> known.put(inst.dest, inst.value);
                case MOVE -> {
                    Integer val = known.get(inst.lhs);
                    if (val != null) {
                        known.put(inst.dest, val);
                    } else {
                        known.remove(inst.dest);
                    }
                }
                case UNARY -> {
                    Integer val = known.get(inst.lhs);
                    if (val != null) {
                        known.put(inst.dest, foldConstUnary(inst.unary, val));
                    } else {
                        known.remove(inst.dest);
                    }
                }
                case BINARY -> {
                    Integer lv = known.get(inst.lhs);
                    Integer rv = known.get(inst.rhs);
                    if (lv != null && rv != null) {
                        known.put(inst.dest, foldConstBinary(inst.binary, lv, rv));
                    } else {
                        known.remove(inst.dest);
                    }
                }
                case LOAD_GLOBAL, CALL -> known.remove(inst.dest);
                case GOTO, RETURN, RETURN_VOID -> known.clear();
                default -> {}
            }

            kept.add(inst);
        }

        if (any) {
            fn.instructions = kept;
        }
        return any;
    }

    // ===== Goto 清理 pass =====
    private static boolean gotoCleanupPass(Function fn) {
        boolean any = false;
        List<Instruction> kept = new ArrayList<>();

        for (int i = 0; i < fn.instructions.size(); i++) {
            Instruction inst = fn.instructions.get(i);
            if (inst.op == InstOp.GOTO && i + 1 < fn.instructions.size()) {
                Instruction next = fn.instructions.get(i + 1);
                if (next.op == InstOp.LABEL && next.label.equals(inst.label)) {
                    any = true;
                    continue; // 删除无用的 goto
                }
            }
            kept.add(inst);
        }

        if (any) {
            fn.instructions = kept;
        }
        return any;
    }

    // ===== 死代码消除 pass =====
    private static boolean dcePass(Function fn) {
        boolean any = false;
        boolean changed = true;

        while (changed) {
            changed = false;
            Map<Integer, Integer> uses = computeUseCount(fn);
            List<Instruction> kept = new ArrayList<>();

            for (Instruction inst : fn.instructions) {
                if (definesSlot(inst)) {
                    if (inst.dest != -1 && !uses.containsKey(inst.dest)) {
                        changed = true;
                        any = true;
                        continue; // 删除死代码
                    }
                }
                kept.add(inst);
            }

            fn.instructions = kept;
        }

        return any;
    }

    // ===== 死块消除 pass =====
    private static boolean deadBlockPass(Function fn) {
        if (fn.instructions.isEmpty()) return false;

        Map<String, Integer> labelPos = new HashMap<>();
        for (int i = 0; i < fn.instructions.size(); i++) {
            Instruction inst = fn.instructions.get(i);
            if (inst.op == InstOp.LABEL) {
                labelPos.put(inst.label, i);
            }
        }

        boolean[] reachable = new boolean[fn.instructions.size];
        Arrays.fill(reachable, false);
        Deque<Integer> worklist = new ArrayDeque<>();
        worklist.add(0);

        while (!worklist.isEmpty()) {
            int idx = worklist.removeFirst();
            if (idx >= fn.instructions.size() || reachable[idx]) continue;
            reachable[idx] = true;

            Instruction inst = fn.instructions.get(idx);
            switch (inst.op) {
                case GOTO -> {
                    Integer target = labelPos.get(inst.label);
                    if (target != null) worklist.add(target);
                }
                case BRANCH -> {
                    if (!inst.label.isEmpty()) {
                        Integer t = labelPos.get(inst.label);
                        if (t != null) worklist.add(t);
                    } else if (idx + 1 < fn.instructions.size()) {
                        worklist.add(idx + 1);
                    }
                    if (!inst.falseLabel.isEmpty()) {
                        Integer t = labelPos.get(inst.falseLabel);
                        if (t != null) worklist.add(t);
                    } else if (idx + 1 < fn.instructions.size()) {
                        worklist.add(idx + 1);
                    }
                }
                case RETURN, RETURN_VOID -> {}
                default -> {
                    if (idx + 1 < fn.instructions.size()) {
                        worklist.add(idx + 1);
                    }
                }
            }
        }

        List<Instruction> kept = new ArrayList<>();
        boolean any = false;
        for (int i = 0; i < fn.instructions.size(); i++) {
            if (reachable[i]) {
                kept.add(fn.instructions.get(i));
            } else {
                any = true;
            }
        }

        if (any) {
            fn.instructions = kept;
        }
        return any;
    }
}
