// ==========================================
// MISERICORDIOSO OS - ULTIMATE MS-DOS EDITION
// ==========================================

extern int handle_keyboard(); // Llama a la funcion de teclado.c

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outb(unsigned short port, unsigned char data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Implementaciones básicas de memoria y cadenas
void* memcpy(void* dest, const void* src, unsigned long n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

int k_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int k_strncmp(const char* s1, const char* s2, int n) {
    while (n > 0 && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void k_strcpy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

int cursor = 0;
int prompt_cursor = 0; // Guarda dónde empieza el área escribible
char current_dir[64] = "C:\\";
unsigned char current_color = 0x07;

const char* valid_dirs[] = {
    "C:\\", "C:\\bin", "C:\\boot", "C:\\dev", "C:\\etc",
    "C:\\home", "C:\\home\\pattitex", "C:\\root", "C:\\usr"
};
const int num_valid_dirs = 9;

// ==========================================
// VARIABLES DEL SHELL (Ahora globales para teclado.c)
// ==========================================
char cmd_buffer[256];
int cmd_len = 0;
int edit_pos = 0; 

// ==========================================
// HISTORIAL DE COMANDOS
// ==========================================
char history[10][256];
int history_count = 0;
int history_pos = 0;

// ==========================================
// MANEJO DE PANTALLA Y CURSOR HARDWARE
// ==========================================

void update_vga_cursor() {
    unsigned short pos = cursor / 2;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((pos >> 8) & 0xFF));
}

void clear_screen() {
    char* video = (char*)0xB8000;
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video[i] = ' ';
        video[i+1] = current_color;
    }
    cursor = 0;
    update_vga_cursor();
}

void print(const char* str) {
    char* video = (char*)0xB8000;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cursor = (cursor / 160 + 1) * 160;
        } else {
            video[cursor] = str[i];
            video[cursor+1] = current_color;
            cursor += 2;
        }
        if (cursor >= 80 * 25 * 2) clear_screen();
    }
    update_vga_cursor();
}

// Redibuja la línea de comando actual (necesario para las flechas)
void redraw_line(const char* cmd, int len, int edit_pos) {
    char* video = (char*)0xB8000;
    // Limpiar desde el prompt hasta el final de la línea
    for(int i = 0; i < 80; i++) {
        video[prompt_cursor + i*2] = ' ';
    }
    // Dibujar el buffer
    for(int i = 0; i < len; i++) {
        video[prompt_cursor + i*2] = cmd[i];
        video[prompt_cursor + i*2 + 1] = current_color;
    }
    // Mover el cursor real a donde estamos editando
    cursor = prompt_cursor + (edit_pos * 2);
    update_vga_cursor();
}

void print_int(unsigned int n) {
    if (n == 0) { print("0"); return; }
    char buf[11];
    int i = 10;
    buf[10] = '\0';
    while (n > 0 && i > 0) {
        buf[--i] = (n % 10) + '0';
        n /= 10;
    }
    print(&buf[i]);
}

void print_time_part(unsigned char val) {
    char buf[3];
    buf[0] = (val / 10) + '0';
    buf[1] = (val % 10) + '0';
    buf[2] = '\0';
    print(buf);
}

