CC = gcc

SRCS = main.c common.c files.c dspProcess.c

TARGET = dsptool

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
