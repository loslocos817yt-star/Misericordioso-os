extern int cursor, prompt_cursor, edit_pos, cmd_len;
extern char cmd_buffer[256];
extern char history[10][256];
extern int history_count, history_pos;
extern unsigned char inb(unsigned short port);
extern void redraw_line(const char* cmd, int len, int edit_pos);
extern void k_strcpy(char* dest, const char* src);

// ==========================================
// TECLADO MODIFICADO PARA BASIC EVIL (BE)
// 9 -> (  |  0 -> )  |  - -> * |  ' -> "
// [ -> {  |  ] -> }
// ==========================================
const char scancode_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '(', ')', '*', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '"', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

int expect_extended = 0;

int handle_keyboard() {
    if (!(inb(0x64) & 1)) return 0;
    unsigned char scancode = inb(0x60);
    if (scancode == 0xE0) { expect_extended = 1; return 0; }
    if (scancode & 0x80) { expect_extended = 0; return 0; }

    if (expect_extended) {
        expect_extended = 0;
        if (scancode == 0x48 && history_count > 0 && history_pos > 0) { // ARRIBA
            k_strcpy(cmd_buffer, history[--history_pos]);
            cmd_len = 0; while(cmd_buffer[cmd_len]) cmd_len++;
            edit_pos = cmd_len;
        } else if (scancode == 0x50) { // ABAJO
            if (history_pos < history_count - 1) k_strcpy(cmd_buffer, history[++history_pos]);
            else { history_pos = history_count; cmd_buffer[0] = 0; }
            cmd_len = 0; while(cmd_buffer[cmd_len]) cmd_len++;
            edit_pos = cmd_len;
        } else if (scancode == 0x4B && edit_pos > 0) edit_pos--; // IZQ
        else if (scancode == 0x4D && edit_pos < cmd_len) edit_pos++; // DER
        redraw_line(cmd_buffer, cmd_len, edit_pos);
        return 0;
    }

    char c = scancode_map[scancode];
    if (c == '\n') return 1; // Enter detectado
    if (c == '\b' && edit_pos > 0) {
        for(int i = edit_pos; i <= cmd_len; i++) cmd_buffer[i-1] = cmd_buffer[i];
        cmd_len--; edit_pos--;
    } else if (c != 0 && cmd_len < 255) {
        for(int i = cmd_len; i > edit_pos; i--) cmd_buffer[i] = cmd_buffer[i-1];
        cmd_buffer[edit_pos++] = c; cmd_len++;
    }
    redraw_line(cmd_buffer, cmd_len, edit_pos);
    return 0;
}