unsigned char bcd_to_bin(unsigned char bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

unsigned char read_cmos(unsigned char reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void print_logo() {
    const char* logo[] = {
        "  __  __ _               _                   _ _                    ___  ____",
        " |  \\/  (_)___  ___  _ __(_) ___ ___  _ __ __| (_) ___  ___  ___    / _ \\/ ___|",
        " | |\\/| | / __|/ _ \\ '__| |/ __/ _ \\| '__/ _` | |/ _ \\/ __|/ _ \\  | | | \\___ \\",
        " | |  | | \\__ \\  __/ |  | | (_| (_) | | | (_| | | (_) \\__ \\ (_) | | |_| |___) |",
        " |_|  |_|_|___/\\___|_|  |_|\\___\\___/|_|  \\__,_|_|\\___/|___/\\___/   \\___/|____/",
        "Escribe 'Help' para obtener ayuda",
        0
    };

    for (int i = 0; logo[i] != 0; i++) {
        print(logo[i]);
        print("\n");
    }
}

// ==========================================
// KERNEL MAIN
// ==========================================

void kernel_main() {
    clear_screen();
    print_logo();
    print("\n");
    print("Misericordioso OS v1.0\n");
    print("Copyright 2026-2036 PattitexTEC\n\n");

    print(current_dir); print("> ");
    prompt_cursor = cursor; // Guardamos el ancla del cursor

    while(1) {
        // La función handle_keyboard (en teclado.c) maneja el buffer, el historial y el cursor.
        // Solo retorna 1 cuando el usuario presiona Enter.
        if (handle_keyboard()) {
            
            cmd_buffer[cmd_len] = '\0';
            print("\n");

            if (cmd_len > 0) {
                // Guardar en historial
                if (history_count < 10) {
                    k_strcpy(history[history_count], cmd_buffer);
                    history_count++;
                } else {
                    for(int i=1; i<10; i++) k_strcpy(history[i-1], history[i]);
                    k_strcpy(history[9], cmd_buffer);
                }
                history_pos = history_count; // Resetear puntero de historial

                // PROCESAMIENTO DE COMANDOS
                if (k_strcmp(cmd_buffer, "help") == 0) {
                    print("Comandos del sistema:\n");
                    print("  HELP   - Muestra esta lista\n");
                    print("  CLS    - Limpia la pantalla\n");
                    print("  VER    - Muestra la version del SO\n");
                    print("  TIME   - Muestra la hora del hardware\n");
                    print("  DATE   - Muestra la fecha del hardware\n");
                    print("  COLOR  - Cambia el color (Ej. color a, color c)\n");
                    print("  ECHO   - Imprime texto en pantalla\n");
                    print("  LS     - Lista directorios\n");
                    print("  CD     - Cambia de directorio\n");
                    print("  REBOOT - Reinicia la computadora\n");
                    print("  LOGO   - Muestra el logo epico\n");
                    print("  MATRIX - Ataque cibernetico visual\n");
                } else if (k_strcmp(cmd_buffer, "cls") == 0) {
                    clear_screen();
                } else if (k_strcmp(cmd_buffer, "ver") == 0) {
                    print("Misericordioso OS v1.0\n");
                } else if (k_strcmp(cmd_buffer, "time") == 0) {
                    unsigned char h = bcd_to_bin(read_cmos(0x04));
                    unsigned char m = bcd_to_bin(read_cmos(0x02));
                    unsigned char s = bcd_to_bin(read_cmos(0x00));
                    print("Hora del sistema: "); print_time_part(h); print(":"); print_time_part(m); print(":"); print_time_part(s); print(" UTC\n");
                } else if (k_strcmp(cmd_buffer, "date") == 0) {
                    unsigned char d = bcd_to_bin(read_cmos(0x07));
                    unsigned char mo = bcd_to_bin(read_cmos(0x08));
                    unsigned char y = bcd_to_bin(read_cmos(0x09));
                    print("Fecha del sistema: "); print_time_part(d); print("/"); print_time_part(mo); print("/20"); print_time_part(y); print("\n");
                } else if (k_strncmp(cmd_buffer, "echo ", 5) == 0) {
                    print(cmd_buffer + 5); print("\n");
                } else if (k_strncmp(cmd_buffer, "color ", 6) == 0) {
                    char col = cmd_buffer[6];
                    if (col == 'a' || col == 'A') current_color = 0x0A;
                    else if (col == 'b' || col == 'B') current_color = 0x09;
                    else if (col == 'c' || col == 'C') current_color = 0x0C;
                    else if (col == '7') current_color = 0x07;
                    else print("Uso: color [a=verde, b=azul, c=rojo, 7=default]\n");

                    if (col == 'a' || col == 'A' || col == 'b' || col == 'B' || col == 'c' || col == 'C' || col == '7') {
                        char* video = (char*)0xB8000;
                        for (int i = 1; i < 80 * 25 * 2; i += 2) video[i] = current_color;
                    }
                } else if (k_strcmp(cmd_buffer, "matrix") == 0) {
                    char* video = (char*)0xB8000;
                    for (int i = 0; i < 80 * 25; i++) {
                        video[i * 2] = (char)((inb(0x60) % 94) + 33);
                        video[i * 2 + 1] = 0x0A;
                    }
                    print("Sistema bajo ataque...\n");
                } else if (k_strcmp(cmd_buffer, "reboot") == 0) {
                    print("Reiniciando...\n");
                    outb(0x64, 0xFE);
                    while(1);
                } else if (k_strcmp(cmd_buffer, "ls") == 0) {
                    if (k_strcmp(current_dir, "C:\\") == 0) print("bin/  boot/  dev/  etc/  home/  root/  usr/\n");
                    else if (k_strcmp(current_dir, "C:\\home") == 0) print("pattitex/\n");
                    else print("\n");
                } else if (k_strncmp(cmd_buffer, "cd", 2) == 0) {
                    // ... tu lógica intocable de CD ...
                    if (cmd_buffer[2] == '\0' || (cmd_buffer[2] == ' ' && cmd_buffer[3] == '\0')) {
                        print(current_dir); print("\n");
                    } else if (cmd_buffer[2] == ' ') {
                        char* target = cmd_buffer + 3;
                        char temp_dir[64];
                        int clen = 0;
                        while(current_dir[clen] != '\0') { temp_dir[clen] = current_dir[clen]; clen++; }
                        temp_dir[clen] = '\0';

                        if (k_strcmp(target, "..") == 0) {
                            if (clen > 3) {
                                int last_slash = clen - 1;
                                while(last_slash > 2 && temp_dir[last_slash] != '\\') last_slash--;
                                if (last_slash == 2) temp_dir[3] = '\0';
                                else temp_dir[last_slash] = '\0';
                            }
                        } else if (k_strcmp(target, "\\") == 0) {
                            temp_dir[0] = 'C'; temp_dir[1] = ':'; temp_dir[2] = '\\'; temp_dir[3] = '\0';
                        } else if (k_strcmp(target, ".") == 0) {
                        } else {
                            if (temp_dir[clen-1] != '\\') temp_dir[clen++] = '\\';
                            for(int i = 0; target[i] != '\0' && clen < 60; i++) temp_dir[clen++] = target[i];
                            temp_dir[clen] = '\0';
                        }

                        int is_valid = 0;
                        for(int i = 0; i < num_valid_dirs; i++) {
                            if (k_strcmp(temp_dir, valid_dirs[i]) == 0) { is_valid = 1; break; }
                        }

                        if (is_valid) {
                            for(int i = 0; i < 64; i++) {
                                current_dir[i] = temp_dir[i];
                                if(temp_dir[i] == '\0') break;
                            }
                            print("Directorio cambiado correctamente.\n");
                        } else {
                            print("Error: El sistema no puede encontrar la ruta especificada.\n");
                        }
                    } else { print("Comando o nombre de archivo no reconocido.\n"); }
                } else if (k_strcmp(cmd_buffer, "logo") == 0) {
                    print_logo();
                } else {
                    print("Comando o nombre de archivo no reconocido.\n");
                }
            }

            // Reset variables for next prompt
            cmd_len = 0;
            edit_pos = 0;
            print(current_dir); print("> ");
            prompt_cursor = cursor; 
        }
    }
}
