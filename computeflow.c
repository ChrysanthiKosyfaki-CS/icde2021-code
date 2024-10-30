#include "minheap.h"
#include "computeflow.h"

void printpath(struct CPattern path, int len) {
	int i;
	printf("Path: ");
	for (i=0;i<len;i++) {
		printf("%d ",path.labels[i]);
	}
	printf("\n");
}

void fprintpath(FILE *fp, struct CPattern path, int len, double flow) {
	int i;
	for (i=0;i<len;i++) {
		fprintf(fp, "%d ",path.labels[i]);
	}
	fprintf(fp, "%f ",flow);
	fprintf(fp, "\n");
}


// searches elem in array; if found return position, else return -1
int searcharray(int *ar, int numelem, int elem)
{
	int i;
	for(i=0;i<numelem;i++)
		if (ar[i]==elem)
			return i;
	return -1; //not found		 
}

// prints DAG to stdout, for debugging purposes
void printDAG(struct DAG *G2)
{
	int i,j;

	printf("printing DAG:\n");
	printf("numnodes=%d\n",G2->numnodes);
	printf("numedges=%d\n",G2->numedges);
	for (i=0; i<G2->numnodes; i++) {
		printf("Node %d has %d outgoing edges:\n",i,G2->node[i].numout);
		for (j=0;j<G2->node[i].numout; j++)
			printf("%d %d\n",G2->edgearray[G2->node[i].outedges[j]]->src,G2->edgearray[G2->node[i].outedges[j]]->dest);
		printf("Node %d has %d incoming edges\n",i,G2->node[i].numinc);
		for (j=0;j<G2->node[i].numinc; j++)
			printf("%d %d\n",G2->edgearray[G2->node[i].incedges[j]]->src,G2->edgearray[G2->node[i].incedges[j]]->dest);		
	}
	for (i=0; i<G2->numedges; i++) {
		printf("Edge %d->%d interactions:\n",G2->edgearray[i]->src,G2->edgearray[i]->dest);
		for(j=0;j<G2->edgearray[i]->numinter;j++)
			printf("time: %f qty: %f\n",G2->edgearray[i]->inter[j].timestamp,G2->edgearray[i]->inter[j].quantity);
	}
}

// write DAG to a file for visualization purposes
void writeDAGtofile(struct DAG *G2, char *filename)
{
	int i,j;
		
	FILE *f = fopen(filename,"w"); //this file will be read by python script and converted to a gv file for dot visualization
	fprintf(f,"%d\n",G2->numnodes);
	for (i=0; i<G2->numedges;i++) {
		//printf("%d->%d: %d interactions\n",G2->edgearray[i]->src,G2->edgearray[i]->dest,G2->edgearray[i]->numinter);
		for (j=0; j<G2->edgearray[i]->numinter; j++) {
			//fprintf(f,"%d %d %.1f %.1f\n",G2->edgearray[i]->src,G2->edgearray[i]->dest,G2->edgearray[i]->inter[j].timestamp,G2->edgearray[i]->inter[j].quantity);
			fprintf(f,"%d %d %.1f %.1f\n",G2->edgearray[i]->src,G2->edgearray[i]->dest,G2->edgearray[i]->inter[j].timestamp,G2->edgearray[i]->inter[j].quantity);
			if (j==4) break; //avoid huge visualization (3 interactions means 3 or more)
		}
	}
	fclose(f);
}	

// frees memory allocated by DAG
void freeDAG(struct DAG *G2)
{	
	for (int i=0; i<G2->numedges;i++) {
		if (G2->edgearray[i]!=NULL) {
			//printf("edge %p\n",G2->edgearray[i]);
			if (G2->edgearray[i]->inter!=NULL) {
				//printf("inter %p\n",G2->edgearray[i]->inter);
				free(G2->edgearray[i]->inter);
				G2->edgearray[i]->inter=NULL;
			}
			free(G2->edgearray[i]);
			G2->edgearray[i]=NULL;
		}
	}
	if (G2->edgearray!=NULL) {
    	free(G2->edgearray);
    	G2->edgearray=NULL;
    } 
	for (int i=0; i<G2->numnodes; i++) {
		if (G2->node[i].incedges != NULL) {
			free(G2->node[i].incedges);
			G2->node[i].incedges=NULL;
		}
		if (G2->node[i].outedges != NULL) {
			free(G2->node[i].outedges);
			G2->node[i].outedges=NULL;
		}
	}
	if (G2->node!=NULL) {
		free(G2->node);
		G2->node=NULL;
	} 
	free(G2);
}


// reads new graph file format - see examples in text files
// expecting a format as follows
// <number of nodes>
// <node1> <number of outgoing edges from node1>
// <node1> <destination 1> <number of interactions> <inter1.time> <inter1.quantity> ...
// <node1> <destination 2> <number of interactions> <inter1.time> <inter1.quantity> ...
// ...
// <node2> <number of outgoing edges from node2>
// <node2> <destination 1> <number of interactions> <inter1.time> <inter1.quantity> ...
// ...
int read_graph(struct Graph *G, FILE *f)
{
	int j,k;

	char *line = NULL; // used for fileread
	size_t len = 0; // used for fileread
	ssize_t read; // used for fileread
	const char delim[2] = "\t"; // used for fileread
	char *token; // used for fileread
	
	int src;
	int dest;
	int numout;
	double ts; //timestamp
	double qty; //quantity
	int numline = 1;
	
	int numedges=0;
	int numinter=0;
	double totalflow=0;
	
	/* read first line */
	/* first line should be <numnodes> */
	read = getline(&line, &len, f);
	if (read==-1)
	{
		printf("ERROR: first line is empty. Exiting...\n");
		return -1;
	};
	token = strtok(line,delim);
	G->numnodes = atoi(token);
	
	
	// Initialize data structure
	G->node = (struct Node*)malloc(G->numnodes*sizeof(struct Node));
	
	// Read graph from file
	while ((read = getline(&line,&len,f)) != -1)	{
		numline++;
		// next line to be read should be <src> <outdegree>
		token = strtok(line,delim);
		src = atoi(token);
		token = strtok(NULL,delim);
		numout = atoi(token);
		numedges+=numout;
		//printf("%d\t%d\n",src,numout);
		
		G->node[src].label = src;
		G->node[src].numout = numout;
		G->node[src].edge = (struct Edge*)malloc(numout*sizeof(struct Edge));
		for (j=0;j<numout;j++) {
			//printf("%d\n",j);
			read = getline(&line,&len,f);
			numline++;
			token = strtok(line,delim);
			G->node[src].edge[j].src = atoi(token);
			if (src != G->node[src].edge[j].src) {
				printf("Problem in line %d: %d %d\n",numline,src,G->node[src].edge[j].src);
				return -1;
			}
			token = strtok(NULL,delim);
			G->node[src].edge[j].dest = atoi(token);
			G->node[atoi(token)].label = atoi(token); 
			token = strtok(NULL,delim);
			G->node[src].edge[j].numinter = atoi(token);
			numinter+=atoi(token);
			G->node[src].edge[j].inter = (struct Interaction*)malloc(G->node[src].edge[j].numinter*sizeof(struct Interaction));
			for(k=0;k<G->node[src].edge[j].numinter;k++) {
				token = strtok(NULL,delim);
				ts = atof(token);
				token = strtok(NULL,delim);
				qty = atof(token);
				totalflow+=qty;
				G->node[src].edge[j].inter[k].timestamp = ts;
				G->node[src].edge[j].inter[k].quantity = qty;
			}
		}
	}
	
	// handle "phantom" nodes: these are nodes that have no outgoing and no incoming edges
	// they correspond to nodes in the original graph with self-loops only (which were disregarded by the conversion program)
	int cphantoms=0;
	for (int i=0;i<G->numnodes;i++)
		if (G->node[i].label!=i) {
			cphantoms++;
			//printf("node %d, label %d, numout %d\n",i,G->node[i].label,G->node[i].numout);
			G->node[i].label = i;
			//if (G->node[i].numout>0)
			//	break;
		}
	printf("numnodes=%d\n",G->numnodes);
	printf("numedges=%d\n",numedges);
	printf("numinteractions=%d\n",numinter);
	printf("average flow per interaction=%f\n",totalflow/numinter);
	printf("numphantoms = %d\n",cphantoms);
	
	fclose(f);
	if (line)
		free(line);
	
	//printGraph(G);
	return 0;
}

// prints graph (for debugging purposes)
void printGraph(struct Graph *G)
{
	int i,j,k;
	for (i=0;i<G->numnodes;i++) {
		printf("node %d, label = %d, numout = %d \n", i, G->node[i].label, G->node[i].numout);
		for (j=0;j<G->node[i].numout;j++) {
			printf("edge %d,%d has %d interactions\n",G->node[i].edge[j].src,G->node[i].edge[j].dest,G->node[i].edge[j].numinter);
			for (k=0;k<G->node[i].edge[j].numinter; k++)
			 	printf("interaction %f %f\n", G->node[i].edge[j].inter[k].timestamp, G->node[i].edge[j].inter[k].quantity);
		}
	}
}

// converts edgearray to DAG
// edgearray[0]->src considered to be source node
// nodes-ids and src/dest are mapped to a continuous id range [0,numnodes-1]
// returns constructed DAG
// set writefile=1 if you want constructed DAG to be written to file DAG.txt
struct DAG *edgearray2DAG(struct Edge** edgearray, int numedges, int sink, int writefile)
{
	int i,j,k;
	struct DAG *G2;
	int writtendest;
	
	//first nodes-ids and src/dest should be mapped to continuous ids in [0,numnodes-1]
	//source node should be 0, sink node should be numnodes-1 
	int *nodesarray = (int *)malloc(numedges*sizeof(int)); //keeps the true labels of the nodes in an array
	int nn =1; // number of nodes
	nodesarray[0]=edgearray[0]->src; //source node of DAG goes first
	for (i=0; i<numedges;i++) {
		if ((searcharray(nodesarray,  nn, edgearray[i]->src))==-1)
			nodesarray[nn++]=edgearray[i]->src; //give new label to newly found node
		if (edgearray[i]->dest!=sink && (searcharray(nodesarray,  nn, edgearray[i]->dest))==-1)
			nodesarray[nn++]=edgearray[i]->dest; //give new label to newly found node		
	}
	nodesarray[nn]=nn; // sink node of DAG goes last and takes last label (artificial)
	nn++;
	/*for (int i=0; i<nn-1; i++)
		printf("%d, ",nodesarray[i]);
	printf("%d\n",nodesarray[nn-1]);*/
	
	G2 = (struct DAG *)malloc(sizeof(struct DAG));
	G2->numnodes = nn;
	G2->node = (struct DAGNode *)malloc(G2->numnodes*sizeof(struct DAGNode));
	for (i=0; i<G2->numnodes; i++) {
		G2->node[i].label = i;
		G2->node[i].numinc = 0; //initialize number of incoming edges to node
		G2->node[i].numout = 0; //initialize number of outgoing edges from node
	}

	//DAG.edgearray is going to be a copy of edgearray because we're going to change the 
	//src, dest of each node
	//we should also copy the interactions
	G2->numedges = numedges;
	G2->edgearray = (struct Edge **)malloc(numedges*sizeof(struct Edge *)); 
	for (i=0; i<numedges;i++) {
		G2->edgearray[i]=(struct Edge *)malloc(numedges*sizeof(struct Edge));
		G2->edgearray[i]->inter=(struct Interaction *)malloc(edgearray[i]->numinter*sizeof(struct Interaction));
		//printf("new inter allocated %p for edgearray %p\n", G2->edgearray[i]->inter, G2->edgearray[i]);
		for(j=0; j<edgearray[i]->numinter; j++)
			G2->edgearray[i]->inter[j]=edgearray[i]->inter[j]; 
		G2->edgearray[i]->numinter=edgearray[i]->numinter;
		G2->edgearray[i]->src=searcharray(nodesarray,  nn, edgearray[i]->src);
		G2->node[G2->edgearray[i]->src].numout++;
		G2->edgearray[i]->dest=searcharray(nodesarray,  nn, edgearray[i]->dest);
		if (edgearray[i]->dest==sink) //special handling of sink node (in case it is the same as source)
			G2->edgearray[i]->dest=nn-1;
		G2->node[G2->edgearray[i]->dest].numinc++;
	}
	
	// mem alloc
	for (i=0; i<G2->numnodes; i++) {
		//printf("Node %d has %d outgoing edges\n",i,G2->node[i].numout);
		//printf("Node %d has %d incoming edges\n",i,G2->node[i].numinc);
		G2->node[i].incedges = (int *)malloc(G2->node[i].numinc*sizeof(int));
		G2->node[i].outedges = (int *)malloc(G2->node[i].numout*sizeof(int));
		G2->node[i].numinc=0; //reset in order to increase again when adding the edges
		G2->node[i].numout=0; //reset in order to increase again when adding the edges	
	}
	
	
	// set incoming and outgoing edges for each node	
	for (i=0; i<numedges;i++) {
		G2->node[G2->edgearray[i]->src].outedges[G2->node[G2->edgearray[i]->src].numout++]=i;
		G2->node[G2->edgearray[i]->dest].incedges[G2->node[G2->edgearray[i]->dest].numinc++]=i;
	}
	
	/*
	for (i=0; i<G2->numnodes; i++) {
		printf("Node %d has %d outgoing edges\n",i,G2->node[i].numout);
		printf("Node %d has %d incoming edges\n",i,G2->node[i].numinc);
	}*/
	
