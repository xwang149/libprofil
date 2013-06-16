LibProfil
=========

Libprofil is a lightweight MPI profiling library intended to "discover" the
topology of MPI applications when this is irregular or not known in advance.
Although the library should work on any MPI implementation that supports
MPI 3.0, I have only tested it on mpich and mvapich. 
If you find any issues with other MPI implementation, please send me an
email.

COMPILE/RUN
-----------

To compile just run:

    $ make

To compile the provided test code, run:

    $ make test

You may want to edit Makefile to point the $(MPI) variable to where your
MPI implementation is installed in your system. 

Also, you can define some options at compile time by setting the $(FLAGS)
variable. For example, you can turn off profiling on collective MPI
functions by adding
    
    -DNOCOLLECTIVES

to $(FLAGS). Please, check Makefile for more info.

You can test that it works by running the tests provided:

    $ mpirun -np 8 ./testc 
    Testing MPI_Send and MPI_Recv between 0 and 1... OK
    Testing MPI_Sendrecv. Send between 0 and 1. Recv between 2 and 0... OK
    Testing MPI_Sendrecv_replace. Send between 0 and 1. Recv between 2 and 0... OK
    Testing MPI_Irecv and MPI_Isend. Send between 0 and 1... OK
    Testing MPI_Send_init and MPI_Recv_init. 0 --> 3... OK
    Testing MPI_Bcast. 4 to all processes... OK
    Testing MPI_Gather. All to 5... OK
    Testing MPI_Gatherv. All to 5... OK
    Testing MPI_Scatter. 6 to all... OK
    Testing MPI_Alltoall. all to all... OK

You should have a "topology.txt" file on the same directory:

    $ cat topology.txt 
    8
    0 1 124
    0 2 4
    0 3 44
    0 4 4
    0 5 84
    0 6 4
    0 7 4
    1 0 44
    1 2 4
    1 3 4
    1 4 4
    1 5 84
    ... etc

The meaning of the file is the following: The first line give us the
number of processes. Then, for each line, we have three columns. The
first is the source of MPI messages. The second, corresponds to the
receiver of MPI messages. Finally, the last column gives us the total
number of bytes transmitted from source to receiver.

To use the library with your own code, you just need to link it to your 
MPI code like this:

FOR C CODE:

    $ mpicc -o myMPIcode myMPIcode.c -L/path/to/libprofil -lprofil

FOR FORTRAN 77 CODE:

    $ mpif77 -o mpiMPIcode myMPIcode.f -L/path/to/libprofil -lprofilf77 -lprofil

OTHER COMMUNICATORS
-------------------

If the library is used as such (without any modification of the original MPI
source code), it will only capture the topology for the processes in
the MPI_COMM_WORLD communicator. If you desire to capture other communicators,
you will need to modify your MPI code to call the Map Discover (MD) library
directly adding:

    #import "md.h" 

to your code. 

You can check the API on profil/md.h. The relevant function calls 
are:

    void MD_Set_comm(MPI_Comm comm);

    void MD_Set_rank(int rank);

    void MD_Set_size(int size);

Realize that it is the responsibility of the MPI program to set all three
pieces of information in the MD library. If only the communicator is set,
the results will be non-sense.

For now, this library only supports profiling one communicator at a
time. TODO for later versions is to add profiling for multiple
communicators at the same time.

ADVANCED FUNCTIONALITY
----------------------

It is possible to do more advanced stuff. For example, it is possible to
Stop the profiling for a while and start it again later on. Two functions
are defined for that:

    void MD_Start_capturing(void);

    void MD_Stop_capturing(void);

You can also reset the data structure any time:

    void MD_Reset_data(void);

You can also print the current topology any time, or return it to a
variable in memory. For more details, please read the comments on
the code in profil/md.c

###
