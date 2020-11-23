/*
 * list.c November 25, 2020
 * Jared Diehl (jmddnb@umsystem.edu)
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "list.h"

List *list_create() {
    List *list = (List*) malloc(sizeof(List));
    list->head = NULL;
    return list;
}

ListNode *list_new_node(int index, int page, int frame) {
    ListNode *node = (ListNode*) malloc(sizeof(ListNode));
    node->index = index;
    node->page = page;
    node->frame = frame;
    node->next = NULL;
    return node;
}

void list_add(List *list, int index, int page, int frame) {
    ListNode *node = list_new_node(index, page, frame);

    if (list->head == NULL) {
        list->head = node;
        return;
    }

    ListNode *next = list->head;
    while (next->next != NULL)
        next = next->next;
    next->next = node;
}

void list_pop(List *list) {
    if (list->head == NULL) return;

    ListNode *node = list->head;
    list->head = list->head->next;
    free(node);
}

int list_remove(List *list, int index, int page, int frame) {
    ListNode *current = list->head;
    ListNode *previous = NULL;

    if (current == NULL) return -1;

    while (current->index != index || current->page != page || current->frame != frame) {
        if (current->next == NULL) return -1;
        else {
            previous = current;
            current = current->next;
        }
    }

    if (current == list->head) {
        int n = current->frame;
        free(current);
        list->head = list->head->next;
        return n;
    } else {
        int n = previous->next->frame;
        free(previous->next);
        previous->next = current->next;
        return n;
    }
}

bool list_contains(List *list, int key) {
    ListNode next;
    next.next = list->head;

    if (next.next == NULL) return false;

    while (next.next->frame != key) {
        if (next.next->next == NULL) return false;
        else next.next = next.next->next;
    }

    return true;
}