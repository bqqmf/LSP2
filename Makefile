OBJECTS = main.o blank.o ssu_score.o
TARGET = ssu_score
CC = gcc -g

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $^ 

main.o: main.c
	$(CC) -c $^

blank.o: blank.c
	$(CC) -c $^ 

ssu_score.o: ssu_score.c
	$(CC) -c $^ 

clean:
	rm *.o
	rm *.txt
