OBJECTS = main.o blank.o ssu_score.o
TARGET = ssu_score
CC = gcc

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $^ -g

main.o: main.c
	$(CC) -c $^ -g 

blank.o: blank.c
	$(CC) -c $^ -g

ssu_score.o: ssu_score.c
	$(CC) -c $^ -g

clean:
	rm score*
