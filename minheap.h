#ifndef __MINHEAP
#define __MINHEAP

#include <stdio.h>
#include <stdlib.h>

/* used by minheap implementation - see minheap.c */
typedef double e_type; /* replace this with your preferred type for the heap key */ 

typedef struct { /* heap element */
	e_type value; /* the value (key) */
	int idx; /* the identifier of the element */
} elem;

void enqueue(e_type el, int idx, elem *heap, int *num_elems);
void movedown(elem *heap, int *num_elems);
int dequeue(elem *el, elem *heap, int *num_elems);
void print_heap(elem *heap, int num_elems);

#endif // __MINHEAP