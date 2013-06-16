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
 
/* Implementation of MPI profiling functions 
 * to be called from FORTRAN
 *
 * */

#include "profil.h"
#include "md.h"

/*
 * NOTES:
 * Fortran functions are subroutines.
 * They don't return anything.
 * Error codes are returned as an
 * output argument "int *ierr".
 * Also, all arguments from Fortran are
 * pointers.
 *
 */

/* MPI_Init */
void mpi_f_init_ (int *ierr) {
	int argc = 0;	/* No arguments in fortran */
	char **argv;	/* Vector of arguments */
	*ierr = MPI_Init(&argc,&argv);
}

/* MPI_Comm_size */
void mpi_f_comm_size_ (MPI_Comm *comm, int *size, int *ierr) {
	*ierr = MPI_Comm_size(*comm,size);
}

/* MPI_Comm_rank */
void mpi_f_comm_rank_ (MPI_Comm *comm, int *rank, int *ierr) {
	*ierr = MPI_Comm_rank(*comm,rank);
}

/* MPI_Finalize */
void mpi_f_finalize_ (int *ierr) {
	*ierr = MPI_Finalize();
}

/* MPI_Send */
void mpi_f_send_ (void *buf, int *count, MPI_Datatype *datatype, int *dest,
						int *tag, MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Send(buf,*count,*datatype,*dest,*tag,*comm);
}

/* MPI_Recv */
void mpi_f_recv_ (void *buf, int *count, MPI_Datatype *datatype, int *source,
						int *tag, MPI_Comm *comm, MPI_Status *status, int *ierr) {
	*ierr = MPI_Recv(buf,*count,*datatype,*source,*tag,*comm,status);
}

/* MPI_Sendrecv */
void mpi_f_sendrecv_ (void *sendbuf, int *sendcount, MPI_Datatype *sendtype,
						int *dest, int *sendtag,
						void *recvbuf, int *recvcount, MPI_Datatype *recvtype,
						int *source, int *recvtag,
						MPI_Comm *comm, MPI_Status *status, int *ierr) {
	*ierr = MPI_Sendrecv(sendbuf,*sendcount,*sendtype,*dest,*sendtag,
								recvbuf,*recvcount,*recvtype,*source,*recvtag,
								*comm,status);
}

/* MPI_Sendrecv_replace */
void mpi_f_sendrecv_replace_ (void *buf, int *count, MPI_Datatype *datatype,
									int *dest, int *sendtag, int *source,
									int *recvtag, MPI_Comm *comm, MPI_Status *status,
									int *ierr) {
	*ierr = MPI_Sendrecv_replace(buf,*count,*datatype,*dest,*sendtag,*source,
									*recvtag,*comm,status);
}

/* MPI_Bsend */
void mpi_f_bsend_ (void *buf, int *count, MPI_Datatype *datatype, int *dest,
						int *tag, MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Bsend(buf,*count,*datatype,*dest,*tag,*comm);
}

/* MPI_Ssend */
void mpi_f_ssend_ (void *buf, int *count, MPI_Datatype *datatype, int *dest,
						int *tag, MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Ssend(buf,*count,*datatype,*dest,*tag,*comm);
}

/* MPI_Rsend */
void mpi_f_rsend_ (void *buf, int *count, MPI_Datatype *datatype, int *dest,
						int *tag, MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Rsend(buf,*count,*datatype,*dest,*tag,*comm);
}



/* MPI_Irecv */
void mpi_f_irecv_ (void *buf, int *count, MPI_Datatype *datatype, int *source,
                                                int *tag, MPI_Comm *comm, MPI_Request *request, int *ierr) {
	int rank;

	rank = MD_Get_rank();
	PROFIL_Ext_Capture_edge(*count,*datatype,*source,rank,*comm);
}



/* MPI_Isend */
void mpi_f_isend_ (void *buf, int *count, MPI_Datatype *datatype, int *dest,
						int *tag, MPI_Comm *comm, MPI_Request *request, int *ierr) {
	int rank;

	rank = MD_Get_rank();
	PROFIL_Ext_Capture_edge(*count,*datatype,rank,*dest,*comm);
}

