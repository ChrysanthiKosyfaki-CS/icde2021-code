#include "computeflow.h"

int main(int argc, char **argv)
{
	int i,j,k;
	FILE *f; // graph input file
	struct Graph G;
	//char fname[50];
	
	struct DAG *G2;
	struct DAG *retDAG=NULL;
	
	struct Edge** edgearray;
	int numedges;

    clock_t t;
    double time_taken;
    
    double flow;
	
	edgearray = (struct Edge **)malloc(MAXEDGES*sizeof(struct Edge *)); //this array holds all edges that exist in valid paths 


	if (argc != 3)
	{
		printf("filename and source-id expected as arguments. Exiting...\n");
		return -1;
	}
	
	f = fopen(argv[1],"r");
	if (f==NULL)
	{
		printf("ERROR: file %s does not exist. Exiting...\n",argv[1]);
		return -1;
	}	
	
    if (read_graph(&G, f)==-1)
		return -1;
	
	int source = atoi(argv[2]);   
	int sink = source;
	
	// write paths from src to dest to a file
	int pathlen = 4;
	// write to edgearray distinct edges on paths from src to dest
	//int totinter = findPaths2(G,source,sink,pathlen,edgearray,&numedges); 
	int totinter = findPaths2(G,source,sink,pathlen,edgearray,&numedges); 
	
	printf("numedges=%d, totinter=%d\n",numedges, totinter);
	
	if (numedges==0) {
		printf("No paths found\n");
		return -1;
	} 
	
	/*printf("num distinct edges in paths=%d\n",numedges);
	for (i=0; i<numedges;i++) {
		printf("%d %d\n",edgearray[i]->src,edgearray[i]->dest);
	}*/
	
	//convert edgearray to DAG
	
	G2 = edgearray2DAG(edgearray, numedges, sink, 1);
	printf("numnodes=%d\n",G2->numnodes);
	//printDAG(G2);
	//return -1;

/*
	printf("processing DAG \n");
	printf("Old Greedy is running \n");
	t = clock();
	flow=computeFlowGreedyOld(*G2);
	t = clock() - t;
    time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Computed flow: %f\n", flow);
    printf("Total time of execution: %f seconds\n", time_taken);
*/
	printf("processing DAG \n");
	printf("Greedy is running \n");
	t = clock();
	flow=computeFlowGreedy(*G2);
	t = clock() - t;
    time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Computed flow: %f\n", flow);
    printf("Total time of execution: %f seconds\n", time_taken);


/*
	printf("Greedy With Inter is running \n");
	struct Interaction *inter = NULL;
	int numinter = 0;
	t = clock();
	flow=computeFlowGreedyWithInter(*G2,&inter,&numinter);
	t = clock() - t;
    time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Computed flow: %f\n", flow);
    printf("Total time of execution: %f seconds\n", time_taken);
    if (inter != NULL) {
    	free(inter);
    	inter = NULL;
    }
*/
    
    if (totinter<10000) {
		printf("LP is running \n");
		t = clock();    
		flow=computeFlowLP(*G2);
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("Computed flow: %f\n", flow);
		printf("Total time of execution: %f seconds\n", time_taken);

/*
		printf("LP With Inter is running \n");
		t = clock();    
		flow=computeFlowLPWithInter(*G2,&inter,&numinter);
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("Computed flow: %f\n", flow);
		printf("Total time of execution: %f seconds\n", time_taken);
		//printf("LP interactions into sink:\n");
		//for(i=0;i<numinter;i++){
		//	printf("%f %f\n",inter[i].timestamp,inter[i].quantity);
		//}
		if (inter != NULL) free(inter);
		*/
    }
/*
   	printf("Recursive Greedy is running \n");
   	t = clock();    
    //flow=findSolubleGreedy(G,*G2);
	flow = decompositionSolve(G,*G2);
	printf("Computed flow: %f\n", flow);
	t = clock() - t;
    time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Total time of execution: %f seconds\n", time_taken);
    */
    
    /*
    printf("Decomp is running \n");
    flow=computeFlow(*G2);
	printf("Computed flow: %f\n", flow);
	return -1;
*/


/*
	printf("new Decomp is running \n");
	t = clock(); 
    flow=compFlow(*G2);
	t = clock() - t;
	time_taken = ((double)t)/CLOCKS_PER_SEC;
	printf("Computed flow: %f\n", flow);
	printf("Total time of execution: %f seconds\n", time_taken);
	return -1;
	*/
	
    int *order = topoorder(G2);
	/*for(i=0;i<G2->numnodes;i++)
		printf("%d ",order[i]);
	printf("\n");*/
	
	//writeDAGtofile(G2,"G2befPre.txt");
	
	if (order) {
		printf("Preprocessing is running \n");
		int numdeletedinter=0;
		int numdeletededges=0;
		int numdeletednodes=0;
		t = clock(); 
		retDAG = preprocessDAG(G2, order, &numdeletedinter, &numdeletededges, &numdeletednodes);
	
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("Preprocessing time: %f seconds\n", time_taken);
		printf("Total deletions: interactions=%d, edges=%d, nodes=%d\n",numdeletedinter,numdeletededges,numdeletednodes);
		//printf("G is now %p\n",G2);

		//writeDAGtofile(G2,"G2aftPre.txt");
		if(retDAG==NULL)
		{
			printf("DAG has no flow: sink disconnected\n");
		}
		else if (totinter-numdeletedinter<10000) {
			writeDAGtofile(retDAG, "retDAG.txt");
			printf("LP after pre is running \n");
			t = clock();    
			flow=computeFlowLP(*retDAG);
			printf("Computed flow: %f\n", flow);
			t = clock() - t;
			time_taken = ((double)t)/CLOCKS_PER_SEC;
			printf("Total time of execution: %f seconds\n", time_taken);
			
			printf("new Decomp is running \n");
			t = clock(); 
			flow=compFlow(*retDAG, 1);
			t = clock() - t;
			time_taken = ((double)t)/CLOCKS_PER_SEC;
			printf("Computed flow: %f\n", flow);
			printf("Total time of execution: %f seconds\n", time_taken);
		}
	
		free(order);
	}
    //printf("G2: %p retDAG: %p\n",G2,retDAG);
    if (G2 == retDAG) {
    	//printf("G2 and retDAG are the same\n");
    	if (retDAG != NULL) 
    		freeDAG(G2); //no need to free retDAG
    }
    else {
        if (G2 != NULL) freeDAG(G2);
	    if (retDAG != NULL) freeDAG(retDAG);
    }
	free(edgearray);
	
   // printf("\n The Total Number of instances %s  = %d",  totalinstances);
    return 0;
}
