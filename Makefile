
CFLAGS  = -g
CC = gcc 

#-----------------------------------------------------------------------------#
all : client server

rClient: client
	sudo ./client

rServer: server
	sudo ./server

client: client.o RawSocket.o

server: server.o RawSocket.o -lm

#-----------------------------------------------------------------------------#

clean :
	$(RM) *.o

#-----------------------------------------------------------------------------#

purge:
	$(RM) client server *.o