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


#ifndef _PROFIL_H
#define _PROFIL_H

#include "mpi.h"

/* -- Definition of data structures -- */

/*

PROGIL_init_request stores information about
an init request (that's it, it captures calls
to MPI_*_init functions). This info is used later
when the application calls MPI_Start to capture
the appropiate messages.

*/
typedef struct PROFIL_init_request {
	int count;				/* number of elements sent */
	MPI_Datatype datatype;			/* type of each element */
	int source;				/* rank of source */
	int dest;				/* rank of destination */
	MPI_Comm comm;				/* communicator */
	MPI_Request *request;			/* communication request */
	int active;				/* 1 if active, 0 otw */
	
	struct PROFIL_init_request *next;	/* next in the list */
} PROFIL_init_request;

/* -- Functions -- */
int PROFIL_Ext_Capture_edge(int count, MPI_Datatype datatype,
				int source, int dest, MPI_Comm comm);

#endif
