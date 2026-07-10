package com.toycc.backend;

import com.toycc.ir.*;
import com.toycc.util.Fatal;

import java.util.*;

/**
 * RISC-V32 代码生成器
 */
public class RiscVEmitter {
    private final Program program;
    private final StringBuilder out = new StringBuilder();

    // 寄存器分配
    private Map<Integer, String> regMap = new HashMap<>();
    private List<String> savedRegs = new ArrayList<>();
    private static final int MAX_REG_ALLOC = 10; // s2-s11

    // 常量槽
    private Map<Integer, Integer> constSlots = new HashMap<>();

    // 缓存
    private List<CacheRow> cache = new ArrayList<>();
    private Map<String, String> sRegHeld = new HashMap<>();
    private static final int CACHE_CAP = 16;

    private static class CacheRow {
        int slot;
        String reg;
        CacheRow(int slot, String reg) {
            this.slot = slot;
            this.reg = reg;
        }
    }

    // 分支目标集合（用于缓存失效）
    private Set<String> branchTargets = new HashSet<>();

    private Function currentFunc = null;
    private int frameSize = 16;
    private int nextLabelId = 0;

    public RiscVEmitter(Program program) {
        this.program = program;
    }

    public String emit() {
        // 发射全局数据
        emitData();
        // 发射函数
        for (Function func : program.functions) {
            emitFunction(func);
        }
        return out.toString();
    }

    private void emitData() {
        if (program.globals.isEmpty()) return;

        out.append("  .data\n");
        for (Global global : program.globals) {
            out.append("  .globl ").append(global.label).append("\n");
            out.append(global.label).append(":\n");
            out.append("  .word ").append(global.value).append("\n");
        }
    }

    private void emitFunction(Function func) {
        currentFunc = func;
        nextLabelId = 0;
        regMap.clear();
        savedRegs.clear();
        constSlots.clear();
        cache.clear();
        sRegHeld.clear();
        branchTargets.clear();

        // 分配寄存器
        allocateRegisters();
        // 计算常量槽
        computeConstSlots();
        // 计算栈帧大小
        frameSize = computeFrameSize();
        // 收集分支目标
        collectBranchTargets();

        out.append("  .text\n");
        out.append("  .globl ").append(func.name).append("\n");
        out.append(func.name).append(":\n");

        // Prologue
        emitAddi("sp", "sp", -frameSize);
        emitSw("ra", frameSize - 4, "sp");
        emitSw("s0", frameSize - 8, "sp");
        for (int i = 0; i < savedRegs.size(); i++) {
            emitSw(savedRegs.get(i), frameSize - 12 - i * 4, "sp");
        }
        emitAddi("s0", "sp", frameSize);

        // 处理参数
        for (int i = 0; i < func.paramSlots.size(); i++) {
            int slot = func.paramSlots.get(i);
            String reg = regMap.get(slot);
            if (reg != null) {
                if (i < 8) {
                    out.append("  mv ").append(reg).append(", a").append(i).append("\n");
                } else {
                    emitLw(reg, (i - 8) * 4, "s0");
                }
            } else {
                int offset = slotOffset(slot);
                if (i < 8) {
                    emitSw("a" + i, offset, "s0");
                } else {
                    emitLw("t0", (i - 8) * 4, "s0");
                    emitSw("t0", offset, "s0");
                }
            }
        }

        // 发射指令
        for (Instruction inst : func.instructions) {
            emitInst(inst);
        }

        // Return label and epilogue
        out.append(returnLabel()).append(":\n");
        for (int i = 0; i < savedRegs.size(); i++) {
            emitLw(savedRegs.get(i), -12 - i * 4, "s0");
        }
        emitLw("ra", -4, "s0");
        emitLw("s0", -8, "s0");
        emitAddi("sp", "sp", frameSize);
        out.append("  ret\n");
    }

