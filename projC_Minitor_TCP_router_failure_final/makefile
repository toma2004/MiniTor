projc: proxy.o router1.o aes.o controlMessage.o main.o
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -g -o projc proxy.o router1.o aes.o controlMessage.o main.o -lcrypto

proxy.o: proxy.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 proxy.c

router1.o: router1.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 router1.c

aes.o: aes.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 aes.c

controlMessage.o: controlMessage.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 controlMessage.c

main.o: main.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 main.c

clean:
	-$(RM) *.o projc
