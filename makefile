CC = gcc
CFLAGS = -g -std=c99 -Wall
OBJS = main.o input_parsing.o llist.o process_control.o shell_commands.o signal_handlers.o utilities.o

smallsh: $(OBJS)
	$(CC) $(CFLAGS) -o smallsh $(OBJS)

main.o: main.c input_parsing.h llist.h signal_handlers.h
	$(CC) $(CFLAGS) -c main.c

input_parsing.o: input_parsing.c input_parsing.h utilities.h
	$(CC) $(CFLAGS) -c input_parsing.c

llist.o: llist.c llist.h
	$(CC) $(CFLAGS) -c llist.c

process_control.o: process_control.c process_control.h llist.h input_parsing.h utilities.h signal_handlers.h
	$(CC) $(CFLAGS) -c process_control.c

shell_commands.o: shell_commands.c shell_commands.h utilities.h
	$(CC) $(CFLAGS) -c shell_commands.c

signal_handlers.o: signal_handlers.c signal_handlers.h
	$(CC) $(CFLAGS) -c signal_handlers.c

utilities.o: utilities.c utilities.h
	$(CC) $(CFLAGS) -c utilities.c

clean:
	rm -f smallsh $(OBJS)