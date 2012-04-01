# Assignment 3 Comp445 Makefile
# Stephen Young
# id: 9736247
# st_youn@encs.concordia.ca

CPP  = g++.exe
LINKCLI = client.o
LINKSRV  = server.o
LINKRT = router.o
OBJ = $(LINKCLI) $(LINKSRV) $(LINKRT) protocol.o ftplib.o
LIBS = -lwsock32
BINSRV = server
BINCLI = client
BINRT = router
RM = rm -f

all: clean update

clean:
	${RM} $(OBJ) $(BINSRV) $(BINCLI)

update: server client router

server: server.o ftplib.o
	$(CPP) $(LINKSRV) -o $(BINSRV) $(LIBS)

client: client.o ftplib.o
	$(CPP) $(LINKCLI) -o $(BINCLI) $(LIBS)

router: router.o
	$(CPP) $(LINKRT) -o $(BINRT) $(LIBS)

client.o: client.cpp
	$(CPP) -c client.cpp -o $(LINKCLI) $(LIBS)

server.o: server.cpp
	$(CPP) -c server.cpp -o $(LINKSRV) $(LIBS)

ftplib.o: ftplib.cpp
	$(CPP) -c ftplib.cpp -o ftplib.o $(LIBS)

protocol.o: protocol.cpp
	$(CPP) -c protocol.cpp -o protocol.o $(LIBS)

router.o: router.cpp
	$(CPP) -c router.cpp -o router.o $(LIBS)