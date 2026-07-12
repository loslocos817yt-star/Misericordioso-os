# Makefile corregido
all: build/os.bin

# Regla para el bootloader
build/boot.o: src/boot.s
	nasm -f elf32 src/boot.s -o build/boot.o

# Regla para kernel.c
build/kernel.o: src/kernel.c
	clang --target=i386-elf -ffreestanding -fno-pie -nostdlib -c src/kernel.c -o build/kernel.o

# Regla para teclado.c
build/teclado.o: src/teclado.c
	clang --target=i386-elf -ffreestanding -fno-pie -nostdlib -c src/teclado.c -o build/teclado.o

# Regla para unir todo
build/os.bin: build/boot.o build/kernel.o build/teclado.o
	ld -m elf_i386 -T linker.ld -o build/os.bin build/boot.o build/kernel.o build/teclado.o

clean:
	rm -rf build/*.o build/os.bin

