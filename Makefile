##Adapted from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -I. -O3
OBJ = server.o 
EXE = server

##Create .o files from .c files. Searches for .c files with same .o names given in OBJ
%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

##Create executable linked file from object files. 
$(EXE): $(OBJ)
	gcc -o $@ $^ $(CFLAGS) -lpthread

##Delete object files
clean:
	/bin/rm $(OBJ)
	/bin/rm $(EXE) 
