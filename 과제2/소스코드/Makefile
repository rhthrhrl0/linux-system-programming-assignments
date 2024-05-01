all : ssu_sdup fmd5 fsha1 help

CC: gcc

ssu_sdup: ssu_sdup.c
	$(CC) -o $@ $<

fmd5: ssu_find-md5.c
	$(CC) -o $@ $< -lssl -lcrypto
	
fsha1: ssu_find-sha1.c
	$(CC) -o $@ $< -lssl -lcrypto

help: ssu_help.c
	$(CC) -o $@ $<

clean: 
	rm ssu_sdup
	rm fmd5
	rm fsha1
	rm help
