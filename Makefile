# Assignment 3 Comp445 Makefile
# Stephen Young
# id: 9736247
# st_youn@encs.concordia.ca

CPP  = g++.exe
LINKCLI = client.o
LINKSRV  = server.o
LINKRT = router.o
OBJ = $(LINKCLI) $(LINKSRV) protocol.o ftplib.o socketlib.o
LIBS = -lwsock32
BINSRV = server
BINCLI = client
BINRT = router
RM = rm -f

all: sync update

clean:
	${RM} $(OBJ) $(BINSRV) $(BINCLI)

sync:
	git pull origin master

update: server client

client: client.o libraries
	$(CPP) $(LINKCLI) -o $(BINCLI) $(LIBS)

client.o: client.cpp
	$(CPP) -c client.cpp -o $(LINKCLI) $(LIBS)

server: server.o libraries
	$(CPP) $(LINKSRV) -o $(BINSRV) $(LIBS)

server.o: server.cpp
	$(CPP) -c server.cpp -o $(LINKSRV) $(LIBS)

router: router.o
	$(CPP) $(LINKRT) -o $(BINRT) $(LIBS)

router.o: router.cpp
	$(CPP) -c router.cpp -o router.o $(LIBS)

libraries: ftplib.cpp socketlib.cpp protocol.cpp
	$(CPP) -c ftplib.cpp -o ftplib.o $(LIBS)
	$(CPP) -c socketlib.cpp -o socketlib.o $(LIBS)
	$(CPP) -c protocol.cpp -o protocol.o $(LIBS)