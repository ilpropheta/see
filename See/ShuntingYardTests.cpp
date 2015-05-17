#include <iostream>
#include "ShuntingYard.h"
#include <iomanip>

using namespace std;

void ExpectOnExprEvaluation(const ShuntingYardCalculator& calc, const char* expr, double expected) 
{
	double actual = calc.Calculate(expr);
	double diff = fabs(actual - expected);
	if (diff < 1e-10) 
	{
		std::cout << "[OK] {" << expr << "} evaluated to " << expected << std::endl;
	}
	else 
	{
		std::cout << "[FAILURE] {" << expr << "} evaluated to " <<
			actual << " instead of " << expected << " !!!!!!!!!!!!!!" << std::endl;
	}
}


int main(int argc, char** argv) 
{
  std::map<std::string, double> vars;
  vars["pi"] = 3.14;
  vars["myConst"] = 20;
  
  auto ctx = CreateSimpleContext(move(vars));
  ctx.unaryOperators["sin"] = [](double d) { return std::sin(d); };
  ctx.unaryOperators["cos"] = [](double d) { return std::cos(d); };
  ctx.binaryOperators[">"] = [](double a, double b) { return a > b ? 1.0 : 0.0; };
  ctx.operatorsPrecedence["sin"] = 4;
  ctx.operatorsPrecedence["cos"] = 4;
  ctx.operatorsPrecedence[">"] = 1;
  
  ShuntingYardCalculator calc(move(ctx));

  ExpectOnExprEvaluation(calc, "sin(3.14/2)>0", 1);
  ExpectOnExprEvaluation(calc, "cos(sin(3.14)+10)*20", -16.7640805693);
  ExpectOnExprEvaluation(calc, "-(10+3)", -13);
  ExpectOnExprEvaluation(calc, "-pi + 1", -2.14);
  ExpectOnExprEvaluation(calc, "myConst + (20+10)*3/2-3", 62.0);
  ExpectOnExprEvaluation(calc, "1+(-2*3+2)", -3);
  ExpectOnExprEvaluation(calc, "2^2", 4);

  return 0;
}

