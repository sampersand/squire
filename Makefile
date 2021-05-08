.PHONY: clean

squire: src/*.c src/*.h
	gcc -fsanitize=undefined,address -g src/*.c -o squire

clean:
	@-rm *.o squire -r squire.dSYM
