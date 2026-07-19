// ==========================================
// MISERICORDIOSO OS - IDE DISK DRIVER
// ==========================================

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);

unsigned short inw(unsigned short port) {
    unsigned short result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outw(unsigned short port, unsigned short data) {
    __asm__ volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

int ide_read_sector(unsigned int lba, unsigned char* buffer) {
    int timeout = 100000;
    while ((inb(0x1F7) & 0x80) && --timeout);
    if (timeout == 0) return 0; // Error: Timeout o sin disco

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);

    timeout = 100000;
    while (!(inb(0x1F7) & 0x08) && --timeout) {
        if (inb(0x1F7) & 0x01) return 0; // Error de hardware
    }
    if (timeout == 0) return 0;

    unsigned short* ptr = (unsigned short*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(0x1F0);
    }
    return 1;
}

void ide_write_sector(unsigned int lba, unsigned char* buffer) {
    int timeout = 100000;
    while ((inb(0x1F7) & 0x80) && --timeout);
    if (timeout == 0) return; // Si no hay disco, abortar sin trabarse

    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);
    outb(0x1F3, (unsigned char)lba);
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x30); 

    timeout = 100000;
    // Esperar a DRQ (bit 3), abortar si hay ERR (bit 0) o si se acaba el tiempo
    while (!(inb(0x1F7) & 0x08) && --timeout) {
        if (inb(0x1F7) & 0x01) return;
    }
    if (timeout == 0) return;

    unsigned short* ptr = (unsigned short*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(0x1F0, ptr[i]);
    }
}
