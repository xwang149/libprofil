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
#include "profil.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

/* -- Global variables -- */

/* TODO: Can I do this better than linear? */

PROFIL_init_request *init_req_head = NULL;
PROFIL_init_request *init_req_tail = NULL;
int init_requests_size = 0;

pthread_mutex_t lock;	/* Only one thread is allowed to 
			 * execute the profiling functions */

/* -- Local function prototypes -- */

int Capture_Edge(int count,
		 MPI_Datatype datatype,
		 int source,
		 int dest,
		 MPI_Comm comm,
		 char *function_name);

int Capture_Request(const void *buf, int count, MPI_Datatype datatype, int source,
		    int dest, int tag, MPI_Comm comm, MPI_Request *request);

PROFIL_init_request *Search_request(MPI_Request *request);

void Free_Requests(void);

/* -- Function implementations -- */

/*
 MPI_Init - Initialize the MPI execution environment and
 data structures for the library

   Input Parameters:
+  argc - Pointer to the number of arguments
-  argv - Pointer to the argument vector
*/
int MPI_Init( int *argc, char ***argv ) {

	int mpi_errno;	/* To return MPI errors */
	int rank, size;

	mpi_errno = PMPI_Init(argc,argv);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;

	/* TODO: MPI_Init will be eliminated!*/
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&size);
	mpi_errno = MD_Init(MPI_COMM_WORLD, rank, size);

	if (mpi_errno < 0) {
		fprintf(stderr,"In profiling MPI_Init: MD_Init \
				failed\n");
		mpi_errno = MPI_ERR_OTHER;
		return mpi_errno;
	}
	
	return MPI_SUCCESS;
}

/*
MPI_Comm_size - Determines the size of the group associated with a communicator

Input Parameter:
. comm - communicator (handle)

Output Parameter:
. size - number of processes in the group of 'comm'  (integer)
*/
int MPI_Comm_size( MPI_Comm comm, int *size ) {

	int mpi_errno;	/* To return MPI errors */
    MPI_Comm _comm;         /* communicator for profiling */

	mpi_errno = PMPI_Comm_size(comm,size);
	if (mpi_errno != MPI_SUCCESS) {
		fprintf(stderr,"In profiling MPI_Comm_size: Error calling \
							PMPI_Comm_size\n");
		return mpi_errno;
	}

   //////////////////////////
   pthread_mutex_lock(&lock);
   //////////////////////////

    MD_Get_comm(&_comm);
    if (_comm == comm)
	    MD_Set_size(*size);

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	
	return MPI_SUCCESS;
}

/*
MPI_Comm_rank - Determines the rank of the calling process in the communicator

Input Argument:
. comm - communicator (handle)

Output Argument:
. rank - rank of the calling process in the group of 'comm'  (integer)
*/
int MPI_Comm_rank( MPI_Comm comm, int *rank ) {

	int mpi_errno;	/* To return MPI errors */
    MPI_Comm _comm;         /* communicator for profiling */
	
	mpi_errno = PMPI_Comm_rank(comm,rank);
	if (mpi_errno != MPI_SUCCESS) {
		fprintf(stderr,"In profiling MPI_Comm_rank: Error calling \
							PMPI_Comm_rank\n");
		return mpi_errno;
	}

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

    MD_Get_comm(&_comm);
    if (_comm == comm)
        MD_Set_rank(*rank);

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;
}

/*
 MPI_Finalize
 Terminates MPI execution environment and
 data structures for the library
 */
int MPI_Finalize(void) {

	int initialized;	/* To check if other
				 * thread already called
				 * MPI_Finalize */

	FILE *fd;		/* file descriptor */
	int rank;		/* My MPI rank */
	int size;		/* Number of tasks */
	char file_name[256];	/* File name */
	long long *conn_buffer;	/* communication buffer for this task */
	long long *conn_matrix;	/* comuunication matrix for the whole app */
	int res;		/* result */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	initialized = MD_Is_initialized();

	if (initialized) {
	
		rank = MD_Get_rank();
		size = MD_Get_size();

		/*if (rank >= 0) {
			sprintf(file_name,"local_map_%d.txt",rank);
			fd = fopen(file_name,"w");
			if (fd != NULL) {
				MD_Print_local_map(fd);
				fclose(fd);
			}
		}*/
		if (rank >= 0 && size >= 0) {
			
			/* Local connectivity buffer */
			res = MD_Return_map_buffer(&conn_buffer);

			if (res < 0) {
				res = MPI_ERR_OTHER;
				pthread_mutex_unlock(&lock);
				return res;
			}

			/* memory for the connectivity matrix */
			conn_matrix = 
				(long long *) malloc(sizeof(long long)*size*size);
			if (!conn_matrix) {
				fprintf(stderr,"In profiling MPI_Finalize: \
						Error calling malloc(): \
						%s\n",strerror(errno));
				pthread_mutex_unlock(&lock);
				res = MPI_ERR_OTHER;
				return res;
			}

			////////////////////////////
			pthread_mutex_unlock(&lock);
			////////////////////////////

			res = PMPI_Allgather(conn_buffer,size,MPI_LONG_LONG,
					    conn_matrix,size,MPI_LONG_LONG,
					    MPI_COMM_WORLD);
			if (res != MPI_SUCCESS) {
				fprintf(stderr,"In profiling MPI_Finalize: \
						Error calling MPI_Allgather\n");
				return res;
			}
			

			//////////////////////////
			pthread_mutex_lock(&lock);
			//////////////////////////

			if (rank == 0) {				
				sprintf(file_name,"topology.txt");
				fd = fopen(file_name,"w");
				if (fd != NULL) {
					MD_Print_SparseMap(fd,conn_matrix);
					fclose(fd);
				}
			}
		}
		
	}

	if (initialized)
		/* -- Freeing PROFIL_init structure -- */
		Free_Requests();

	/* -- Finalizing MD -- */
	MD_Finalize();

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Finalize();
}