    private void allocateRegisters() {
        List<Integer> namedSlots = currentFunc.namedSlots;
        if (namedSlots.isEmpty()) return;

        Map<Integer, Integer> uses = new HashMap<>();
        for (Instruction inst : currentFunc.instructions) {
            for (int s : readsOf(inst)) {
                uses.merge(s, 1, Integer::sum);
            }
        }

        List<Map.Entry<Integer, Integer>> ranked = new ArrayList<>();
        for (int s : namedSlots) {
            int u = uses.getOrDefault(s, 0);
            if (u > 0) {
                ranked.add(Map.entry(u, s));
            }
        }

        ranked.sort((a, b) -> {
            if (!a.getKey().equals(b.getKey())) return b.getKey() - a.getKey();
            return a.getValue() - b.getValue();
        });

        int pick = Math.min(MAX_REG_ALLOC, ranked.size());
        for (int i = 0; i < pick; i++) {
            int slot = ranked.get(i).getValue();
            String reg = "s" + (2 + i);
            regMap.put(slot, reg);
            savedRegs.add(reg);
        }
    }

    private void computeConstSlots() {
        Map<Integer, Integer> defs = new HashMap<>();
        Map<Integer, Integer> values = new HashMap<>();

        for (Instruction inst : currentFunc.instructions) {
            if (inst.op == InstOp.CONST && inst.dest != -1) {
                defs.merge(inst.dest, 1, Integer::sum);
                values.put(inst.dest, inst.value);
            } else if (definesSlot(inst) && inst.dest != -1) {
                defs.merge(inst.dest, 2, Integer::sum); // mark non-const
            }
        }

        for (Map.Entry<Integer, Integer> e : defs.entrySet()) {
            if (e.getValue() == 1) {
                constSlots.put(e.getKey(), values.get(e.getKey()));
            }
        }
    }

    private int computeFrameSize() {
        int localBytes = 8 + savedRegs.size() * 4 + currentFunc.slotCount * 4;
        return Math.max(16, ((localBytes + 15) / 16) * 16);
    }

    private void collectBranchTargets() {
        for (Instruction inst : currentFunc.instructions) {
            if (inst.op == InstOp.GOTO) {
                branchTargets.add(inst.label);
            } else if (inst.op == InstOp.BRANCH) {
                if (!inst.label.isEmpty()) branchTargets.add(inst.label);
                if (!inst.falseLabel.isEmpty()) branchTargets.add(inst.falseLabel);
            }
        }
        branchTargets.add(returnLabel());
    }

    private String returnLabel() {
        return ".L_" + currentFunc.name + "_return";
    }

    // ===== 发射辅助方法 =====

    private void emitAddi(String rd, String rs, int imm) {
        if (imm >= -2048 && imm <= 2047) {
            out.append("  addi ").append(rd).append(", ").append(rs).append(", ").append(imm).append("\n");
        } else {
            out.append("  li t6, ").append(imm).append("\n");
            out.append("  add ").append(rd).append(", ").append(rs).append(", t6\n");
        }
    }

    private void emitSw(String reg, int offset, String base) {
        if (offset >= -2048 && offset <= 2047) {
            out.append("  sw ").append(reg).append(", ").append(offset).append("(").append(base).append(")\n");
        } else {
            out.append("  li t6, ").append(offset).append("\n");
            out.append("  add t6, ").append(base).append(", t6\n");
            out.append("  sw ").append(reg).append(", 0(t6)\n");
        }
    }

    private void emitLw(String reg, int offset, String base) {
        if (offset >= -2048 && offset <= 2047) {
            out.append("  lw ").append(reg).append(", ").append(offset).append("(").append(base).append(")\n");
        } else {
            out.append("  li t6, ").append(offset).append("\n");
            out.append("  add t6, ").append(base).append(", t6\n");
            out.append("  lw ").append(reg).append(", 0(t6)\n");
        }
    }

    private int slotOffset(int slot) {
        return -12 - savedRegs.size() * 4 - slot * 4;
    }

