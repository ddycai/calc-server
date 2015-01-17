/********************************************************************************
 * parser.h
 * 
 * Computer Science 3357a
 * Expression Parser
 *
 * Author: Duncan Cai
 * 
 * Expression parser interface
*******************************************************************************/

#ifndef PARSER_H
#define PARSER_H

#define OK				1
#define MISMATCH		2
#define INVALID_EXPR	3

/*
 * Parses the given expression and stores it in result
 *
 * expr: the expression to be parsed
 * result: a pointer to the result variable
 *
 * return: status code indicating the result of the parsing
 * If OK, then parse was successful and result is stored in result
 * Otherwise, parse failed and can be due to MISMATCH or INVALID_EXPR
 */
int parse_expr(const char * expr, int * result);

#endif
