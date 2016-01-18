LZ4DIR = lz4/lib
CFLAGS += -DDEBUG -DRGCDEBUG
FLAGS  := $(CFLAGS) $(LDFLAGS)

#.PHONY: liblz4

#all: liblz4 lz4ring mftpd
#all: lz4ring mftpd
all: mftpd mftpc

#lz4ring: $(LZ4DIR)/lz4.o blockStreaming_ringBuffer.c
#		$(CC)      $(FLAGS) $^ -o $@$(EXT)
#liblz4:
#	@cd $(LZ4DIR); $(MAKE) -e all

mftpd: mftpd.o utility.o cwd.o file.o bsrb.o protocol.o $(LZ4DIR)/lz4.o
	$(CC) $(FLAGS) $^ -pthread -o $@

mftpc: cli.o utility.o cwd.o file.o bsrb.o protocol.o $(LZ4DIR)/lz4.o
	$(CC) $(FLAGS) $^ -lreadline -pthread -o $@

mftpd.o: mftpd.c
	$(CC) $(FLAGS) -c $^ -o $@

cli.o: cli.c
	$(CC) $(FLAGS) -c $^ -o $@

protocol.o: protocol.c
	$(CC) $(FLAGS) -c $^ -o $@

file.o: file.c
	$(CC) $(FLAGS) -c $^ -o $@

bsrb.o: blockStreaming_ringBuffer.c
	$(CC) $(FLAGS) -c $^ -o $@

utility.o: utility.c
	$(CC) $(FLAGS) -c $^ -o $@

cwd.o: cwd.c
	$(CC) $(FLAGS) -c $^ -o $@

rubgc.o: rubgc.c
	$(CC) $(FLAGS) -c $^ -o $@


clean:
	rm -f *.o
