/* *
 * Copyright 2013 Illinois Institute of Technology 
 * http://bluesky.cs.iit.edu/libprofil
 * email: eberroca@iit.edu
 */
 
/* This file is part of Libprofil.

    libprofil is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Libprofil is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Libprofil.  If not, see <http://www.gnu.org/licenses/>.
*/ 

#include "md.h"
#include "mpi.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/* -- Definition of Globals -- */

/* This is to avoid problems if MPI_Init is called
 * multiple times (could happen?) */
int MD_initialized = 0;

/* This variable allows the application to turn
 * on and off the capture of new edges */
int MD_capture_edges = 0;

/* Neighbors */
size_t MD_in_degree = 0;
size_t MD_out_degree = 0;
int *MD_neighbors_in = NULL;
int *MD_neighbors_out = NULL;

/* *dests holds all the edges like rank-->dests[i], for all possible i */
MD_edge **MD_dests = NULL;

/* *orgs holds all the edges like orgs[j]-->rank, for all possible j */
MD_edge **MD_orgs = NULL;

MPI_Comm MD_comm = MPI_COMM_WORLD;  /* communicator for the graph */
int MD_rank = -1;       /* rank of the task */
int MD_size = -1;       /* total number of tasks */

/* MD_Memory_alloc
 Allocates all the needed memory for the MD library
*/
int MD_Memory_alloc(void) {

	if (MD_size < 0) {
		fprintf(stderr,"MD_Memory_alloc: MD_size does not have \
				a valid value!!\n");
		return -1;
	}

	/* -- Initializing vectors for the logical graph -- */
	MD_neighbors_in = (int *) malloc(sizeof(int) * MD_size);
	MD_neighbors_out = (int *) malloc(sizeof(int) * MD_size);
	MD_dests = (MD_edge **) malloc(sizeof(MD_edge *) * MD_size);
	MD_orgs = (MD_edge **) malloc(sizeof(MD_edge *) * MD_size);

	if (!MD_neighbors_in || !MD_neighbors_out ||
		!MD_dests ||
		!MD_orgs) {
		fprintf(stderr,"MD_Memory_alloc: malloc() failed: \
				%s\n",strerror(errno));
		return -1;
	}

	/* Setting all the edge pointers to NULL */
	memset((void *) MD_dests, 0, sizeof(MD_edge *) * MD_size);
	memset((void *) MD_orgs, 0, sizeof(MD_edge *) * MD_size);

	return 0;
}

void MD_Memory_free(void) {

	int i;		/* iterator */
	int neighbor;	/* one of my neighbors */

	for (i=0; i < MD_in_degree; i++) {
		neighbor = MD_neighbors_in[i];
		free(MD_orgs[neighbor]);
	}
	for (i=0; i < MD_out_degree; i++) {
		neighbor = MD_neighbors_out[i];
		free(MD_dests[neighbor]);
	}

	free(MD_neighbors_in);
	free(MD_neighbors_out);
	free(MD_dests);
	free(MD_orgs);
	MD_in_degree = 0;
	MD_out_degree = 0;

}

/*
 MD_Init
 Initializes MAP DISCOVER. This function should be called
 from HMAP_Init before any other MD's functions
 are called.
 The function returns -1 if an error ocurred or 0 otherwise
 */
int MD_Init(MPI_Comm comm, int rank, int size) {

	/* Avoid problems if MPI_Init is called
	 * multiple times (could happen) */
	if (MD_initialized)
		return 0;

	MD_comm = comm;	
	MD_size = size;
	MD_rank = rank;

	/* -- Allocating memory for the data structures -- */
	if (MD_Memory_alloc() < 0)
		return -1;

	MD_initialized = 1;
	MD_capture_edges = 1;
	return 0;
}

/* MD_Start_capturing
 It does exactly what its name suggests :)
*/
void MD_Start_capturing(void) {
	MD_capture_edges = 1;
}

/* MD_Stop_capturing
 It does exactly what its name suggests :)
*/
void MD_Stop_capturing(void) {
	MD_capture_edges = 0;
}

