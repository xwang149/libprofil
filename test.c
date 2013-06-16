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

#include "mpi.h"
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#define NUMTASKS 8

int main (int argc, char *argv[]) {

	int numtasks, rank, len, rc;
	char hostname[MPI_MAX_PROCESSOR_NAME];
	int buffer[10];
	int buffer2[20];
	int buffer3[NUMTASKS*10];
	int buffer4[NUMTASKS*10 + (NUMTASKS-1)];
	int displs[NUMTASKS];
	int recvcounts[NUMTASKS];
	int i, mpi_errno;
	int rank__;
	MPI_Status status;

	MPI_Request request;

	rc = MPI_Init(&argc,&argv);
	if (rc != MPI_SUCCESS) {
		printf("Error starting MPI program. Termination.\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
	}
	
	MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Get_processor_name(hostname, &len);
	
	if (numtasks < NUMTASKS) {
		if (!rank)
			printf("I need at least %d tasks!!!\n",NUMTASKS);
		MPI_Finalize();
		return -1;
	}

	/*
	 * PT2PT
	 */

	if (!rank)
		printf("Testing MPI_Send and MPI_Recv between 0 and 1... ");

	/* Sending a buffer of 10 integers to process 1  */
	if (!rank) { // producer (rank == 0)
		for (i=0; i < 10; i++)
			buffer[i] = i;
		
		mpi_errno = MPI_Send(buffer,10,MPI_INT,1,1,MPI_COMM_WORLD);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Send!\n");
	}
	if (rank == 1) { // consumer (rank == 1)
		for (i=0; i < 10; i++)
			buffer[i] = -1;
		
		mpi_errno = MPI_Recv(buffer,10,MPI_INT,0,1,MPI_COMM_WORLD,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Recv!\n");	

		for (i=0; i < 10; i++) {
			if (buffer[i] != i)
				printf("??? buffer[%d]=%d\n",i,buffer[i]);
		}
	}

	if (!rank && mpi_errno == MPI_SUCCESS) {
		printf("OK\n");
		printf("Testing MPI_Sendrecv. Send between 0 and 1. Recv between 2 and 0... ");
	}
	
	if (!rank) { // (rank == 0)
		for (i=0; i < 10; i++) // producer 
			buffer[i] = i;
		for (i=0; i < 20; i++) // consumer
			buffer2[i] = -1;		

		mpi_errno = MPI_Sendrecv(buffer,10,MPI_INT,1,2,
					 buffer2,20,MPI_INT,2,3,MPI_COMM_WORLD,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Sendrecv!\n");
		
		for (i=0; i < 20; i++) {
			if (buffer2[i] != 20 - i)
				printf("??? buffer2[%d]=%d\n",i,buffer2[i]);
		}
	}
	if (rank == 1) { // (rank == 1)
		for (i=0; i < 10; i++) // consumer
			buffer[i] = -1;
		mpi_errno = MPI_Recv(buffer,10,MPI_INT,0,2,MPI_COMM_WORLD,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Recv!\n");
		
		for (i=0; i < 10; i++) {
			if (buffer[i] != i)
				printf("??? buffer[%d]=%d\n",i,buffer[i]);
		}
	}
	if (rank == 2) { // (rank == 2)
		for (i=0; i < 20; i++) // producer
			buffer2[i] = 20 - i;
		mpi_errno = MPI_Send(buffer2,20,MPI_INT,0,3,MPI_COMM_WORLD);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Send!\n");
	}

	if (!rank && mpi_errno == MPI_SUCCESS) {
		printf("OK\n");
	}
	
	if (!rank){
		printf("Testing MPI_Sendrecv_replace. Send between 0 and 1. Recv between 2 and 0... ");
	}
	if (!rank) { // (rank == 0)
		for (i=0; i < 10; i++) // producer (and consumer!)
			buffer[i] = i;
		
		mpi_errno = MPI_Sendrecv_replace(buffer,10,MPI_INT,1,4,2,5,MPI_COMM_WORLD,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Sendrecv!\n");

		for (i=0; i < 10; i++) {
			if (buffer[i] != 10 - i)
				printf("??? buffer[%d]=%d\n",i,buffer[i]);
		}
	}	
	if (rank == 1) { // (rank == 1)
		for (i=0; i < 10; i++) // consumer
			buffer[i] = -1;

		mpi_errno = MPI_Recv(buffer,10,MPI_INT,0,4,MPI_COMM_WORLD,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Recv!\n");

		for (i=0; i < 10; i++) {
			if (buffer[i] != i)
				printf("??? buffer[%d]=%d\n",i,buffer[i]);
		}
	}
	if (rank == 2) { // (rank == 2)
		for (i=0; i < 10; i++) // producer
			buffer[i] = 10 - i;
		
		mpi_errno = MPI_Send(buffer,10,MPI_INT,0,5,MPI_COMM_WORLD);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Send!\n");
	}

	if (!rank && mpi_errno == MPI_SUCCESS) {
		printf("OK\n");
	}

	if (!rank) {
		printf("Testing MPI_Irecv and MPI_Isend. Send between 0 and 1... ");
	}

        if (rank == 1) { // producer 
                for (i=0; i < 10; i++)
                        buffer[i] = i;

                mpi_errno = MPI_Isend(buffer,10,MPI_INT,0,6,MPI_COMM_WORLD,&request);
                if (mpi_errno != MPI_SUCCESS)
                        printf("Something went wrong in the MPI_Isend!\n");

		sleep(2);
		
		mpi_errno = MPI_Wait(&request,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Wait!\n");
        }
        if (rank == 0) { // consumer 
                for (i=0; i < 10; i++)
                        buffer[i] = -1;

                mpi_errno = MPI_Irecv(buffer,10,MPI_INT,1,6,MPI_COMM_WORLD,&request);
                if (mpi_errno != MPI_SUCCESS)
                        printf("Something went wrong in the MPI_Irecv!\n");

		mpi_errno = MPI_Wait(&request,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Wait!\n");

                for (i=0; i < 10; i++) {
                        if (buffer[i] != i)
                                printf("??? buffer[%d]=%d\n",i,buffer[i]);
                }
        }

	if (!rank && mpi_errno == MPI_SUCCESS)
		printf("OK\n");

	if (!rank) {
		printf("Testing MPI_Send_init and MPI_Recv_init. 0 --> 3... ");
	}
	
	if (rank == 0) { // producer
		for (i=0; i < 10; i++)
			buffer[i] = i;

		mpi_errno = MPI_Send_init(buffer,10,MPI_INT,
					  3,7,MPI_COMM_WORLD,
					  &request);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Send_init!\n");

		//sleep(3);
	
		mpi_errno = MPI_Start(&request);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Start!\n");

		mpi_errno = MPI_Wait(&request,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Wait!\n");

		mpi_errno = MPI_Request_free(&request);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Request_free!\n");
	}
	if (rank == 3) { // Consumer
		for (i=0; i < 10; i++)
			buffer[i] = -1;
	
		mpi_errno = MPI_Recv_init(buffer,10,MPI_INT,
					  0,7,MPI_COMM_WORLD,
					  &request);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Recv_init!\n");

		sleep(7);

		mpi_errno = MPI_Start(&request);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Start!\n");

		mpi_errno = MPI_Wait(&request,&status);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Wait!\n");

		mpi_errno = MPI_Request_free(&request);
		if (mpi_errno != MPI_SUCCESS)
			printf("Something went wrong in the MPI_Request_free!\n");
	}

	if (!rank && mpi_errno == MPI_SUCCESS)
		printf("OK\n");
	
	/*
	 * COLL
	 */

	if (!rank)
		printf("Testing MPI_Bcast. 4 to all processes... ");

	for(i=0; i < 10; i++) {
		if (rank == 4) { // producer
			buffer[i] = (int) pow((double)2,(double)i); // 2^i
		} else { // consumer
			buffer[i] = -1;
		}
	}	

	mpi_errno = MPI_Bcast(buffer,10,MPI_INT,4,MPI_COMM_WORLD);
	if (mpi_errno != MPI_SUCCESS)
		printf("Something went wrong in the MPI_Bcast!\n");
	
	if (rank != 4) {
		for (i=0; i < 10; i++)
			if (buffer[i] != (int) pow((double)2,(double)i)) // 2^i
				printf("??? buffer[%d]=%d\n",i,buffer[i]);
	}

	if (!rank && mpi_errno == MPI_SUCCESS)
		printf("OK\n");

	if (!rank)
		printf("Testing MPI_Gather. All to 5... ");

	if (rank == 5) // consumer
		for (i=0; i < NUMTASKS*10; i++)
			buffer3[i] = -1;
		
	
	// producer
	for (i=0; i < 10; i++)
		buffer[i] = (10*rank) + i;
	
	mpi_errno = MPI_Gather(buffer,10,MPI_INT,
			       buffer3,10,MPI_INT,5,MPI_COMM_WORLD);
	if (mpi_errno != MPI_SUCCESS)
		printf("Something went wrong in the MPI_Gather!\n");

	if (rank == 5) {
		rank__ = -1;
		for (i=0; i < NUMTASKS*10; i++) {			
			if (i % 10 == 0)
				rank__++;
			if (buffer3[i] != (10*rank__) + (i % 10))
				printf("??? buffer3[%d]=%d vs %d\n",i,buffer3[i],(10*rank__)+(i % 10));
		}
	}

	if (!rank && mpi_errno == MPI_SUCCESS)
		printf("OK\n");

	if (!rank)
		printf("Testing MPI_Gatherv. All to 5... ");

	if (rank == 5) { // consumer
		for (i=0; i < NUMTASKS*10 + (NUMTASKS-1); i++)
			buffer4[i] = -1;
		for (i=0; i < NUMTASKS; i++)
			displs[i] = (10*i) + i;
		for (i=0; i < NUMTASKS; i++)
			recvcounts[i] = 10;
	}

	// producer
	for (i=0; i < 10; i++)
		buffer[i] = (10*rank) + (10 - i);
	
	mpi_errno = MPI_Gatherv(buffer,10,MPI_INT,
				buffer4,recvcounts,displs,
				MPI_INT,5,MPI_COMM_WORLD);
	if (mpi_errno != MPI_SUCCESS)
		printf("Something went wrong in the MPI_Gatherv!\n");
	
/*	if (rank == 5) {
		for (i=0; i < NUMTASKS*10 + (NUMTASKS-1); i++)
			printf("buffer4[%d]=%d\n",i,buffer4[i]);		
	}
*/

	if (!rank && mpi_errno == MPI_SUCCESS)
		printf("OK\n");

	if (!rank)
		printf("Testing MPI_Scatter. 6 to all... ");

	if (rank == 6) {// producer
		for (i=0; i < NUMTASKS; i++)
			buffer[i] = i;
	}
	// consumer
	buffer2[0] = -1;
	
	mpi_errno = MPI_Scatter(buffer,1,MPI_INT,
				buffer2,1,MPI_INT,6,MPI_COMM_WORLD);
	if (mpi_errno != MPI_SUCCESS)
		printf("Something went wrong in the MPI_Scatter!\n");

	if (buffer2[0] != rank)
		printf("??? result=%d vs %d\n",buffer2[0],rank);

	if (!rank && mpi_errno==MPI_SUCCESS)
		printf("OK\n");

	if (!rank)
		printf("Testing MPI_Alltoall. all to all... ");
	
	for (i=0; i < NUMTASKS; i++) { // to send
		buffer[i] = i;
	}
	for (i=0; i < NUMTASKS; i++) { // to recv
		buffer2[i] = -1;
	}

	mpi_errno = MPI_Alltoall(buffer,1,MPI_INT,buffer2,1,MPI_INT,MPI_COMM_WORLD);
	if (mpi_errno != MPI_SUCCESS)
		printf("Something went wrong in the MPI_Alltoall\n");

	// all processes should have an array of size NUMTASKS with its
	// rank repeated all over. For example, for process 3:
	// buffer2[] = [3, 3, 3, ..., 3]

	for (i=0; i < NUMTASKS; i++)
		if (buffer2[i] != rank)
			printf("??? buffer2[%d]=%d (for tasks %d)\n",i,buffer2[i],rank);

	if (!rank && mpi_errno==MPI_SUCCESS)
		printf("OK\n");

	MPI_Finalize();

	return 0;
}
