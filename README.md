# icde2021-code
Flow Computation in Temporal Interaction Networks

Instructions for compiling and running the code

1) Download lplib version 5.5 from https://sourceforge.net/projects/lpsolve/. You have to add to the same directory as this code 
- precompiled library file "liblpsolve55.a" and
- header files "lp_Hash.h", "lp_lib.h", "lp_matrix.h", "lp_mipbb.h", "lp_SOS.h", "lp_types.h", "lp_utils.h"

2) run make on your terminal to compile the code

3) To use the code, you will need a directed graph file in the following format:
numberofvertices
src1 outdegree
src1 dest1 numinteractions time1 flow1 time2 flow2 ...
src1 dest2 numinteractions time1 flow1 time2 flow2 ...
...
src2 outdegree
src2 dest1 numinteractions time1 flow1 time2 flow2 ...
src2 dest2 numinteractions time1 flow1 time2 flow2 ...
...

An example graph file (graph.txt) is given in this distribution

4) Running ./computeflowsingle  <graph_filename> <vertex-id> does the following:
a) the graph in graph_filename is read into memory
b) all paths from source=vertex-id to destination=vertex-id having length at most 4 are found and merged to form a directed acyclic graph (DAG). 
c) The following algorithms are run on the resulting DAG:
- the greedy algorithm
- LP
- A preprocessing algorithm that removes irrelevant interactions, etc.
- LP after preprocessing
- Graph simplification and then LP after preprocessing 

Example:make
        ./computeflowsingle graph.txt 1