/* MPI_Ibsend */
void mpi_f_ibsend_ (void *buf, int *count, MPI_Datatype *datatype, int *dest,
						int *tag, MPI_Comm *comm, MPI_Fint *request, int *ierr) {
	int rank;

	rank = MD_Get_rank();
	PROFIL_Ext_Capture_edge(*count,*datatype,rank,*dest,*comm);
}

/* MPI_Issend */
void mpi_f_issend_ (void *buf, int *count, MPI_Datatype *datatype, int *dest,
						int *tag, MPI_Comm *comm, MPI_Fint *request, int *ierr) {
	int rank;

	rank = MD_Get_rank();
	PROFIL_Ext_Capture_edge(*count,*datatype,rank,*dest,*comm);
}

/* MPI_Irsend */
void mpi_f_irsend_ (void *buf, int *count, MPI_Datatype *datatype, int *dest,
						int *tag, MPI_Comm *comm, MPI_Fint *request, int *ierr) {
	int rank;

	rank = MD_Get_rank();
	PROFIL_Ext_Capture_edge(*count,*datatype,rank,*dest,*comm);
}

/* MPI_Send_init */
void mpi_f_send_init_ (void *buf, int *count, MPI_Datatype *datatype,
						int *dest, int *tag, MPI_Comm *comm,
						MPI_Request *request, int *ierr) {
	*ierr = MPI_Send_init(buf,*count,*datatype,*dest,*tag,*comm,request);
}

/* MPI_Bsend_init */
void mpi_f_bsend_init_ (void *buf, int *count, MPI_Datatype *datatype,
						int *dest, int *tag, MPI_Comm *comm, 
						MPI_Request *request, int *ierr) {
	*ierr = MPI_Bsend_init(buf,*count,*datatype,*dest,*tag,*comm,request);
}

/* MPI_Ssend_init */
void mpi_f_ssend_init_ (void *buf, int *count, MPI_Datatype *datatype,
						int *dest, int *tag, MPI_Comm *comm, MPI_Request *request,
						int *ierr) {
	*ierr = MPI_Ssend_init(buf,*count,*datatype,*dest,*tag,*comm,request);
}

/* MPI_Rsend_init */
void mpi_f_rsend_init_ (void *buf, int *count, MPI_Datatype *datatype,
						int *dest, int *tag, MPI_Comm *comm, MPI_Request *request,
						int *ierr) {
	*ierr = MPI_Rsend_init(buf,*count,*datatype,*dest,*tag,*comm,request);
}						

/* MPI_Recv_init */
void mpi_f_recv_init_ (void *buf, int *count, MPI_Datatype *datatype,
							int *source, int *tag, MPI_Comm *comm,
							MPI_Request *request, int *ierr) {
	*ierr = MPI_Recv_init(buf,*count,*datatype,*source,*tag,*comm,request);
}

/* MPI_Start */
void mpi_f_start_ (MPI_Request *request, int *ierr) {
	*ierr = MPI_Start(request);
}

/* MPI_Startall */
void mpi_f_startall_ (int *count, MPI_Request array_of_requests[], int *ierr) {
	*ierr = MPI_Startall(*count, array_of_requests);	
}

/* MPI_Request_free */
void mpi_f_request_free_ (MPI_Fint *request, int *ierr) {
	*ierr = MPI_Request_free(request);
} 

/* MPI_Bcast */
void mpi_f_bcast_ (void *buffer, int *count, MPI_Datatype *datatype,
						int *root, MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Bcast(buffer,*count,*datatype,*root,*comm);
}

/* MPI_Gather */
void mpi_f_gather_ (void *sendbuf, int *sendcnt, MPI_Datatype *sendtype,
						void *recvbuf, int *recvcnt, MPI_Datatype *recvtype,
						int *root, MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Gather(sendbuf,*sendcnt,*sendtype,recvbuf,*recvcnt,
								*recvtype,*root,*comm);
}

