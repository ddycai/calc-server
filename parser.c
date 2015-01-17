/********************************************************************************
 * parser.c
 * 
 * Computer Science 3357a
 * Expression Parser
 *
 * Author: Duncan Cai
 * 
 * Implementation of expression parser using shunting yard algorithm.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include "parser.h"
#include "stack.h"

#define UNARY_MIN	'~'

#define NONE 0
#define IS_OPERAND 1
#define IS_OPERATOR	2
#define IS_LEFT_P	3
#define IS_RIGHT_P 4

/*
 * Returns true if given char is a digit
 */
bool is_num(char c)
{
	return c >= '0' && c <= '9';
}

/*
 * Returns true if given char is an operator
 */
bool is_op(char c)
{
	return c == '+' || c == '/' || c == '-' || c == '*' || c == UNARY_MIN;
}

/*
 * Returns the precedence of the given operator
 * Or -1 if not an operator
 */
int precedence(char c)
{
	if(c == '+' || c == '-') return 1;
	if(c == '/' || c == '*') return 2;
	if(c == UNARY_MIN) return 3;
	return -1;
}

/*
 * Inspects operator and performs the operation on the operand stack
 *
 * num_stk: the operand stack
 * op: the operator
 *
 * returns: status code indicating any errors; if OK, then no errors
 * otherwise, describes the error
 */
int operate(stack * num_stk, char op) {
	int a, b;
	//The given operator is not valid
	if(!is_op(op))
	{
		return INVALID_EXPR;
	}
	// The given operator is unary
	else if(op == UNARY_MIN)
	{
		if(stack_size(num_stk) > 0)
		{
			stack_push(num_stk, -stack_pop(num_stk));
			return OK;
		}
		else
		{
			syslog(LOG_ERR, "Insufficient elements in operand stack");
			return INVALID_EXPR;
		}
	}
	// If not unary, operand stack must have at least two elements
	else if(stack_size(num_stk) >= 2)
	{
		b = stack_pop(num_stk);
		a = stack_pop(num_stk);
	}
	else
	{
		syslog(LOG_ERR, "Insufficient elements in operand stack");
		return INVALID_EXPR;
	}
	int result;
	syslog(LOG_DEBUG, "eval %d %c %d\n", a, op, b);
	switch(op)
	{
		case '+':
			result = a + b;
			break;
		case '-':
			result = a - b;
			break;
		case '*':
			result = a * b;
			break;
		case '/':
			if(b == 0)
			{
				syslog(LOG_ERR, "Cannot divide by zero");
				return INVALID_EXPR;
			}	
			result = a / b;
			break;
		default:
			return INVALID_EXPR;
	}
	stack_push(num_stk, result);
	return OK;
}

/*
 * Parses the given expression and stores it in result
 * The actual parsing is done in _parse_expr, this method
 * just ensures that the stacks are always freed
 *
 * expr: the expression to be parsed
 * result: a pointer to the result variable
 *
 * return: status code indicating the result of the parsing
 * If OK, then parse was successful and result is stored in result
 * Otherwise, parse failed and can be due to MISMATCH or INVALID_EXPR
 */
int parse_expr(const char * expr, int * result)
{
	// Create stacks
	stack * num_stk = stack_init(); // The operand stack
	stack * ops_stk = stack_init(); // The operator stack

	// For logging debug messages
	num_stk->push_format = "pushed %d";
	num_stk->pop_format = "popped %d";
	ops_stk->push_format = "pushed %c";
	ops_stk->pop_format = "popped %c";

	// Parse the expression
	int status_code = _parse_expr(num_stk, ops_stk, expr, result);

	// Free stacks
	stack_free(num_stk);
	stack_free(ops_stk);
	return status_code;
}

/*
 * Parses the given expression and stores it in result
 *
 * num_stk: the operand stack
 * ops_stk: the operator stack
 * expr: the expression to be parsed
 * result: a pointer to the result
 *
 * return: a status code (described above)
 */
