#
# Copyright 2013 Illinois Institute of Technology 
# http://bluesky.cs.iit.edu/libprofil
# email: eberroca@iit.edu
#
 
# This file is part of Libprofil.
#
#    libprofil is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Libprofil is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Libprofil.  If not, see <http://www.gnu.org/licenses/>.
# 


# Makefile

MPI=/opt/mpich-install

CC=$(MPI)/bin/mpicc
F77=$(MPI)/bin/mpif77
INCLUDE=$(MPI)/include

PROFILDIR=./profil
PROFILF77DIR=./profilf

OBJS=$(PROFILDIR)/md.o $(PROFILDIR)/profil.o $(PROFILDIR)/fbridge.o $(PROFILF77DIR)/profil.o ./test/test.o ./test/testf.o

################
# Options are:
# -DNOCOLLECTIVES : This will turn off profiling on collective MPI routines (i.e. MPI_Bcast())

FLAGS=
################

LDCFLAGS=-L. -lprofil -lm -lpthread
LDF77FLAGS=-L. -lprofilf77
#INCFLAGS=-I$(INCLUDE)

#PARMETIS=/home/eberroca/ParMetis-3.1.1

all : clibrary flibrary

clibrary : $(PROFILDIR)/profil.o $(PROFILDIR)/md.o
#	$(CC) $(FLAGS) $(INCFLAGS) -shared -o libprofil.so $(PROFILDIR)/profil.o $(PROFILDIR)/md.o
	ar -cvq libprofil.a $(PROFILDIR)/profil.o $(PROFILDIR)/md.o

flibrary : $(PROFILF77DIR)/profil.o $(PROFILDIR)/profil.o $(PROFILDIR)/md.o $(PROFILDIR)/fbridge.o
#	$(F77) $(FLAGS) $(INCFLAGS) -shared -o libprofilf77.so $(PROFILDIR)/profil.o $(PROFILF77DIR)/profil.o $(PROFILDIR)/md.o $(PROFILDIR)/fbridge.o
	ar -cvq libprofilf77.a $(PROFILF77DIR)/profil.o $(PROFILDIR)/profil.o $(PROFILDIR)/md.o $(PROFILDIR)/fbridge.o

$(PROFILDIR)/%.o : $(PROFILDIR)/%.c
	$(CC) $(FLAGS) $(INCFLAGS) -Wall -fPIC -o $@ -c $^

$(PROFILF77DIR)/%.o : $(PROFILF77DIR)/%.f
	$(F77) $(FLAGS) $(INCFLAGS) -Wall -fPIC -o $@ -c $^

test : flibrary clibrary 
	$(CC) $(FLAGS) $(INCFLAGS) test.c -o testc $(LDCFLAGS)
	$(F77) $(FLAGS) $(INCFLAGS) test.f -o testf $(LDF77FLAGS) $(LDCFLAGS)

clean :
	rm -fr $(OBJS) libprofil.a libprofilf77.a testc testf topology*