/*
 MPI_Send
 All the edges discovered in this function are directed edges
 going from the rank sending the message to the destination.

Input Parameters:
+ buf - initial address of send buffer (choice) 
. count - number of elements in send buffer (nonnegative integer) 
. datatype - datatype of each send buffer element (handle) 
. dest - rank of destination (integer) 
. tag - message tag (integer) 
- comm - communicator (handle)

Notes:
This routine may block until the message is received by the destination 
process.

 */
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
		MPI_Comm comm) {
	
	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the send operation -- */
	mpi_errno = Capture_Edge(count,datatype,rank,dest,comm,"MPI_Send");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Send(buf,count,datatype,dest,tag,comm);
	
}

/*
  MPI_Recv
  All edges discovered in this function are directed edges going
  from source to rank

Output Parameters:
+ buf - initial address of receive buffer (choice)
- status - status object (Status)

Input Parameters:
+ count - maximum number of elements in receive buffer (integer)
. datatype - datatype of each receive buffer element (handle)
. source - rank of source (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Notes:
The 'count' argument indicates the maximum length of a message; the actual
length of the message can be determined with 'MPI_Get_count'.

*/
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	     MPI_Comm comm, MPI_Status *status) {

	int rank;		/* My own rank */
	int mpi_errno;		/* errno returned by MPI */
	int real_count;		/* Real number of elements transferred */
	
	/* -- Calling the true MPI function -- */
	mpi_errno = PMPI_Recv(buf,count,datatype,source,tag,comm,status);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;
	
	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Get the real count of elements transferred (count here
		only indicates the maximum lenght of a message -- */
	mpi_errno = PMPI_Get_count(status,datatype,&real_count);
	if (mpi_errno != MPI_SUCCESS) {
		fprintf(stderr,"In profiling MPI_Recv: PMPI_Get_count \
				failed -- mpi_errno=%d\n",mpi_errno);
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}
	if (real_count == MPI_UNDEFINED) {
		fprintf(stderr,"In profiling MPI_Recv: count of received \
				elements is MPI_UNDEFINED\n");
		mpi_errno = MPI_ERR_OTHER;
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}
	
	/* -- Capturing the recv operationg -- */	
	mpi_errno = Capture_Edge(real_count,datatype,source,rank,comm,"MPI_Recv");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;
}

/*
MPI_Sendrecv - Sends and receives a message
 Capturing two edges (profil)

Input Parameters:
+ sendbuf - initial address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - type of elements in send buffer (handle)
. dest - rank of destination (integer)
. sendtag - send tag (integer)
. recvcount - number of elements in receive buffer (integer)
. recvtype - type of elements in receive buffer (handle)
. source - rank of source (integer)
. recvtag - receive tag (integer)
- comm - communicator (handle)

Output Parameters:
+ recvbuf - initial address of receive buffer (choice)
- status - status object (Status).  This refers to the receive operation.

*/
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
		 int dest, int sendtag,
		 void *recvbuf, int recvcount, MPI_Datatype recvtype,
		 int source, int recvtag,
		 MPI_Comm comm, MPI_Status *status) {

	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */
	int real_count;		/* Real number of elements transferred */

	/* -- Calling the true MPI function -- */
	mpi_errno = PMPI_Sendrecv(sendbuf,sendcount,sendtype,dest,sendtag,
				  recvbuf,recvcount,recvtype,source,recvtag,
				  comm,status);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the send operation -- */
	mpi_errno = Capture_Edge(sendcount,sendtype,rank,
				 dest,comm,"MPI_Sendrecv");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	/* -- Get the real count of elements transferred (recvcount here
	  	only indicates the maximum lenght of a message) -- */
	mpi_errno = PMPI_Get_count(status,recvtype,&real_count);
	if (mpi_errno != MPI_SUCCESS) {
		fprintf(stderr,"In profiling MPI_Sendrecv: PMPI_Get_count \
				failed -- mpi_errno=%d\n",mpi_errno);
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}
	if (real_count == MPI_UNDEFINED) {
		fprintf(stderr,"In profiling MPI_Sendrecv: count of received \
				elements is MPI_UNDEFINED\n");
		mpi_errno = MPI_ERR_OTHER;
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	/* -- Capturing the recv operation -- */
	mpi_errno = Capture_Edge(real_count,recvtype,source,
				 rank,comm,"MPI_Sendrecv");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;

}

/*
    MPI_Sendrecv_replace - Sends and receives using a single buffer
	Same as MPI_Sendrecv (profil)

Input Parameters:
+ count - number of elements in send and receive buffer (integer)
. datatype - type of elements in send and receive buffer (handle)
. dest - rank of destination (integer)
. sendtag - send message tag (integer)
. source - rank of source (integer)
. recvtag - receive message tag (integer)
- comm - communicator (handle)

Output Parameters:
+ buf - initial address of send and receive buffer (choice)
- status - status object (Status)

*/
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
			 int dest, int sendtag,
			 int source, int recvtag,
			 MPI_Comm comm, MPI_Status *status) {

	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	/* -- Calling the true MPI function -- */
	mpi_errno = PMPI_Sendrecv_replace(buf,count,datatype,dest,sendtag,
					  source,recvtag,comm,status);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();	

	/* -- Capturing the send operation -- */
	mpi_errno = Capture_Edge(count,datatype,rank,
				 dest,comm,"MPI_Sendrecv_replace");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	/* -- Capturing the recv operation -- */
	mpi_errno = Capture_Edge(count,datatype,source,
				 rank,comm,"MPI_Sendrecv_replace");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;

}