/* MPI_Gatherv */
void mpi_f_gatherv_ (void *sendbuf, int *sendcnt, MPI_Datatype *sendtype,
						void *recvbuf, int *recvcnts, int *displs,
						MPI_Datatype *recvtype, int *root, MPI_Comm *comm,
						int *ierr) {
	*ierr = MPI_Gatherv(sendbuf,*sendcnt,*sendtype,recvbuf,recvcnts,
							  displs,*recvtype,*root,*comm);
}

/* MPI_Scatter */
void mpi_f_scatter_ (void *sendbuf, int *sendcnt, MPI_Datatype *sendtype,
						void *recvbuf, int *recvcnt, MPI_Datatype *recvtype,
						int *root, MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Scatter(sendbuf,*sendcnt,*sendtype,recvbuf,*recvcnt,
							  *recvtype,*root,*comm);
}

/* MPI_Scatterv */
void mpi_f_scatterv_ (void *sendbuf, int *sendcnts, int *displs,
							MPI_Datatype *sendtype, void *recvbuf, int *recvcnt,
							MPI_Datatype *recvtype, int *root, MPI_Comm *comm,
							int *ierr) {
	*ierr = MPI_Scatterv(sendbuf,sendcnts,displs,*sendtype,recvbuf,*recvcnt,
							*recvtype,*root,*comm);
}

/* MPI_Allgather */
void mpi_f_allgather_ (void *sendbuf, int *sendcount, MPI_Datatype *sendtype,
							void *recvbuf, int *recvcount, MPI_Datatype *recvtype,
							MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Allgather(sendbuf,*sendcount,*sendtype,recvbuf,*recvcount,
							*recvtype,*comm);
}

/* MPI_Allgatherv */
void mpi_f_allgatherv_ (void *sendbuf, int *sendcount, MPI_Datatype *sendtype,
							void *recvbuf, int *recvcounts, int *displs,
							MPI_Datatype *recvtype, MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Allgatherv(sendbuf,*sendcount,*sendtype,recvbuf,recvcounts,
							displs,*recvtype,*comm);
}

/* MPI_Alltoall */
void mpi_f_alltoall_ (void *sendbuf, int *sendcount, MPI_Datatype *sendtype,
							void *recvbuf, int *recvcount, MPI_Datatype *recvtype,
							MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Alltoall(sendbuf,*sendcount,*sendtype,recvbuf,*recvcount,
							*recvtype,*comm);
}

/* MPI_Alltoallv */
void mpi_f_alltoallv_ (void *sendbuf, int *sendcnts, int *sdispls,
							MPI_Datatype *sendtype, void *recvbuf, int *recvcnts,
							int *rdispls, MPI_Datatype *recvtype, MPI_Comm *comm,
							int *ierr) {
	*ierr = MPI_Alltoallv(sendbuf,sendcnts,sdispls,*sendtype,recvbuf,
							recvcnts,rdispls,*recvtype,*comm);
}

/* MPI_Reduce */
void mpi_f_reduce_ (void *sendbuf, int *recvbuf, int *count, 
							MPI_Datatype *datatype, MPI_Op *op, int *root,
							MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Reduce(sendbuf,recvbuf,*count,*datatype,*op,*root,*comm);
}

/* MPI_Allreduce */
void mpi_f_allreduce_ (void *sendbuf, void *recvbuf, int *count,
							MPI_Datatype *datatype, MPI_Op *op, 
							MPI_Comm *comm, int *ierr) {
	*ierr = MPI_Allreduce(sendbuf,recvbuf,*count,*datatype,*op,*comm);
}

/* MPI_Reduce_scatter */
void mpi_f_reduce_scatter_ (void *sendbuf, void *recvbuf, int *recvcnts,
							MPI_Datatype *datatype, MPI_Op *op, MPI_Comm *comm,
							int *ierr) {
	*ierr = MPI_Reduce_scatter(sendbuf,recvbuf,recvcnts,*datatype,*op,
							*comm);
}

/* MPI_Scan */
void mpi_f_scan_ (void *sendbuf, void *recvbuf, int *count, 
						 MPI_Datatype *datatype, MPI_Op *op, MPI_Comm *comm,
						 int *ierr) {
	*ierr = MPI_Scan(sendbuf,recvbuf,*count,*datatype,*op,*comm);
}

