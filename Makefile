kostsh : kostsh_utils.o main.o
	gcc -pthread -o kostsh kostsh_utils.o main.o

kostsh_utils.o :
	gcc -c -o kostsh_utils.o kostsh_utils.c
	
main.o :
	gcc -c -o main.o main.c

clean :
	rm *.o kostsh
