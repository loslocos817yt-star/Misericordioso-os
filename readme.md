> [!IMPORTANT]
> **Instrucciones:**
>
> 1. `make`
> 
> 2. `qemu-system-x86_64 -kernel build/os.bin -drive file=disk.img,format=raw,if=ide -display curses`
> 3. (si no tienes el disk.img) `dd if=/dev/zero of=miboveda.img bs=1M count=4`

> [!TIP]
> Ejecútalo en Termux.
> 
