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


#ifndef _MD_H
#define _MD_H

#include "mpi.h"
#include <string.h>
#include <stdio.h>

//#define MD_SIZEVECTORS 2000000

/* -- Definition of the data structure -- */

typedef struct MD_edge {
	int rank;
	long long total_msg, total_bytes;
} MD_edge;

/* -- Function prototypes -- */

int MD_Init(MPI_Comm comm, int rank, int size);

void MD_Start_capturing(void);

void MD_Stop_capturing(void);

void MD_Finalize(void);

void MD_Reset_data(void);

int MD_Add_local_edge(int org, int dest, int bytes);

int MD_Print_local_map(FILE *fd);

int MD_Return_local_map(MD_edge **in_edges, 
			int *in_degree,
			MD_edge **out_edges,
			int *out_degree);

int MD_Return_map_buffer(long long **conn_buffer);

int MD_Print_map(FILE *fd, const long long *topology_matrix);

int MD_Print_SparseMap(FILE *fd, const long long *topology_matrix);

void MD_Set_comm(MPI_Comm comm);

void MD_Get_comm(MPI_Comm *comm);

void MD_Set_rank(int rank);

int MD_Get_rank(void);

void MD_Set_size(int size);

int MD_Get_size(void);

int MD_Is_initialized(void);

#endif
