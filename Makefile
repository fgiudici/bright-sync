bsync: src/bsync.o src/bsync.h
	mkdir -p bin
	$(CC) -o bin/bsync -Wall src/bsync.c
clean:
	rm -rf src/*.o bin
