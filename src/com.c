// ==========================================
// MISERICORDIOSO OS - MANEJO DE COMANDOS
//XD ==========================================

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
extern void print_int(unsigned int n); // Importado para los numeros de linea

extern unsigned char current_color;
extern char current_dir[64];
extern const char scancode_map[128]; // Importamos el mapa del teclado

// DRIVERS IDE
extern int ide_read_sector(unsigned int lba, unsigned char* buffer);
extern void ide_write_sector(unsigned int lba, unsigned char* buffer);

// ==========================================
// SISTEMA DE ARCHIVOS REAL (EN DISK.IMG)
// ==========================================

typedef struct {
    char name[14];
    char parent[16];
    unsigned char is_dir;
    unsigned char active;
} FileNode;

FileNode vfs[16];
int vfs_ready = 0;

void save_vfs() {
    ide_write_sector(100, (unsigned char*)vfs);
}

void init_vfs() {
    if (vfs_ready) return;
    ide_read_sector(100, (unsigned char*)vfs);
    if (vfs[0].parent[0] != 'C' || vfs[0].parent[1] != ':') {
        for(int i = 0; i < 16; i++) vfs[i].active = 0;
        k_strcpy(vfs[0].parent, "C:\\"); k_strcpy(vfs[0].name, "bin"); vfs[0].is_dir = 1; vfs[0].active = 1;
        k_strcpy(vfs[1].parent, "C:\\"); k_strcpy(vfs[1].name, "home"); vfs[1].is_dir = 1; vfs[1].active = 1;
        k_strcpy(vfs[2].parent, "C:\\home"); k_strcpy(vfs[2].name, "pattitex"); vfs[2].is_dir = 1; vfs[2].active = 1;
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
            save_vfs();
            return;
        }
    }
    print("Error: Sector de particion lleno.\n");
}

// ==========================================
// MODO EDITOR (EL BLOC DE NOTAS)
// ==========================================
void editor_mode(int vfs_index) {
    unsigned char edit_buffer[512];
    int e_len = 0;
    int lba = 101 + vfs_index; // Cada archivo tiene su propio sector (101 a 116)

    // Leer el contenido del archivo
    ide_read_sector(lba, edit_buffer);
    while(e_len < 511 && edit_buffer[e_len] != '\0') e_len++;

    int redraw = 1;
    while(1) {
        if (redraw) {
            clear_screen();
            print("--- BLOC DE NOTAS --- [ESC] Guardar y Salir\n");
            
            // Dibujar la linea 1
            int line = 1;
            print_int(line); print(" | ");
            
            // Renderizar el texto
            for(int i = 0; i < e_len; i++) {
                char str[2] = {edit_buffer[i], '\0'};
                print(str);
                if (edit_buffer[i] == '\n') {
                    line++;
                    print_int(line); print(" | ");
                }
            }
            redraw = 0;
        }

        // Esperar input del teclado (bloqueante)
        if (inb(0x64) & 1) {
            unsigned char scancode = inb(0x60);
            if (scancode & 0x80) continue; // Ignorar cuando sueltas la tecla

            if (scancode == 0x01) { // 0x01 es la tecla ESCAPE
                break;
            } else {
                char c = scancode_map[scancode];
                if (c == '\b') {
                    if (e_len > 0) { e_len--; redraw = 1; }
                } else if (c != 0 && e_len < 511) {
                    edit_buffer[e_len++] = c;
                    redraw = 1;
                }
            }
        }
    }

    // Rellenar de ceros el sobrante para limpiar basura
    edit_buffer[e_len] = '\0';
    for(int i = e_len + 1; i < 512; i++) edit_buffer[i] = 0;
    
    // Guardar en el disco duro
    ide_write_sector(lba, edit_buffer);
    
    clear_screen();
    print("Archivo guardado en el disco.\n");
}

// ==========================================
// PROCESAMIENTO DE COMANDOS
// ==========================================
void process_command(char* cmd_buffer) {
    init_vfs(); 

    if (k_strcmp(cmd_buffer, "help") == 0) {
        print("Comandos del sistema:\n");
        print("  HELP     - Muestra esta lista\n");
        print("  CLS      - Limpia la pantalla\n");
        print("  VER      - Muestra la version del SO\n");
        print("  TIME     - Muestra la hora del hardware\n");
        print("  DATE     - Muestra la fecha del hardware\n");
        print("  COLOR    - Cambia el color (Ej. color a)\n");
        print("  LS       - Lista del Disco Duro (Sector 100)\n");
        print("  CD       - Entra o sale de directorios\n");
        print("  MKDIR    - Crea carpetas (-c) o archivos (-a)\n");
        print("  EDIT     - Abre el editor de texto (Bloc de notas)\n");
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
    
    // SISTEMA DE ARCHIVOS
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
            add_node(current_dir, name, (flag == 'c') ? 1 : 0);
            if (flag == 'c') print("Carpeta grabada al disco.\n");
            else print("Archivo grabado al disco.\n");
        }
    }

    // MODO EDITOR
    else if (k_strncmp(cmd_buffer, "edit ", 5) == 0) {
        char* name = cmd_buffer + 5;
        if (name[0] == '\0') {
            print("Uso: edit <archivo>\n");
        } else {
            int found_idx = -1;
            // Buscar si ya existe
            for(int i = 0; i < 16; i++) {
                if (vfs[i].active && vfs[i].is_dir == 0 &&
                    k_strcmp(vfs[i].parent, current_dir) == 0 &&
                    k_strcmp(vfs[i].name, name) == 0) {
                    found_idx = i;
                    break;
                }
            }
            
            // Si no existe, lo crea automaticamente
            if (found_idx == -1) {
                for(int i = 0; i < 16; i++) {
                    if(!vfs[i].active) {
                        k_strcpy(vfs[i].parent, current_dir);
                        k_strcpy(vfs[i].name, name);
                        vfs[i].is_dir = 0;
                        vfs[i].active = 1;
                        save_vfs();
                        
                        // Limpiar el sector del nuevo archivo
                        unsigned char ceros[512];
                        for(int j=0; j<512; j++) ceros[j] = 0;
                        ide_write_sector(101 + i, ceros);
                        
                        found_idx = i;
                        break;
                    }
                }
            }
            
            // Lanzar el editor
            if (found_idx != -1) {
                editor_mode(found_idx);
            } else {
                print("Error: No hay espacio en disco.\n");
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
        }
    }
    else if (cmd_buffer[0] != '\0') print("Comando no reconocido.\n");
}
