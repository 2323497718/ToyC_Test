# ToyC Compiler

一个用 Java 实现的 ToyC 语言编译器，将 ToyC 代码编译为 RISC-V32 汇编。

## 项目结构

```
ToyC_Test/
├── build.gradle              # Gradle 构建配置
├── settings.gradle           # Gradle 设置
├── src/main/
│   ├── antlr4/
│   │   └── com/toycc/frontend/
│   │       └── ToyC.g4      # ANTLR 语法定义
│   └── java/com/toycc/
│       ├── Main.java         # 主程序入口
│       ├── ast/              # 抽象语法树节点
│       ├── frontend/         # 解析器
│       ├── sema/            # 语义分析器
│       ├── ir/               # 中间表示
│       ├── optim/            # 优化 passes
│       └── backend/          # RISC-V 后端
```

## 构建

### 使用 Gradle

```bash
# 构建项目
./gradlew build

# 或者只编译
./gradlew compileJava

# 清理
./gradlew clean
```

### 使用 IDE

在 IntelliJ IDEA 或 Eclipse 中导入 Gradle 项目即可。

## 使用方法

```bash
# 基本用法：从 stdin 读取，输出到 stdout
java -jar build/libs/toycc.jar < input.tc > output.s

# 开启优化
java -jar build/libs/toycc.jar -opt < input.tc > output.s
```

## 编译流程

1. **词法/语法分析** - ANTLR 解析 ToyC 源码
2. **语义分析** - 检查类型、作用域、返回值等
3. **IR 生成** - 转换为三地址代码
4. **优化** - 常量折叠、复制传播、死代码消除等
5. **代码生成** - 输出 RISC-V32 汇编

## 支持的语言特性

- 全局变量/常量 (const)
- 函数定义 (int/void 返回类型)
- 局部变量和赋值
- 算术/逻辑/关系表达式
- if/else 条件语句
- while 循环
- break/continue
- return 语句
- 函数调用和递归

## 优化 passes

- 常量折叠 (const folding)
- 代数简化 (algebraic simplification)
- 复制传播 (copy propagation)
- 公共子表达式消除 (CSE)
- 分支折叠 (branch folding)
- 死代码消除 (DCE)
- 死块消除 (dead block elimination)
- Goto 清理

## 测试

参考 `ToyC-main/tests/perf/` 中的 `.tc` 测试用例。

## 许可证

MIT