	if (writefile) {	
		//verify DAG and write it to a file for visualization purposes
		//initialize file
		//sprintf(fname,"%s_paths.txt",filename);
		FILE *f = fopen("DAG.txt","w"); //this file will be read by python script and converted to a gv file for dot visualization
		fprintf(f,"%d\n",G2->numnodes);
		for (i=0; i<numedges;i++) {
			//printf("%d->%d: %d interactions\n",G2->edgearray[i]->src,G2->edgearray[i]->dest,G2->edgearray[i]->numinter);
			for (j=0; j<G2->edgearray[i]->numinter; j++) {
				//if you want to visualize with original node labels uncomment the following block and comment the one next to block comment
				
				writtendest = nodesarray[G2->edgearray[i]->dest]; //used in order to recover original node labels 
				if (edgearray[i]->dest==sink)
					writtendest = sink;
				fprintf(f,"%d %d %.1f %.1f\n",nodesarray[G2->edgearray[i]->src],writtendest,G2->edgearray[i]->inter[j].timestamp,G2->edgearray[i]->inter[j].quantity);
				 
				//fprintf(f,"%d %d %.1f %.1f\n",G2->edgearray[i]->src,G2->edgearray[i]->dest,G2->edgearray[i]->inter[j].timestamp,G2->edgearray[i]->inter[j].quantity);
				if (j==4) break; //avoid huge visualization (6th and later interactions on edges are not shown)
			}
		}
		fclose(f);
	}

	free(nodesarray);
	
	return G2;
}

// used by function topoorder, which follows
int visit(struct DAG *G, int n, int *order, int *curpos, int *perm, int *temp)
{
	if (perm[n])
		return 0;
	if (temp[n]) {
		printf("ERROR: not a DAG\n");
		return -1;
	}
	
	temp[n]=1;
	
	for(int i=0; i<G->node[n].numout; i++)
		if (visit(G, G->edgearray[G->node[n].outedges[i]]->dest, order, curpos, perm, temp))
			return -1;

    temp[n]=0;
    perm[n]=1;
    order[(*curpos)--]=n;
    return 0;
}

// returns in an array the positions of nodes of the DAG in topological order
// implements the DFS algorithm [see book on Algorithms, Cormen et al.]
// complexity is O(V+E)
int* topoorder(struct DAG *G)
{
	int isdag = 1;
	int curpos = G->numnodes-1;
	int *order = (int *)malloc(G->numnodes*sizeof(int));
	int *perm = (int *)calloc(G->numnodes,sizeof(int)); //calloc initializes contents to 0
	int *temp = (int *)calloc(G->numnodes,sizeof(int));
	
	//for(int i=0; i<G->numnodes; i++)
	//	printf("%d %d %d\n",order[i],perm[i], temp[i]);
		
	if (visit(G, 0, order, &curpos, perm, temp)==-1) {
		//not a DAG
		isdag = 0;
	}
	
	free(perm);
	free(temp);
	if (isdag)
		return order;
	else {
		free(order);
		return NULL;
	}
}

// deletes a DAG node n and its parents recursively (if applicable)
// used by preprocessDAG function below
// returns -1 if source node of DAG is deleted, else returns 0
int delnode(struct DAG *G, int n, int **deletededges, int *numdeletededges, int **deletednodes, int *numdeletednodes, int **numdeletedout, int *numdeletedinter)
{
	int ret = 0;
	//printf("node %d deleted (RECURSION)\n",n);
	(*deletednodes)[n]=1;
	(*numdeletednodes)++;
	
	for (int j=0; j<G->node[n].numinc; j++) { //for each incoming edge of current node
		if (!(*deletededges)[G->node[n].incedges[j]]) {
			//printf("edge %d->%d deleted\n",G->edgearray[G->node[n].incedges[j]]->src,G->edgearray[G->node[n].incedges[j]]->dest);
			free(G->edgearray[G->node[n].incedges[j]]->inter);
			G->edgearray[G->node[n].incedges[j]]->inter = NULL;
			G->edgearray[G->node[n].incedges[j]]->numinter = 0;
			(*numdeletedinter)+=G->edgearray[G->node[n].incedges[j]]->numinter; //update number of deleted interactions
			(*deletededges)[G->node[n].incedges[j]]=1;
			(*numdeletededges)++;
			(*numdeletedout)[G->edgearray[G->node[n].incedges[j]]->src]++;
			if ((*numdeletedout)[G->edgearray[G->node[n].incedges[j]]->src]==G->node[G->edgearray[G->node[n].incedges[j]]->src].numout)
			{
				//origin node of deleted edge has no more outgoing edges
				//this node should be deleted recursively
				if (G->edgearray[G->node[n].incedges[j]]->src==0) {
					//we are about to delete the source node of the DAG
					//this means that the DAG has 0 flow
					return -1;
				}
				//printf("node %d will be deleted\n",G->edgearray[G->node[n].incedges[j]]->src);
				ret = delnode(G,G->edgearray[G->node[n].incedges[j]]->src,deletededges,numdeletededges,deletednodes,numdeletednodes,numdeletedout,numdeletedinter);
				if (ret==-1)
					return -1;
			}
		}
	}
	
	return ret;
}

// accesses DAG in topological order "*order" and removes interactions that cannot contribute to flow
// an interaction is removed if its timestamp is smaller than the min-timestamp of all incoming
// interactions to its source node
// if an edge has 0 interactions then remove edge 
// if a node has no incoming edges, remove node and recursively its children if needed 
// if a node has no outgoing edges, node is deleted by function delnode 
// if source of DAG loses connection to sink, we discover this and return a NULL DAG 
// else if some edges are deleted, constructs and creates a new DAG
// else returns the input DAG (with some interactions possibly deleted)
struct DAG *preprocessDAG(struct DAG *G, int *order, int *numdeletedinter, int *numdeletededges, int *numdeletednodes)
{
	double m; // auxiliary variable
	int ret = 0; //returned value
	
	struct DAG *newdag = NULL; 
		
	struct Interaction *newinter;
	int newnuminter;
	
	//marks deleted edges
	int *deletededges = (int *)calloc(G->numedges,sizeof(int));
	//marks deleted nodes
	int *deletednodes = (int *)calloc(G->numnodes,sizeof(int));
	//keeps track of number of outgoing deleted edges from each node	
	int *numdeletedout = (int *)calloc(G->numnodes,sizeof(int)); 

	*numdeletedinter=0; // total number of removed interactions
	*numdeletededges=0; // total number of removed edges
	*numdeletednodes=0; // total number of removed nodes
/*
	for (int i=0; i<G->numedges; i++) {
		deletededges[i]=0;
		if (G->edgearray[i]->src == 0 && G->edgearray[i]->dest == 3)
			printf("the edge of interest is %d\n",i);
	}
	for (int i=0; i<G->numnodes; i++) {
		deletednodes[i]=0;
		numdeletedout[i]=0;
	}
	*/

/*	
	for(int i=0; i<G->numedges; i++)
		printf("%d ",deletededges[i]);
	printf("\n");
*/	
	// sanity check
/*	for (int i=0; i<G->numedges; i++)
		if (!deletededges[i])
			printf("edge %d->%d is there\n",G->edgearray[i]->src,G->edgearray[i]->dest);
*/
	// source node of DAG is always first in the topo order, so we skip it, as it has no incoming edges
	for(int i=1; i<G->numnodes; i++)
	{
		//printf("Examining node %d\n",order[i]);
		
		//step 1: find min timestamp in all incoming edges to current node
		double mintimein = MAXTIME; // CAUTION: make sure MAXTIME is larger than all timestamps in your graph
		for (int j=0; j<G->node[order[i]].numinc; j++)
			if (!deletededges[G->node[order[i]].incedges[j]])
				if ((m=G->edgearray[G->node[order[i]].incedges[j]]->inter[0].timestamp)<mintimein)
					mintimein=m;
		
		if (mintimein==MAXTIME) // special case where all incoming edges to current node are deleted
		{
			// ALL incoming edges to this node are marked as deleted
			// we should now mark this node as deleted
			// and mark as deleted all its outgoing edges
			//printf("node %d deleted\n",order[i]);
			//printf("node %d will be deleted\n",order[i]);
			deletednodes[order[i]]=1;
			(*numdeletednodes)++;
			for (int j=0; j<G->node[order[i]].numout; j++) { //for each outgoing edge of current node
				if (!deletededges[G->node[order[i]].outedges[j]]) {
					//printf("edge %d->%d deleted\n",G->edgearray[G->node[order[i]].outedges[j]]->src,G->edgearray[G->node[order[i]].outedges[j]]->dest);
					free(G->edgearray[G->node[order[i]].outedges[j]]->inter);
					(*numdeletedinter)+=G->edgearray[G->node[order[i]].outedges[j]]->numinter;
					G->edgearray[G->node[order[i]].outedges[j]]->inter = NULL;
					G->edgearray[G->node[order[i]].outedges[j]]->numinter = 0;
					deletededges[G->node[order[i]].outedges[j]]=1;
					(*numdeletededges)++;
					numdeletedout[order[i]]++;
				}
			}
			if (i==G->numnodes-1)
			{
				//sink node is deleted!
				//flow is 0
				//printf("Sink node deleted!\n");
				ret = -1;
				break;
			}
		}	
		//printf("mintimestamp into node %d is %f\n",order[i],mintimein);
		else //not all incoming edges to current node are deleted
		{
			for (int j=0; j<G->node[order[i]].numout; j++) { //for each outgoing edge of current node
				if (!deletededges[G->node[order[i]].outedges[j]]) {
					newinter = (struct Interaction *)malloc(G->edgearray[G->node[order[i]].outedges[j]]->numinter*sizeof(struct Interaction));
					newnuminter=0;
					for (int k=0; k<G->edgearray[G->node[order[i]].outedges[j]]->numinter; k++)
						if (G->edgearray[G->node[order[i]].outedges[j]]->inter[k].timestamp>mintimein) {
							//printf("timestamp %f from node %d should be deleted\n",G->edgearray[G->node[order[i]].outedges[j]]->inter[k].timestamp,order[i]);
							newinter[newnuminter].timestamp=G->edgearray[G->node[order[i]].outedges[j]]->inter[k].timestamp;
							newinter[newnuminter++].quantity=G->edgearray[G->node[order[i]].outedges[j]]->inter[k].quantity;
						}
						else (*numdeletedinter)++;
					free(G->edgearray[G->node[order[i]].outedges[j]]->inter);
					G->edgearray[G->node[order[i]].outedges[j]]->inter=newinter;
					G->edgearray[G->node[order[i]].outedges[j]]->numinter=newnuminter;
					if (!newnuminter) // all interactions on edge are removed
					{
						//mark edge as deleted
						//printf("edge %d->%d deleted\n",G->edgearray[G->node[order[i]].outedges[j]]->src,G->edgearray[G->node[order[i]].outedges[j]]->dest);
						deletededges[G->node[order[i]].outedges[j]]=1;
						(*numdeletededges)++;
						numdeletedout[order[i]]++;
					}
				}
			}
			if (G->node[order[i]].numout>0 && numdeletedout[order[i]]==G->node[order[i]].numout)
			{
				// ALL outgoing edges from this node are marked as deleted
				// and this node is not the sink
				// this node and its incoming edges should be deleted
				// then, delete recursively the node's parent if applicable
				// we must keep track the number of deleted edges for each node
				//printf("node %d is deleted\n",order[i]);
				ret = delnode(G,order[i],&deletededges,numdeletededges,&deletednodes,numdeletednodes,&numdeletedout,numdeletedinter);
				if (ret==-1) break; //DAG's source node deleted
			}
		}
	}
	
	//printf("Total deletions: interactions=%d, edges=%d, nodes=%d\n",*numdeletedinter,*numdeletededges,*numdeletednodes);
	

	if (!ret && (*numdeletededges))
	{
		// if at least one edge is deleted, we update the DAG
		
		// not the fastest way, but this is what we have now
		struct Edge **edgearray = (struct Edge **)malloc(G->numedges*sizeof(struct Edge *));
		int numedges = 0;

		// copy non-deleted edge to edgearray
		int sourcepos = -1; //used to swap edges if first edge in edgearray is not from source
		for (int i=0; i<G->numedges; i++)
			if (!deletededges[i]) {
				//printf("edge %d->%d remains\n",G->edgearray[i]->src,G->edgearray[i]->dest);
				if (sourcepos == -1 && G->edgearray[i]->src==0)
					sourcepos = numedges;
				edgearray[numedges++]=G->edgearray[i];
			}
		struct Edge *tmp = edgearray[sourcepos];
		edgearray[sourcepos]=edgearray[0];
		edgearray[0]=tmp;

		// create the new DAG
		newdag = edgearray2DAG(edgearray, numedges, G->numnodes-1, 0);
		//writeDAGtofile(newdag);
		free(edgearray);
		//freeDAG(G);
		//G = newdag;
		//printf("G is now %p\n",G);
		//return newdag;
		ret = -1; // in order to return newdag
	}
	
	
	free(deletededges);
	free(deletednodes);
	free(numdeletedout);

	if (ret==-1)
		return newdag; // should be NULL if no newdag is constructed
	else
		return G;
}

