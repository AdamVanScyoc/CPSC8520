include Make.defines

PROGS =	 client server

COBJECTS =	utils.o  
CSOURCES =	utils.c 

CPLUSOBJECTS = 

COMMONSOURCES =

CPLUSSOURCES =

CLEANFILES = 	client.o server.o 

SOURCES =       $(CSOURCES) $(CPLUSSOURCES)


all:	${PROGS}


client:		client.o $(CPLUSOBJECTS) $(COBJECTS) $(LIBS) $(COMMONSOURCES) $(SOURCES)
		${CC} ${LINKOPTIONS}  $@ client.o $(CPLUSOBJECTS) $(COBJECTS) $(BASELIBS) $(LIBS) $(LINKFLAGS)

server:		server.o $(CPLUSOBJECTS) $(COBJECTS) $(LIBS) $(COMMONSOURCES) $(SOURCES)
		${CC} ${LINKOPTIONS} $@ server.o $(CPLUSOBJECTS) $(COBJECTS) $(BASELIBS) $(LIBS) $(LINKFLAGS)


.cc.o:	
	$(CPLUS) $(CPLUSFLAGS) $<

.c.o:	
	$(CC) $(CFLAGS) $<



backup:
	rm -f UDPEchoV1.tar
	rm -f UDPEchoV1.tar.gz
	tar -cf UDPEchoV1.tar *
	gzip -f UDPEchoV1.tar

clean:
		rm -f ${PROGS} ${COBJECTS} ${CPLUSOBJECTS} ${CLEANFILES}


