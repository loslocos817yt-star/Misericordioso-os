// ==========================================
// MISERICORDIOSO OS - IDE DISK DRIVER
// ==========================================

extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);

// Nueva función para leer 16 bits del bus IDE
unsigned short inw(unsigned short port) {
    unsigned short result;
    __asm__ volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Lee 1 sector (512 bytes) usando LBA28
// Retorna 1 si tuvo exito, 0 si fallo por timeout
int ide_read_sector(unsigned int lba, unsigned char* buffer) {
    int timeout = 100000; // Timeout de seguridad

    // 1. Esperar a que el disco no este ocupado (BSY = bit 7)
    while ((inb(0x1F7) & 0x80) && --timeout);
    if (timeout == 0) return 0; // Error: Disco no responde

    // 2. Enviar parametros LBA
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // Drive 0 (Master), LBA mode
    outb(0x1F2, 1);                           // Leer 1 sector
    outb(0x1F3, (unsigned char)lba);          // LBA bits 0-7
    outb(0x1F4, (unsigned char)(lba >> 8));   // LBA bits 8-15
    outb(0x1F5, (unsigned char)(lba >> 16));  // LBA bits 16-23
    
    // 3. Enviar Comando: Leer Sector (0x20)
    outb(0x1F7, 0x20);                        

    // 4. Esperar a que el disco tenga los datos listos (DRQ = bit 3)
    timeout = 100000;
    while (!(inb(0x1F7) & 0x08) && --timeout);
    if (timeout == 0) return 0; // Error: No hay datos

    // 5. Extraer 256 "words" (512 bytes) del puerto de datos
    unsigned short* ptr = (unsigned short*)buffer;
    for (int i = 0; i < 256; i++) {
        ptr[i] = inw(0x1F0);
    }

    return 1; // Exito
}