// suboptimal way to find the edge with the currently minimum timestamp
// used by greedy algorithm(s) 
int find_minedge(struct Edge **edgearray, int numedges, int *ptr) {
	int minedge=-1;
	double mintime = MAXTIME;
	int i;
	
	for(i=0;i<numedges;i++) {
		if (ptr[i]==edgearray[i]->numinter) {
			//no more interactions on edge
			continue;
		}
		
		if (edgearray[i]->inter[ptr[i]].timestamp < mintime) {
			mintime = edgearray[i]->inter[ptr[i]].timestamp;
			minedge = i;
		}
	}
	return minedge;
}

// takes as input an array of graph node ids that form a path
// measures the flow along the path
double processInstanceChain(struct Graph G, int *instance, int numnodes)
{
	int i,j,k;
	double *buffer;
	struct Edge **edgearray;
	int numedges = 0;
	int c = 0;
	int *ptr;
	int minedge;
	double flow;
	
	buffer = (double *)malloc(numnodes*sizeof(double));
	for(i=1;i<numnodes;i++) {
		buffer[i]=0.0;
		//numedges += inst.node[i].numout;
	}
	buffer[0]=MAXFLOW; // a very big number	

	//edgearray[i] should point to the i-th graph edge in the pattern instance
	edgearray = (struct Edge **)malloc((numnodes-1)*sizeof(struct Edge *)); 
	ptr = (int *)malloc((numnodes-1)*sizeof(int));
	for(i=0;i<numnodes-1;i++) {
		ptr[i]=0;
		for(j=0;j<G.node[instance[i]].numout;j++) {
			if (G.node[instance[i]].edge[j].dest == instance[i+1]) {
				edgearray[i] = &G.node[instance[i]].edge[j];
				break;
			}
		}
	}
	
	/*
	for(i=0;i<numnodes-1;i++) {
		printf("%d %d \n",edgearray[i]->src,edgearray[i]->dest);
	}*/

	//merge interactions on edges and compute flow
	while ((minedge = find_minedge(edgearray, numnodes-1, ptr)) != -1) {
		//printf("minedge:%d,src:%d,dest:%d,ptr:%d,time:%f\n", minedge, edgearray[minedge]->src,  edgearray[minedge]->dest, ptr[minedge], edgearray[minedge]->inter[ptr[minedge]].timestamp);
		flow = (buffer[minedge] < edgearray[minedge]->inter[ptr[minedge]].quantity) ? buffer[minedge] : edgearray[minedge]->inter[ptr[minedge]].quantity ;
		buffer[minedge] -= flow;
		buffer[minedge+1] += flow;		
		ptr[minedge]++;
	}
	
	/*for (i=0; i<numnodes; i++) {
		printf("flow at node %d is %f\n",instance[i], buffer[i]);
	}*/
	
	free(edgearray);
	free(ptr);
	free(buffer);
	return buffer[numnodes-1];
}

// computes the flow throughout a DAG from its source (node at position 0) 
// to its sink (node at position G.numnodes-1)
// uses the GREEDY algorithm, which does not necessarily compute the max flow
// but it is very fast (cost near linear to the number of interactions) 
// TODO: cost can be reduced if find_minedge is implemented using a heap
double computeFlowGreedyOld(struct DAG G)
{
    int i,j,k;
    double *buffer; // array of buffers, one for each node of the DAG
    int *ptr; // array with pointers to the current interaction per edge
    int minedge; 
    double flow;
    
    buffer = (double *)malloc(G.numnodes*sizeof(double));
    for(i=1;i<G.numnodes;i++) {
        buffer[i]=0.0; // initially, all buffers, except the buffer of the source are 0
    }
    buffer[0]=MAXFLOW; // a very big number, make sure MAXFLOW is larger than the sum of all flows on all interactions in the DAG

    ptr = (int *)malloc((G.numedges)*sizeof(int)); // it could be faster if we used calloc 
    for(i=0;i<G.numedges;i++) {
        ptr[i]=0;
    }

    //merge interactions on edges and compute flow
    while ((minedge = find_minedge(G.edgearray, G.numedges, ptr)) != -1) {
        //printf("minedge:%d,src:%d,dest:%d,ptr:%d,time:%f,qty:%f\n", minedge, G.edgearray[minedge]->src,  G.edgearray[minedge]->dest, ptr[minedge], G.edgearray[minedge]->inter[ptr[minedge]].timestamp, G.edgearray[minedge]->inter[ptr[minedge]].quantity);
        flow = (buffer[G.edgearray[minedge]->src] < G.edgearray[minedge]->inter[ptr[minedge]].quantity) ? buffer[G.edgearray[minedge]->src] : G.edgearray[minedge]->inter[ptr[minedge]].quantity ;
        //printf("flow=%f\n",flow);
        buffer[G.edgearray[minedge]->src] -= flow;
        buffer[G.edgearray[minedge]->dest] += flow;
        ptr[minedge]++;
    }
    
    flow = buffer[G.numnodes-1];
    //printf("flow at node %d is %f\n",G.numnodes-1, flow);

	free(buffer);
	free(ptr);
	
    return flow;
}

// computes the flow throughout a DAG from its source (node at position 0) 
// to its sink (node at position G.numnodes-1)
// uses the GREEDY algorithm, which does not necessarily compute the max flow
// but it is very fast (cost near linear to the number of interactions) 
// TODO: cost can be reduced if find_minedge is implemented using a heap
double computeFlowGreedy(struct DAG G)
{
    int i,j,k;
    double *buffer; // array of buffers, one for each node of the DAG
    int *ptr; // array with pointers to the current interaction per edge
    int minedge; 
    double flow;
    elem *heapq = (elem *)malloc((G.numedges)*sizeof(elem)); //array of heap elements for merging edges
    elem tempE; // dummy element, used for dequeueing
    int numheapqelem = 0; //number of elements in heapq
    
    buffer = (double *)malloc(G.numnodes*sizeof(double));
    for(i=1;i<G.numnodes;i++) {
        buffer[i]=0.0; // initially, all buffers, except the buffer of the source are 0
    }
    buffer[0]=MAXFLOW; // a very big number, make sure MAXFLOW is larger than the sum of all flows on all interactions in the DAG

    ptr = (int *)malloc((G.numedges)*sizeof(int)); // it could be faster if we used calloc 
    for(i=0;i<G.numedges;i++) {
        ptr[i]=0;
        enqueue(G.edgearray[i]->inter[ptr[i]].timestamp, i, heapq, &numheapqelem);
        //printf("edgeid=%d (%d->%d, %f, %f)\n",i,G.edgearray[i]->src,G.edgearray[i]->dest,G.edgearray[i]->inter[ptr[i]].timestamp,G.edgearray[i]->inter[ptr[i]].quantity);
    }
    //print_heap(heapq, numheapqelem);

    //merge interactions on edges and compute flow
    while (numheapqelem) {
    	//printf("top edge(%d): %d->%d, (%f,%f)\n",heapq[0].idx,G.edgearray[heapq[0].idx]->src,G.edgearray[heapq[0].idx]->dest,heapq[0].value,G.edgearray[heapq[0].idx]->inter[ptr[heapq[0].idx]].quantity);
        flow = (buffer[G.edgearray[heapq[0].idx]->src] < G.edgearray[heapq[0].idx]->inter[ptr[heapq[0].idx]].quantity) ? buffer[G.edgearray[heapq[0].idx]->src] : G.edgearray[heapq[0].idx]->inter[ptr[heapq[0].idx]].quantity ;
        //printf("flow=%f\n",flow);
        buffer[G.edgearray[heapq[0].idx]->src] -= flow;
        buffer[G.edgearray[heapq[0].idx]->dest] += flow;
        ptr[heapq[0].idx]++;
        if (ptr[heapq[0].idx]<G.edgearray[heapq[0].idx]->numinter) {
        	heapq[0].value = G.edgearray[heapq[0].idx]->inter[ptr[heapq[0].idx]].timestamp;
        	movedown(heapq,&numheapqelem);
        }
        else
        	dequeue(&tempE, heapq, &numheapqelem);
    }

	/*	
    //merge interactions on edges and compute flow
    while ((minedge = find_minedge(G.edgearray, G.numedges, ptr)) != -1) {
        //printf("minedge:%d,src:%d,dest:%d,ptr:%d,time:%f,qty:%f\n", minedge, G.edgearray[minedge]->src,  G.edgearray[minedge]->dest, ptr[minedge], G.edgearray[minedge]->inter[ptr[minedge]].timestamp, G.edgearray[minedge]->inter[ptr[minedge]].quantity);
        flow = (buffer[G.edgearray[minedge]->src] < G.edgearray[minedge]->inter[ptr[minedge]].quantity) ? buffer[G.edgearray[minedge]->src] : G.edgearray[minedge]->inter[ptr[minedge]].quantity ;
        //printf("flow=%f\n",flow);
        buffer[G.edgearray[minedge]->src] -= flow;
        buffer[G.edgearray[minedge]->dest] += flow;
        ptr[minedge]++;
    }
    */
    
    flow = buffer[G.numnodes-1];
    //printf("flow at node %d is %f\n",G.numnodes-1, flow);

	free(buffer);
	free(ptr);
	free(heapq);
	
    return flow;
}

// same as computeFlowGreedy BUT
// records all incoming interactions to sink, which accumulate to total flow
// into variable **inter; *numinter will hold the number of interactions 
double computeFlowGreedyWithInterOld(struct DAG G, struct Interaction **inter, int *numinter)
{
    int i,j,k;
    double *buffer; // array of buffers, one for each node of the DAG
    int *ptr; // array with pointers to the current interaction per edge
    int minedge;
    double flow;
    int totinter = 0; // for mem allocation only
    
    //initialize returned data
    *numinter = 0; 
    int sink = G.numnodes-1; //id of sink
    for (i=0; i<G.node[sink].numinc; i++)
    	totinter += G.edgearray[G.node[sink].incedges[i]]->numinter;
    //printf("total no of interactions into sink=%d\n",totinter);
    *inter = (struct Interaction *)malloc(totinter*sizeof(struct Interaction));
    
    buffer = (double *)malloc(G.numnodes*sizeof(double));
    for(i=1;i<G.numnodes;i++) {
        buffer[i]=0.0; // initially, all buffers, except the buffer of the source are 0
    }
    buffer[0]=MAXFLOW; // a very big number, make sure MAXFLOW is larger than the sum of all flows on all interactions in the DAG

    ptr = (int *)malloc((G.numedges)*sizeof(int)); // it could be faster if we used calloc 
    for(i=0;i<G.numedges;i++) {
        ptr[i]=0;
    }

    //merge interactions on edges and compute flow
    while ((minedge = find_minedge(G.edgearray, G.numedges, ptr)) != -1) {
        //printf("minedge:%d,src:%d,dest:%d,ptr:%d,time:%f,qty:%f\n", minedge, G.edgearray[minedge]->src,  G.edgearray[minedge]->dest, ptr[minedge], G.edgearray[minedge]->inter[ptr[minedge]].timestamp, G.edgearray[minedge]->inter[ptr[minedge]].quantity);
        flow = (buffer[G.edgearray[minedge]->src] < G.edgearray[minedge]->inter[ptr[minedge]].quantity) ? buffer[G.edgearray[minedge]->src] : G.edgearray[minedge]->inter[ptr[minedge]].quantity ;
        //printf("flow=%f\n",flow);
        buffer[G.edgearray[minedge]->src] -= flow;
        buffer[G.edgearray[minedge]->dest] += flow;
        if (G.edgearray[minedge]->dest==sink && flow>0) { // record non-zero-flow incoming interactions to sink
        	(*inter)[(*numinter)].timestamp =  G.edgearray[minedge]->inter[ptr[minedge]].timestamp;
        	(*inter)[(*numinter)++].quantity =  flow;
        }
        ptr[minedge]++;
    }
    
    flow = buffer[G.numnodes-1];
    //printf("flow at sink node %d is %f, based on interactions:\n",G.numnodes-1, buffer[G.numnodes-1]);
    //for (i=0; i<*numinter; i++)
    //	printf("%f %f\n",(*inter)[i].timestamp, (*inter)[i].quantity);

	free(buffer);
	free(ptr);
	
    return flow;
}

