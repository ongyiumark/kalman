#include "symbol_table.h"

////////////////////////////////////
// SYMBOL TABLE CLASS
////////////////////////////////////
SymbolTable::SymbolTable() : parent(NULL) {}
SymbolTable::SymbolTable(SymbolTable* par) : parent(par) {}

Value* SymbolTable::get_value(const std::string name) const
{
	if (symbols.count(name) > 0) return symbols.at(name);
	if (parent) return parent->get_value(name);
	return new Null();
}

void SymbolTable::set_value(const std::string name, Value* val)
{
	symbols[name] = val;
}

void SymbolTable::remove_value(const std::string name)
{
	if (symbols.count(name) > 0)
		symbols.erase(symbols.find(name));
}

SymbolTable* SymbolTable::get_parent() const
{
	return parent;
}