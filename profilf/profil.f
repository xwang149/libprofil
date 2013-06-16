c
c Copyright 2013 Illinois Institute of Technology
c http://bluesky.cs.iit.edu/libprofil
c email: eberroca@iit.edu
c

c This file is part of Libprofil.
c
c    libprofil is free software: you can redistribute it and/or modify
c    it under the terms of the GNU General Public License as published by
c    the Free Software Foundation, either version 3 of the License, or
c    (at your option) any later version.
c
c    Libprofil is distributed in the hope that it will be useful,
c    but WITHOUT ANY WARRANTY; without even the implied warranty of
c    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
c    GNU General Public License for more details.
c
c    You should have received a copy of the GNU General Public License
c    along with Libprofil.  If not, see <http://www.gnu.org/licenses/>.
c


c
c Fortran routines to profil MPI calls.
c They directly call the fortran bridge
c functions to use the C version of the
c library.
c

      subroutine MPI_Init ( ierr )
         call MPI_F_Init(ierr)
      end

      subroutine MPI_Comm_size ( comm, numproc, ierr )
         call MPI_F_Comm_size(comm,numproc,ierr)
      end

      subroutine MPI_Comm_rank ( comm, id, ierr )
         call MPI_F_Comm_rank(comm,id,ierr)
      end

      subroutine MPI_Finalize ( ierr )
         call MPI_F_Finalize(ierr)
      end

      subroutine MPI_Send ( buf, cnt, datatype, dest, tag, comm, ierr )
         call MPI_F_Send(buf,cnt,datatype,dest,tag,comm,ierr)
      end

      subroutine MPI_Recv ( buf, cnt, datatype, source, tag, comm,
     +                      stats, ierr)
         call MPI_F_Recv(buf,cnt,datatype,source,tag,comm,stats,ierr)
      end

      subroutine MPI_Sendrecv ( sendbuf, sendcount, sendtype, dest,
     +                          sendtag, recvbuf, recvcount, recvtype,
     +                          source, recvtag, comm, stats, ierr )
         call MPI_F_Sendrecv(sendbuf,sendcount,sendtype,dest,sendtag,
     +                       recvbuf,recvcount,recvtype,source,recvtag,
     +                       comm,stats,ierr)
      end

      subroutine MPI_Sendrecv_replace ( buf, cnt, datatype, dest,
     +                                  sendtag, source, recvtag,
     +                                  comm, stats, ierr )
         call MPI_F_Sendrecv_replace(buf,cnt,datatype,dest,sendtag,
     +                               source,recvtag,comm,stats,ierr)
      end

      subroutine MPI_Bsend ( buf, cnt, datatype, dest, tag, comm, ierr )
         call MPI_F_Bsend(buf,cnt,datatype,dest,tag,comm,ierr)
      end

      subroutine MPI_Ssend ( buf, cnt, datatype, dest, tag, comm, ierr )
         call MPI_F_Ssend (buf,cnt,datatype,dest,tag,comm,ierr)
      end

      subroutine MPI_Rsend ( buf, cnt, datatype, dest, tag, comm, ierr )
         call MPI_F_Rsend(buf,cnt,datatype,dest,tag,comm,ierr)
      end

      subroutine MPI_Irecv ( buf, cnt, datatype, source, tag, comm,
     +                       request, ierr )
         call MPI_Irecv_(buf,cnt,datatype,source,tag,comm,request,ierr)
         call MPI_F_Irecv(buf,cnt,datatype,source,tag,comm,request,ierr)
      end


      subroutine MPI_Isend ( buf, cnt, datatype, dest, tag, comm,
     +                       request, ierr )
         call MPI_Isend_(buf,cnt,datatype,dest,tag,comm,request,ierr)
         call MPI_F_Isend(buf,cnt,datatype,dest,tag,comm,request,ierr)
      end

      subroutine MPI_Ibsend ( buf, cnt, datatype, dest, tag, comm,
     +                        request, ierr )
         call MPI_Ibsend_(buf,cnt,datatype,dest,tag,comm,request,ierr)
         call MPI_F_Ibsend(buf,cnt,datatype,dest,tag,comm,request,ierr)
      end

      subroutine MPI_Issend ( buf, cnt, datatype, dest, tag, comm,
     +                        request, ierr )
         call MPI_Issend_(buf,cnt,datatype,dest,tag,comm,request,ierr)
         call MPI_F_Issend(buf,cnt,datatype,dest,tag,comm,request,ierr)
      end

      subroutine MPI_Irsend ( buf, cnt, datatype, dest, tag, comm,
     +                        request, ierr )
         call MPI_Irsend_(buf,cnt,datatype,dest,tag,comm,request,ierr)
         call MPI_F_Irsend(buf,cnt,datatype,dest,tag,comm,request,ierr)
      end

      subroutine MPI_Send_init ( buf, cnt, datatype, dest, tag, comm,
     +                           request, ierr )
         call MPI_F_Send_init(buf,cnt,datatype,dest,tag,comm,request,
     +                        ierr)
      end

      subroutine MPI_Bsend_init ( buf, cnt, datatype, dest, tag, comm,
     +                            request, ierr )
         call MPI_F_Bsend_init(buf,cnt,datatype,dest,tag,comm,request,
     +                         ierr)
      end

      subroutine MPI_Ssend_init ( buf, cnt, datatype, dest, tag, comm,
     +                            request, ierr )
         call MPI_F_Ssend_init(buf,cnt,datatype,dest,tag,comm,request,
     +                         ierr)
      end

      subroutine MPI_Rsend_init ( buf, cnt, datatype, dest, tag, comm,
     +                            request, ierr )
         call MPI_F_Rsend_init(buf,cnt,datatype,dest,tag,comm,request,
     +                         ierr)
      end

      subroutine MPI_Recv_init ( buf, cnt, datatype, source, tag, comm,
     +                           request, ierr )
         call MPI_F_Recv_init(buf,cnt,datatype,source,tag,comm,request,
     +                        ierr)
      end

      subroutine MPI_Start ( request, ierr )
         call MPI_F_Start(request,ierr)
      end

      subroutine MPI_Startall ( cnt, array_of_requests, ierr )
         call MPI_F_Startall(cnt,array_of_requests,ierr) 
      end

      subroutine MPI_Request_free ( request, ierr )
         call MPI_F_Request_free(request,ierr)
      end

      subroutine MPI_Bcast ( buffer, cnt, datatype, root, comm, ierr )
         call MPI_F_Bcast(buffer,cnt,datatype,root,comm,ierr)
      end

      subroutine MPI_Gather ( sendbuf, sendcnt, sendtype, recvbuf,
     +                        recvcnt, recvtype, root, comm, ierr )
         call MPI_F_Gather(sendbuf,sendcnt,sendtype,recvbuf,recvcnt,
     +                     recvtype,root,comm,ierr)
      end

      subroutine MPI_Gatherv ( sendbuf, sendcnt, sendtype, recvbuf,
     +                         recvcnts, displs, recvtype, root,
     +                         comm, ierr )
         call MPI_F_Gatherv(sendbuf,sendcnt,sendtype,recvbuf,recvcnts,
     +                      displs,recvtype,root,comm,ierr)
      end

      subroutine MPI_Scatter ( sendbuf, sendcnt, sendtype, recvbuf,
     +                         recvcnt, recvtype, root, comm, ierr )
         call MPI_F_Scatter(sendbuf,sendcnt,sendtype,recvbuf,recvcnt,
     +                      recvtype,root,comm,ierr)
      end

      subroutine MPI_Scatterv ( sendbuf, sendcnts, displs, sendtype,
     +                          recvbuf, recvcnt, recvtype, root, comm,
     +                          ierr )
         call MPI_F_Scatterv(sendbuf,sendcnts,displs,sendtype,recvbuf,
     +                       recvcnt,recvtype,root,comm,ierr)
      end

      subroutine MPI_Allgather ( sendbuf, sendcount, sendtype, recvbuf,
     +                           recvcount, recvtype, comm, ierr )
         call MPI_F_Allgather(sendbuf,sendcount,sendtype,recvbuf,
     +                        recvcount,recvtype,comm,ierr)
      end

      subroutine MPI_Allgatherv ( sendbuf, sendcount, sendtype, recvbuf,
     +                            recvcounts, displs, recvtype, comm,
     +                            ierr )
         call MPI_F_Allgatherv(sendbuf,sendcount,sendtype,recvbuf,
     +                         recvcounts,displs,recvtype,comm,ierr)
      end

      subroutine MPI_Alltoall ( sendbuf, sendcount, sendtype, recvbuf,
     +                          recvcount, recvtype, comm, ierr )
         call MPI_F_Alltoall(sendbuf,sendcount,sendtype,recvbuf,
     +                       recvcount,recvtype,comm,ierr)
      end

      subroutine MPI_Alltoallv ( sendbuf, sendcnts, sdispls, sendtype,
     +                           recvbuf, recvcnts, rdispls, recvtype,
     +                           comm, ierr )
         call MPI_F_Alltoallv(sendbuf,sendcnts,sdispls,sendtype,recvbuf,
     +                        recvcnts,rdispls,recvtype,comm,ierr)
      end

      subroutine MPI_Reduce ( sendbuf, recvbuf, cnt, datatype, op, root,
     +                        comm, ierr )
         call MPI_F_Reduce(sendbuf,recvbuf,cnt,datatype,op,root,comm,
     +                     ierr)
      end

      subroutine MPI_Allreduce ( sendbuf, recvbuf, cnt, datatype, op,
     +                           comm, ierr )
         call MPI_F_Allreduce(sendbuf,recvbuf,cnt,datatype,op,comm,ierr)
      end

      subroutine MPI_Reduce_scatter ( sendbuf, recvbuf, recvcnts,
     +                                datatype, op, comm, ierr )
         call MPI_F_Reduce_scatter(sendbuf,recvbuf,recvcnts,datatype,
     +                             op,comm,ierr)
      end

      subroutine MPI_Scan ( sendbuf, recvbuf, cnt, datatype, op, comm,
     +                      ierr )
         call MPI_F_Scan(sendbuf,recvbuf,cnt,datatype,op,comm,ierr)
      end