/*
MD_Finalize
Finalizes the data structure freeing all the
used memory
*/
void MD_Finalize(void) {

	if (!MD_initialized)
		return;

	/* -- Freeing pointers -- */
	MD_Memory_free();
	
	MD_capture_edges = 0;
	MD_initialized = 0;

}

/*
MD_Reset_data
Resets all data structures of MD (as if not
edges have been captured)
*/
void MD_Reset_data(void) {

	int i;          /* iterator */
	int neighbor;   /* one of my neighbors */

	if (!MD_initialized)
		return;

	for (i=0; i < MD_in_degree; i++) {
		neighbor = MD_neighbors_in[i];
		free(MD_orgs[neighbor]);
	}
	for (i=0; i < MD_out_degree; i++) {
		neighbor = MD_neighbors_out[i];
		free(MD_dests[neighbor]);
	}


	/* -- Setting arrays to null -- */
	memset((void *) MD_orgs, 0, sizeof(MD_edge *)*MD_size);
	memset((void *) MD_dests, 0, sizeof(MD_edge *)*MD_size);
        
	MD_in_degree = 0;
    MD_out_degree = 0;

}

/*
 MD_Add_local_edge
 A new edge is added to the local map org-->dest
 bytes is the number of bytes transmited in this
 particular communication
 0 for success, -1 otherwise
 */
int MD_Add_local_edge(int org, int dest, int bytes) {
	
	MD_edge *edge;		/* To add a new edge */

	/* -- Are we capturing at all? -- */
	if (!MD_capture_edges)
		return 0;

	/* -- Self edges are not captured -- */
	if (org == dest)
		return 0;

	/* -- Checking if the data structure has been
	 * initialized -- */
	if (!MD_initialized) {
		fprintf(stderr,"MD_Add_local_edge: MAP DISCOVER has \
				not been initialized!\n");
		return -1;
	}

	/* -- Checking if the rank has been captured -- */
	if (MD_rank < 0) {
		fprintf(stderr,"MD_Add_local_edge: I don't know my rank \
				yet!\n");
		return -1;
	}	

	if (org == MD_rank) {
		
		/* -- Checking if the edge already exists -- */
		if (!MD_dests[dest]) {
			/* The edge does not exist, so I need to
		 	 * create a new edge element */

			edge = (MD_edge *) malloc(sizeof(MD_edge));
			if (!edge) {
				fprintf(stderr,"MD_Add_local_edge: malloc() \
						failed: %s\n",strerror(errno));
				return -1;
			}

			edge->total_msg = 1;
			edge->total_bytes = bytes;

			/* Updating the vector MD_dests */
			MD_dests[dest] = edge;
			/* Updating the vectors MD_neighbors_out* */
			MD_neighbors_out[MD_out_degree] = dest;
			MD_out_degree++;			

		} else {	
			/* The edge exists! I need to update it */
			MD_dests[dest]->total_msg += 1;
			MD_dests[dest]->total_bytes += bytes;
		}

	} else if (dest == MD_rank) {

		/* -- Checking if the edge already exists -- */
		if (!MD_orgs[org]) {
			/* The edge does not exist, do I need to
			   create a new edge element */
			
			edge = (MD_edge *) malloc(sizeof(MD_edge));
			if (!edge) {
				fprintf(stderr,"MD_Add_local_edge: malloc() \
						failed: %s\n",strerror(errno));
				return -1;
			}

			edge->total_msg = 1;
			edge->total_bytes = bytes;
			
			/* Updating the vector MD_orgs */
			MD_orgs[org] = edge;
			/* Updating the vector MD_neighbors */
			MD_neighbors_in[MD_in_degree] = org;
			MD_in_degree++;
		} else {
			/* The edge exists! I need to update it */
			MD_orgs[org]->total_msg += 1;
			MD_orgs[org]->total_bytes += bytes;
		}

	} else {
		/* Something is wrong. The edge should be
		 * source --> rank or rank --> dest */
		fprintf(stderr,"MD_Add_local_edge: The ranks \
				for the edge seems to be incorrect. \
				Either org or dest should be equal to \
				the rank of the calling task. org=\
				%d dest=%d rank=%d\n",
				org,dest,MD_rank);
		return -1;
	}

	return 0;
}

