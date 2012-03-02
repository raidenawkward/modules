EXEC := b

SRC := $(shell ls *.c)
OBJ := $(SRC:.c=.o)

default:$(OBJ)
	$(CC) $(OBJ) -o $(EXEC)
clean:
	rm -rf $(OBJ) $(EXEC)
