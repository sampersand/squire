.PHONY: clean

squire: *.c *.h
	gcc *.c -o squire

clean:
	@-rm *.o squire -r squire.dSYM
