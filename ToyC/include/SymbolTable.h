#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>

namespace ToyC {

enum class SymbolKind {
    Variable,
    Constant,
    Function,
    Parameter
};

enum class ValueType {
    Int,
    Void
};

struct Symbol {
    std::string name;
    SymbolKind kind;
    ValueType valueType;
    bool isConst;
    int scopeLevel;
    int offset;          // Stack offset for variables/params
    bool isGlobal;       // Global variable or constant
    std::optional<int> constValue;  // For compile-time constants

    Symbol() : name(""), kind(SymbolKind::Variable), valueType(ValueType::Int),
               isConst(false), scopeLevel(0), offset(0), isGlobal(false) {}

    Symbol(const std::string& n, SymbolKind k, ValueType vt, bool c, int lvl)
        : name(n), kind(k), valueType(vt), isConst(c), scopeLevel(lvl),
          offset(0), isGlobal(false), constValue(std::nullopt) {}
};

class SymbolTable {
public:
    SymbolTable();

    void pushScope();
    void popScope();

    bool declare(const std::string& name, SymbolKind kind, ValueType valueType,
                 bool isConst = false, std::optional<int> constVal = std::nullopt);
    std::optional<Symbol*> lookup(const std::string& name);
    std::optional<Symbol*> lookupCurrentScope(const std::string& name);

    int getCurrentScopeLevel() const { return scopes.size() - 1; }

    void setGlobalOffset(int offset) { globalOffset = offset; }
    int getNextGlobalOffset() { return globalOffset++; }
    void setLocalOffset(int offset) { localOffset = offset; }
    int getNextLocalOffset() { return localOffset++; }
    int getLocalOffset() const { return localOffset; }

    int getParamCount() const { return paramCount; }
    void setParamCount(int count) { paramCount = count; }
    void resetParamCount() { paramCount = 0; }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
    int globalOffset;
    int localOffset;
    int paramCount;
};

} // namespace ToyC

#endif // SYMBOL_TABLE_H
