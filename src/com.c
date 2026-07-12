// ==========================================
// MISERICORDIOSO OS - MANEJO DE COMANDOS
// ==========================================

extern void print(const char* str);
extern void clear_screen();
extern void print_logo();
extern int k_strcmp(const char* s1, const char* s2);
extern int k_strncmp(const char* s1, const char* s2, int n);
extern void k_strcpy(char* dest, const char* src);
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);
extern unsigned char bcd_to_bin(unsigned char bcd);
extern unsigned char read_cmos(unsigned char reg);
extern void print_time_part(unsigned char val);

extern unsigned char current_color;
extern char current_dir[64];

// DRIVERS IDE (Lectura y Escritura)
extern int ide_read_sector(unsigned int lba, unsigned char* buffer);
extern void ide_write_sector(unsigned int lba, unsigned char* buffer);

// ==========================================
// SISTEMA DE ARCHIVOS REAL (EN DISK.IMG)
// ==========================================

// Esta estructura ocupa EXACTAMENTE 32 bytes
typedef struct {
    char name[14];        // Nombre
    char parent[16];      // Ruta donde vive
    unsigned char is_dir; // 1 = Carpeta, 0 = Archivo
    unsigned char active; // 1 = Existe, 0 = Borrado/Vacio
} FileNode;

// 16 Nodos * 32 bytes = 512 Bytes (Un sector exacto)
FileNode vfs[16]; 
int vfs_ready = 0;

// Guarda el VFS completo en el LBA 100 del disco duro real
void save_vfs() {
    ide_write_sector(100, (unsigned char*)vfs);
}

void init_vfs() {
    if (vfs_ready) return;
    
    // Leemos el sector 100 del disco a la memoria
    ide_read_sector(100, (unsigned char*)vfs);

    // Si es la primera vez (no hay firma valida), lo formateamos
    if (vfs[0].parent[0] != 'C' || vfs[0].parent[1] != ':') {
        for(int i = 0; i < 16; i++) vfs[i].active = 0;
        
        // Carpetas base
        k_strcpy(vfs[0].parent, "C:\\"); k_strcpy(vfs[0].name, "bin"); vfs[0].is_dir = 1; vfs[0].active = 1;
        k_strcpy(vfs[1].parent, "C:\\"); k_strcpy(vfs[1].name, "home"); vfs[1].is_dir = 1; vfs[1].active = 1;
        k_strcpy(vfs[2].parent, "C:\\home"); k_strcpy(vfs[2].name, "pattitex"); vfs[2].is_dir = 1; vfs[2].active = 1;
        
        // Guardamos los cambios iniciales al disco
        save_vfs();
    }
    vfs_ready = 1;
}

void add_node(const char* parent, const char* name, int is_dir) {
    for(int i = 0; i < 16; i++) {
        if(!vfs[i].active) {
            k_strcpy(vfs[i].parent, parent);
            k_strcpy(vfs[i].name, name);
            vfs[i].is_dir = is_dir;
            vfs[i].active = 1;
            save_vfs(); // SE ESCRIBE DIRECTO AL DISK.IMG
            return;
        }
    }
    print("Error: Sector de particion lleno (Max 16 elementos).\n");
}

