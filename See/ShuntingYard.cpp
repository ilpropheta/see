// Revisitation of https://github.com/bamos/cpp-expression-parser

#include "ShuntingYard.h"
#include <iostream>
#include <sstream>

using namespace std;

// TokenEvaluator(s) ---------------------------------

// Scalar evaluator
ScalarEvaluator::ScalarEvaluator(double val) 
	: value(val)
{

}

void ScalarEvaluator::evaluate(EvaluationContext& evaluation)
{
	evaluation.push(value);
}


// Generic Binary Operator
BinaryOpEvaluator::BinaryOpEvaluator(std::function<double(double, double)> fn)
	: evalFn(std::move(fn))
{

}

void BinaryOpEvaluator::evaluate(EvaluationContext& evaluation)
{
	if (evaluation.size() < 2)
	{
		throw std::domain_error("Something wrong with this expression");
	}

	double right = evaluation.top(); evaluation.pop();
	double left = evaluation.top(); evaluation.pop();
	evaluation.push(evalFn(left, right));
}

// Generic Unary Operator/Function
UnaryFunctionEvaluator::UnaryFunctionEvaluator(UnaryFunction fn)
	: evalFn(std::move(fn))
{

}

void UnaryFunctionEvaluator::evaluate(EvaluationContext& evaluation)
{
	double toEval = evaluation.top(); evaluation.pop();
	evaluation.pop(); // discard left (unary trick)
	evaluation.push(evalFn(toEval));
}

// TokenEvaluator(s) ---------------------------------

// Utilities -----------------------------------------
inline bool isAlphaOrUnderscore(char c)
{
	return c && (isalpha(c) || c == '_');
}

inline const char* skipWhitespaces(const char* expr)
{
	while (*expr && isspace(*expr)) ++expr;
	return expr;
}

std::unique_ptr<TokenEvaluator> CreateEvaluator(const ExpressionContext& ctx, const std::string& opName)
{
	{
		auto it = ctx.binaryOperators.find(opName);
		if (it != end(ctx.binaryOperators))
			return make_unique<BinaryOpEvaluator>(it->second);
	}
	auto it = ctx.unaryOperators.find(opName);
	if (it != end(ctx.unaryOperators))
		return make_unique<UnaryFunctionEvaluator>(it->second);
	
	throw domain_error("This operator or function is unknown: " + opName);
}
// Utilities -----------------------------------------

const char* HandleDigit(const char* expr, ExpressionVisitor& visitor) 
{
	if (isdigit(*expr))
	{
		char* nextChar = 0;
		double digit = strtod(expr, &nextChar);
		advance(expr, std::distance(expr, static_cast<const char*>(nextChar)));
		visitor.OnDigit(digit);
	}
	return expr;
}

const char* HandleWord(const char* expr, ExpressionVisitor& visitor)
{
	if (isAlphaOrUnderscore(*expr))
	{
		std::stringstream ss;
		while (isAlphaOrUnderscore(*expr))
		{
			ss << *expr;
			++expr;
		}
		visitor.OnWord(ss.str());
	}
	return expr;
}

const char* HandleOperator(const char* expr, ExpressionVisitor& visitor)
{
	if (!*expr || isdigit(*expr) || isAlphaOrUnderscore(*expr))
		return expr;

	switch (*expr)
	{
		case '(':
			visitor.OnOperator("(");
			++expr;
			break;
		case ')':
			visitor.OnOperator(")");
			++expr;
			break;
		default:
		{
			std::stringstream ss;
			ss << *expr;
			++expr;
			while (*expr && !isspace(*expr) && !isdigit(*expr)
				&& !isAlphaOrUnderscore(*expr) && *expr != '(' && *expr != ')')
			{
				ss << *expr;
				++expr;
			}
			ss.clear();
			std::string str;
			ss >> str;

			visitor.OnOperator(str);
		}
	}
	return expr;
}

void ExpressionParser::Parse(const char* expr, ExpressionVisitor& visitor)
{
	expr = skipWhitespaces(expr);
	while (*expr)
	{
		expr = HandleDigit(expr, visitor);
		expr = HandleWord(expr, visitor);
		expr = HandleOperator(expr, visitor);
		expr = skipWhitespaces(expr);
	}
}

