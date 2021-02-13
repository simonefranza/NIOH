CC            := gcc
CCFLAGS       := -Wall -Wextra -pedantic -g 
CCLINKS 			:= -lcurses -lpthread -lpanel
PROG 					:= main
LIBS 					:= machdr.c wManager.c nw_header.h

.PHONY: bin 

bin:			## compiles project to executable binary
	@echo "[+] Compiling binary"
	$(CC) $(CCFLAGS) -o $(PROG) $(PROG).c $(LIBS) $(CCLINKS)