// same as computeFlowGreedy BUT
// records all incoming interactions to sink, which accumulate to total flow
// into variable **inter; *numinter will hold the number of interactions 
double computeFlowGreedyWithInter(struct DAG G, struct Interaction **inter, int *numinter)
{
    int i,j,k;
    double *buffer; // array of buffers, one for each node of the DAG
    int *ptr; // array with pointers to the current interaction per edge
    int minedge;
    double flow;
    int totinter = 0; // for mem allocation only
    elem *heapq = (elem *)malloc((G.numedges)*sizeof(elem)); //array of heap elements for merging edges
    elem tempE; // dummy element, used for dequeueing
    int numheapqelem = 0; //number of elements in heapq
    
    //initialize returned data
    *numinter = 0; 
    int sink = G.numnodes-1; //id of sink
    for (i=0; i<G.node[sink].numinc; i++)
    	totinter += G.edgearray[G.node[sink].incedges[i]]->numinter;
    //printf("total no of interactions into sink=%d\n",totinter);
    *inter = (struct Interaction *)malloc(totinter*sizeof(struct Interaction));
    
    buffer = (double *)malloc(G.numnodes*sizeof(double));
    for(i=1;i<G.numnodes;i++) {
        buffer[i]=0.0; // initially, all buffers, except the buffer of the source are 0
    }
    buffer[0]=MAXFLOW; // a very big number, make sure MAXFLOW is larger than the sum of all flows on all interactions in the DAG

    ptr = (int *)malloc((G.numedges)*sizeof(int)); // it could be faster if we used calloc 
    for(i=0;i<G.numedges;i++) {
        ptr[i]=0;
        enqueue(G.edgearray[i]->inter[ptr[i]].timestamp, i, heapq, &numheapqelem);
    }
    
	//merge interactions on edges and compute flow
    while (numheapqelem) {
        //printf("minedge:%d,src:%d,dest:%d,ptr:%d,time:%f,qty:%f\n", minedge, G.edgearray[minedge]->src,  G.edgearray[minedge]->dest, ptr[minedge], G.edgearray[minedge]->inter[ptr[minedge]].timestamp, G.edgearray[minedge]->inter[ptr[minedge]].quantity);
        flow = (buffer[G.edgearray[heapq[0].idx]->src] < G.edgearray[heapq[0].idx]->inter[ptr[heapq[0].idx]].quantity) ? buffer[G.edgearray[heapq[0].idx]->src] : G.edgearray[heapq[0].idx]->inter[ptr[heapq[0].idx]].quantity ;
        //printf("flow=%f\n",flow);
        buffer[G.edgearray[heapq[0].idx]->src] -= flow;
        buffer[G.edgearray[heapq[0].idx]->dest] += flow;
        if (G.edgearray[heapq[0].idx]->dest==sink && flow>0) { // record non-zero-flow incoming interactions to sink
        	(*inter)[(*numinter)].timestamp =  heapq[0].value; //timestamp of top element
        	(*inter)[(*numinter)++].quantity =  flow;
        }
        ptr[heapq[0].idx]++;
        if (ptr[heapq[0].idx]<G.edgearray[heapq[0].idx]->numinter) {
        	heapq[0].value = G.edgearray[heapq[0].idx]->inter[ptr[heapq[0].idx]].timestamp;
        	movedown(heapq,&numheapqelem);
        }
        else
        	dequeue(&tempE, heapq, &numheapqelem);
    }
        
/*
    //merge interactions on edges and compute flow
    while ((minedge = find_minedge(G.edgearray, G.numedges, ptr)) != -1) {
        //printf("minedge:%d,src:%d,dest:%d,ptr:%d,time:%f,qty:%f\n", minedge, G.edgearray[minedge]->src,  G.edgearray[minedge]->dest, ptr[minedge], G.edgearray[minedge]->inter[ptr[minedge]].timestamp, G.edgearray[minedge]->inter[ptr[minedge]].quantity);
        flow = (buffer[G.edgearray[minedge]->src] < G.edgearray[minedge]->inter[ptr[minedge]].quantity) ? buffer[G.edgearray[minedge]->src] : G.edgearray[minedge]->inter[ptr[minedge]].quantity ;
        //printf("flow=%f\n",flow);
        buffer[G.edgearray[minedge]->src] -= flow;
        buffer[G.edgearray[minedge]->dest] += flow;
        if (G.edgearray[minedge]->dest==sink && flow>0) { // record non-zero-flow incoming interactions to sink
        	(*inter)[(*numinter)].timestamp =  G.edgearray[minedge]->inter[ptr[minedge]].timestamp;
        	(*inter)[(*numinter)++].quantity =  flow;
        }
        ptr[minedge]++;
    }
*/
    
    flow = buffer[G.numnodes-1];
    //printf("flow at sink node %d is %f, based on interactions:\n",G.numnodes-1, buffer[G.numnodes-1]);
    //for (i=0; i<*numinter; i++)
    //	printf("%f %f\n",(*inter)[i].timestamp, (*inter)[i].quantity);

	free(buffer);
	free(ptr);
	free(heapq);
	
    return flow;
}


// computes the flow throughout a DAG from its source (node at position 0) 
// to its sink (node at position G.numnodes-1)
// converts problem to LP computation problem
// each interaction (except those on outgoing edges from the source) is a variable
// see the paper for details on the formulation
double computeFlowLP(struct DAG G)
{
    int i,j,k;
    double flow;

    lprec *lp;
    int *colno = NULL,ret = 0;
    REAL *row = NULL;
    int Ncol=0; /* total number of variables, excludes those from source*/
    int totinter=0; /* total number of interactions, including those from source*/
    int curinter=0;
    int l;
    struct CompleteInteraction *inters; /* complete list of interactions in DAG */
    
    int *numincinters; // number of incoming interactions per node
    int **incinters; // list of indexes to inters for each node (incoming interactions)
    int *numoutinters; // number of incoming interactions per node
    int **outinters; // list of indexes to inters for each node (incoming interactions)
    
    int *map; // map[i] = variable-id corresponding to interaction i
    
    clock_t t;
    double time_taken;

    numincinters = (int *)malloc(G.numnodes*sizeof(int));
    incinters = (int **)malloc(G.numnodes*sizeof(int *));
    numoutinters = (int *)malloc(G.numnodes*sizeof(int));
    outinters = (int **)malloc(G.numnodes*sizeof(int *));
    for (i=0; i<G.numnodes;i++) {
        numincinters[i]=0;
        numoutinters[i]=0;
    }
    
    /* count number of interactions and variables */
    for (i=0; i<G.numedges;i++) {
        //printf("%d %d\n",G.edgearray[i]->src,G.edgearray[i]->dest);
        totinter += G.edgearray[i]->numinter;
        numincinters[G.edgearray[i]->dest]+=G.edgearray[i]->numinter;
        numoutinters[G.edgearray[i]->src]+=G.edgearray[i]->numinter;
    }
    for (i=0; i<G.numnodes;i++) {
        incinters[i]=(int *)malloc(numincinters[i]*sizeof(int) );
        numincinters[i] = 0;
        outinters[i]=(int *)malloc(numoutinters[i]*sizeof(int) );
        numoutinters[i] = 0;
    }
    
    inters = (struct CompleteInteraction *)malloc(totinter*sizeof(struct CompleteInteraction));
    map = (int  *)malloc(totinter*sizeof(int));

    int n=0;
    for (i=0; i<G.numedges;i++)
        for (j=0; j<G.edgearray[i]->numinter; j++) {
            incinters[G.edgearray[i]->dest][numincinters[G.edgearray[i]->dest]++] = n;
            outinters[G.edgearray[i]->src][numoutinters[G.edgearray[i]->src]++] = n;
            inters[n].src = G.edgearray[i]->src;
            if (G.edgearray[i]->src==0)
                map[n] = -1;
            else
                map[n] = Ncol++;
            
            inters[n].dest = G.edgearray[i]->dest;
            inters[n].timestamp = G.edgearray[i]->inter[j].timestamp;
            inters[n++].quantity = G.edgearray[i]->inter[j].quantity;
        }
    
    //printf("Ncol=%d\n",Ncol);
    
    /*
    //print interactions
    for (i=0;i<totinter;i++) {
        printf("%d %d %f %f\n",inters[i].src,inters[i].dest,inters[i].timestamp,inters[i].quantity);
    }

    //print map
    for (i=0;i<totinter;i++) {
        printf("%d %d\n",i, map[i]);
    }
    
    printf("number of variables=%d\n",Ncol);

    //print interactions per node
    for (i=0;i<G.numnodes;i++) {
        printf("node %d incoming interactions:\n",i);
        for (j=0;j<numincinters[i];j++)
            printf("%d %d %f %f\n",inters[incinters[i][j]].src,inters[incinters[i][j]].dest,inters[incinters[i][j]].timestamp,inters[incinters[i][j]].quantity);
        printf("node %d outgoing interactions:\n",i);
        for (j=0;j<numoutinters[i];j++)
            printf("%d %d %f %f\n",inters[outinters[i][j]].src,inters[outinters[i][j]].dest,inters[outinters[i][j]].timestamp,inters[outinters[i][j]].quantity);

    }
    */
  
    
    /* We will build the model row by row */
    lp = make_lp(0, Ncol);
    
    
    if(lp == NULL)
      ret = 1; /* couldn't construct a new model... */

    if(ret == 0) {
      /* let us name our variables. Not required, but can be useful for debugging */
        for (i=0;i<Ncol;i++) {
        	char snum[10];
        	sprintf(snum, "x%d", i+1);
            set_col_name(lp, i+1, snum);
        }
      /* create space large enough for one row */
      colno = (int *) malloc(Ncol * sizeof(*colno));
      row = (REAL *) malloc(Ncol * sizeof(*row));
      if((colno == NULL) || (row == NULL))
        ret = 2;
    }
    

    if(ret == 0) {
        set_add_rowmode(lp, TRUE);  /* makes building the model faster if it is done rows by row */
            
//        curinter =0; //current interaction
        for (i=0; i<totinter; i++) {
            if (inters[i].src != 0) {
                //printf("%d\n",i);
                /* two constraints per interaction*/
                
                /* first constraint is upper bound based on flow on interaction*/
                j = 0;
                
                for(l=0; l<map[i]; l++) {
                    colno[j] = l+1; /* first column */
                    row[j++] = 0; /* coefficient */
                }

                colno[j] = map[i]+1; /* second column */
                row[j++] = 1; /* coefficient */
                l++;

                for(; l<Ncol; l++) {
                    colno[j] = l+1; /* first column */
                    row[j++] = 0; /* coefficient */
                }
                
                /*
                printf("basic constraint:\n");
                if (j!=Ncol)
                    printf("problem\n");
                else
                    for (int s=0; s<Ncol; s++)
                        printf("colno:%d row:%f\n",colno[s],row[s]);
                printf("less than:%f\n",inters[i].quantity);
                */
                
                /* add the row to lpsolve */
                if(!add_constraintex(lp, Ncol, row, colno, LE, inters[i].quantity))
                  ret = 3;

                //printf("ok\n");
                /* second constraint is based on feasible flow transfer*/
                //initialize all
                for(j=0; j<Ncol; j++) {
                    colno[j] = j+1;
                    row[j] = 0;
                }
            
                //set variable corresponding to interaction
                colno[map[i]] = map[i]+1;
                row[map[i]] = 1;

                //consider other interactions
                double fromsource=0;
                for (int s=0; s<totinter; s++) {
                    if (s==i) continue;
                    if (inters[s].src==0 && inters[s].dest==inters[i].src && inters[s].timestamp<inters[i].timestamp)
                        fromsource+=inters[s].quantity;
                    else { // check incoming/outgoing interactions to/from src
                        if (inters[s].src!=0 && inters[s].dest==inters[i].src && inters[s].timestamp<inters[i].timestamp) {
                            colno[map[s]]=map[s]+1;
                            row[map[s]] = -1;
                        }
                        else if (inters[s].src==inters[i].src && inters[s].timestamp<inters[i].timestamp) {
                            colno[map[s]]=map[s]+1;
                            row[map[s]] = 1;
                        }
                    }
                }
                
                /*
                printf("other constraint:\n");
                for (int s=0; s<Ncol; s++)
                        printf("colno:%d row:%f\n",colno[s],row[s]);
                printf("less than:%f\n",fromsource);
                */
                
                /* add the row to lpsolve */
                if(!add_constraintex(lp, j, row, colno, LE, fromsource))
                  ret = 3;

            }
        }
    }
    
 
    if(ret == 0) {
		set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */

		/* set the objective function */
		
		//initalization
		for (i=0; i<Ncol; i++) {
			colno[i] = i+1; 
			row[i] = 0;
		}

		//now set variables corresponding to interactions that have as destination the sink node
		for (i=0; i<totinter; i++)
            if (inters[i].src != 0 && inters[i].dest == G.numnodes-1)
            	row[map[i]]=1;
            	
        /*
        printf("objective function for maximization:\n");
		for (int i=0; i<Ncol; i++)
				printf("colno:%d row:%f\n",colno[i],row[i]);
		*/
		
		/* set the objective in lpsolve */
		if(!set_obj_fnex(lp, Ncol, row, colno))
		  ret = 4;
  	}
  

    if(ret == 0) {
      /* set the object direction to maximize */
      set_maxim(lp);

      /* just out of curioucity, now show the model in lp format on screen */
      /* this only works if this is a console application. If not, use write_lp and a filename */
      //write_LP(lp, stdout);
      /* write_lp(lp, "model.lp"); */

      /* I only want to see important messages on screen while solving */
      set_verbose(lp, IMPORTANT);

      /* Now let lpsolve calculate a solution */
      //t = clock();    
      ret = solve(lp);
      //t = clock() - t;
      //time_taken = ((double)t)/CLOCKS_PER_SEC;
      //printf("Total time of lpsolve: %f seconds\n", time_taken);
      
      if(ret == OPTIMAL)
        ret = 0;
      else
        ret = 5;
    }
        
  
    if(ret == 0) {
       /* a solution is calculated, now lets get some results */

       /* objective value */
       //printf("Objective value: %f\n", get_objective(lp));

       /* variable values */
       //get_variables(lp, row);
       
       //for(j = 0; j < Ncol; j++)
       //  printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);

       /* we are done now */
       
       // total flow is objective value + total incoming flow directly from source to sink
		double directflow =0;
		//now set variables corresponding to interactions that have as destination the sink node
		for (i=0; i<totinter; i++)
			if (inters[i].src == 0 && inters[i].dest == G.numnodes-1)
				directflow+=inters[i].quantity;
		flow = get_objective(lp)+directflow;
		//printf("Total flow: %f\n", get_objective(lp)+directflow);
     }
     
     

     /* free allocated memory */
     if(row != NULL)
       free(row);
     if(colno != NULL)
       free(colno);

     if(lp != NULL) {
       /* clean up such that all used memory by lpsolve is freed */
       delete_lp(lp);
     }
     
     
     for (i=0; i<G.numnodes;i++) {
        free(incinters[i]);
        free(outinters[i]);
     }
     free(numincinters);
     free(incinters);
     free(numoutinters);
     free(outinters);
	 free(inters);
	 free(map);
	 
     return(flow);
    
}

