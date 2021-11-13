all: pimon

CFLAGS=`curl-config --cflags` -g
LIBS=`curl-config --libs`

pimon: pimon.o
	$(CC) -o $@ $(LIBS) $<

install: pimon
	install pimon /usr/bin

clean:
	rm -rf pimon pimon.o