// ==========================================
// PROCESAMIENTO DE COMANDOS
// ==========================================
void process_command(char* cmd_buffer) {
    init_vfs(); // Asegurarse de que el disco esté leido

    if (k_strcmp(cmd_buffer, "help") == 0) {
        print("Comandos del sistema:\n");
        print("  HELP     - Muestra esta lista\n");
        print("  CLS      - Limpia la pantalla\n");
        print("  VER      - Muestra la version del SO\n");
        print("  TIME     - Muestra la hora del hardware\n");
        print("  DATE     - Muestra la fecha del hardware\n");
        print("  COLOR    - Cambia el color (Ej. color a)\n");
        print("  ECHO     - Imprime texto en pantalla\n");
        print("  LS       - Lista del Disco Duro (Sector 100)\n");
        print("  CD       - Entra o sale de directorios\n");
        print("  MKDIR    - Crea carpetas (-c) o archivos (-a)\n");
        print("  REBOOT   - Reinicia la computadora\n");
    } 
    else if (k_strcmp(cmd_buffer, "cls") == 0) clear_screen();
    else if (k_strcmp(cmd_buffer, "ver") == 0) print("Misericordioso OS v1.0\n");
    else if (k_strcmp(cmd_buffer, "time") == 0) {
        unsigned char h = bcd_to_bin(read_cmos(0x04));
        unsigned char m = bcd_to_bin(read_cmos(0x02));
        unsigned char s = bcd_to_bin(read_cmos(0x00));
        print("Hora del sistema: "); print_time_part(h); print(":"); print_time_part(m); print(":"); print_time_part(s); print(" UTC\n");
    } 
    else if (k_strcmp(cmd_buffer, "date") == 0) {
        unsigned char d = bcd_to_bin(read_cmos(0x07));
        unsigned char mo = bcd_to_bin(read_cmos(0x08));
        unsigned char y = bcd_to_bin(read_cmos(0x09));
        print("Fecha del sistema: "); print_time_part(d); print("/"); print_time_part(mo); print("/20"); print_time_part(y); print("\n");
    } 
    else if (k_strncmp(cmd_buffer, "echo ", 5) == 0) {
        print(cmd_buffer + 5); print("\n");
    } 
    else if (k_strncmp(cmd_buffer, "color ", 6) == 0) {
        char col = cmd_buffer[6];
        if (col == 'a' || col == 'A') current_color = 0x0A;
        else if (col == 'b' || col == 'B') current_color = 0x09;
        else if (col == 'c' || col == 'C') current_color = 0x0C;
        else if (col == '7') current_color = 0x07;
        else print("Uso: color [a, b, c, 7]\n");

        if (col == 'a' || col == 'A' || col == 'b' || col == 'B' || col == 'c' || col == 'C' || col == '7') {
            char* video = (char*)0xB8000;
            for (int i = 1; i < 80 * 25 * 2; i += 2) video[i] = current_color;
        }
    } 
    else if (k_strcmp(cmd_buffer, "reboot") == 0) {
        print("Reiniciando...\n");
        outb(0x64, 0xFE);
        while(1);
    } 
    
    // ==========================================
    // SISTEMA DE ARCHIVOS (LECTURA Y ESCRITURA EN DISCO)
    // ==========================================
    
    else if (k_strcmp(cmd_buffer, "ls") == 0) {
        int empty = 1;
        for(int i = 0; i < 16; i++) {
            if (vfs[i].active && k_strcmp(vfs[i].parent, current_dir) == 0) {
                print(vfs[i].name);
                if (vfs[i].is_dir) print("/");
                print("  ");
                empty = 0;
            }
        }
        if (empty) print("(Vacio)");
        print("\n");
    } 
    
    else if (k_strncmp(cmd_buffer, "mkdir", 5) == 0) {
        if (cmd_buffer[5] != ' ' || cmd_buffer[6] != '-' || (cmd_buffer[7] != 'c' && cmd_buffer[7] != 'a') || cmd_buffer[8] != ' ') {
            print("Uso: mkdir -c <carpeta>   |   mkdir -a <archivo>\n");
        } else {
            char flag = cmd_buffer[7];
            char* name = cmd_buffer + 9;
            if (name[0] == '\0') {
                print("Error: Necesitas ponerle un nombre.\n");
            } else {
                add_node(current_dir, name, (flag == 'c') ? 1 : 0);
                if (flag == 'c') print("Carpeta grabada al disco duro.\n");
                else print("Archivo grabado al disco duro.\n");
            }
        }
    }
    
    else if (k_strncmp(cmd_buffer, "cd", 2) == 0) {
        if (cmd_buffer[2] == '\0' || (cmd_buffer[2] == ' ' && cmd_buffer[3] == '\0')) {
            print(current_dir); print("\n");
        } else if (cmd_buffer[2] == ' ') {
            char* target = cmd_buffer + 3;
            
            if (k_strcmp(target, "..") == 0) {
                int clen = 0; while(current_dir[clen]) clen++;
                if (clen > 3) {
                    int last = clen - 1;
                    while (last > 2 && current_dir[last] != '\\') last--;
                    if (last == 2) current_dir[3] = '\0';
                    else current_dir[last] = '\0';
                }
            } else if (k_strcmp(target, "\\") == 0) {
                current_dir[0] = 'C'; current_dir[1] = ':'; current_dir[2] = '\\'; current_dir[3] = '\0';
            } else if (k_strcmp(target, ".") == 0) {
            } else {
                int found = 0;
                for(int i = 0; i < 16; i++) {
                    if (vfs[i].active && vfs[i].is_dir && 
                        k_strcmp(vfs[i].parent, current_dir) == 0 && 
                        k_strcmp(vfs[i].name, target) == 0) {
                        
                        int clen = 0; while(current_dir[clen]) clen++;
                        if (current_dir[clen-1] != '\\') current_dir[clen++] = '\\';
                        for(int j = 0; target[j] != '\0'; j++) current_dir[clen++] = target[j];
                        current_dir[clen] = '\0';
                        found = 1; break;
                    }
                }
                if (!found) print("Error: Carpeta no encontrada.\n");
            }
        } else {
            print("Comando no reconocido.\n");
        }
    } 
    else if (cmd_buffer[0] != '\0') {
        print("Comando no reconocido.\n");
    }
}
