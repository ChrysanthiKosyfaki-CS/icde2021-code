#ifndef __COMPFLOW
#define __COMPFLOW

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "lp_lib.h"

#define MAXINSTANCES 1000000
#define MAXFLOW 10000000000
#define MAXTIME 1000000000000

#define MAXEDGES 10000 //max number of edges in a custom DAG extracted from the graph -> can cause seg.faults
#define MAXEDGESPATH 100 //max number of edges in a path DAG extracted from the graph (decomp)
//#define MAXOUT 1000 //max number of outgoing edges in a node of a custom DAG extracted from the graph
//#define MAXIN 1000 //max number of outgoing edges in a node of a custom DAG extracted from the graph
#define MAXINTER 10000
#define MAXNODES 10000 //max number of nodes in a DAG

struct DAG {
    int numnodes; // number of nodes in DAG
	struct DAGNode* node; // keeps track of incoming and outgoing edges to a node
    int numedges; // number of edges in DAG
    struct Edge **edgearray; // array of all DAG's edges (with their interactions)
};

struct DAGNode {
	int label; // should be equal to the order of the node in the DAGNode array of the DAG
    int numinc; // number of incoming edges to this node
    int numout; // number of outgoing edges from this node
    int *incedges; // indices to incoming edges in edgearray of DAG
    int *outedges; // indices to outgoing edges in edgearray of DAG
};

struct Graph {
	int numnodes;
	struct Node* node;
};

struct Edge {
	int src;
	int dest;
	int numinter;
	struct Interaction* inter;
};

struct Node {
	int label;
	int numout;
	struct Edge* edge;
};

struct Interaction {
	double timestamp;
	double quantity;
};


struct CPattern {
	int numnodes;
	int *labels;
};

struct CompleteInteraction {
    int src;
    int dest;
    double timestamp;
    double quantity;
};

void printpath(struct CPattern path, int len);
void fprintpath(FILE *fp, struct CPattern path, int len, double flow);

// searches elem in array; if found return position, else return -1
int searcharray(int *ar, int numelem, int elem);

// prints DAG to stdout, for debugging purposes
void printDAG(struct DAG *G2);

// write DAG to a file for visualization purposes
void writeDAGtofile(struct DAG *G2, char *filename);

// frees memory allocated by DAG
void freeDAG(struct DAG *G2);

// reads new graph file format - see examples in text files
int read_graph(struct Graph *G, FILE *f);

// prints graph (for debugging purposes)
void printGraph(struct Graph *G);

// converts edgearray to DAG
struct DAG *edgearray2DAG(struct Edge** edgearray, int numedges, int sink, int writefile);

// returns in an array the positions of nodes of the DAG in topological order
int* topoorder(struct DAG *G);

// accesses DAG in topological order "*order" and removes interactions that cannot contribute to flow
struct DAG *preprocessDAG(struct DAG *G, int *order, int *numdeletedinter, int *numdeletededges, int *numdeletednodes);

// suboptimal way to find the edge with the currently minimum timestamp
// used by greedy algorithm(s) 
int find_minedge(struct Edge **edgearray, int numedges, int *ptr);

// takes as input an array of graph node ids that form a path
// measures the flow along the path
double processInstanceChain(struct Graph G, int *instance, int numnodes);

// computes the flow throughout a DAG from its source (node at position 0) 
double computeFlowGreedyOld(struct DAG G);
double computeFlowGreedy(struct DAG G);

// same as computeFlowGreedy BUT records all incoming interactions to sink, which accumulate to total flow
double computeFlowGreedyWithInterOld(struct DAG G, struct Interaction **inter, int *numinter);
double computeFlowGreedyWithInter(struct DAG G, struct Interaction **inter, int *numinter);

// computes the flow throughout a DAG from its source (node at position 0) 
double computeFlowLP(struct DAG G);

//compare two interactions by time; used by qsort call in function computeFlowLPWithInter
int compInter(const void *a, const void *b);

// same as computeFlowLP; returns incoming interactions to sink, which accumulate to total flow
double computeFlowLPWithInter(struct DAG G, struct Interaction **inter, int *numinter);

// finds all paths from sourcenode to destnode up to a maximum length;
int findPaths2(struct Graph G, int sourcenode, int destnode, int maxlen, struct Edge** edgearray, int *numedges);
int findPaths3(struct Graph G, int sourcenode, int destnode, int maxlen, struct Edge** edgearray, int *numedges);


// takes as input an edgearray where edges are ordered by direction
double simplifyChain(struct Edge **edgearray, int numedges, struct Interaction **inter, int *numinter);

// computes flow in DAG G by first finding all reducible chains from the source node
double compFlow(struct DAG G, int writeDAG);

#endif // __COMPFLOW