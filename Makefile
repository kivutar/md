SHARED := -dynamiclib
TARGET := md_libretro.dylib
CFLAGS += -O3

OBJ = cpu.o mem.o vdp.o z80.o ym.o md.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) $(SHARED) -fPIC -o $@ $^ $(CFLAGS)

clean:
	rm *.o md_libretro.*