    private List<Integer> readsOf(Instruction inst) {
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

    private boolean definesSlot(Instruction inst) {
        return switch (inst.op) {
            case CONST, MOVE, LOAD_GLOBAL, UNARY, BINARY, CALL -> true;
            default -> false;
        };
    }

    // ===== 指令发射 =====

    private void emitInst(Instruction inst) {
        switch (inst.op) {
            case LABEL -> {
                if (branchTargets.contains(inst.label)) {
                    invalidateCache();
                }
                out.append(inst.label).append(":\n");
            }
            case GOTO -> {
                invalidateCache();
                out.append("  j ").append(inst.label).append("\n");
            }
            case BRANCH -> {
                loadSlot("a0", inst.lhs);
                if (!inst.label.isEmpty()) {
                    out.append("  bnez a0, ").append(inst.label).append("\n");
                }
                if (!inst.falseLabel.isEmpty()) {
                    out.append("  beqz a0, ").append(inst.falseLabel).append("\n");
                }
            }
            case CONST -> {
                // 如果是常量槽且未分配寄存器，完全省略
                Integer cv = constSlots.get(inst.dest);
                if (cv != null && cv == inst.value && !regMap.containsKey(inst.dest)) {
                    return;
                }
                out.append("  li a0, ").append(inst.value).append("\n");
                storeSlot("a0", inst.dest);
            }
            case MOVE -> {
                loadSlot("a0", inst.lhs);
                storeSlot("a0", inst.dest);
            }
            case LOAD_GLOBAL -> {
                out.append("  la t0, ").append(inst.name).append("\n");
                out.append("  lw a0, 0(t0)\n");
                storeSlot("a0", inst.dest);
            }
            case STORE_GLOBAL -> {
                loadSlot("a0", inst.lhs);
                out.append("  la t0, ").append(inst.name).append("\n");
                out.append("  sw a0, 0(t0)\n");
            }
            case UNARY -> emitUnary(inst);
            case BINARY -> emitBinary(inst);
            case CALL -> emitCall(inst);
            case RETURN -> {
                loadSlot("a0", inst.lhs);
                out.append("  j ").append(returnLabel()).append("\n");
                invalidateCache();
            }
            case RETURN_VOID -> {
                out.append("  li a0, 0\n");
                out.append("  j ").append(returnLabel()).append("\n");
                invalidateCache();
            }
        }
    }

    private void emitUnary(Instruction inst) {
        loadSlot("a0", inst.lhs);
        switch (inst.unary) {
            case PLUS -> {} // nop
            case MINUS -> out.append("  neg a0, a0\n");
            case NOT -> out.append("  seqz a0, a0\n");
        }
        storeSlot("a0", inst.dest);
    }

    private void emitBinary(Instruction inst) {
        int kr = 0, kl = 0;
        boolean rConst = isConstSlot(inst.rhs, kr);
        boolean lConst = isConstSlot(inst.lhs, kl);

        // 处理常量优化
        if (rConst) {
            if (tryOptimizeConstRhs(inst, kr, lConst, kl)) return;
        }
        if (lConst) {
            if (tryOptimizeConstLhs(inst, kl, kr)) return;
        }

        // 通用处理
        String t0Src = ensureInReg("t0", inst.lhs, inst.rhs, inst.dest);
        String a0Src = ensureInReg("a0", inst.rhs, inst.lhs, inst.dest);

        String destReg = getDestReg(inst.dest);
        String opName = getOpName(inst.binary);
        
        if (destReg != null) {
            out.append("  ").append(opName).append(" ")
               .append(destReg).append(", ").append(t0Src).append(", ").append(a0Src).append("\n");
            dropReg(destReg);
        } else {
            out.append("  ").append(opName).append(" a0, ")
               .append(t0Src).append(", ").append(a0Src).append("\n");
            storeSlot("a0", inst.dest);
        }
    }

    private boolean tryOptimizeConstRhs(Instruction inst, int kr, boolean lConst, int kl) {
        switch (inst.binary) {
            case ADD -> {
                if (kr >= -2048 && kr <= 2047) {
                    String src = ensureInReg("t0", inst.lhs, inst.rhs, inst.dest);
                    emitOpImmToDest("addi", src, kr, inst.dest);
                    return true;
                }
            }
            case SUB -> {
                if (-kr >= -2048 && -kr <= 2047) {
                    String src = ensureInReg("t0", inst.lhs, inst.rhs, inst.dest);
                    emitOpImmToDest("addi", src, -kr, inst.dest);
                    return true;
                }
            }
            case MUL -> {
                if (kr == 0) {
                    out.append("  li a0, 0\n");
                    storeSlot("a0", inst.dest);
                    return true;
                }
                if (kr == 1) {
                    loadSlot("a0", inst.lhs);
                    storeSlot("a0", inst.dest);
                    return true;
                }
                if (kr == -1) {
                    String src = ensureInReg("t0", inst.lhs, inst.rhs, inst.dest);
                    out.append("  neg a0, ").append(src).append("\n");
                    storeSlot("a0", inst.dest);
                    return true;
                }
                if (kr > 0 && (kr & (kr - 1)) == 0) {
                    String src = ensureInReg("t0", inst.lhs, inst.rhs, inst.dest);
                    emitOpImmToDest("slli", src, log2pow2(kr), inst.dest);
                    return true;
                }
            }
            case DIV -> {
                if (kr == 1) {
                    loadSlot("a0", inst.lhs);
                    storeSlot("a0", inst.dest);
                    return true;
                }
                if (kr > 1 && (kr & (kr - 1)) == 0) {
                    String src = ensureInReg("t0", inst.lhs, inst.rhs, inst.dest);
                    int k = log2pow2(kr);
                    out.append("  srai a0, ").append(src).append(", 31\n");
                    out.append("  srli a0, a0, ").append(32 - k).append("\n");
                    out.append("  add a0, ").append(src).append(", a0\n");
                    out.append("  srai a0, a0, ").append(k).append("\n");
                    storeSlot("a0", inst.dest);
                    return true;
                }
                if (kr != 0 && kr != 1) {
                    emitDiv(inst, kr, false);
                    return true;
                }
            }
            case MOD -> {
                if (kr == 1 || kr == -1) {
                    out.append("  li a0, 0\n");
                    storeSlot("a0", inst.dest);
                    return true;
                }
                if (kr > 1 && (kr & (kr - 1)) == 0) {
                    String src = ensureInReg("t0", inst.lhs, inst.rhs, inst.dest);
                    int k = log2pow2(kr);
                    out.append("  srai a0, ").append(src).append(", 31\n");
                    out.append("  srli a0, a0, ").append(32 - k).append("\n");
                    out.append("  add a0, ").append(src).append(", a0\n");
                    out.append("  srai a0, a0, ").append(k).append("\n");
                    out.append("  slli a0, a0, ").append(k).append("\n");
                    out.append("  sub a0, ").append(src).append(", a0\n");
                    storeSlot("a0", inst.dest);
                    return true;
                }
                if (kr != 0 && kr != 1) {
                    emitDiv(inst, kr, true);
                    return true;
                }
            }
            default -> {}
        }
        return false;
    }

    private boolean tryOptimizeConstLhs(Instruction inst, int kl, int kr) {
        // 只有可交换的运算才能交换左右操作数
        if (inst.binary != BinaryOp.ADD && inst.binary != BinaryOp.MUL) {
            return false;
        }
        if (inst.binary == BinaryOp.ADD && kl >= -2048 && kl <= 2047) {
            String src = ensureInReg("t0", inst.rhs, inst.lhs, inst.dest);
            emitOpImmToDest("addi", src, kl, inst.dest);
            return true;
        }
        if (inst.binary == BinaryOp.MUL && kl > 0 && (kl & (kl - 1)) == 0) {
            String src = ensureInReg("t0", inst.rhs, inst.lhs, inst.dest);
            emitOpImmToDest("slli", src, log2pow2(kl), inst.dest);
            return true;
        }
        return false;
    }

    private void emitDiv(Instruction inst, int d, boolean isMod) {
        DivMagic m = computeDivMagic(d);
        String src = ensureInReg("t0", inst.lhs, inst.rhs, inst.dest);

        out.append("  li t1, ").append(m.M).append("\n");
        out.append("  mulh a0, ").append(src).append(", t1\n");
        if (m.addMarker) {
            if (d > 0) out.append("  add a0, a0, ").append(src).append("\n");
            else out.append("  sub a0, a0, ").append(src).append("\n");
        }
        if (m.s > 0) out.append("  srai a0, a0, ").append(m.s).append("\n");
        if (d > 0) {
            out.append("  srli t1, ").append(src).append(", 31\n");
            out.append("  add a0, a0, t1\n");
        } else {
            out.append("  srli t1, a0, 31\n");
            out.append("  add a0, a0, t1\n");
        }
        if (!isMod) {
            storeSlot("a0", inst.dest);
        } else {
            out.append("  li t1, ").append(d).append("\n");
            out.append("  mul a0, a0, t1\n");
            out.append("  sub a0, ").append(src).append(", a0\n");
            storeSlot("a0", inst.dest);
        }
    }

    private static class DivMagic {
        int M;
        int s;
        boolean addMarker;
    }

    private DivMagic computeDivMagic(int d) {
        DivMagic result = new DivMagic();
        int twoP31 = 0x80000000;
        int ad = d < 0 ? -d : d;
        int t = twoP31 + (d >> 31);
        int anc = t - 1 - t % ad;
        int p = 31;
        int q1 = twoP31 / anc;
        int r1 = twoP31 - q1 * anc;
        int q2 = twoP31 / ad;
        int r2 = twoP31 - q2 * ad;

        do {
            p++;
            q1 *= 2; r1 *= 2;
            if (r1 >= anc) { q1++; r1 -= anc; }
            q2 *= 2; r2 *= 2;
            if (r2 >= ad) { q2++; r2 -= ad; }
        } while (q1 < ad - r2 || (q1 == ad - r2 && r1 == 0));

        result.M = q2 + 1;
        if (d < 0) result.M = -result.M;
        result.s = p - 32;
        result.addMarker = (d > 0 && result.M < 0) || (d < 0 && result.M > 0);
        return result;
    }

    private int log2pow2(int v) {
        int r = 0;
        while ((1 << r) < v) r++;
        return r;
    }

    private void emitCall(Instruction inst) {
        int count = inst.args.size();
        int registerCount = Math.min(count, 8);
        int overflowCount = count > 8 ? count - 8 : 0;

        // 溢出参数先入栈
        if (overflowCount > 0) {
            emitAddi("sp", "sp", -overflowCount * 4);
            for (int i = 0; i < overflowCount; i++) {
                int argIndex = 8 + i;
                loadSlot("a0", inst.args.get(argIndex));
                emitSw("a0", i * 4, "sp");
            }
        }

        // 参数寄存器 (逆序加载)
        for (int k = registerCount - 1; k >= 0; k--) {
            loadSlot("a" + k, inst.args.get(k));
        }

        out.append("  jal ").append(inst.name).append("\n");

        if (overflowCount > 0) {
            emitAddi("sp", "sp", overflowCount * 4);
        }

        invalidateCache();
        storeSlot("a0", inst.dest);
    }

    // ===== 寄存器管理 =====

    private void loadSlot(String reg, int slot) {
        // 检查是否已分配寄存器
        String mapped = regMap.get(slot);
        if (mapped != null) {
            String held = sRegHeld.get(reg);
            if (held != null && held.equals(mapped)) return;
            out.append("  mv ").append(reg).append(", ").append(mapped).append("\n");
            sRegHeld.put(reg, mapped);
            dropReg(reg);
            return;
        }

        sRegHeld.remove(reg);

        // 检查常量
        Integer cv = constSlots.get(slot);
        if (cv != null) {
            dropReg(reg);
            out.append("  li ").append(reg).append(", ").append(cv).append("\n");
            return;
        }

        // 检查缓存
        for (CacheRow c : cache) {
            if (c.slot == slot) {
                if (!reg.equals(c.reg)) {
                    out.append("  mv ").append(reg).append(", ").append(c.reg).append("\n");
                    dropReg(reg);
                    addCache(slot, reg);
                }
                promote(slot);
                return;
            }
        }

        dropReg(reg);
        emitLw(reg, slotOffset(slot), "s0");
    }

    private void storeSlot(String reg, int slot) {
        String mapped = regMap.get(slot);
        if (mapped != null) {
            out.append("  mv ").append(mapped).append(", ").append(reg).append("\n");
            return;
        }

        emitSw(reg, slotOffset(slot), "s0");
        addCache(slot, reg);
    }

    private void emitOpImmToDest(String op, String src, int imm, int dest) {
        String destReg = getDestReg(dest);
        if (destReg != null) {
            out.append("  ").append(op).append(" ").append(destReg).append(", ").append(src).append(", ").append(imm).append("\n");
            dropReg(destReg);
        } else {
            out.append("  ").append(op).append(" a0, ").append(src).append(", ").append(imm).append("\n");
            storeSlot("a0", dest);
        }
    }

    private String getDestReg(int dest) {
        String reg = regMap.get(dest);
        if (reg != null) {
            dropReg(reg);
            List<String> toRemove = new ArrayList<>();
            for (Map.Entry<String, String> e : sRegHeld.entrySet()) {
                if (e.getValue().equals(reg)) {
                    toRemove.add(e.getKey());
                }
            }
            for (String k : toRemove) sRegHeld.remove(k);
            return reg;
        }
        return null;
    }

    private String ensureInReg(String preferred, int slot, int otherSlot, int dest) {
        // 如果 slot 已经在寄存器中且不是目标寄存器
        for (Map.Entry<Integer, String> e : regMap.entrySet()) {
            if (e.getKey() == slot) {
                String reg = e.getValue();
                if (!reg.equals(preferred)) {
                    out.append("  mv ").append(preferred).append(", ").append(reg).append("\n");
                }
                return preferred;
            }
        }

        loadSlot(preferred, slot);
        return preferred;
    }

    private boolean isConstSlot(int slot, Integer outVal) {
        Integer val = constSlots.get(slot);
        if (val != null) {
            outVal = val;
            return true;
        }
        return false;
    }

    private void dropReg(String reg) {
        cache.removeIf(c -> c.reg.equals(reg));
    }

    private void addCache(int slot, String reg) {
        cache.removeIf(c -> c.slot == slot);
        cache.add(new CacheRow(slot, reg));
        while (cache.size() > CACHE_CAP) {
            cache.remove(0);
        }
    }

    private void promote(int slot) {
        for (int i = 0; i < cache.size(); i++) {
            if (cache.get(i).slot == slot && i + 1 < cache.size()) {
                CacheRow row = cache.remove(i);
                cache.add(row);
                break;
            }
        }
    }

    private void invalidateCache() {
        cache.clear();
        sRegHeld.clear();
    }

    private String getOpName(BinaryOp op) {
        return switch (op) {
            case LESS, LESS_EQUAL, GREATER_EQUAL -> "slt";
            case GREATER -> "sgt";
            case EQUAL, NOT_EQUAL -> "sub";
            case ADD, SUB -> op == BinaryOp.ADD ? "add" : "sub";
            case MUL -> "mul";
            case DIV -> "div";
            case MOD -> "rem";
        };
    }
}
