/*
 * list.h November 24, 2020
 * Jared Diehl (jmddnb@umsystem.edu)
 */

#ifndef LIST_H
#define LIST_H

typedef struct ListNode {
	int index;
	int page;
	int frame;
	struct ListNode *next;
} ListNode;

typedef struct {
	ListNode *head;
} List;

List *list_create();
void list_add(List*, int, int, int);
void list_pop(List*);
int list_remove(List*, int, int, int);
bool list_contains(List*, int);

#endif