extern int handle_keyboard();
extern void process_command(char* cmd_buffer); // Declaramos nuestra nueva funcion

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outb(unsigned short port, unsigned char data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

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

int k_strlen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

int cursor = 0;
int prompt_cursor = 0;
char current_dir[64] = "C:\\";
unsigned char current_color = 0x07;

const char* valid_dirs[] = {
    "C:\\", "C:\\bin", "C:\\boot", "C:\\dev", "C:\\etc",
    "C:\\home", "C:\\home\\pattitex", "C:\\root", "C:\\usr"
};
const int num_valid_dirs = 9;

char cmd_buffer[256];
int cmd_len = 0;
int edit_pos = 0;

char history[10][256];
int history_count = 0;
int history_pos = 0;

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

void redraw_line(const char* cmd, int len, int edit_pos) {
    char* video = (char*)0xB8000;
    for(int i = 0; i < 80; i++) {
        video[prompt_cursor + i*2] = ' ';
    }
    for(int i = 0; i < len; i++) {
        video[prompt_cursor + i*2] = cmd[i];
        video[prompt_cursor + i*2 + 1] = current_color;
    }
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

/* ============================================================
   INTERFAZ ESTILO MIDNIGHT COMMANDER
   ============================================================ */

#define MC_NEGRO      0
#define MC_AZUL       1
#define MC_VERDE      2
#define MC_CIAN       3
#define MC_ROJO       4
#define MC_MAGENTA    5
#define MC_CAFE       6
#define MC_GRIS_CL    7
#define MC_GRIS_OS    8
#define MC_AZUL_CL    9
#define MC_VERDE_CL   10
#define MC_CIAN_CL    11
#define MC_ROJO_CL    12
#define MC_MAGENTA_CL 13
#define MC_AMARILLO   14
#define MC_BLANCO     15

unsigned char mc_color(unsigned char fg, unsigned char bg) {
    return (unsigned char)((bg << 4) | fg);
}

void put_char_at(int x, int y, char c, unsigned char color) {
    char* video = (char*)0xB8000;
    int offset = (y * 80 + x) * 2;
    video[offset]     = c;
    video[offset + 1] = color;
}

void put_str_at(int x, int y, const char* str, unsigned char color) {
    for (int i = 0; str[i] != '\0'; i++) {
        put_char_at(x + i, y, str[i], color);
    }
}

void put_str_at_right(int x, int w, int y, const char* str, unsigned char color) {
    int len = k_strlen(str);
    int pad = w - len;
    if (pad < 0) pad = 0;
    put_str_at(x + pad, y, str, color);
}

void fill_rect(int x, int y, int w, int h, char c, unsigned char color) {
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            put_char_at(x + i, y + j, c, color);
}

#define BOX_H  205
#define BOX_V  186
#define BOX_TL 201
#define BOX_TR 187
#define BOX_BL 200
#define BOX_BR 188

void draw_double_box(int x, int y, int w, int h, unsigned char color) {
    put_char_at(x, y, (char)BOX_TL, color);
    put_char_at(x + w - 1, y, (char)BOX_TR, color);
    put_char_at(x, y + h - 1, (char)BOX_BL, color);
    put_char_at(x + w - 1, y + h - 1, (char)BOX_BR, color);
    for (int i = 1; i < w - 1; i++) {
        put_char_at(x + i, y, (char)BOX_H, color);
        put_char_at(x + i, y + h - 1, (char)BOX_H, color);
    }
    for (int j = 1; j < h - 1; j++) {
        put_char_at(x, y + j, (char)BOX_V, color);
        put_char_at(x + w - 1, y + j, (char)BOX_V, color);
    }
}

void draw_mc_menu_bar() {
    unsigned char c = mc_color(MC_NEGRO, MC_CIAN);
    fill_rect(0, 0, 80, 1, ' ', c);
    put_str_at(1, 0, "Izquierda", c);
    put_str_at(12, 0, "Archivo", c);
    put_str_at(21, 0, "Comando", c);
    put_str_at(30, 0, "Opciones", c);
    put_str_at(41, 0, "Derecha", c);
}

void draw_mc_function_bar(int row) {
    const char* labels[10] = {
        "Ayuda","Menu","Ver","Editar","Copiar",
        "Mover","Mkdir","Borrar","Menus","Salir"
    };
    unsigned char keyc = mc_color(MC_BLANCO, MC_NEGRO);
    unsigned char lblc = mc_color(MC_NEGRO, MC_CIAN);
    fill_rect(0, row, 80, 1, ' ', lblc);
    int x = 0;
    for (int i = 0; i < 10; i++) {
        char num[3];
        if (i < 9) { num[0] = '1' + i; num[1] = '\0'; }
        else       { num[0] = '1'; num[1] = '0'; num[2] = '\0'; }
        put_str_at(x, row, num, keyc);
        x += k_strlen(num);
        put_str_at(x, row, labels[i], lblc);
        x += k_strlen(labels[i]) + 1;
    }
}

/* Dibuja el logo (el mismo arreglo de print_logo) centrado dentro de un
   recuadro de coordenadas dadas, usando put_str_at en vez de print(),
   para que quede fijo dentro del marco estilo mc. */
void draw_logo_in_box(int x, int y, int w, int h, unsigned char color) {
    const char* logo[] = {
        "  __  __ _               _                   _ _                    ___  ____",
        " |  \\/  (_)___  ___  _ __(_) ___ ___  _ __ __| (_) ___  ___  ___    / _ \\/ ___|",
        " | |\\/| | / __|/ _ \\ '__| |/ __/ _ \\| '__/ _` | |/ _ \\/ __|/ _ \\  | | | \\___ \\",
        " | |  | | \\__ \\  __/ |  | | (_| (_) | | | (_| | | (_) \\__ \\ (_) | | |_| |___) |",
        " |_|  |_|_|___/\\___|_|  |_|\\___\\___/|_|  \\__,_|_|\\___/|___/\\___/   \\___/|____/",
        0
    };
    int row = y + 2;
    for (int i = 0; logo[i] != 0; i++) {
        put_str_at(x + 2, row, logo[i], color);
        row++;
    }
    row++;
    put_str_at(x + 2, row, "Misericordioso OS v1.0", color);
    row++;
    put_str_at(x + 2, row, "Copyright 2026-2036 PattitexTEC", color);
    row += 2;
    put_str_at(x + 2, row, "Escribe 'Help' para obtener ayuda", color);
}

void draw_mc_interface() {
    clear_screen();

    unsigned char box_c = mc_color(MC_BLANCO, MC_CAFE);
    int top = 0;
    int h = 22; /* filas 0..21, deja fila 22 libre para el prompt */

    fill_rect(0, top + 1, 80, h - 2, ' ', box_c);
    draw_double_box(0, top, 80, h, box_c);
    draw_logo_in_box(0, top, 80, h, box_c);
}

/* ============================================================
   FIN DE LA INTERFAZ MIDNIGHT COMMANDER
   ============================================================ */

void kernel_main() {
    draw_mc_interface();

    current_color = mc_color(MC_BLANCO, MC_AZUL);
    cursor = (22 * 80 + 0) * 2; /* fila 22: justo debajo del recuadro del logo, encima de la barra F1-F10 */
    print(current_dir); print("> ");
    prompt_cursor = cursor;

    while(1) {
        if (handle_keyboard()) {
            cmd_buffer[cmd_len] = '\0';
            print("\n");

            if (cmd_len > 0) {
                if (history_count < 10) {
                    k_strcpy(history[history_count], cmd_buffer);
                    history_count++;
                } else {
                    for(int i=1; i<10; i++) k_strcpy(history[i-1], history[i]);
                    k_strcpy(history[9], cmd_buffer);
                }
                history_pos = history_count;

                // AQUI LLAMAMOS A LA MAGIA DE COM.C
                process_command(cmd_buffer);
            }

            cmd_len = 0;
            edit_pos = 0;
            print(current_dir); print("> ");
            prompt_cursor = cursor;
        }
    }
}