/*
 MD_Print_local_map
 Prints out the local topology for the current task.
 The format is of an interaction per line:
 
ranki	rankj	msgs	bytes
 
 Each column is separated by a \t character

 Returns 0 for success or -1 otherwise.
 */
int MD_Print_local_map(FILE *fd) {
	
	int i;			/* Iterator */
	MD_edge *in_edges;	/* in edges */
	MD_edge *out_edges;	/* out edges */
	int in_degree;		/* degree of in connections */
	int out_degree;		/* degree of out connections */
	int rank_i;		/* Variables to print each edge */
	int rank_j;
	long long total_msg;
	long long total_bytes;

	/* -- Checking if the data structure has been
	 * initialized -- */
	if (!MD_initialized) {
		fprintf(stderr,"MD_Print_local_map: MAP DISCOVER has \
			not been initialized!\n");
		return -1;
	}

	/* -- Getting local map -- */
	if (MD_Return_local_map(&in_edges,
				&in_degree,
				&out_edges,
				&out_degree) < 0)
		return -1;

	/* -- printing "in" edges -- */
	for (i=0; i < in_degree; i++) {
		rank_i = in_edges[i].rank;
		rank_j = MD_rank;
		total_msg = in_edges[i].total_msg;
		total_bytes = in_edges[i].total_bytes;
		/* -- Printing the edge -- */
		if (fprintf(fd,"%d\t%d\t%lld\t%lld\n",
						rank_i,
						rank_j,
						total_msg,
						total_bytes) < 0) {
			fprintf(stderr,"MD_Print_local_map: Error calling \
					fprintf\n");
			free(in_edges);
			free(out_edges);
			return -1;
		}
	}	
	/* -- printing "out" edges -- */
	for (i=0; i < out_degree; i++) {
		rank_i = MD_rank;
		rank_j = out_edges[i].rank;
		total_msg = out_edges[i].total_msg;
		total_bytes = out_edges[i].total_bytes;
		/* -- Printing the edge -- */
		if (fprintf(fd,"%d\t%d\t%lld\t%lld\n",
						rank_i,
						rank_j,
						total_msg,
						total_bytes) < 0) {
			fprintf(stderr,"MD_Print_local_map: Error calling \
					fprintf\n");
			free(in_edges);
			free(out_edges);
			return -1;
		}
	}

	free(in_edges);
	free(out_edges);

	/* -- flushing -- */
	if (fflush(fd) != 0) {
		fprintf(stderr,"MD_Print_local_map: Error calling fflush(): \
				%s\n",strerror(errno));
		return -1;
	}
	
	return 0;
}

/*
 MD_Print_map
 Prints out the entire topology graph for the
 application as a [flatten] matrix, where entry (i*MD_size,j) 
 reprent the number of bytes transmited from
 i to j in the application
*/
int MD_Print_map(FILE *fd, const long long *topology_matrix) {

	int i, j;			/* iterators */	

	/* -- Checking if the data structure has been
	 * initialized -- */
	if (!MD_initialized) {
		fprintf(stderr,"MD_Print_map: MAP DISCOVER has \
				not been initialized!\n");
		return -1;
	}
	
	for (i=0; i < MD_size; i++)
		for (j=0; j < MD_size; j++) {
			fprintf(fd,"%lld",
				topology_matrix[i*MD_size + j]);
			if (j == MD_size - 1)
				fprintf(fd,"\n");
			else
				fprintf(fd,"\t");
		}

	return 0;
}

/*
 MD_Print_SparseMap
 Prints out the entire topology graph for the
 application as a list of edges, one per line, where
 a line has the form:
 i j b
 representing the edge i->j. 'b' are the bytes transmitted
 from i to j.
*/
int MD_Print_SparseMap(FILE *fd, const long long *topology_matrix) {

	int i, j;	/* iterators */
	
	/* -- Checking if the data structure has been
	 * initialized -- */
	if (!MD_initialized) {
		fprintf(stderr,"MD_Print_SparseMap: MAP DISCOVER has \
				not been initialized!\n");
		return -1;
	}

	/* Printing number of processes */
	fprintf(fd,"%d\n",MD_size);

	for (i=0; i < MD_size; i++)
		for (j=0; j < MD_size; j++)
			if (topology_matrix[i*MD_size + j] > 0)
				fprintf(fd,"%d %d %lld\n",i,j,topology_matrix[i*MD_size + j]);
			
	return 0;
}