//compare two interactions by time; used by qsort call in function computeFlowLPWithInter
int compInter(const void *a, const void *b) {
	if (((struct Interaction *)a)->timestamp > ((struct Interaction *)b)->timestamp)
    	return 1;
  	else if (((struct Interaction *)a)->timestamp < ((struct Interaction *)b)->timestamp)
    	return -1;
  	else
    	return 0; 
}

// same as computeFlowLP
// returns incoming interactions to sink, which accumulate to total flow
double computeFlowLPWithInter(struct DAG G, struct Interaction **inter, int *numinter)
{
    int i,j,k;
    double flow;

    lprec *lp;
    int *colno = NULL,ret = 0;
    REAL *row = NULL;
    int Ncol=0; /* total number of variables, excludes those from source*/
    int totinter=0; /* total number of interactions, including those from source*/
    int curinter=0;
    int l;
    struct CompleteInteraction *inters; /* complete list of interactions in DAG */
    
    int *numincinters; // number of incoming interactions per node
    int **incinters; // list of indexes to inters for each node (incoming interactions)
    int *numoutinters; // number of incoming interactions per node
    int **outinters; // list of indexes to inters for each node (incoming interactions)
    
    int *map; // map[i] = variable-id corresponding to interaction i
    int *revmap; // revmap[i] = interaction-id corresponding to variable i
    
    clock_t t;
    double time_taken;
    
    //double *timestamps; //maps variables to timestamps of corresponding interactions

    //initialize returned data
    int ti = 0; // for mem allocation only
    *numinter = 0; 
    int sink = G.numnodes-1; //id of sink
    for (i=0; i<G.node[sink].numinc; i++)
    	ti += G.edgearray[G.node[sink].incedges[i]]->numinter;
    //printf("total no of interactions into sink=%d\n",ti);
    *inter = (struct Interaction *)malloc(ti*sizeof(struct Interaction));
    
    numincinters = (int *)malloc(G.numnodes*sizeof(int));
    incinters = (int **)malloc(G.numnodes*sizeof(int *));
    numoutinters = (int *)malloc(G.numnodes*sizeof(int));
    outinters = (int **)malloc(G.numnodes*sizeof(int *));
    for (i=0; i<G.numnodes;i++) {
        numincinters[i]=0;
        numoutinters[i]=0;
    }
    
    /* count number of interactions and variables */
    for (i=0; i<G.numedges;i++) {
        //printf("%d %d\n",G.edgearray[i]->src,G.edgearray[i]->dest);
        totinter += G.edgearray[i]->numinter;
        numincinters[G.edgearray[i]->dest]+=G.edgearray[i]->numinter;
        numoutinters[G.edgearray[i]->src]+=G.edgearray[i]->numinter;
        //if (G.edgearray[i]->src != 0)
        //    Ncol += G.edgearray[i]->numinter;
    }
    for (i=0; i<G.numnodes;i++) {
        incinters[i]=(int *)malloc(numincinters[i]*sizeof(int) );
        numincinters[i] = 0;
        outinters[i]=(int *)malloc(numoutinters[i]*sizeof(int) );
        numoutinters[i] = 0;
    }
    
    inters = (struct CompleteInteraction *)malloc(totinter*sizeof(struct CompleteInteraction));
    map = (int  *)malloc(totinter*sizeof(int)); //map interaction-id to variable-id (from 0)
    revmap = (int  *)malloc(totinter*sizeof(int)); //map variable-id to interaction-id

    int n=0;
    for (i=0; i<G.numedges;i++)
        for (j=0; j<G.edgearray[i]->numinter; j++) {
            incinters[G.edgearray[i]->dest][numincinters[G.edgearray[i]->dest]++] = n;
            outinters[G.edgearray[i]->src][numoutinters[G.edgearray[i]->src]++] = n;
            inters[n].src = G.edgearray[i]->src;
            if (G.edgearray[i]->src==0)
                map[n] = -1;
            else {
                map[n] = Ncol;
                revmap[Ncol] = n;
                Ncol++;
            }
            
            inters[n].dest = G.edgearray[i]->dest;
            inters[n].timestamp = G.edgearray[i]->inter[j].timestamp;
            inters[n++].quantity = G.edgearray[i]->inter[j].quantity;
        }
    
    /*
    //print interactions
    for (i=0;i<totinter;i++) {
        printf("%d %d %f %f\n",inters[i].src,inters[i].dest,inters[i].timestamp,inters[i].quantity);
    }

    //print map
    for (i=0;i<totinter;i++) {
        printf("%d %d\n",i, map[i]);
    }
    
    printf("number of variables=%d\n",Ncol);

    //print interactions per node
    for (i=0;i<G.numnodes;i++) {
        printf("node %d incoming interactions:\n",i);
        for (j=0;j<numincinters[i];j++)
            printf("%d %d %f %f\n",inters[incinters[i][j]].src,inters[incinters[i][j]].dest,inters[incinters[i][j]].timestamp,inters[incinters[i][j]].quantity);
        printf("node %d outgoing interactions:\n",i);
        for (j=0;j<numoutinters[i];j++)
            printf("%d %d %f %f\n",inters[outinters[i][j]].src,inters[outinters[i][j]].dest,inters[outinters[i][j]].timestamp,inters[outinters[i][j]].quantity);

    }
    */
    
    /* We will build the model row by row */
    lp = make_lp(0, Ncol);
    
    if(lp == NULL)
      ret = 1; /* couldn't construct a new model... */

    if(ret == 0) {
      /* let us name our variables. Not required, but can be useful for debugging */
        for (i=0;i<Ncol;i++) {
        	char snum[10];
        	sprintf(snum, "x%d", i+1);
            set_col_name(lp, i+1, snum);
        }
      /* create space large enough for one row */
      colno = (int *) malloc(Ncol * sizeof(*colno));
      row = (REAL *) malloc(Ncol * sizeof(*row));
      if((colno == NULL) || (row == NULL))
        ret = 2;
    }

    if(ret == 0) {
        set_add_rowmode(lp, TRUE);  /* makes building the model faster if it is done rows by row */
            
        for (i=0; i<totinter; i++) {
            if (inters[i].src != 0) {
                //printf("%d\n",i);
                /* two constraints per interaction*/
                
                /* first constraint is upper bound based on flow on interaction*/
                j = 0;
                
                for(l=0; l<map[i]; l++) {
                    colno[j] = l+1; /* first column */
                    row[j++] = 0; /* coefficient */
                }

                colno[j] = map[i]+1; /* second column */
                row[j++] = 1; /* coefficient */
                l++;

                for(; l<Ncol; l++) {
                    colno[j] = l+1; /* first column */
                    row[j++] = 0; /* coefficient */
                }
                
                /*
                printf("basic constraint:\n");
                if (j!=Ncol)
                    printf("problem\n");
                else
                    for (int s=0; s<Ncol; s++)
                        printf("colno:%d row:%f\n",colno[s],row[s]);
                printf("less than:%f\n",inters[i].quantity);
                */
                
                /* add the row to lpsolve */
                if(!add_constraintex(lp, Ncol, row, colno, LE, inters[i].quantity))
                  ret = 3;

                //printf("ok\n");
                /* second constraint is based on feasible flow transfer*/
                //initialize all
                for(j=0; j<Ncol; j++) {
                    colno[j] = j+1;
                    row[j] = 0;
                }
            
                //set variable corresponding to interaction
                colno[map[i]] = map[i]+1;
                row[map[i]] = 1;

                //consider other interactions
                double fromsource=0;
                for (int s=0; s<totinter; s++) {
                    if (s==i) continue;
                    if (inters[s].src==0 && inters[s].dest==inters[i].src && inters[s].timestamp<inters[i].timestamp)
                        fromsource+=inters[s].quantity;
                    else { // check incoming/outgoing interactions to/from src
                        if (inters[s].src!=0 && inters[s].dest==inters[i].src && inters[s].timestamp<inters[i].timestamp) {
                            colno[map[s]]=map[s]+1;
                            row[map[s]] = -1;
                        }
                        else if (inters[s].src==inters[i].src && inters[s].timestamp<inters[i].timestamp) {
                            colno[map[s]]=map[s]+1;
                            row[map[s]] = 1;
                        }
                    }
                }
                
                /*
                printf("other constraint:\n");
                for (int s=0; s<Ncol; s++)
                        printf("colno:%d row:%f\n",colno[s],row[s]);
                printf("less than:%f\n",fromsource);
                */
                
                /* add the row to lpsolve */
                if(!add_constraintex(lp, j, row, colno, LE, fromsource))
                  ret = 3;

            }
        }
    }
    
 
    if(ret == 0) {
		set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */

		/* set the objective function */
		
		//initalization
		for (i=0; i<Ncol; i++) {
			colno[i] = i+1; 
			row[i] = 0;
		}

		//now set variables corresponding to interactions that have as destination the sink node
		for (i=0; i<totinter; i++)
            if (inters[i].src != 0 && inters[i].dest == G.numnodes-1)
            	row[map[i]]=1;
            	
        /*
        printf("objective function for maximization:\n");
		for (int i=0; i<Ncol; i++)
				printf("colno:%d row:%f\n",colno[i],row[i]);
		*/
		
		/* set the objective in lpsolve */
		if(!set_obj_fnex(lp, Ncol, row, colno))
		  ret = 4;
  	}
  

    if(ret == 0) {
      /* set the object direction to maximize */
      set_maxim(lp);

      /* just out of curioucity, now show the model in lp format on screen */
      /* this only works if this is a console application. If not, use write_lp and a filename */
      //write_LP(lp, stdout);
      /* write_lp(lp, "model.lp"); */

      /* I only want to see important messages on screen while solving */
      set_verbose(lp, IMPORTANT);
  
      /* Now let lpsolve calculate a solution */
      //t = clock();   
      //printf("solving LP now\n"); 
      ret = solve(lp);
      //printf("ret=%d\n",ret); 
      //t = clock() - t;
      //time_taken = ((double)t)/CLOCKS_PER_SEC;
      //printf("Total time of lpsolve: %f seconds\n", time_taken);
      
      if(ret == OPTIMAL)
        ret = 0;
      else
        ret = 5;
    }
        
  
    if(ret == 0) {
       /* a solution is calculated, now lets get some results */

       /* objective value */
       //printf("Objective value: %f\n", get_objective(lp));

       /* variable values */
       get_variables(lp, row);
       
       for(j = 0; j < Ncol; j++) {
         //printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);
         if(inters[revmap[j]].dest == G.numnodes-1 && row[j]>0) {
         	(*inter)[(*numinter)].timestamp =  inters[revmap[j]].timestamp;
         	(*inter)[(*numinter)++].quantity =  row[j];
         }
       }

       //for(j = 0; j < Ncol; j++)
       //  printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);

       /* we are done now */
       
       // total flow is objective value + total incoming flow directly from source to sink
		double directflow =0;
		//now set variables corresponding to interactions that have as destination the sink node
		for (i=0; i<totinter; i++)
			if (inters[i].src == 0 && inters[i].dest == G.numnodes-1) {
				(*inter)[(*numinter)].timestamp =  inters[i].timestamp;
				(*inter)[(*numinter)++].quantity =  inters[i].quantity;
				directflow+=inters[i].quantity;
			}
		qsort((*inter), *numinter, sizeof(struct Interaction), compInter);
		flow = get_objective(lp)+directflow;
		//printf("Total flow: %f, based on interactions:\n", get_objective(lp)+directflow);
	    //for (i=0; i<*numinter; i++)
    	//	printf("%f %f\n",(*inter)[i].timestamp, (*inter)[i].quantity);
     }
     
     /* free allocated memory */
     if(row != NULL)
       free(row);
     if(colno != NULL)
       free(colno);

     if(lp != NULL) {
       /* clean up such that all used memory by lpsolve is freed */
       delete_lp(lp);
     }
     
     
     for (i=0; i<G.numnodes;i++) {
        free(incinters[i]);
        free(outinters[i]);
     }
     free(numincinters);
     free(incinters);
     free(numoutinters);
     free(outinters);
	 free(inters);
	 free(map);
	 free(revmap);
	 
     return(flow);
}

