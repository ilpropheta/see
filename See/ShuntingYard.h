#pragma once

#include <map>
#include <stack>
#include <string>
#include <queue>
#include <memory>
#include <functional>

struct TokenEvaluator;

// some aliases
using EvaluationContext = std::stack <double> ;
using EvaluatorsQueue = std::queue<std::unique_ptr<TokenEvaluator>>;
using BinaryFunction = std::function <double(double, double)> ;
using UnaryFunction = std::function <double(double)>;

// common interface for evaluators
struct TokenEvaluator
{
	virtual ~TokenEvaluator() {}
	virtual void evaluate(EvaluationContext& evaluation) = 0;
};

// scalar evaluator (handles numbers)
struct ScalarEvaluator : TokenEvaluator
{
	ScalarEvaluator(double val);
	void evaluate(EvaluationContext& evaluation) override;
private:
	double value;
};

// binary operator
struct BinaryOpEvaluator : TokenEvaluator
{
	BinaryOpEvaluator(BinaryFunction fn);
	void evaluate(EvaluationContext& evaluation) override;
private:
	BinaryFunction evalFn;
};

// unary operator
struct UnaryFunctionEvaluator : TokenEvaluator
{
	UnaryFunctionEvaluator(UnaryFunction fn);
	void evaluate(EvaluationContext& evaluation) override;
private:
	UnaryFunction evalFn;
};

// used to handle parsing
struct ExpressionVisitor
{
	virtual ~ExpressionVisitor() = default;
	virtual void OnDigit(double value) = 0;
	virtual void OnWord(const std::string& name) = 0;
	virtual void OnOperator(const std::string& name) = 0;
};

// contains context information (e.g. constants and functions available)
struct ExpressionContext
{
	std::map<std::string, BinaryFunction> binaryOperators;
	std::map<std::string, UnaryFunction> unaryOperators;
	std::map<std::string, int> operatorsPrecedence;
	std::map<std::string, double> constantsTable;
};

// creates a classical {+, -, *, /, ^} context
ExpressionContext CreateSimpleContext(std::map<std::string, double> constants);

// specific visitor to transform an expression to reverse polish notation (RPN)
struct RPNVisitor : ExpressionVisitor
{
	RPNVisitor(const ExpressionContext& ctx);
	
	virtual void OnDigit(double value) override;
	virtual void OnWord(const std::string& name) override;
	virtual void OnOperator(const std::string& name) override;

	EvaluatorsQueue GetExpressionAsRPN();

private:
	const ExpressionContext& context;
	std::stack<std::string> operators;
	EvaluatorsQueue evaluators;
	bool lastTokenReadWasOperator;
};

// uses RPNVisitor to convert expr to RPN (hides the specific visitor from the caller)
class RPNConverter
{
public:
	static EvaluatorsQueue ConvertToRPN(const char* expr, const ExpressionContext& expressionContext);
};

// generic parser of digits, words and operators
class ExpressionParser
{
public:
	static void Parse(const char* expr, ExpressionVisitor& visitor);
};

// calculator of expressions which uses the ShuntingYard algorithm
class ShuntingYardCalculator
{
public:
	// here you can pass the context you want (default is the simple one)
	ShuntingYardCalculator(ExpressionContext ctx = CreateSimpleContext({}));
	// simple context + your constants (it's just a sugar function, since CreateSimpleContext
	// already receives contants as parameter)
	ShuntingYardCalculator(std::map<std::string, double> constants);

	// what you are probably interested in: calculating the result of expr
	double Calculate(const char* expr) const;
	
	// what you are probably interested in: calculating the result of expr (string version)
	double Calculate(const std::string& expr) const;
private:
	// constants, operators and precedence
	const ExpressionContext expressionContext;
};