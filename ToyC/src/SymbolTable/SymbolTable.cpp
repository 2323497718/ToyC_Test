#include "SymbolTable.h"

namespace ToyC {

SymbolTable::SymbolTable() : globalOffset(0), localOffset(0), paramCount(0) {
    // Initialize global scope
    scopes.emplace_back();
}

void SymbolTable::pushScope() {
    scopes.emplace_back();
}

void SymbolTable::popScope() {
    if (scopes.size() > 1) {
        scopes.pop_back();
    }
}

bool SymbolTable::declare(const std::string& name, SymbolKind kind, ValueType valueType,
                          bool isConst, std::optional<int> constVal) {
    // Check if already declared in current scope
    auto& currentScope = scopes.back();
    if (currentScope.find(name) != currentScope.end()) {
        return false;
    }
    
    Symbol sym(name, kind, valueType, isConst, getCurrentScopeLevel());
    sym.constValue = constVal;
    
    // Set offset based on scope
    if (getCurrentScopeLevel() == 0) {
        sym.isGlobal = true;
        sym.offset = getNextGlobalOffset();
    } else {
        sym.offset = getNextLocalOffset();
    }
    
    currentScope[name] = sym;
    return true;
}

std::optional<Symbol*> SymbolTable::lookup(const std::string& name) {
    // Search from innermost to outermost scope
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return &found->second;
        }
    }
    return std::nullopt;
}

std::optional<Symbol*> SymbolTable::lookupCurrentScope(const std::string& name) {
    auto& currentScope = scopes.back();
    auto found = currentScope.find(name);
    if (found != currentScope.end()) {
        return &found->second;
    }
    return std::nullopt;
}

} // namespace ToyC
