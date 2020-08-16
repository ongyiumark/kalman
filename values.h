#ifndef VALUES_H
#define VALUES_H

#include "position.h"

#include <iostream>
#include <string>

class Value
{
private:
	std::string type;
	Position start, end;
public:
	Value();
	Value(std::string t, Position s, Position e);
	virtual void print(std::ostream& os) const = 0;
};

std::ostream& operator<<(std::ostream& os, const Value &val);

class Integer : public Value
{
private:
	long long value;
public:
	Integer(long long x);
	void print(std::ostream& os) const;

	long long get_value();
};

#endif