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

void kernel_main() {
    clear_screen();
    print_logo();
    print("\n");
    print("Misericordioso OS v1.0\n");
    print("Copyright 2026-2036 PattitexTEC\n\n");

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