/*
    MPI_Irecv - Begins a nonblocking receive
	Same as MPI_recv (profil)

Input Parameters:
+ buf - initial address of receive buffer (choice)
. count - number of elements in receive buffer (integer)
. datatype - datatype of each receive buffer element (handle)
. source - rank of source (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
	      MPI_Comm comm, MPI_Request *request) {

	int rank;		/* My own rank */
	int mpi_errno;		/* errno returned by MPI */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////
	
	/* -- Finding my rank -- */
	rank = MD_Get_rank();
	
	/* -- Capturing the recv operation -- */
	mpi_errno = Capture_Edge(count,datatype,source,rank,comm,"MPI_Irecv");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Irecv(buf,count,datatype,source,tag,comm,request);

}

/*
  MPI_Bsend
  Same as MPI_Send (profiling)
  

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (nonnegative integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Notes:
This send is provided as a convenience function; it allows the user to
send messages without worring about where they are buffered (because the
user `must` have provided buffer space with 'MPI_Buffer_attach').

*/

int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm) {

	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */	
	rank = MD_Get_rank();	

	/* -- Capturing the send operation -- */
	mpi_errno = Capture_Edge(count,datatype,rank,dest,comm,"MPI_Bsend");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Bsend(buf,count,datatype,dest,tag,comm);
	
}

/*@
    MPI_Ssend - Blocking synchronous send
    Same as MPI_Send (profiling)
Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (nonnegative integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)
*/
int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm) {
	
	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();
	
	/* -- Capturing the send operation -- */
	mpi_errno = Capture_Edge(count,datatype,rank,dest,comm,"MPI_Ssend");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Ssend(buf,count,datatype,dest,tag,comm);	

}

/*
    MPI_Rsend - Blocking ready send
    Same as MPI_Send (profiling)
Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (nonnegative integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)
*/
int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm) {

	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the send operation -- */
	mpi_errno = Capture_Edge(count,datatype,rank,dest,comm,"MPI_Rsend");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}
	
	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Rsend(buf,count,datatype,dest,tag,comm);

}

/*
    MPI_Isend - Begins a nonblocking send
    Same as MPI_Send (profiling)

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	      MPI_Comm comm, MPI_Request *request) {

	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();
	
	/* -- Capturing the send operation -- */	
	mpi_errno = Capture_Edge(count,datatype,rank,dest,comm,"MPI_Isend");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Isend(buf,count,datatype,dest,tag,comm,request);
	
}

/*
    MPI_Ibsend - Starts a nonblocking buffered send
	Same as MPI_Send (profiling)

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	       MPI_Comm comm, MPI_Request *request) {

	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();
	
	/* -- Capturing the send operation -- */	
	mpi_errno = Capture_Edge(count,datatype,rank,dest,comm,"MPI_Ibsend");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}
	
	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Ibsend(buf,count,datatype,dest,tag,comm,request);

}

/*
    MPI_Issend - Starts a nonblocking synchronous send
    Same as MPI_Send(profiling)

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	       MPI_Comm comm, MPI_Request *request) {

	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the send operation -- */	
	mpi_errno = Capture_Edge(count,datatype,rank,dest,comm,"MPI_Issend");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Issend(buf,count,datatype,dest,tag,comm,request);

}

/*
    MPI_Irsend - Starts a nonblocking ready send
	Same as MPI_Send (profiling)

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements in send buffer (integer)
. datatype - datatype of each send buffer element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	       MPI_Comm comm, MPI_Request *request) {

	int mpi_errno;		/* errno returned by MPI */
	int rank;		/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();
	
	/* -- Capturing the send operation -- */
	mpi_errno = Capture_Edge(count,datatype,rank,dest,comm,"MPI_Irsend");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Irsend(buf,count,datatype,dest,tag,comm,request);	

} 

