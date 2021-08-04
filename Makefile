OBJ = cpu.o mem.o vdp.o z80.o ym.o md.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

md: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm *.o
