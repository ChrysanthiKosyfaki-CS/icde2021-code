CC       = gcc
CCOPTS   = -c -O
LINK     = gcc
LINKOPTS = liblpsolve55.a -lm -ldl

.c.o: 
	$(CC) $(CCOPTS) $<

all: computeflowsingle 

minheap.o: minheap.c

computeflow.o: computeflow.c

computeflowsingle.o: computeflowsingle.c

computeflowsingle: computeflow.o computeflowsingle.o minheap.o
	$(LINK) -o computeflowsingle computeflowsingle.o computeflow.o minheap.o $(LINKOPTS)

clean:
	rm *o computeflowsingle

