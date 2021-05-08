.PHONY: clean

squire: src/*.c src/*.h
	gcc src/*.c -o squire

clean:
	@-rm *.o squire -r squire.dSYM
