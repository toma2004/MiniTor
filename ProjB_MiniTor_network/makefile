projb: proxy.o router1.o aes.o main.o
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -g -o projb proxy.o router1.o aes.o main.o -lcrypto

proxy.o: proxy.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 proxy.c

router1.o: router1.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 router1.c

aes.o: aes.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 aes.c

main.o: main.c
	gcc -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -c -g -Wall -Wextra -std=c99 main.c

clean:
	-$(RM) *.o projb