/*
// used by findPaths2 function below to discover paths from a given source to a given sink
// in order to construct a DAG
// WARNING: certain edges are disqualified (if they are susceptible to close cycles)
void expandpath_findpaths2(struct Graph G,  struct CPattern path, int i, int destnode, int len, int maxlen, struct Edge** curedges,  struct Edge** edgearray, int *numedges, int *totinter) 
{
	int j,k;
	int endlabel;
	
	int pathfound = 0; // flags that valid path is found
	int loops = 0; // flags that a loop is found
	int chain = 0; // flags a loop-less chain, e.g. x->y->z->w
	int valid = 1; // used to detect an invalid path, e.g. x->y->z->y
	
	if (len>1) { // only paths of at least two vertices are interesting
		endlabel=path.labels[len-1];
		if (G.node[destnode].label == endlabel) {// found valid path
			//printf("path found!\n");
			//printpath(path,len);
			pathfound = 1; // path found!
			if  ((*numedges)+len <MAXEDGES) { //ignore paths exceeding total edges limit
				for (k=0;k<len-1;k++) { //for each edge in curedges (current path)
					// check if edge is already in edgearray (suboptimal way)
					for (j=0;j<*numedges;j++)
						// avoid same edge again or loop edge (unless it is last edge on path)		
						if (edgearray[j]==curedges[k] ||(k<len-2 && curedges[k]->src==edgearray[j]->dest && curedges[k]->dest==edgearray[j]->src && path.labels[0]!=curedges[k]->src)) //this is better
		//				if (edgearray[j]==curedges[k] ||(k<len-2 && curedges[k]->src==edgearray[j]->dest && curedges[k]->dest==edgearray[j]->src))
						//					if (edgearray[j]==curedges[k]) 
							break;
					if (j==*numedges) {// not broken
						//printf("new edge: %d->%d\n",curedges[k]->src,curedges[k]->dest);
						edgearray[(*numedges)++]=curedges[k];
						(*totinter)+=curedges[k]->numinter;
					}
				}
			}
			else return;
		}
		else if (path.labels[0] == endlabel) {// loop condition
			loops = 1; // loop found!
		}
	}
					
	if (len == 1 || (len<maxlen && !pathfound && !loops)){ // expand single vertices or chain paths with less than maxlen nodes
		for(j=0; j<G.node[i].numout; j++) {
			//printf("going to %d\n",G.node[i].edge[j].dest);
			path.labels[len] = G.node[i].edge[j].dest;
			curedges[len-1] = &G.node[i].edge[j];
			expandpath_findpaths2(G,path,G.node[i].edge[j].dest,destnode,len+1,maxlen, curedges, edgearray, numedges, totinter);
		}
	}
}
*/

//returns 1 if dest is reachable from src in edgearray
//slow and suboptimal, but it works (normally we need data structures)
int reachable(int dest, int src, int destnode, struct Edge** edgearray, int numedges, struct Edge** curedges, int len)
{
	int found = 0;
	//if(dest==2 && src==1) printf("here\n");
	for (int i=0; i<numedges; i++) {
		if (edgearray[i]->src == src)
		{
			if (edgearray[i]->dest == dest) // found
				return 1;
			if (edgearray[i]->dest != destnode) {
				found = reachable(dest, edgearray[i]->dest, destnode, edgearray, numedges, curedges, len);
				if (found) return 1;
			}
		}
	}
	// examine also current edges array
	for (int i=0; i<len-2; i++) {
		if (curedges[i]->src == src)
		{
			if (curedges[i]->dest == dest) // found
				return 1;
			if (curedges[i]->dest != destnode) {
				found = reachable(dest, curedges[i]->dest, destnode, edgearray, numedges, curedges, len);
				if (found) return 1;
			}
		}
	}
	return found;
}

//returns 1 if dest is reachable from src in edgearray
//slow and suboptimal, but it works (normally we need data structures)
//trying to make it faster by avoiding redundant recursions
int reachable2(int dest, int src, int destnode, struct Edge** edgearray, int numedges, struct Edge** curedges, int len, int *examinednodes, int *numexaminednodes)
{
	int i,j;
	int found = 0;
	//if(dest==55) printf("here\n");
	for (i=0; i<numedges; i++) {
		if (edgearray[i]->src == src)
		{
			//if(dest==55) printf("here2\n");
			if (edgearray[i]->dest == dest) // found
				return 1;
			if (edgearray[i]->dest != destnode) {
				for(j=0; j<*numexaminednodes; j++)
					if (edgearray[i]->dest==examinednodes[j])
						break;
				//if (dest==55) printf("here3:nextsrc=%d,dest=%d,j=%d,*numexaminednodes=%d\n",edgearray[i]->dest,dest,j,*numexaminednodes);
				if (j==*numexaminednodes) //not examined before
					found = reachable2(dest, edgearray[i]->dest, destnode, edgearray, numedges, curedges, len, examinednodes, numexaminednodes);
				if (found) return 1;
				//if (dest==55) printf("here4\n");
				examinednodes[(*numexaminednodes)++] = edgearray[i]->dest; //mark as examined
				//if (dest==55) printf("%d\n",edgearray[i]->dest);
			}
		}
	}
	//if(dest==55) printf("here-cur\n");
	if (found) return 1;
	// examine also current edges array
	for (i=0; i<len-2; i++) {
		if (curedges[i]->src == src)
		{
			if (curedges[i]->dest == dest) // found
				return 1;
			if (curedges[i]->dest != destnode) {
				for(j=0; j<*numexaminednodes; j++)
					if (curedges[i]->dest==examinednodes[j])
						break;
				if (j==*numexaminednodes) //not examined before
					found = reachable2(dest, curedges[i]->dest, destnode, edgearray, numedges, curedges, len,  examinednodes, numexaminednodes);
				if (found) return 1;
				examinednodes[(*numexaminednodes)++] = edgearray[i]->dest; //mark as examined
				//if (dest==55) printf("%d\n",edgearray[i]->dest);
			}
		}
	}
	return found;
}

// used by findPaths2 function below to discover paths from a given source to a given sink
// in order to construct a DAG
// WARNING: certain edges are disqualified (if they are susceptible to close cycles)
void expandpath_findpaths2(struct Graph G,  struct CPattern path, int i, int destnode, int len, int maxlen, struct Edge** curedges,  struct Edge** edgearray, int *numedges, int *totinter) 
{
	int j,k;
	int endlabel;
	
	int pathfound = 0; // flags that valid path is found
	int loops = 0; // flags that a loop is found
	int chain = 0; // flags a loop-less chain, e.g. x->y->z->w
	int valid = 1; // used to detect an invalid path, e.g. x->y->z->y
	int *examinednodes = (int *)malloc(MAXEDGES*sizeof(int));
	int numexaminednodes;  // used in reachability test for loops
	
	if (len>1) { // only paths of at least two vertices are interesting
		endlabel=path.labels[len-1];
		if (G.node[destnode].label == endlabel) {// found valid path
			//printf("path found!\n");
			//printpath(path,len);
			pathfound = 1; // path found!
			if  ((*numedges)+len <MAXEDGES) { //ignore paths exceeding total edges limit
				for (k=0;k<len-1;k++) { //for each edge in curedges (current path)
					// check if edge is already in edgearray (suboptimal way)
					for (j=0;j<*numedges;j++)
						// avoid same edge again or loop edge (unless it is last edge on path)
						if (edgearray[j]==curedges[k]) 
		//				if (edgearray[j]==curedges[k] ||(k<len-2 && curedges[k]->src==edgearray[j]->dest && curedges[k]->dest==edgearray[j]->src && path.labels[0]!=curedges[k]->src)) //this is better: works for DAGs where src=dest
		//				if (edgearray[j]==curedges[k] ||(k<len-2 && curedges[k]->src==edgearray[j]->dest && curedges[k]->dest==edgearray[j]->src))
						//					if (edgearray[j]==curedges[k]) 
							break;
					if (j==*numedges) {// not broken
						//printf("new edge: %d->%d\n",curedges[k]->src,curedges[k]->dest);
						edgearray[(*numedges)++]=curedges[k];
						(*totinter)+=curedges[k]->numinter;
						//printf("%d: added %d->%d\n",*numedges,curedges[k]->src,curedges[k]->dest);
					}
				}
			}
			else {
				free(examinednodes);
				return;
			}
		}
		else if (path.labels[0] == endlabel) {// loop condition
			loops = 1; // loop found!
		}
		
		else if (len>2)
		{
			// check if current edge closes cycle (together with current paths)
			// in this case path is rejected: we set loops=1
			
			// if curedges[len-2]->src is reachable from curedges[len-2]->dest in edgearray, then current path should be rejected
			
			//if (curedges[len-2]->src== 2 && curedges[len-2]->dest==1) {
			//	printf("%d edges:\n",*numedges);
			//	for(j=0; j<*numedges; j++)
			//		printf("%d->%d\n",edgearray[j]->src,edgearray[j]->dest);
			//	printf("curedges:\n");
			//	for(j=0; j<len-1; j++)
			//		printf("%d->%d\n",curedges[j]->src,curedges[j]->dest);
			//}
			//printf("examining: %d->%d (len=%d, numedges=%d): \n",curedges[len-2]->src,curedges[len-2]->dest,len,*numedges);
			numexaminednodes = 0;
			if (reachable2(curedges[len-2]->src, curedges[len-2]->dest, destnode, edgearray, *numedges, curedges, len, examinednodes, &numexaminednodes))
				loops = 1; //signals invalid path	
				
			//printf("loops=%d\n",loops);			
		}
	}
					
	if (len == 1 || (len<maxlen && !pathfound && !loops)){ // expand single vertices or chain paths with less than maxlen nodes
		for(j=0; j<G.node[i].numout; j++) {
			//printf("going to %d\n",G.node[i].edge[j].dest);
			path.labels[len] = G.node[i].edge[j].dest;
			if (path.labels[len-1]==path.labels[len]) continue; //avoid selfloop expansion (a->a)
			curedges[len-1] = &G.node[i].edge[j];
			expandpath_findpaths2(G,path,G.node[i].edge[j].dest,destnode,len+1,maxlen, curedges, edgearray, numedges, totinter);
		}
	}
	free(examinednodes);
}

// finds all paths from sourcenode to destnode up to a maximum length 
// paths should not contain the same node twice (except if sourcenode=destnode) 
// (this constraint is not yet fully implemented)
// adds all distinct edges in these paths to edgearray
// does not add or continue paths which close cycles based on current set of edges
int findPaths2(struct Graph G, int sourcenode, int destnode, int maxlen, struct Edge** edgearray, int *numedges)
{
	int i,j,k;
	int totinter = 0; //total number of interactions in edges of resulting edgearray
	
	struct Edge **curedges = (struct Edge **)malloc(maxlen*sizeof(struct Edge *)); //holds edges in current path 
	
	(*numedges) = 0; //number of edges in valid paths
	
	//int *visited;
	int len=0;
	struct CPattern path; //holds instance of path
	
	path.labels = (int *)malloc(maxlen*sizeof(int));
	path.numnodes = 1;
	path.labels[0]=G.node[sourcenode].label;
	expandpath_findpaths2(G,path,sourcenode,destnode,len+1,maxlen, curedges, edgearray, numedges, &totinter); 
	
	//for(i=0;i<*numedges;i++)
	//	printf("%d->%d\n",edgearray[i]->src,edgearray[i]->dest);
	
	free(curedges);
	free(path.labels);
	
	return totinter;
}


