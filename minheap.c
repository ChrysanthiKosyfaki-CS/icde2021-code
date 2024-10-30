/*implementation of a minheap (priority queue)*/
#include <time.h>
#include "minheap.h"

/*returns the parent of a heap position*/
int parent(int posel)
{
  if (posel%2) /*odd*/
        return posel/2;
  else
        return (posel-1)/2;
}

/*enqueues element*/
/*heap[0] is the element with the smallest value*/
/*every element is greater or equal to its parent*/
void enqueue(e_type el, int idx, elem *heap, int *num_elems)
{
    e_type tmp;
    int tmpidx;
    int p;
    int posel;

    posel = *num_elems; //last position
    heap[(*num_elems)].value = el;
    heap[(*num_elems)++].idx = idx;

    while(posel >0)
    {
        p=parent(posel);
        if (el<heap[p].value)
        {
	  /* swap element with its parent */
          tmp = heap[p].value;
          heap[p].value = el;
          heap[posel].value = tmp;
            
	  tmpidx = heap[p].idx;
          heap[p].idx = idx;
          heap[posel].idx = tmpidx;
            
          posel = parent(posel);
        }
        else break;
    }
}

/* moves down the root element */
/* used by dequeue (see below) */
void movedown(elem *heap, int *num_elems)
{
    e_type tmp;
    int tmpidx;
    int posel = 0; //root
    int swap;
    /*while posel is not a leaf and heap[posel].value > any of childen*/
    while (posel*2+1 < *num_elems) /*there exists a left son*/
    {
        if (posel*2+2< *num_elems) /*there exists a right son*/
        {
            if(heap[posel*2+1].value<heap[posel*2+2].value)
                swap = posel*2+1;
            else
                swap = posel*2+2;
        }
        else
            swap = posel*2+1;

        if (heap[posel].value > heap[swap].value) /*larger than smallest son*/
        {
	    /*swap elements*/
            tmp = heap[swap].value;
            heap[swap].value = heap[posel].value;
            heap[posel].value = tmp;
            
	    tmpidx = heap[swap].idx;
            heap[swap].idx = heap[posel].idx;
            heap[posel].idx = tmpidx;
            
	    posel = swap;
        }
        else break;
    }
}

/* returns the root element, puts the last element as root and moves it down */
int dequeue(elem *el, elem *heap, int *num_elems)
{
    if ((*num_elems)==0) /* empty queue */
        return 0;

    el->value = heap[0].value;
    el->idx = heap[0].idx;
    heap[0].value = heap[(*num_elems)-1].value;
    heap[0].idx = heap[(*num_elems)-1].idx;
    (*num_elems)--;
    movedown(heap, num_elems);
    return 1;
}

void print_heap(elem *heap, int num_elems) {
  int i;

  for (i=0; i<num_elems; i++)
    printf("%.2f(%d) ", heap[i].value, heap[i].idx);
  printf("\n");
}

int intcomp(const void *x1, const void *x2)
{
  if (*(int *)x1>*(int *)x2)
    return -1;
  else if (*(int *)x1<*(int *)x2)
    return 1;
  else
    return 0;
}

/*application of a heap
 *scan a list of n numbers and return the k largest
 */
 /*
int main(int argc, char *argv[])
{
  int i;
  int k,n;
  int *ar; // array of elements from which topk elem. is to be computed 

  elem *topk; //array of elements on heap 
  int num_elems; //number of elements on heap (eventually k) 

  n=100; k=10;

  ar = (int *)malloc(n*sizeof(int));
  topk = (elem *)malloc(k*sizeof(elem));
  num_elems = 0;
  srand(time(NULL));
  for(i=0; i<n; i++) {
    ar[i] = rand()%1000;

    if (i<k) // enqueue all first k elements 
      enqueue((e_type)ar[i], i, topk, &num_elems);
    else {
      // enqueue only if larger than the k-th element 
      if (ar[i] > topk[0].value) {
		//replace root with the new element 
		topk[0].value = (e_type)ar[i];
		topk[0].idx = i;
		movedown(topk, &num_elems);
      }
    }

    printf("%d ", ar[i]);
  }
  printf("\n");

  printf("top %d: ", k);
  for (i=0; i<k; i++)
    printf("%.0f(%d) ", topk[i].value, topk[i].idx);
  printf("\n");

  //verification
  qsort(ar,n,sizeof(int),intcomp);
  for(i=0; i<k; i++)
    printf("%d ",ar[i]);
  printf("\n");
  
  free(ar);
  free(topk);
  return 0;
}*/






