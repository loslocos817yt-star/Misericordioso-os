// ==========================================
// MISERICORDIOSO OS - MANEJO DE COMANDOS
// ==========================================

// Importamos lo que necesitamos de kernel.c
extern void print(const char* str);
extern void clear_screen();
extern void print_logo();
extern int k_strcmp(const char* s1, const char* s2);
extern int k_strncmp(const char* s1, const char* s2, int n);
extern unsigned char inb(unsigned short port);
extern void outb(unsigned short port, unsigned char data);
extern unsigned char bcd_to_bin(unsigned char bcd);
extern unsigned char read_cmos(unsigned char reg);
extern void print_time_part(unsigned char val);

extern unsigned char current_color;
extern char current_dir[64];
extern const char* valid_dirs[];
extern const int num_valid_dirs;

void process_command(char* cmd_buffer) {
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