/*
// used by findPaths2 function below to discover paths from a given source to a given sink
// in order to construct a DAG
// WARNING: certain edges are disqualified (if they are susceptible to close cycles)
void expandpath_findpaths3(struct Graph G,  struct CPattern path, int i, int destnode, int len, int maxlen, struct Edge** curedges,  struct Edge** edgearray, int *numedges, int *totinter, int *nodearray, int *numnodes) 
{
	int j,k,l;
	int endlabel;
	
	int pathfound = 0; // flags that valid path is found
	int loops = 0; // flags that a loop is found
	int chain = 0; // flags a loop-less chain, e.g. x->y->z->w
	int valid = 1; // used to detect an invalid path, e.g. x->y->z->y
	
	if (len>1) { // only paths of at least two vertices are interesting
		endlabel=path.labels[len-1];
		if (G.node[destnode].label == endlabel) {// found valid path
			//printf("path found!\n");
			//printpath(path,len);
			pathfound = 1; // path found!
			if  ((*numedges)+len <MAXEDGES) { //ignore paths exceeding total edges limit
				for (k=0;k<len-1;k++) { //for each edge in curedges (current path)
					// check if edge is already in edgearray (suboptimal way)
					for (j=0;j<*numedges;j++)
						// avoid same edge again or loop edge (unless it is last edge on path)
						if (edgearray[j]==curedges[k]) 
		//				if (edgearray[j]==curedges[k] ||(k<len-2 && curedges[k]->src==edgearray[j]->dest && curedges[k]->dest==edgearray[j]->src && path.labels[0]!=curedges[k]->src)) //this is better: works for DAGs where src=dest
		//				if (edgearray[j]==curedges[k] ||(k<len-2 && curedges[k]->src==edgearray[j]->dest && curedges[k]->dest==edgearray[j]->src))
						//					if (edgearray[j]==curedges[k]) 
							break;
					if (j==*numedges) {// not broken
						//printf("new edge: %d->%d\n",curedges[k]->src,curedges[k]->dest);
						edgearray[(*numedges)++]=curedges[k];
						(*totinter)+=curedges[k]->numinter;
						//printf("%d: added %d->%d\n",*numedges,curedges[k]->src,curedges[k]->dest);
						//add node in nodearray (if not there)
						if (curedges[k]->dest!=endlabel)
						{
							for(l=0;l<*numnodes;l++)
								if (nodearray[l]==curedges[k]->dest)
									break; //found
							if (l == *numnodes) //not broken
								nodearray[(*numnodes)++]= curedges[k]->dest;
						}
					}
				}
			}
			else return;
		}
		else if (path.labels[0] == endlabel) {// loop condition
			loops = 1; // loop found!
		}
		else if (len>2)
		{
			// check if current edge closes cycle (together with current paths)
			// in this case path is rejected: we set loops=1
			
			// if curedges[len-2]->src is reachable from curedges[len-2]->dest in edgearray, then current path should be rejected
			
			//if (curedges[len-2]->src== 2 && curedges[len-2]->dest==1) {
			//	printf("%d edges:\n",*numedges);
			//	for(j=0; j<*numedges; j++)
			//		printf("%d->%d\n",edgearray[j]->src,edgearray[j]->dest);
			//	printf("curedges:\n");
			//	for(j=0; j<len-1; j++)
			//		printf("%d->%d\n",curedges[j]->src,curedges[j]->dest);
			//}
			//printf("examining: %d->%d (len=%d): \n",curedges[len-2]->src,curedges[len-2]->dest,len);			
			if (reachable(curedges[len-2]->src, curedges[len-2]->dest, destnode, edgearray, *numedges, curedges, len))
				loops = 1; //signals invalid path	
				
			//printf("loops=%d\n",loops);			
		}
	}
					
	if (len == 1 || (len<maxlen && !pathfound && !loops)) { // expand single vertices or chain paths with less than maxlen nodes
		for(j=0; j<G.node[i].numout; j++) {
			//printf("going to %d\n",G.node[i].edge[j].dest);
			path.labels[len] = G.node[i].edge[j].dest;
			if (path.labels[len-1]==path.labels[len]) continue; //avoid selfloop expansion (a->a)
			curedges[len-1] = &G.node[i].edge[j];
			expandpath_findpaths3(G,path,G.node[i].edge[j].dest,destnode,len+1,maxlen, curedges, edgearray, numedges, totinter, nodearray, numnodes);
		}
	}
}

// finds all paths from sourcenode to destnode up to a maximum length 
// paths should not contain the same node twice (except if sourcenode=destnode) 
// (this constraint is not yet fully implemented)
// adds all distinct edges in these paths to edgearray
// does not add or continue paths which close cycles based on current set of edges
int findPaths3(struct Graph G, int sourcenode, int destnode, int maxlen, struct Edge** edgearray, int *numedges)
{
	int i,j,k;
	int totinter = 0; //total number of interactions in edges of resulting edgearray
	
	struct Edge **curedges = (struct Edge **)malloc(maxlen*sizeof(struct Edge *)); //holds edges in current path 
	
	(*numedges) = 0; //number of edges in valid paths
	
	//int *visited;
	int len=0;
	struct CPattern path; //holds instance of path
	
	path.labels = (int *)malloc(maxlen*sizeof(int));
	path.numnodes = 1;
	path.labels[0]=G.node[sourcenode].label;
	int *nodearray = (int *)malloc(MAXNODES*sizeof(int)); //keeps track of nodes in edgearray in order of addition (except source,sink)
	int numnodes = 0;
	expandpath_findpaths3(G,path,sourcenode,destnode,len+1,maxlen, curedges, edgearray, numedges, &totinter, nodearray, &numnodes); 
	
	for(i=0;i<numnodes;i++)
		printf("%d ,",nodearray[i]);
	printf("\n");
	
	free(curedges);
	free(path.labels);
	
	return totinter;
}
*/

// takes as input an edgearray where edges are ordered by direction
// computes flow using Greedy
// source node at edge 0 is source, dest node at edge numedges-1 is sink
// returns incoming interactions to sink, which accumulate to total flow
double simplifyChain(struct Edge **edgearray, int numedges, struct Interaction **inter, int *numinter)
{
    int i,j,k;
    double *buffer;
    int *ptr;
    int minedge;
    double flow;
    
    //initialize returned data
    *numinter = 0; 
    //int sink = edgearray[numedges-1]->dest; //id of sink
    *inter = (struct Interaction *)malloc(edgearray[numedges-1]->numinter*sizeof(struct Interaction));
    
    buffer = (double *)malloc((numedges+1)*sizeof(double));
    for(i=1;i<numedges+1;i++) {
        buffer[i]=0.0;
    }
    buffer[0]=MAXFLOW; // a very big number

    ptr = (int *)malloc((numedges)*sizeof(int));
    for(i=0;i<numedges;i++) {
        ptr[i]=0;
    }

    //merge interactions on edges and compute flow
    while ((minedge = find_minedge(edgearray, numedges, ptr)) != -1) {
        //printf("minedge:%d,src:%d,dest:%d,ptr:%d,time:%f,qty:%f\n", minedge, G.edgearray[minedge]->src,  G.edgearray[minedge]->dest, ptr[minedge], G.edgearray[minedge]->inter[ptr[minedge]].timestamp, G.edgearray[minedge]->inter[ptr[minedge]].quantity);
        flow = (buffer[minedge] < edgearray[minedge]->inter[ptr[minedge]].quantity) ? buffer[minedge] : edgearray[minedge]->inter[ptr[minedge]].quantity ;
        //printf("flow=%f\n",flow);
        buffer[minedge] -= flow;
        buffer[minedge+1] += flow;
        if ((minedge==numedges-1) && flow>0) {
        	(*inter)[(*numinter)].timestamp =  edgearray[minedge]->inter[ptr[minedge]].timestamp;
        	(*inter)[(*numinter)++].quantity =  flow;
        }
        ptr[minedge]++;
    }
    
/*
    for (i=0; i<G.numnodes; i++) {
        //printf("flow at node %d is %f\n",G.node[i].label, buffer[i]);
        printf("flow at node %d is %f\n",i, buffer[i]);
    }*/
    flow = buffer[numedges];
    //printf("flow at sink node %d is %f, based on interactions:\n",G.numnodes-1, buffer[G.numnodes-1]);
    //for (i=0; i<*numinter; i++)
    //	printf("%f %f\n",(*inter)[i].timestamp, (*inter)[i].quantity);

	//printf("inter pointer: %p\n",(*inter));
	free(buffer);
	free(ptr);
	
    return flow;
}

// computes flow in DAG G by first finding all reducible chains from the source node
// after reduction the resulting DAG is solved using LP 
double compFlow(struct DAG G, int writeDAG)
{
	int i,k,l;
	int changed=1; //marks whether at one pass of nodes, there are changes due to chains found
	int numpasses = 0; //marks number of passes over all nodes
	int *deletednodes = (int *)calloc(G.numnodes,sizeof(int)); //marks "deleted" nodes
	int numdelnodes = 0;
	struct Interaction *inter=NULL;
	int numinter;
	int prevdest;
	double flow;
	
	while (changed)
	{
		changed = 0; //2nd pass may not be necessary 
		numpasses +=1;
		//printf("PASS %d\n",numpasses);
		for (i=0; i<G.node[0].numout; i++) //for each outgoing edge of source node
		{
			int dest = G.edgearray[G.node[0].outedges[i]]->dest;
			if (!deletednodes[dest] && G.node[dest].numout==1 && G.node[dest].numinc==1) //dest is a chain node
			{
				changed = 1; // we are going to have changes
					
				// this array keeps track of the edges in the chain
				struct Edge **edgearray = (struct Edge **)malloc(G.numnodes*sizeof(struct Edge *));
				// initially current edge: i->dest
				edgearray[0]=G.edgearray[G.node[0].outedges[i]];
				int numedges=1;

				while (G.node[dest].numout==1 && G.node[dest].numinc==1)
				{
					prevdest = dest;
					deletednodes[prevdest] = 1; //this node is going to be "deleted"
					numdelnodes++;
					dest = G.edgearray[G.node[prevdest].outedges[0]]->dest;
					edgearray[numedges++]=G.edgearray[G.node[prevdest].outedges[0]];
				}
				/*
				printf("start=%d, dest=%d, numedges=%d\n",0,dest,numedges);
				for(k=0;k<numedges;k++)
					printf("%d->%d\n", edgearray[k]->src,edgearray[k]->dest);
				*/
				
				//chain simplification; run greedy
				numinter=0;
				double flow = simplifyChain(edgearray, numedges, &inter, &numinter);
				
				/*
				printf("Interactions:\n");
				for(k=0;k<numinter;k++)
					printf("%f %f\n", inter[k].timestamp,inter[k].quantity);
				printf("flow=%f\n",flow);*/
				
				//check if edge 0->dest exists
				for (k=0; k<G.node[0].numout; k++)
					if (G.edgearray[G.node[0].outedges[k]]->dest == dest)
						break; // edge already exists
				if (k==G.node[0].numout)
				{
					// edge does not exist
					// replace previous edge

					//printf("Edge %d->%d does not exist, replacing dest of edge %d\n",0,dest,G.node[0].outedges[i]);

					G.edgearray[G.node[0].outedges[i]]->dest = dest;
					G.edgearray[G.node[0].outedges[i]]->numinter = numinter;
					if (G.edgearray[G.node[0].outedges[i]]->inter != NULL)
						free(G.edgearray[G.node[0].outedges[i]]->inter);
					G.edgearray[G.node[0].outedges[i]]->inter = inter;
					inter = NULL;
					// new edge must go first (swapping)
					// old edge must be deleted (swap with last one?)
					// update numout
				
					// find and replace previous incoming edge to dest with new edge
					for(l=0; l<G.node[dest].numinc; l++)
						if (G.edgearray[G.node[dest].incedges[l]]->src==prevdest)
							break;
					if (l==G.node[dest].numinc) //not found
						printf("ERROR: prevdest %d not found in incoming edges of dest %d\n",prevdest,dest);
					else
						//G.edgearray[G.node[dest].incedges[l]]->src=i;	
						G.node[dest].incedges[l]=G.node[0].outedges[i];	
					
					/*
					printf("Printing Incoming Edges of node %d\n",dest);
					for(int w=0; w<G.node[dest].numinc; w++)
						printf("incoming %d->%d\n",G.edgearray[G.node[dest].incedges[w]]->src,G.edgearray[G.node[dest].incedges[w]]->dest);
								*/
				}
				else 
				{
					// edge exists => merge interactions in existing edge
					
					/*
					printf("Edge %d->%d exists with order %d\n",0,dest,k);
					printf("Merging with interactions:\n");
					for(l=0;l<G.edgearray[G.node[0].outedges[k]]->numinter;l++)
						printf("%f %f\n", G.edgearray[G.node[0].outedges[k]]->inter[l].timestamp,G.edgearray[G.node[0].outedges[k]]->inter[l].quantity);
					printf("new total no of interactions=%d\n",numinter+G.edgearray[G.node[0].outedges[k]]->numinter);
					*/
					
					G.edgearray[G.node[0].outedges[k]]->inter = (struct Interaction *)realloc(G.edgearray[G.node[0].outedges[k]]->inter, (numinter+G.edgearray[G.node[0].outedges[k]]->numinter)*sizeof(struct Interaction));
					for(l=0; l<numinter; l++)
						G.edgearray[G.node[0].outedges[k]]->inter[G.edgearray[G.node[0].outedges[k]]->numinter++]=inter[l];
					
					/*
					printf("After merging before sorting:\n");
					for(l=0;l<G.edgearray[G.node[0].outedges[k]]->numinter;l++)
						printf("%f %f\n", G.edgearray[G.node[0].outedges[k]]->inter[l].timestamp,G.edgearray[G.node[0].outedges[k]]->inter[l].quantity);
					*/
					
					qsort(G.edgearray[G.node[0].outedges[k]]->inter, G.edgearray[G.node[0].outedges[k]]->numinter, sizeof(struct Interaction), compInter);
				
					/*
					printf("After merging and sorting:\n");
					for(l=0;l<G.edgearray[G.node[0].outedges[k]]->numinter;l++)
						printf("%f %f\n", G.edgearray[G.node[0].outedges[k]]->inter[l].timestamp,G.edgearray[G.node[0].outedges[k]]->inter[l].quantity);
					*/
						
					// Old edges must be deleted
					// 1) delete outgoing edge G.node[i].outedges[j]
					// update G.node[i].numout
					
					G.node[0].outedges[i]=G.node[0].outedges[G.node[0].numout-1];
					G.node[0].numout--;
					i--;
					
					
					// 2) delete incoming edge prevdest->dest
					// update G.node[dest].numinc
					for(l=0; l<G.node[dest].numinc; l++)
						if (G.edgearray[G.node[dest].incedges[l]]->src==prevdest)
							break;
					if (l==G.node[dest].numinc) {//not found
						printf("ERROR: prevdest %d not found in incoming edges of dest %d\n",prevdest,dest);
					}
					else {
						//"transfer" last incoming edge to current position
						G.node[dest].incedges[l]=G.node[dest].incedges[G.node[dest].numinc-1];
						G.node[dest].numinc--;
					}
					
					/*
					printf("Printing Incoming Edges of node %d\n",dest);
					for(int w=0; w<G.node[dest].numinc; w++)
						printf("incoming %d->%d\n",G.edgearray[G.node[dest].incedges[w]]->src,G.edgearray[G.node[dest].incedges[w]]->dest);
					*/
				}
			
				free(edgearray);
				if (inter!=NULL) {
					free(inter);
					inter = NULL;
				}
			}
		}
	}
	//printf("numpasses=%d\n",numpasses);
	
	// after DAG has been simplified by iteratively simplifying chains
	// construct a new DAG for LP algorithm to solve	
	
	struct Edge **edgearray = (struct Edge **)malloc(G.numedges*sizeof(struct Edge *));
	int numedges =0;
	
	//add all edges that include a non-deleted node
	int sourcepos = -1; //used to swap edges if first edge in edgearray is not from source
	for (i=0; i<G.numedges; i++)
		if (!deletednodes[G.edgearray[i]->src] && !deletednodes[G.edgearray[i]->dest])
		{
			//printf("adding edge %d: %d->%d\n",i, G.edgearray[i]->src,G.edgearray[i]->dest);
			if (sourcepos == -1 && G.edgearray[i]->src==0)
				sourcepos = numedges;
			edgearray[numedges++] = G.edgearray[i];
		}
	struct Edge *tmp = edgearray[sourcepos];
	edgearray[sourcepos]=edgearray[0];
	edgearray[0]=tmp;
	
	//printf("numedges=%d, sink=%d\n",numedges,G.numnodes-1);
	
	//for (i=0; i<numedges; i++)
	//	printf("edgearray edge %d: %d->%d\n",i, edgearray[i]->src,edgearray[i]->dest);
		
	//printf("DAG reduced to %d nodes and %d edges\n",G.numnodes-numdelnodes,numedges);
	if (numedges>1)
	{
		struct DAG *G2 = edgearray2DAG(edgearray, numedges, G.numnodes-1, 0);
		//printDAG(G2);
		if (writeDAG) writeDAGtofile(G2, "decompDAG.txt");
		flow=computeFlowLP(*G2);
		freeDAG(G2);
	}
	else // just a single edge; sum up all quantities on its interactions 
	{
		flow =0;
		for(i=0;i<edgearray[0]->numinter;i++)
			flow += edgearray[0]->inter[i].quantity;
	}
	
	free(deletednodes);
	free(edgearray);
	
	return flow;
}

