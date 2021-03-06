See
=======

Simple expression evaluator in C++. Revisitation of the [Shunting-yard algorithm](http://en.wikipedia.org/wiki/Shunting-yard_algorithm).

### In a nutshell ###

Inspired by https://github.com/bamos/cpp-expression-parser, I have redesigned the code, provided some abstractions and added some features.

Already supported features:
* Unary operators. +, -
* Binary operators. +, -, /, +
* Custom constant names

I added:
* Custom binary operators support
* Custom (unary) functions support
* split between parsing and RPN (Reverse Polish Notation) transformation

### Example ###

``` cpp
#include <iostream>
#include "shunting-yard.h"
int main() 
{
	std::map<std::string, double> consts;
	consts["e"] = 2.71;
 	ShuntingYardCalculator calc(move(consts));
  	std::cout << calc.Calculate("-e + 2") << std::endl;
}
```

### Adding operators/functions ###

``` cpp
// automatically provides {+,-,/,*,^}
auto ctx = CreateSimpleContext();
ctx.unaryOperators["sin"] = [](double d) { return std::sin(d); };
ctx.operatorsPrecedence["sin"] = 4;
ctx.binaryOperators[">"] = [](double left, double right) { return left>right ? 1.0 : 0.0; };
ctx.operatorsPrecedence[">"] = 1;

ShuntingYardCalculator calc(move(ctx));
std::cout << calc.Calculate("sin(3.14/2)>0") << std::endl;
```

### A design experiment ###

I have opted for a visitor approach to split parsing and RPN transformation. There is a simple visitor interface like this:

``` cpp
struct ExpressionVisitor
{
	virtual ~ExpressionVisitor() = default;
	virtual void OnDigit(double value) = 0;
	virtual void OnWord(const std::string& name) = 0;
	virtual void OnOperator(const std::string& name) = 0;
};
```

A concrete visitor maintains the state of the parsing. For example, a `RPNVisitor` is provided, which handles the RPN transformation. You can use the parser directly by calling:

``` cpp
static void ExpressionParser::Parse(const char* expr, ExpressionVisitor& visitor); 
```

### Some open points ###

* Some cases are not handled (e.g. --8 or e^-2)
* Stateless functions could be referenced instead of copied (e.g. it's possible to create a sort of `BinaryOpEvaluator` which references `BinaryFunction` instead of copying it)
* More information on errors
* Vector types
 
## Why "see"?

You may think "See stands for Simple Expression Evaluator". It's the half of the truth: you know I always name my github repo with Roman words and this time I was no less so...See also means "See! Non ci credo!" that is "I don't believe you!". The more "e" you use, the more enphasis you get! For example "Seeeee" is stronger than "Seee".
