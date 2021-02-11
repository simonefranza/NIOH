CC            := gcc
CCFLAGS       := -Wall -Wextra -pedantic -g 
CCLINKS 			:= -lcurses -lpthread -lpanel
PROG 					:= main
LIBS 					:= machdr.h wManager.c

.PHONY: bin 

bin:			## compiles project to executable binary
	@echo "[+] Compiling binary"
	$(CC) $(CCFLAGS) -o $(PROG) $(PROG).c $(LIBS) $(CCLINKS)

