SRC:=peibol.cpp sqlite_excp.cpp sqlite_db.cpp envi_raw_msg.cpp CTimer.cpp CTrace.cpp CClientSocket.cpp CServerSocket.cpp CMessage.cpp
HDRS:=sqlite_excp.h sqlite_db.h envi_raw_msg.h CTimer.hpp CTrace.hpp CClientSocket.hpp CServerSocket.hpp CCallbackInterface.hpp CMessage.hpp
EXE:=peibol

VFDLIBDIR:=$(HOME)/VFD/libvfd
INCDIR:=.
LIBS:=-lpcrecpp -lsqlite3 -lbcm2835 -lrt -lpthread
STA_LIBS:=$(VFDLIBDIR)/libvfd.a
CPPFLAGS:=-g -I$(INCDIR) -I$(VFDLIBDIR)



$(EXE): $(SRC:.cpp=.o)
	$(CXX) -g $^ $(STA_LIBS) $(LIBS) -o $@

depend: $(SRC)
	@echo Construyendo dependencias
	@rm -f .depend && for i in $^ ; do \
	   $(CXX) -M $(CPPFLAGS) "$$i" >> .depend ; \
	done
	@echo "C'est fini!"

clean:
	rm -f $(SRC:.cpp=.o) $(EXE)

backup:
	tar cfj bak`date +%Y%j%H%M`.tar.bz2 $(SRC) $(HDRS) Makefile

-include .depend

