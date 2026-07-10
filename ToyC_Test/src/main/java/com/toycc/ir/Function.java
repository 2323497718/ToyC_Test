package com.toycc.ir;

import java.util.*;

/**
 * 函数 IR
 */
public class Function {
    public String name;
    public List<Integer> paramSlots = new ArrayList<>();
    public int slotCount = 0;
    public List<Integer> namedSlots = new ArrayList<>();  // 用户命名的局部变量槽
    public List<Instruction> instructions = new ArrayList<>();

    public int newSlot() {
        return slotCount++;
    }

    public int newTemp() {
        return newSlot();
    }

    public void addInstruction(Instruction inst) {
        instructions.add(inst);
    }
}
