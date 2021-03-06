LZ4DIR = lz4/lib
CFLAGS += -Wall -O3
#CFLAGS += -Wall -O3 -g -DDEBUG -fsanitize=address
FLAGS  := $(CFLAGS) $(LDFLAGS)

FLAGS  := $(CFLAGS) $(LDFLAGS)

#.PHONY: liblz4

#all: liblz4 lz4ring mftpd
#all: lz4ring mftpd
all: mftpc mftpd

#lz4ring: $(LZ4DIR)/lz4.o blockStreaming_ringBuffer.c
#		$(CC)      $(FLAGS) $^ -o $@$(EXT)
#liblz4:
#	@cd $(LZ4DIR); $(MAKE) -e all


UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)

mftpc: mftpc.o bsrb.o cwd.o file.o ls.o protocol.o sock.o timeutil.o utility.o $(LZ4DIR)/lz4.o
	$(CC) $(FLAGS) $^ -lcurl -lreadline -lrt -o $@
mftpd: mftpd.o bsrb.o cwd.o file.o ls.o protocol.o sock.o timeutil.o utility.o $(LZ4DIR)/lz4.o
	$(CC) $(FLAGS) $^ -lcurl -lrt -pthread -o $@

endif

ifeq ($(UNAME_S),Darwin)

mftpc: mftpc.o bsrb.o cwd.o file.o ls.o protocol.o sock.o timeutil.o utility.o $(LZ4DIR)/lz4.o
	$(CC) $(FLAGS) $^ -lcurl -lreadline -o $@
mftpd: mftpd.o bsrb.o cwd.o file.o ls.o protocol.o sock.o timeutil.o utility.o $(LZ4DIR)/lz4.o
	$(CC) $(FLAGS) $^ -lcurl -pthread -o $@

endif



bsrb.o: blockStreaming_ringBuffer.c
	$(CC) $(FLAGS) -c $^ -o $@


cwd.o: cwd.c
	$(CC) $(FLAGS) -c $^ -o $@

file.o: file.c
	$(CC) $(FLAGS) -c $^ -o $@

ls.o: ls.c
	$(CC) $(FLAGS) -c $^ -o $@

mftpc.o: mftpc.c
	$(CC) $(FLAGS) -c $^ -o $@

mftpd.o: mftpd.c
	$(CC) $(FLAGS) -c $^ -o $@

protocol.o: protocol.c
	$(CC) $(FLAGS) -c $^ -o $@

#rubgc.o: rubgc.c
#	$(CC) $(FLAGS) -c $^ -o $@

sock.o: sock.c
	$(CC) $(FLAGS) -c $^ -o $@

timeutil.o:  timeutil.c
	$(CC) $(FLAGS) -c $^ -o $@

utility.o: utility.c
	$(CC) $(FLAGS) -c $^ -o $@

clean:
	rm -f *.o
