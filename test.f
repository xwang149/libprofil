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

c Program name
      program testf      

c Including headers
      include "mpif.h"

c Variables
      integer error
      integer id
      integer p
      integer i
      integer stats(MPI_STATUS_SIZE)
      CHARACTER*50 greeting 

c MPI Initialization
      call MPI_Init ( error )

c Get the number of processes
      call MPI_Comm_size ( MPI_COMM_WORLD, p, error )

c Get the rank
      call MPI_Comm_rank ( MPI_COMM_WORLD, id, error )

      write (greeting,'(a,i3)') 'My ID is ', id

c
c Print a message
c
      if ( id .eq. 0 ) then
         write (*,'(a)') ' '
         write (*,'(a)') 'HELLO_MPI - Master process:'
         write (*,'(a,i3)' ) ' The number of processes is ', p
         
         do i=1, p-1
            call MPI_Recv(greeting, 50, MPI_CHARACTER, i, 1,
     +                    MPI_COMM_WORLD, stats, error)
            write (6,*) greeting
         end do

      else
         call MPI_Send(greeting, 50, MPI_CHARACTER, 0, 1,
     +                 MPI_COMM_WORLD, error)
      end if

      if ( id .eq. 0 ) then
         write(*,'(a)') 'That is all for now'
      end if

c Terminate MPI
      call MPI_Finalize ( error )

c End of program
      stop
      end
