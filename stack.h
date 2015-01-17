/********************************************************************************
 * stack.h
 * 
 * Computer Science 3357a
 * Stack Interface
 *
 * Author: Duncan Cai
 * 
 * Stack interface containing basic stack operations (push, pop, top, size etc.)
*******************************************************************************/

#ifndef STACK_H
#define STACK_H
#include <stdbool.h>

//A stack node
struct node
{
	int data;
	struct node* next;	
};

//A stack container
typedef struct
{
	struct node* head;
	size_t _num_elements;	// Stores the stack size
	const char * push_format; // Syslogs on every push according to this format
	const char * pop_format; // Syslogs on every pop according to this format
} stack;

/*
 * Initialize a stack dynamically
 * Must be freed with stack_free
 *
 * return: a pointer to a new stack
 */
stack* stack_init();

/*
 * Pushes data onto stack
 *
 * s: pointer to stack
 * data: data to be pushed
 */
void stack_push(stack* s, int data);

/*
 * Removes data from top of stack and returns it
 *
 * s: pointer to stack
 *
 * return: element at top of the stack
 */
int stack_pop(stack* s);

/*
 * Logs the contents of the stack using syslog
 *
 * s: pointer to stack
 * format: format to log the data
 */
void stack_log(stack* s, const char * format);

/*
 * Returns the top of the stack
 *
 * s: pointer to stack
 *
 * return: element at the top of the stack
 */
int stack_top(stack* s);

/*
 * Returns the size of the stack
 *
 * s: pointer to stack
 *
 * return: the size of the stack
 */
size_t stack_size(stack* s);

/*
 * Returns true if the stack is empty
 *
 * s: pointer to stack
 *
 * return: true if stack is empty
 */
bool stack_empty(stack* s);

/*
 * Frees memory allocated to stack and its nodes
 *
 * s: pointer to stack
 *
 */
void stack_free(stack* s);
#endif
