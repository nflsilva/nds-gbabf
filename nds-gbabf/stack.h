#pragma once

#include <stdlib.h>

#define STACK_MAX_SIZE 100

struct Stack {
	int top;
	char* data[STACK_MAX_SIZE];
};
typedef struct Stack Stack_t;

bool s_isFull(Stack_t* stack) {
    if(!stack) return false;
    return stack->top == STACK_MAX_SIZE;
};

bool s_isEmpty(Stack_t* stack) {
    if(!stack) return true;
    return stack->top == -1;
};

bool s_push(Stack_t* stack, char* item) {
    if(!stack || s_isFull(stack)) return false;
    stack->data[++stack->top] = item;
    return true;
};

char* s_pop(Stack_t* stack) {
    if(!stack || s_isEmpty(stack)) return NULL;
    return stack->data[stack->top--];
};

char* s_peek(Stack_t* stack) {
    if(!stack || s_isEmpty(stack)) return NULL;
    return stack->data[stack->top];
};

char* s_peekAt(Stack_t* stack, int index) {
    if(!stack || s_isEmpty(stack) || stack->top < index) return NULL;
    return stack->data[index];
};

int s_size(Stack_t* stack) {
    if(!stack) return 0;
    return stack->top + 1;
}

Stack_t* s_createStack() {
    Stack_t* stack = (Stack_t*)malloc(sizeof(Stack_t));
    stack->top = -1;
    return stack;
}

void s_destroy(Stack_t* stack) {
    if(!stack) return;
    while(stack->top > 0) {
        free(s_pop(stack));
    }
    free(stack);
}
