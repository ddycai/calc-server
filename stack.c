/********************************************************************************
 * stack.c
 * 
 * Computer Science 3357a
 * Linked Stack Implementation
 *
 * Author: Duncan Cai
 * 
 * Implementation of a linked stack.
*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <syslog.h>
#include "stack.h"

stack* stack_init()
{
	stack* stk = malloc(sizeof(stack));
	stk->head = NULL;
	stk->push_format = NULL;
	stk->pop_format = NULL;
	stk->_num_elements = 0;
	return stk;
}

void stack_push(stack* stk, int data)
{
	struct node* tmp = malloc(sizeof(struct node));
	if(tmp == NULL)
		exit(EXIT_FAILURE);
	tmp->data = data;
	tmp->next = stk->head;
	stk->head = tmp;
	stk->_num_elements++;
	if(stk->push_format != NULL)
		syslog(LOG_DEBUG, stk->push_format, data);
}

int stack_pop(stack* stk)
{
	struct node* tmp = stk->head;
	stk->head = tmp->next;
	int element = tmp->data;
	free(tmp);
	stk->_num_elements--;
	if(stk->pop_format != NULL)
		syslog(LOG_DEBUG, stk->pop_format, element);
	return element;
}

void stack_log(stack* stk, const char * format)
{
	struct node* current = stk->head;
	if(current == NULL)
	{
		syslog(LOG_DEBUG, "empty");
	} else {
		while(current != NULL)
		{
			syslog(LOG_DEBUG, format, current->data);
			current = current->next;
		}
	}
}

int stack_top(stack* stk)
{
	return stk->head->data;
}

size_t stack_size(stack* stk)
{
	return stk->_num_elements;
}

bool stack_empty(stack* stk) {
	return stk->head == NULL;
}

void stack_free(stack* stk)
{
	while(!stack_empty(stk))
	{
		stack_pop(stk);
	}
	free(stk);
}
