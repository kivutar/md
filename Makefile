SHARED := -shared
TARGET := md_libretro.so

ifeq ($(shell uname -s),) # win
	SHARED := -shared
	TARGET := md_libretro.dll
else ifneq ($(findstring Darwin,$(shell uname -s)),) # osx
	SHARED := -dynamiclib
	TARGET := md_libretro.dylib
endif

CFLAGS += -O3

OBJ = cpu.o mem.o vdp.o z80.o ym.o md.o compat.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) $(SHARED) -fPIC -flto -o $@ $^ $(CFLAGS)

clean:
	rm $(OBJ) $(TARGET)