int _parse_expr(stack * num_stk, stack * ops_stk, const char * expr, int * result)
{
	int tmp;
	int status_code; // Stores the status code to be returned
	int last_token = NONE; // Stores the type of the last token read
	int i = 0;

	// Iterate through the expression char by char
	while(expr[i] != '\0')
	{
		//Ignore spaces and newlines
		if(expr[i] == ' ' || expr[i] == '\n' || expr[i] == '\r')
		{
			i++;
			continue;
		}

		//Parse number token and push onto stack
		if(is_num(expr[i]))
		{
			tmp = expr[i] - '0';
			while(is_num(expr[i + 1]))
			{
				tmp = tmp * 10 + (int)(expr[i + 1] - '0');
				i++;
			}
			stack_push(num_stk, tmp);
			last_token = IS_OPERAND;
		}
		//If token is minus, then check if unary
		else if(expr[i] == '-' && (last_token == NONE || last_token == IS_OPERATOR || last_token == IS_LEFT_P))
		{
			stack_push(ops_stk, UNARY_MIN);
			last_token = IS_OPERATOR;
		}
		//If operator, then place onto stack according to precedence
		else if(is_op(expr[i]))
		{
			// Remove higher precedence operators and perform them
			while(!stack_empty(ops_stk) && is_op(stack_top(ops_stk)))
			{
				if(precedence(expr[i]) <= precedence(stack_top(ops_stk)))
				{
					if((status_code = operate(num_stk, stack_pop(ops_stk))) != OK)
						return status_code;
				} else break;
			}
			stack_push(ops_stk, expr[i]);
			last_token = IS_OPERATOR;
		}
		// If left parenthesis then push onto stack
		else if(expr[i] == '(')
		{
			stack_push(ops_stk, expr[i]);
			last_token = IS_LEFT_P;
		}
		//If right parenthesis, evaluate stack until left parenthesis
		else if(expr[i] == ')')
		{
			syslog(LOG_DEBUG, "encountered )\n");
			while(!stack_empty(ops_stk) && stack_top(ops_stk) != '(')
			{
				if((status_code = operate(num_stk, stack_pop(ops_stk))) != OK)
					return status_code;
			}
			// Pop out the remaining left bracket
			// unless stack is empty then mismatch
			if(!stack_empty(ops_stk))
				stack_pop(ops_stk);
			else
			{
				syslog(LOG_ERR, "too many right parentheses");
				return MISMATCH;
			}
			last_token = IS_RIGHT_P;
		}
		// Otherwise, invalid token
		else
		{
			return INVALID_EXPR;
		}

		syslog(LOG_DEBUG, "OPS: ");
		stack_log(ops_stk, "%c");
		syslog(LOG_DEBUG, "NUM: ");
		stack_log(num_stk, "%d");
		
		i++;
	}

	//Evaluate remaining stack
	while(!stack_empty(ops_stk))
	{
		if(is_op(stack_top(ops_stk))) {
			if((status_code = operate(num_stk, stack_pop(ops_stk))) != OK)
				return status_code;
		} else if(stack_top(ops_stk) == '(') {
			return MISMATCH;
		}
	}

	//There should only be one item left in num stack
	if(stack_size(num_stk) == 0)
	{
		syslog(LOG_ERR, "Operand stack is empty");
		return INVALID_EXPR;
	}
	else if(stack_size(num_stk) > 1)
	{
		syslog(LOG_ERR, "Operands remaining in stack");
		return INVALID_EXPR;
	}
	else
	{
		*result = stack_pop(num_stk);
	}

	return OK;

}

/* 
int main()
{

	openlog("parse-expr", LOG_PERROR | LOG_PID | LOG_NDELAY, LOG_USER);
	setlogmask(LOG_UPTO(LOG_ERR));

	size_t nbytes = 80;
	char* expr = (char*)malloc(nbytes + 1);
	int bytes_read;
	int result;

	while(getline(&expr, &nbytes, stdin) != -1)
	{
		if(expr[0] == '\n')
			exit(EXIT_SUCCESS);
		int status_code = parse_expr(expr, &result);
		switch(status_code)
		{
			case OK:
				printf("%s = %d\n", expr, result);
				break;
			case INVALID_EXPR:
				printf("invalid-expr\n");
				break;
			case MISMATCH:
				printf("mismatch\n");
				break;
		}
	}

	closelog();
}*/