EvaluatorsQueue RPNVisitor::GetExpressionAsRPN()
{
	while (!operators.empty())
	{
		evaluators.push(CreateEvaluator(context, operators.top()));
		operators.pop();
	}
	return move(evaluators);
}

EvaluatorsQueue RPNConverter::ConvertToRPN(const char* expr, const ExpressionContext& expressionContext)
{
	RPNVisitor visitor(expressionContext);
	ExpressionParser::Parse(expr, visitor);
	return visitor.GetExpressionAsRPN();
}

ShuntingYardCalculator::ShuntingYardCalculator(ExpressionContext ctx)
	: expressionContext(move(ctx))
{

}

ShuntingYardCalculator::ShuntingYardCalculator(std::map<std::string, double> constants)
	: expressionContext(CreateSimpleContext(move(constants)))
{

}

double ShuntingYardCalculator::Calculate(const std::string& expr) const
{
	return Calculate(expr.c_str());
}

double ShuntingYardCalculator::Calculate(const char* expr) const
{
	auto rpn = RPNConverter::ConvertToRPN(expr, expressionContext);
	EvaluationContext evaluation;
	while (!rpn.empty())
	{
		rpn.front()->evaluate(evaluation);
		rpn.pop();
	}
	return evaluation.top();
}

ExpressionContext CreateSimpleContext(std::map<std::string, double> constants)
{
	static const map<string, BinaryFunction> binOperators{
		{ "*", std::multiplies<>() },
		{ "+", std::plus<>() },
		{ "-", std::minus<>() },
		{ "/", std::divides<>() },
		{ "^", [](double base, double exp) { return std::pow(base, exp); } },
	};

	static const map<string, UnaryFunction> unOperators{
		{ "+", [](double d) { return d; } },
		{ "-", std::negate<>() },
	};

	static const map<string, int> opPrecedence{
		{ "(", -1 }, { "+", 2 }, { "-", 2 },
		{ "*", 3 }, { "/", 3 },
		{ "^", 4 }
	};

	return{ binOperators, unOperators, opPrecedence, move(constants) };
}

RPNVisitor::RPNVisitor(const ExpressionContext& ctx)
	: context(ctx)
{

}

void RPNVisitor::OnDigit(double value)
{
	evaluators.push(make_unique<ScalarEvaluator>(value));
	lastTokenReadWasOperator = false;
}

void RPNVisitor::OnWord(const std::string& name)
{
	{
		auto it = context.unaryOperators.find(name);
		if (it != end(context.unaryOperators))
		{
			evaluators.push(make_unique<ScalarEvaluator>(0.0)); // unary trick
			OnOperator(name);
			return;
		}
	}
	auto it = context.constantsTable.find(name);
	if (it == context.constantsTable.end())
	{
		throw std::domain_error("Unable to find constant or function called " + name);
	}
	evaluators.push(make_unique<ScalarEvaluator>(it->second));
	lastTokenReadWasOperator = false;
}

void RPNVisitor::OnOperator(const std::string& name)
{
	if (!name.compare("("))
	{
		operators.push("(");
	}
	else if (!name.compare(")"))
	{
		while (operators.top().compare("("))
		{
			evaluators.push(CreateEvaluator(context, operators.top()));
			operators.pop();
		}
		operators.pop();
	}
	else
	{
		if (lastTokenReadWasOperator)
		{
			// unary trick (e.g. -10 = 0 - 10)
			if (context.unaryOperators.count(name)) {
				evaluators.push(make_unique<ScalarEvaluator>(0.0));
			}
			else
			{
				throw std::domain_error("Unrecognized unary operator or function: " + name);
			}
		}

		// p(o) is the precedence of an operator o
		//
		// If the token is an operator, o1, then
		//   While there is an operator token, o2, at the top
		//       and p(o1) <= p(o2), then
		//     pop o2 off the stack onto the output queue.
		//   Push o1 on the stack.
		while (!operators.empty() && 
				context.operatorsPrecedence.at(name) <= context.operatorsPrecedence.at(operators.top()))
		{
			evaluators.push(CreateEvaluator(context, operators.top()));
			operators.pop();
		}
		operators.push(name);
		lastTokenReadWasOperator = true;
	}
}