all: build/os.bin

#Ayuda xdddd

build/boot.o: src/boot.s
	nasm -f elf32 src/boot.s -o build/boot.o

build/kernel.o: src/kernel.c
	clang --target=i386-elf -ffreestanding -fno-pie -nostdlib -c src/kernel.c -o build/kernel.o

build/teclado.o: src/teclado.c
	clang --target=i386-elf -ffreestanding -fno-pie -nostdlib -c src/teclado.c -o build/teclado.o

build/com.o: src/com.c
	clang --target=i386-elf -ffreestanding -fno-pie -nostdlib -c src/com.c -o build/com.o

build/storage.o: src/storage.c
	clang --target=i386-elf -ffreestanding -fno-pie -nostdlib -c src/storage.c -o build/storage.o

build/os.bin: build/boot.o build/kernel.o build/teclado.o build/com.o build/storage.o
	ld -m elf_i386 -T linker.ld -o build/os.bin build/boot.o build/kernel.o build/teclado.o build/com.o build/storage.o

clean:
	rm -rf build/*.o build/os.bin