/*
MD_Return_local_map
Returns the local map for this task in two
vectors of MD_edge type. The memory for
the vectors is allocated inside this function,
and the size of them is written on in_degree and
out_degree (memory should be allocated for these
two scalar variables)

*/
int MD_Return_local_map(MD_edge **in_edges,
			int *in_degree,
			MD_edge **out_edges,
			int *out_degree) {

	int i;			/* Iterators */
	int neighbor;		/* rank of one of my neighbors */

	/* -- Checking if the data structure has been
	 * initialized -- */
	if (!MD_initialized) {
		fprintf(stderr,"MD_Add_local_edge: MAP DISCOVER has \
			not been initialized!\n");
		return -1;
	}
        	
	/* -- Allocating memory -- */
	*in_edges = (MD_edge *) malloc(sizeof(MD_edge)*MD_in_degree);
	*out_edges = (MD_edge *) malloc(sizeof(MD_edge)*MD_out_degree);

	if (*in_edges == NULL || *out_edges == NULL) {
		fprintf(stderr,"MD_Return_local_map: Error calling malloc(): \
				%s\n",strerror(errno));
		return -1;
	}

	/* -- Filling degrees -- */
	*in_degree = MD_in_degree;
	*out_degree = MD_out_degree;

    for (i=0; i < MD_in_degree; i++) {
                neighbor = MD_neighbors_in[i];
		(*in_edges)[i].rank = neighbor;
		(*in_edges)[i].total_msg = MD_orgs[neighbor]->total_msg;
		(*in_edges)[i].total_bytes = MD_orgs[neighbor]->total_bytes;
	}
	for (i=0; i < MD_out_degree; i++) {
		neighbor = MD_neighbors_out[i];
		(*out_edges)[i].rank = neighbor;
		(*out_edges)[i].total_msg = MD_dests[neighbor]->total_msg;
		(*out_edges)[i].total_bytes = MD_dests[neighbor]->total_bytes;      	
	}
        
	return 0;
}

/*
MD_Return_map_buffer

Returns an array of size MD_size with all the neighbors
to which this particular task has sent a message to and
the bytes transmitted.
*/
int MD_Return_map_buffer(long long **conn_buffer) {

	int i;			/* iterator */

	/* -- Checking if the data structure has been
	 * initialized -- */
	if (!MD_initialized) {
		fprintf(stderr,"MD_Add_local_edge: MAP DISCOVER has \
				not been initialized!\n");
		return -1;
	}

	/* -- Rank and Size should have meaningful values -- */
	if (MD_rank == -1 || MD_size == -1)
		return -1;

	/* -- Preparing my buffer to send -- */
	*conn_buffer = (long long *) malloc(sizeof(long long)*MD_size);
	if (*conn_buffer == NULL) {
		fprintf(stderr,"MD_Return_matrix_map: malloc() failed: \
				%s\n",strerror(errno));
		return -1;
	}
	memset((void *)*conn_buffer,0,sizeof(long long)*MD_size); // init to 0

	/* -- Filling my buffer -- */
	for (i=0; i < MD_out_degree; i++)
		(*conn_buffer)[MD_neighbors_out[i]] = 
			MD_dests[MD_neighbors_out[i]]->total_bytes;

	return 0;
}

/* -- Setters and Getters -- */

void MD_Set_comm(MPI_Comm comm) {
	MD_comm = comm;
}

void MD_Get_comm(MPI_Comm *comm) {
	*comm = MD_comm;
}

void MD_Set_rank(int rank) {
        MD_rank = rank;
}

int MD_Get_rank(void) {
        return MD_rank;
}

void MD_Set_size(int size) {
	MD_size = size;
}

int MD_Get_size(void) {
	return MD_size;
}

int MD_Is_initialized(void) {
	return MD_initialized;
}