/*
// examines possible DAGs with same node as source and sink
// takes source candidates from 2-hop loops file
int runtestloops(struct Graph G, char *floops)
{
	int i,j,k;
	FILE *f; // input file with 2-hop loops
	FILE *fout; // output file 

	char *line = NULL; // used for fileread
	size_t len = 0; // used for fileread
	ssize_t read; // used for fileread
	const char delim[2] = " "; // used for fileread
	char *token; // used for fileread
	int numline = 0;
	int numdags = 0;
	
	int prevsrc = -1;
	
	int source;
	int sink;
	int totinter;
	
	double flow;
	
	struct DAG *G2=NULL;
	struct Edge** edgearray;
	int numedges;
	
	int *order;
	int numdeletedinter;
	int numdeletededges;
	int numdeletednodes;
	struct DAG *retDAG=NULL;

    clock_t t;
    double time_taken;
	
	edgearray = (struct Edge **)malloc(MAXEDGES*sizeof(struct Edge *)); //this array holds all edges that exist in valid paths 
	
	f = fopen(floops,"r");
	if (f==NULL)
	{
		printf("ERROR: file %s does not exist.\n",floops);
		return -1;
	}
	
	fout = fopen("newexps.txt","w");
	fprintf(fout,"source\tsink\tnumnodes\tnumedges\ttotInteractions\tGreedyFlow\tGreedyTime\tLPFlow\tLPTime\tNumNodeDel\tNumEdgeDel\tNumInterDel\tPrepTime\tPreLPFlow\tPreLPTime\tPreSimLPFlow\tPreSimLPTime\n");
	
	// Read loops from file
	while ((read = getline(&line,&len,f)) != -1)	{
		numline++;
		// next line to be read should be <src> <outdegree>
		token = strtok(line,delim);
		source = atoi(token);
		if (source==prevsrc)
			continue;
		prevsrc = source;
		numdags++;
		printf("source: %d\n",source);
		sink=source;
		printf("sink: %d\n",sink);
		fprintf(fout,"%d\t%d\t",source,sink); //write source to file
		
		totinter=findPaths2(G,source,sink,4,edgearray,&numedges); 
		printf("numedges=%d, totinter=%d\n",numedges, totinter);
		
		G2 = edgearray2DAG(edgearray, numedges, sink, 0);
		printf("numnodes=%d\n",G2->numnodes);
		fprintf(fout,"%d\t%d\t%d\t",G2->numnodes,numedges,totinter); //write numnodes, numedges, numinter to file
			
		if (numedges==0) {
			printf("No paths found\n");
			return -1;
		}
		
		// running simple Greedy
		t = clock();    
		flow=computeFlowGreedy(*G2);
		t = clock() - t;
    	time_taken = ((double)t)/CLOCKS_PER_SEC;
    	printf("Gr: %f flow, in %f seconds\n", flow, time_taken);
		fprintf(fout,"%f\t%f\t",flow, time_taken);

		// running LP		
		t = clock();    
		if (totinter<10000) //run LP only if number of interactions is not extremely large
		//if (totinter<0) 
			flow=computeFlowLP(*G2);
		else
			flow = -1;
		t = clock() - t;
    	time_taken = ((double)t)/CLOCKS_PER_SEC;
    	printf("LP: %f flow, in %f seconds\n", flow, time_taken);
    	fprintf(fout,"%f\t%f\t",flow, time_taken);

		// running preprocessingDAG (without flow computation)
		order = topoorder(G2);	
		if (order) {
			numdeletedinter = 0;
			numdeletededges = 0;
			numdeletednodes = 0;	
			t = clock(); 
			retDAG = preprocessDAG(G2, order, &numdeletedinter, &numdeletededges, &numdeletednodes);
			t = clock() - t;
			time_taken = ((double)t)/CLOCKS_PER_SEC;
			printf("Preprocessing time: %f seconds\n", time_taken);
			printf("Total deletions: interactions=%d, edges=%d, nodes=%d\n",numdeletedinter,numdeletededges,numdeletednodes);
			fprintf(fout,"%d\t%d\t%d\t%f\t",numdeletednodes, numdeletededges, numdeletedinter, time_taken);

			if(retDAG==NULL)
			{
				printf("DAG has no flow: sink disconnected\n");
				fprintf(fout,"%f\t%f\t",0.0, 0.0);
				fprintf(fout,"%f\t%f\n",0.0, 0.0);
			}
			else if (totinter<10000) { //run LP only if number of interactions is not extremely large
				// LP after preprocessing
				t = clock();    
				flow=computeFlowLP(*retDAG);
				t = clock() - t;
				time_taken = ((double)t)/CLOCKS_PER_SEC;
				printf("PreLP: %f flow, in %f seconds\n", flow, time_taken);
				fprintf(fout,"%f\t%f\t",flow, time_taken);
					
				// LP after preprocessing and DAG simplification
				t = clock(); 
				flow=compFlow(*retDAG);
				t = clock() - t;
				time_taken = ((double)t)/CLOCKS_PER_SEC;
				printf("PreSimLP: %f flow, in %f seconds\n", flow, time_taken);
				fprintf(fout,"%f\t%f\n",flow, time_taken);
			}
			else 
			{
				fprintf(fout,"%f\t%f\t",-1.0, 0.0);
				fprintf(fout,"%f\t%f\n",-1.0, 0.0);
			}
	
			free(order);

			if (retDAG != NULL) {
				freeDAG(retDAG);
			}
		}
		else {
			fprintf(fout,"%d\t%d\t%d\t%f\t",0, 0, 0, 0.0);
			fprintf(fout,"%f\t%f\t",0.0, 0.0);
			fprintf(fout,"%f\t%f\n",0.0, 0.0);		
		}
			
    	//free(edgearray);
    
		//fprintf(fout,"%d\t%d\t%d\t%d\t%d\t%f\t%f\t%f\t%f\t%f\t%f\n",source,sink,G2->numnodes,numedges,totinter,flow1,time_taken1,flow2,time_taken2,flow3,time_taken3);
		fflush(fout);

		if (G2 != retDAG && G2 != NULL) {
			freeDAG(G2);
			G2=NULL;
		}

		//if (!(numdags%100))
		//	break;
		//printf("source: %d\n",src);		
	}
	fclose(fout);
	free(edgearray);
	return 0;
}
*/

/*
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


	if (argc != 2)
	{
		printf("filename expected as argument. Exiting...\n");
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
	
	
	
	if (runtestloops(G, "bc2.txtl2.txt")==-1)
		return -1;
	else return 0;
	
	
	//read_pattern(&p,"pattern_file1.txt");
	//printPattern(p);


	// preprocessing function: generates preprocessed tables and files
	//dfs_pre(G,4,argv[1]);
	
	//int source = 0; //use with data_lp.txt to test lp result vs greedy
	//int sink = 4;
	
	
	//int source = 0; // use with data_paper2_new.txt
	//int sink = 0;
	//int source = 0; // use with test_graph1.txt
	//int sink = 7;
	//int source = 0; // use with test_graph3.txt, test_graph4.txt
	//int sink = 9;

	//int source = 0; // use with 22395_in.txt
	//int sink = 16;

	//int source = 0; // use with compoexam2.txt
	//int sink = 16;

	
	//int source = 19; // use with v12.txt and bc2.txt (also whatever is next)
	//int sink = 11933038;
	//int source = 19; //too many interaction: LP does not run
	//int sink = 19;
	//int source = 148; // few nodes, huge number of interactions
	//int sink = 148;
	//int source = 200; // different result greedy and LP ** had cycle: now eliminated ** 
	//int sink = 200; //huge number of interactions - 19 nodes - preprocessing only kills 2 edges
	//int source = 294; // interesting case soluble by greedy, nice visualization 
	//int sink = 294;
	//int source = 343; // pure DAG, but LP gives smaller result than greedy (not in bc2) 
	//int sink = 343; // time ties exist (not in bc2)
	//int source = 347; // tiny DAG, soluble by greedy 
	//int sink = 347; 
	//int source = 487; // *** pure DAG, but LP gives smaller result than greedy (not in bc2)
	//int sink = 487; // time ties exist (not in bc2)
	//int source = 489; // pure DAG, LP gives greater result than greedy (not in bc2)
	//int sink = 489; // 
	//int source = 670; // small DAG, greedy=LP, easy problem
	//int sink = 670; 
	//int source = 689; // small DAG, greedy=LP (by chance), easy problem
	//int sink = 689;  
	//int source = 732; // nice mid-sized DAG, greedy=LP, easy problem
	//int sink = 732; 
	//int source = 757; // easy sparse DAG, greedy=LP (by chance)
	//int sink = 757; 
	//int source = 5086; 
	//int sink = 5086; 
	//int source = 293;
	//int sink = 38112;
	//int source = 4459; // extremely large number of edges and interactions on them
	//int sink = 4459; // LP does not run 
	//int source = 7549; // 20-40 nodes and edges, contains MIC
	//int sink = 7549; // 
	//int source = 511; // huuuge DAG (nodes=282,edges=995,inter=110407) 
	//int sink = 511; // greedyRec crashes
	//int source = 1705158; //  
	//int sink = 1705158; // 
	//int source = 7898; // good case for studying MIC identification  
	//int sink = 7898; // 
	//int source = 22395; // good case for studying MIC identification  
	//int sink = 22395; // 
	//int source = 1754750; // 0 flow
	//int sink = 1754750; // 
	//int source = 123; // 0 flow
	//int sink = 123; // 
	//int source = 925; // 0 flow
	//int sink = 925; // 
	//int source = 143; // LP beats Greedy, preprocessing rocks
	//int sink = 143; // 
	//int source = 343; // LP beats Greedy, preprocessing is ok
	//int sink = 343; // 
	//int source = 830; // LP beats Greedy, 6 secs slow, preprocessing drops it to 5sec
	//int sink = 830; // many nodes, many edges, many interactions, good case to study
	//int source = 4376; // LP beats Greedy, preprocessing halves time
	//int sink = 4376; // 150->130 nodes, many edges, good case to study
	//int source = 5399; // LP beats Greedy, cheap
	//int sink = 5399; // good case to study for reducing graph, good visio
	//int source = 5504; // LP beats Greedy, cheap
	//int sink = 5504; // good case to study for reducing graph, good visio
	//int source = 6754; // LP beats Greedy, cheap
	//int sink = 6754; // ok case to study, not much room for reducing graph, good visio
	//int source = 7440; // LP beats Greedy, 
	//int sink = 7440; // good case to study for reducing graph
	//int source = 7549; // LP beats Greedy, 
	//int sink = 7549; // good case to study for reducing graph
	int source = 55120; // buggy  
	int sink = 55120; // 
	//212526 very slow (>1000sec) great reduction (149sec)
	
	// write paths from src to dest to a file
	int pathlen = 4;
	// write to edgearray distinct edges on paths from src to dest
	int totinter = findPaths2(G,source,sink,pathlen,edgearray,&numedges); 
	
	printf("numedges=%d, totinter=%d\n",numedges, totinter);
	
	if (numedges==0) {
		printf("No paths found\n");
		return -1;
	} 
	

	//convert edgearray to DAG
	
	G2 = edgearray2DAG(edgearray, numedges, sink, 1);
	printf("numnodes=%d\n",G2->numnodes);
	//printDAG(G2);
	//return -1;

	printf("processing DAG \n");
	printf("Greedy is running \n");
	t = clock();
	flow=computeFlowGreedy(*G2);
	t = clock() - t;
    time_taken = ((double)t)/CLOCKS_PER_SEC;
    printf("Computed flow: %f\n", flow);
    printf("Total time of execution: %f seconds\n", time_taken);


    
    if (totinter<10000) {
		printf("LP is running \n");
		t = clock();    
		flow=computeFlowLP(*G2);
		t = clock() - t;
		time_taken = ((double)t)/CLOCKS_PER_SEC;
		printf("Computed flow: %f\n", flow);
		printf("Total time of execution: %f seconds\n", time_taken);


    }

	
    int *order = topoorder(G2);
	
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
		else if (totinter<10000) {
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
			flow=compFlow(*retDAG);
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
}*/