/*
    MPI_Send_init - Create a persistent request for a standard send

	Here I just capture the function parameters in the list
	of requests started by the application. This requests
	will be used later when the user calls MPI_Start. (profil)

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements sent (integer)
. datatype - type of each element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

.seealso: MPI_Start, MPI_Startall, MPI_Request_free
*/
int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest,
		  int tag, MPI_Comm comm, MPI_Request *request) {
				
	int mpi_errno;	/* errno returned by MPI */
	int rank;	/* My rank */

	/* -- Calling the true MPI function -- */
	mpi_errno = PMPI_Send_init(buf,count,datatype,dest,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the init request -- */
	mpi_errno = 
		Capture_Request(buf,count,datatype,rank,dest,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;
}

/*
    MPI_Bsend_init - Builds a handle for a buffered send

	Same as MPI_Send_init (profil)

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements sent (integer)
. datatype - type of each element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest,
		   int tag, MPI_Comm comm, MPI_Request *request) {

	int mpi_errno;	/* errno returned by MPI */	
        int rank;       /* My rank */

	/* -- Calling the true MPI function -- */
	mpi_errno = PMPI_Bsend_init(buf,count,datatype,dest,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the init request -- */
	mpi_errno = 
		Capture_Request(buf,count,datatype,rank,dest,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;
}

/*
    MPI_Ssend_init - Creates a persistent request for a synchronous send

	Same as MPI_Send_init (profil)

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements sent (integer)
. datatype - type of each element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest,
		   int tag, MPI_Comm comm, MPI_Request *request) {

	int mpi_errno;	/* errno returned by MPI */
	int rank;       /* My rank */

	/* -- Calling the true MPI function -- */
	mpi_errno = PMPI_Ssend_init(buf,count,datatype,dest,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the init request -- */
	mpi_errno = 
		Capture_Request(buf,count,datatype,rank,dest,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;
}

/*
    MPI_Rsend_init - Creates a persistent request for a ready send

	Same as MPI_Send_init (profil)

Input Parameters:
+ buf - initial address of send buffer (choice)
. count - number of elements sent (integer)
. datatype - type of each element (handle)
. dest - rank of destination (integer)
. tag - message tag (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest,
		   int tag, MPI_Comm comm, MPI_Request *request) {

	int mpi_errno;	/* errno returned by MPI */
	int rank;       /* My rank */

	/* -- Calling the true MPI function -- */
	mpi_errno = PMPI_Rsend_init(buf,count,datatype,dest,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the init request -- */
	mpi_errno = 
		Capture_Request(buf,count,datatype,rank,dest,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;
}

/*
r
    MPI_Recv_init - Create a persistent request for a receive

	Capture of function parameters to be used later on
	in MPI_Start

Input Parameters:
+ buf - initial address of receive buffer (choice)
. count - number of elements received (integer)
. datatype - type of each element (handle)
. source - rank of source or 'MPI_ANY_SOURCE' (integer)
. tag - message tag or 'MPI_ANY_TAG' (integer)
- comm - communicator (handle)

Output Parameter:
. request - communication request (handle)

*/
int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source,
		  int tag, MPI_Comm comm, MPI_Request *request) {
		
	int mpi_errno;	/* errno returned by MPI */
	int rank;	/* My rank */	

	/* -- Calling the true MPI function -- */
	mpi_errno = PMPI_Recv_init(buf,count,datatype,source,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS)
		return mpi_errno;

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the init request -- */
	mpi_errno =
		Capture_Request(buf,count,datatype,source,rank,tag,comm,request);
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;
}

/*
    MPI_Start - Initiates a communication with a persistent request handle

	Here I will capture the particular edge source-->dest thanks to
	the data stored in the PROFIL init list

Input Parameter:
. request - communication request (handle)

*/
int MPI_Start (MPI_Request *request) {

	PROFIL_init_request *init_request;	/* Data for this edge */
	int mpi_errno;				/* errno returned by MPI */
	
	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Searching for this request -- */
	init_request = Search_request(request);	

	if (init_request) {

		/* Otherwise I haven't find it. I don't exit
		 * if the request is not found, I just pass it
		 * to the true MPI function, who deals with the error */

		mpi_errno = Capture_Edge(init_request->count,
					 init_request->datatype,
					 init_request->source,
					 init_request->dest,
					 init_request->comm,
					 "MPI_Start");
		if (mpi_errno != MPI_SUCCESS) {
			pthread_mutex_unlock(&lock);
			return mpi_errno;
		}	
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Start(request);
}

/*
 MPI_Startall - Starts a collection of persistent requests
	I handle this calling multiple times to MPI_Start (profil)

Input Parameters:
+ count - list length (integer)
- array_of_requests - array of requests (array of handle)

   Notes:

   Unlike 'MPI_Waitall', 'MPI_Startall' does not provide a mechanism for
   returning multiple errors nor pinpointing the request(s) involved.
   Futhermore, the behavior of 'MPI_Startall' after an error occurs is not
   defined by the MPI standard.  If well-defined error reporting and behavior
   are required, multiple calls to 'MPI_Start' should be used instead.

*/
int MPI_Startall(int count, MPI_Request array_of_requests[]) {

	int i;					/* Iterator */
	PROFIL_init_request *init_request;	/* Data for this edge */
	int mpi_errno;				/* errno returned by MPI */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////
	
	for (i=0; i < count; i++) {
		/* -- Searching for this request -- */		
		init_request = Search_request(&(array_of_requests[i]));		

		if (init_request) { /* No handling case when NULL*/
			mpi_errno = Capture_Edge(init_request->count,
						 init_request->datatype,
						 init_request->source,
						 init_request->dest,
						 init_request->comm,
						 "MPI_Startall");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Startall(count,array_of_requests);
}

/*
   MPI_Request_free - Fress a communication request object
	(I will mark the particular request as freed in profil)

Input Parameter:
. request - communication request (handle)

*/
int MPI_Request_free(MPI_Request *request) {
	
	PROFIL_init_request *init_request;	/* Data for this edge */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////
	
	/* -- Searching for this request -- */
	init_request = Search_request(request);

	if (init_request)
		init_request->active = 0;

	/* If the request is not found, we don't do anything.
	   The real MPI function will deal with that error case */
	
	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Request_free(request);
}

/* TODO: Should I also capture MPI_Cancel ?? */

#ifndef NOCOLLECTIVES

/*
MPI_Bcast - Broadcasts a message from the process with rank "root" to
            all other processes of the communicator
	(n-1 edges, from root to all other ranks)
Input/Output Parameter:
. buffer - starting address of buffer (choice)

Input Parameters:
+ count - number of entries in buffer (integer)
. datatype - data type of buffer (handle)
. root - rank of broadcast root (integer)
- comm - communicator (handle)
*/
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
	      MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */	
	int rank;	/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing the n-1 sends -- */
	if (rank == root) {
		size = MD_Get_size();

		for (i=0; i < size; i++) {
			if (i != root) {
				/* -- Capturing the send operation -- */
				mpi_errno = Capture_Edge(count,datatype,root,i,
					 		 comm,"MPI_Bcast");
				if (mpi_errno != MPI_SUCCESS) {
					pthread_mutex_unlock(&lock);
					return mpi_errno;
				}
			}
		}
	} else { /* Just one recv from root */
		/* -- Capturing the recv operation -- */
		mpi_errno = Capture_Edge(count,datatype,root,rank,
					 comm,"MPI_Bcast");
		if (mpi_errno != MPI_SUCCESS) {
			pthread_mutex_unlock(&lock);
			return mpi_errno;
		}
	}
	
	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Bcast(buffer,count,datatype,root,comm);
}

/*
MPI_Gather - Gathers together values from a group of processes
	(n-1 edges, from all processes to root)

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements for any single receive (integer,
significant only at root)
. recvtype - data type of recv buffer elements
(significant only at root) (handle)
. root - rank of receiving process (integer)
- comm - communicator (handle)

Output Parameter:
. recvbuf - address of receive buffer (choice, significant only at 'root')
*/
int MPI_Gather(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
	       void *recvbuf, int recvcnt, MPI_Datatype recvtype,
	       int root, MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;       /* My rank */
	
	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();
	
	/* -- Capturing n-1 recvs -- */		
	if (rank == root) {
		size = MD_Get_size();

		for (i=0; i < size; i++) {
			if (i != root) {
				/* -- Capturing the recv operation -- */
				mpi_errno = Capture_Edge(recvcnt,recvtype,i,
							 root,comm,
							 "MPI_Gather");
				if (mpi_errno != MPI_SUCCESS) {
					pthread_mutex_unlock(&lock);
					return mpi_errno;
				}
			}
		}
	} else { /* -- One send to root -- */
		/* -- Capturing the send operation -- */
		mpi_errno = Capture_Edge(sendcnt,sendtype,rank,root,comm,
					 "MPI_Gather");
		if (mpi_errno != MPI_SUCCESS) {
			pthread_mutex_unlock(&lock);
			return mpi_errno;
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Gather(sendbuf,sendcnt,sendtype,
			   recvbuf,recvcnt,recvtype,root,comm);
}

/*
MPI_Gatherv - Gathers into specified locations from all processes in a group
	(n-1 edges, from all processes to root)

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - data type of send buffer elements (handle)
. recvcounts - integer array (of length group size)
containing the number of elements that are received from each process
(significant only at 'root')
. displs - integer array (of length group size). Entry
 'i'  specifies the displacement relative to recvbuf  at
which to place the incoming data from process  'i'  (significant only
at root)
. recvtype - data type of recv buffer elements
(significant only at 'root') (handle)
. root - rank of receiving process (integer)
- comm - communicator (handle)

Output Parameter:
. recvbuf - address of receive buffer (choice, significant only at 'root')
*/
int MPI_Gatherv(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
		void *recvbuf, const int *recvcnts, const int *displs,
		MPI_Datatype recvtype, int root, MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing n-1 recvs -- */
 	if (rank == root) {
		size = MD_Get_size();

		for (i=0; i < size; i++) {
			if (i != root) {
				/* -- Capturing the recv operation -- */
				mpi_errno = Capture_Edge(recvcnts[i],recvtype,i,
							 root,comm,
							 "MPI_Gatherv");
				if (mpi_errno != MPI_SUCCESS) {
					pthread_mutex_unlock(&lock);
					return mpi_errno;
				}
			}
		}
	} else { /* -- One send to root -- */
		/* -- Capturing the send operation -- */
		mpi_errno = Capture_Edge(sendcnt,sendtype,rank,root,comm,
					 "MPI_Gatherv");
		if (mpi_errno != MPI_SUCCESS) {
			pthread_mutex_unlock(&lock);
			return mpi_errno;
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Gatherv(sendbuf,sendcnt,sendtype,
			    recvbuf,recvcnts,displs,
			    recvtype,root,comm);
}

/*
MPI_Scatter - Sends data from one process to all other processes in a
communicator
	(n-1 edges, from root to all processes)

Input Parameters:
+ sendbuf - address of send buffer (choice, significant
only at 'root')
. sendcount - number of elements sent to each process
(integer, significant only at 'root')
. sendtype - data type of send buffer elements (significant only at 'root')
(handle)
. recvcount - number of elements in receive buffer (integer)
. recvtype - data type of receive buffer elements (handle)
. root - rank of sending process (integer)
- comm - communicator (handle)

Output Parameter:
. recvbuf - address of receive buffer (choice)

*/
int MPI_Scatter(const void *sendbuf, int sendcnt, MPI_Datatype sendtype,
		void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root,
		MPI_Comm comm) {
	
	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* my rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */
	rank = MD_Get_rank();	

	if (rank == root) {
		size = MD_Get_size();

		for (i=0; i < size; i++) {
			if (i != root) {
				/* -- Capturing the send operation -- */
				mpi_errno = Capture_Edge(sendcnt,sendtype,
							 root,i,
							 comm,"MPI_Scatter");
				if (mpi_errno != MPI_SUCCESS) {
					pthread_mutex_unlock(&lock);
					return mpi_errno;
				}
			}
		}
	} else {
		/* --  Capturing the recv operation -- */
		mpi_errno = Capture_Edge(recvcnt,recvtype,root,rank,
					 comm,"MPI_Scatter");
		if (mpi_errno != MPI_SUCCESS) {
			pthread_mutex_unlock(&lock);
			return mpi_errno;
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Scatter(sendbuf,sendcnt,sendtype,recvbuf,recvcnt,
			    recvtype,root,comm);
}

/*
MPI_Scatterv - Scatters a buffer in parts to all processes in a communicator
	(n-1 edges, from root to all processes)

Input Parameters:
+ sendbuf - address of send buffer (choice, significant only at 'root')
. sendcounts - integer array (of length group size)
specifying the number of elements to send to each processor
. displs - integer array (of length group size). Entry
 'i'  specifies the displacement (relative to sendbuf  from
which to take the outgoing data to process  'i'
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements in receive buffer (integer)
. recvtype - data type of receive buffer elements (handle)
. root - rank of sending process (integer)
- comm - communicator (handle)

Output Parameter:
. recvbuf - address of receive buffer (choice)

*/
int MPI_Scatterv(const void *sendbuf, const int *sendcnts, const int *displs,
		 MPI_Datatype sendtype, void *recvbuf, int recvcnt,
		 MPI_Datatype recvtype,
		 int root, MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* my rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */
	rank = MD_Get_rank();

	if (rank == root) {
		size = MD_Get_size();	

		for (i=0; i < size; i++) {
			if (i != root) {
				/* -- Capturing the send operation -- */
				mpi_errno = Capture_Edge(sendcnts[i],sendtype,
							 root,i,
							 comm,"MPI_Scatterv");
				if (mpi_errno != MPI_SUCCESS) {
					pthread_mutex_unlock(&lock);
					return mpi_errno;
				}
			}
		}
	} else {
		/* --  Capturing the recv operation -- */
		mpi_errno = Capture_Edge(recvcnt,recvtype,root,rank,
					 comm,"MPI_Scatterv");
		if (mpi_errno != MPI_SUCCESS) {
			pthread_mutex_unlock(&lock);
			return mpi_errno;
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Scatterv(sendbuf,sendcnts,displs,sendtype,
			     recvbuf,recvcnt,recvtype,root,comm);

}

/*
MPI_Allgather - Gathers data from all tasks and deliver the combined data
                 to all tasks
	(n^2 edges, all to all)

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements received from any process (integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameter:
. recvbuf - address of receive buffer (choice)
*/
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
		  void *recvbuf, int recvcount, MPI_Datatype recvtype,
		  MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* my rank */
	
	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */	
	rank = MD_Get_rank();

	/* -- Find the size -- */
	size = MD_Get_size();
	
	for (i=0; i < size; i++) {
		if (i != rank) {
			/* -- Capturing the send and recv op -- */
			/* Send */
			mpi_errno = Capture_Edge(sendcount,sendtype,rank,i,
						 comm,"MPI_Allgather");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
			/* recv */
			mpi_errno = Capture_Edge(recvcount,recvtype,i,rank,
						 comm,"MPI_Allgather");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
		}	
	}
	
	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////
	
	/* -- Calling the true MPI function -- */
	return PMPI_Allgather(sendbuf,sendcount,sendtype,recvbuf,
			      recvcount,recvtype,comm);
}

/*
MPI_Allgatherv - Gathers data from all tasks and deliver the combined data
                 to all tasks
	(n^2 edges, all to all)

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - data type of send buffer elements (handle)
. recvcounts - integer array (of length group size)
containing the number of elements that are to be received from each process
. displs - integer array (of length group size). Entry
 'i'  specifies the displacement (relative to recvbuf ) at
which to place the incoming data from process  'i'
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameter:
. recvbuf - address of receive buffer (choice)

*/
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
		   void *recvbuf, const int *recvcounts, const int *displs,
		   MPI_Datatype recvtype, MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* my rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */
	rank = MD_Get_rank();	

	/* -- Find the size -- */
	size = MD_Get_size();

	for (i=0; i < size; i++) {
		if (i != rank) {
			/* -- Capturing the send and recv op -- */
			/* Send */
			mpi_errno = Capture_Edge(sendcount,sendtype,rank,i,
						 comm,"MPI_Allgatherv");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
			/* recv */
			mpi_errno = Capture_Edge(recvcounts[i],recvtype,i,rank,
						 comm,"MPI_Allgatherv");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
		}

	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Allgatherv(sendbuf,sendcount,sendtype,
			       recvbuf,recvcounts,displs,
			       recvtype,comm);
}

/*
MPI_Alltoall - Sends data from all to all processes
	(n^2 edges, all to all)

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements to send to each process (integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements received from any process (integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameter:
. recvbuf - address of receive buffer (choice)

*/
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
		 void *recvbuf, int recvcount, MPI_Datatype recvtype,
		 MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* my rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */
	rank = MD_Get_rank();

	/* -- Find the size -- */	
	size = MD_Get_size();

	for (i=0; i < size; i++) {
		if (i != rank) {
			/* -- capturing the send and recv op -- */
			/* send */
			mpi_errno = Capture_Edge(sendcount,sendtype,rank,i,
						 comm,"MPI_Alltoall");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
			/* recv */
			mpi_errno = Capture_Edge(recvcount,recvtype,i,rank,
						 comm,"MPI_Alltoall");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Alltoall(sendbuf,sendcount,sendtype,
			     recvbuf,recvcount,recvtype,
			     comm);
}

/*
MPI_Alltoallv - Sends data from all to all processes; each process may
   send a different amount of data and provide displacements for the input
   and output data.
	(n^2 edges, all to all)

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcounts - integer array equal to the group size
specifying the number of elements to send to each processor
. sdispls - integer array (of length group size). Entry
 'j'  specifies the displacement (relative to sendbuf  from
which to take the outgoing data destined for process  'j'
. sendtype - data type of send buffer elements (handle)
. recvcounts - integer array equal to the group size
specifying the maximum number of elements that can be received from
each processor
. rdispls - integer array (of length group size). Entry
 'i'  specifies the displacement (relative to recvbuf  at
which to place the incoming data from process  'i'
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameter:
. recvbuf - address of receive buffer (choice)

*/
int MPI_Alltoallv(const void *sendbuf, const int *sendcnts, const int *sdispls,
		  MPI_Datatype sendtype, void *recvbuf, const int *recvcnts,
		  const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm) {

	/* TODO: Look into this. recvcounts does not have the number
	 * of elements received by rather the maximum allowed. How to
	 * know the actuall number of elements received? */
	
	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* my rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */
	rank = MD_Get_rank();

	/* -- Find the size -- */
	size = MD_Get_size();
	
	for (i=0; i < size; i++) {
		if (i != rank) {
			/* -- Capturing the send and recv op -- */
			/* Send */
			mpi_errno = Capture_Edge(sendcnts[i],sendtype,rank,i,
						 comm,"MPI_Alltoallv");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
			/* recv */
			mpi_errno = Capture_Edge(recvcnts[i],recvtype,i,rank,
						 comm,"MPI_Alltoallv");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
		}	
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Alltoallv(sendbuf,sendcnts,sdispls,
			      sendtype,recvbuf,recvcnts,
			      rdispls,recvtype,comm);
}

/*
MPI_Reduce - Reduces values on all processes to a single value
	(n-1 edges, all to root)

Input Parameters:
+ sendbuf - address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - reduce operation (handle) 
. root - rank of root process (integer) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - address of receive buffer (choice, 
 significant only at 'root')
*/
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	       MPI_Op op, int root, MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* My rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Finding my rank -- */
	rank = MD_Get_rank();

	/* -- Capturing n-1 recvs -- */
	if (rank == root) {
		size = MD_Get_size();

		for (i=0; i < size; i++) {
			if (i != root) {
				/* -- Capturing the recv operation -- */
				mpi_errno = Capture_Edge(count,datatype,i,
							 root,comm,
							 "MPI_Reduce");
				if (mpi_errno != MPI_SUCCESS) {
					pthread_mutex_unlock(&lock);
					return mpi_errno;
				}
			}
		}
	} else { /* -- One send to root -- */
		/* -- Capturing the send operation -- */
		mpi_errno = Capture_Edge(count,datatype,rank,root,comm,
					 "MPI_Reduce");
		if (mpi_errno != MPI_SUCCESS) {
			pthread_mutex_unlock(&lock);
			return mpi_errno;
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Reduce(sendbuf,recvbuf,count,datatype,op,root,comm);
}

/*
MPI_Allreduce - Combines values from all processes and distributes the result
                back to all processes
	(n^2 edges, all to all)

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in send buffer (integer) 
. datatype - data type of elements of send buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - starting address of receive buffer (choice)
*/
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
		  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */	
	int i;		/* iterator */
	int rank;	/* my rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */
	rank = MD_Get_rank();

	/* -- Find the size -- */
	size = MD_Get_size();

	for (i=0; i < size; i++) {
		if (i != rank) {
			/* -- capturing the send and recv op -- */
			/* send */
			mpi_errno = Capture_Edge(count,datatype,rank,i,
						 comm,"MPI_Allreduce");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
			/* recv */
			mpi_errno = Capture_Edge(count,datatype,i,rank,
						 comm,"MPI_Allreduce");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Allreduce(sendbuf,recvbuf,count,datatype,
			      op,comm);
}

/*
MPI_Reduce_scatter - Combines values and scatters the results

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. recvcounts - integer array specifying the 
number of elements in result distributed to each process.
Array must be identical on all calling processes. 
. datatype - data type of elements of input buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - starting address of receive buffer (choice)
*/
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int *recvcnts,
		       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* my rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */
	rank = MD_Get_rank();

	/* -- Find the size -- */
	size = MD_Get_size();
	
	for (i=0; i < size; i++) {
		if (i != rank) {
			/* -- capturing the send and recv op -- */
			if (recvcnts[i] > 0) {
				/* send */
				mpi_errno = Capture_Edge(1,datatype,rank,i,
							 comm,"MPI_Reduce_scatter");
				if (mpi_errno != MPI_SUCCESS) {
					pthread_mutex_unlock(&lock);
					return mpi_errno;
				}
			}
			if (recvcnts[rank] > 0) {
				/* recv */
				mpi_errno = Capture_Edge(1,datatype,i,rank,
							 comm,"MPI_Reduce_scatter");
				if (mpi_errno != MPI_SUCCESS) {
					pthread_mutex_unlock(&lock);
					return mpi_errno;
				}
			}
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Reduce_scatter(sendbuf,recvbuf,recvcnts,
				   datatype,op,comm);
}

/*
MPI_Scan - Computes the scan (partial reductions) of data on a collection of
           processes
	(n^2, all to all)

Input Parameters:
+ sendbuf - starting address of send buffer (choice) 
. count - number of elements in input buffer (integer) 
. datatype - data type of elements of input buffer (handle) 
. op - operation (handle) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - starting address of receive buffer (choice) 
*/
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	     MPI_Op op, MPI_Comm comm) {

	int size;	/* Number of tasks */
	int mpi_errno;	/* errno returned by MPI */
	int i;		/* iterator */
	int rank;	/* my rank */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////

	/* -- Find my rank -- */
	rank = MD_Get_rank();

	/* -- Find the size -- */
	size = MD_Get_size();

	for (i=0; i < size; i++) {
		if (i != rank) {
			/* -- capturing the send and recv op -- */
			/* send */
			mpi_errno = Capture_Edge(count,datatype,rank,i,
						 comm,"MPI_Scan");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
			/* recv */
			mpi_errno = Capture_Edge(count,datatype,i,rank,
						 comm,"MPI_Scan");
			if (mpi_errno != MPI_SUCCESS) {
				pthread_mutex_unlock(&lock);
				return mpi_errno;
			}
		}
	}

	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	/* -- Calling the true MPI function -- */
	return PMPI_Scan(sendbuf,recvbuf,count,datatype,op,comm);
}

#endif

/*
  Capture_Request

   Captures the parameters of the function into the PROFIL_init
   structure

*/
int Capture_Request(const void *buf, int count, MPI_Datatype datatype, int source,
		    int dest, int tag, MPI_Comm comm, MPI_Request *request) {

	PROFIL_init_request *init_request;	/* To capture the call */

	/* -- Allocating memory for the new init_request -- */
	init_request =
		(PROFIL_init_request *) malloc(sizeof(PROFIL_init_request));
	if (!init_request) {
		fprintf(stderr,"Capture_Request: malloc() failed: \
				%s\n",strerror(errno));

		return MPI_ERR_OTHER;	
	}

	/* -- Filling data -- */
	init_request->count = count;
	init_request->datatype = datatype;
	init_request->source = source;
	init_request->dest = dest;
	init_request->comm = comm;
	init_request->request = request;
	init_request->active = 1;
	init_request->next = NULL;

	/* -- Inserting init_request at the end of the list -- */
	
	if (init_requests_size == 0) { // CASE 1 HEAD==TAIL==NULL
		init_req_head = init_request;
		init_req_tail = init_request;
	} else { // CASE 2 Updating only TAIL
		init_req_tail->next = init_request;
		init_req_tail = init_request;
	}
	init_requests_size++;

	return MPI_SUCCESS;
}

int PROFIL_Ext_Capture_edge(int count, MPI_Datatype datatype, int source,
				int dest, MPI_Comm comm) {
	
	int mpi_errno;          /* errno returned by MPI */

	//////////////////////////
	pthread_mutex_lock(&lock);
	//////////////////////////
	
	/* -- Capturing the recv operation -- */
	mpi_errno = Capture_Edge(count,datatype,source,dest,comm,"PROFIL_Ext_Capture_edge");
	if (mpi_errno != MPI_SUCCESS) {
		pthread_mutex_unlock(&lock);
		return mpi_errno;
	}
	
	////////////////////////////
	pthread_mutex_unlock(&lock);
	////////////////////////////

	return MPI_SUCCESS;
}

/*
  Search_request

  Searches all the init requests and returns the one
  with the pointer to request equal to request (input). 
*/
PROFIL_init_request *Search_request(MPI_Request *request) {

	int i;					/* Iterator */
	PROFIL_init_request *init_request;	/* Request requested :-o */

	init_request = init_req_head;
	for (i=0; i < init_requests_size; i++) {
		if ((init_request->request == request)
			&& init_request->active)
			break;
		init_request = init_request->next;
	}

	if (i == init_requests_size) /* not found! */
		return NULL;
	return init_request;

}


/*
  Free_Requests

  Frees all the memory allocated to store the PROFIL_init
  structure
*/
void Free_Requests(void) {

	int i;					/* Iterator */
	PROFIL_init_request *aux_pointer;	/* To help while freeing */

	/* Iteration over all objects in the list */
	for (i=0; i < init_requests_size; i++) {
		aux_pointer = init_req_head->next;
		free(init_req_head);
		init_req_head = aux_pointer;
	}

	init_requests_size = 0;
	init_req_head = NULL;
	init_req_tail = NULL;
}

/*
  Capture_Edge
  Auxiliary function to capture the interaction.
  To be called from the Send/Recv-type MPI profiling functions

Input Parameters:
. count - number of elements in send buffer (nonnegative integer)
. datatype - datatype of each send buffer element (handle)
. source - rank of the source (integer)
. dest - rank of destination (integer)
- comm - communicator (handle)
+ function_name - array with the name of the function calling
                  Capture send (char array ending in '\0') 
*/
int Capture_Edge(int count, 
		 MPI_Datatype datatype,
		 int source, 
		 int dest, 
		 MPI_Comm comm,
		 char *function_name) {

	int mpi_errno;          /* errno returned by MPI */
	int size;               /* size of the MPI data type used */
	MPI_Comm _comm;         /* communicator for profiling */
	
	MD_Get_comm(&_comm);
	if (_comm != comm)
		return MPI_SUCCESS;

	/* -- Get the size of the MPI data type used -- */
	mpi_errno = PMPI_Type_size(datatype,&size);
	if (mpi_errno != MPI_SUCCESS) {
		fprintf(stderr,"In profiling %s: PMPI_Type_size \
				failed -- mpi_errno=%d\n",
				function_name,mpi_errno);
		return mpi_errno;
	}
	size *= count; /* Bytes transferred */

	/** The edge discovered is "source-->dest" **/
	mpi_errno = MD_Add_local_edge(source,dest,size);

	if (mpi_errno < 0) {
		fprintf(stderr,"In profiling %s: MD_Add_local_edge \
				failed\n",function_name);
		mpi_errno = MPI_ERR_OTHER;
		return mpi_errno;
	}

	return MPI_SUCCESS;
}


