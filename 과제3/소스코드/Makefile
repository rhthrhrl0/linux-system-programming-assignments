all : ssu_sfinder

CC: gcc

ssu_sfinder: ssu_sfinder.c
	$(CC) -o $@ $< -lssl -lcrypto -lpthread

clean: 
	rm ssu_sfinder
