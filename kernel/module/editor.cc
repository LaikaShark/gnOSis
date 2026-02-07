#include <system.h>

// ── VGA constants ───────────────────────────────────────────────────
#define ED_COLS   80
#define ED_ROWS   25
#define ED_TEXT_ROWS 23   // rows 1..23
#define ED_TAB_STOP  4

#define ED_BUF_SIZE 8192

// ── Attribute helpers ───────────────────────────────────────────────
#define ED_ATTR(bg,fg)  (u8int)(((bg) << 4) | (fg))
#define ED_BAR_ATTR     ED_ATTR(BLUE, WHITE)
#define ED_TEXT_ATTR     ED_ATTR(BLACK, LIGHT_GREY)

// ── Static data ─────────────────────────────────────────────────────
static u16int *ed_vga = (u16int*)0xB8000;

static char    ed_buf[ED_BUF_SIZE];
static int     ed_size;          // bytes of text in buffer
static int     ed_cursor;        // cursor position in buffer
static int     ed_top_line;      // first visible line number
static int     ed_modified;
static char    ed_filename[20];
static const char *ed_message;   // transient status message
static int     ed_exit_pending;  // Ctrl+X pressed once with unsaved changes

// ── VGA helpers ─────────────────────────────────────────────────────

static void ed_vga_put(int x, int y, char ch, u8int attr)
{
    ed_vga[y * ED_COLS + x] = (u16int)ch | ((u16int)attr << 8);
}

static void ed_vga_cursor(int x, int y)
{
    u16int pos = y * ED_COLS + x;
    io::outb(0x3D4, 14);
    io::outb(0x3D5, (pos >> 8) & 0xFF);
    io::outb(0x3D4, 15);
    io::outb(0x3D5, pos & 0xFF);
}

static void ed_clear_row(int y, u8int attr)
{
    for (int x = 0; x < ED_COLS; x++)
        ed_vga_put(x, y, ' ', attr);
}

static int ed_write_str(int x, int y, const char *s, u8int attr)
{
    while (*s && x < ED_COLS)
        ed_vga_put(x++, y, *s++, attr);
    return x;
}

static int ed_write_dec(int x, int y, int n, u8int attr)
{
    // simple decimal print, right at (x,y)
    char tmp[12];
    int i = 0;
    if (n == 0) { tmp[i++] = '0'; }
    else
    {
        if (n < 0) { ed_vga_put(x++, y, '-', attr); n = -n; }
        int rev[12], ri = 0;
        while (n > 0) { rev[ri++] = n % 10; n /= 10; }
        while (ri > 0) tmp[i++] = '0' + rev[--ri];
    }
    for (int j = 0; j < i && x < ED_COLS; j++)
        ed_vga_put(x++, y, tmp[j], attr);
    return x;
}

// ── Buffer queries ──────────────────────────────────────────────────

// Count newlines before pos → gives 0-based line number
static int ed_cursor_line()
{
    int line = 0;
    for (int i = 0; i < ed_cursor; i++)
        if (ed_buf[i] == '\n') line++;
    return line;
}

// Return buffer offset where 'line' starts (0-based)
static int ed_line_start(int line)
{
    int pos = 0;
    int ln = 0;
    while (ln < line && pos < ed_size)
    {
        if (ed_buf[pos] == '\n') ln++;
        pos++;
    }
    return pos;
}

// Length of line starting at 'start' (chars until \n or EOF, not including \n)
static int ed_line_len(int start)
{
    int len = 0;
    while (start + len < ed_size && ed_buf[start + len] != '\n')
        len++;
    return len;
}

// Visual column of buffer position 'pos' on a line starting at 'start'
static int ed_visual_col(int start, int pos)
{
    int vcol = 0;
    for (int i = start; i < pos; i++)
    {
        if (ed_buf[i] == '\t')
            vcol = (vcol / ED_TAB_STOP + 1) * ED_TAB_STOP;
        else
            vcol++;
    }
    return vcol;
}

// Buffer position from visual column on line starting at 'start'
static int ed_pos_from_vcol(int start, int vcol)
{
    int vc = 0;
    int pos = start;
    while (pos < ed_size && ed_buf[pos] != '\n' && vc < vcol)
    {
        if (ed_buf[pos] == '\t')
            vc = (vc / ED_TAB_STOP + 1) * ED_TAB_STOP;
        else
            vc++;
        pos++;
    }
    return pos;
}

// Total line count in buffer
static int ed_total_lines()
{
    int lines = 1;
    for (int i = 0; i < ed_size; i++)
        if (ed_buf[i] == '\n') lines++;
    return lines;
}

// ── Editing operations ──────────────────────────────────────────────

static void ed_insert(char ch)
{
    if (ed_size >= ED_BUF_SIZE - 1) return;
    // Shift right from cursor
    for (int i = ed_size; i > ed_cursor; i--)
        ed_buf[i] = ed_buf[i - 1];
    ed_buf[ed_cursor] = ch;
    ed_size++;
    ed_cursor++;
    ed_modified = 1;
    ed_exit_pending = 0;
}

static void ed_backspace()
{
    if (ed_cursor == 0) return;
    ed_cursor--;
    for (int i = ed_cursor; i < ed_size - 1; i++)
        ed_buf[i] = ed_buf[i + 1];
    ed_size--;
    ed_modified = 1;
    ed_exit_pending = 0;
}

static void ed_delete_char()
{
    if (ed_cursor >= ed_size) return;
    for (int i = ed_cursor; i < ed_size - 1; i++)
        ed_buf[i] = ed_buf[i + 1];
    ed_size--;
    ed_modified = 1;
    ed_exit_pending = 0;
}

// ── Navigation ──────────────────────────────────────────────────────

// Persistent target visual column for up/down movement
static int ed_target_vcol = -1;

static void ed_move_left()
{
    if (ed_cursor > 0) ed_cursor--;
    ed_target_vcol = -1;
}

static void ed_move_right()
{
    if (ed_cursor < ed_size) ed_cursor++;
    ed_target_vcol = -1;
}

static void ed_move_up()
{
    int line = ed_cursor_line();
    if (line == 0) return;
    int ls = ed_line_start(line);
    int vcol = (ed_target_vcol >= 0) ? ed_target_vcol : ed_visual_col(ls, ed_cursor);
    if (ed_target_vcol < 0) ed_target_vcol = vcol;
    int prev_start = ed_line_start(line - 1);
    ed_cursor = ed_pos_from_vcol(prev_start, vcol);
}

static void ed_move_down()
{
    int line = ed_cursor_line();
    int total = ed_total_lines();
    if (line >= total - 1) return;
    int ls = ed_line_start(line);
    int vcol = (ed_target_vcol >= 0) ? ed_target_vcol : ed_visual_col(ls, ed_cursor);
    if (ed_target_vcol < 0) ed_target_vcol = vcol;
    int next_start = ed_line_start(line + 1);
    ed_cursor = ed_pos_from_vcol(next_start, vcol);
}

static void ed_move_home()
{
    int line = ed_cursor_line();
    ed_cursor = ed_line_start(line);
    ed_target_vcol = -1;
}

static void ed_move_end()
{
    int line = ed_cursor_line();
    int ls = ed_line_start(line);
    ed_cursor = ls + ed_line_len(ls);
    ed_target_vcol = -1;
}

static void ed_page_up()
{
    for (int i = 0; i < ED_TEXT_ROWS; i++)
        ed_move_up();
}

static void ed_page_down()
{
    for (int i = 0; i < ED_TEXT_ROWS; i++)
        ed_move_down();
}

// ── Display ─────────────────────────────────────────────────────────

static void ed_ensure_visible()
{
    int line = ed_cursor_line();
    if (line < ed_top_line)
        ed_top_line = line;
    if (line >= ed_top_line + ED_TEXT_ROWS)
        ed_top_line = line - ED_TEXT_ROWS + 1;
}

static void ed_draw_title()
{
    ed_clear_row(0, ED_BAR_ATTR);
    int x = ed_write_str(1, 0, "EDIT: ", ED_BAR_ATTR);
    x = ed_write_str(x, 0, ed_filename, ED_BAR_ATTR);
    if (ed_modified)
        x = ed_write_str(x, 0, " *", ED_BAR_ATTR);

    // Line and column on the right
    int line = ed_cursor_line();
    int ls = ed_line_start(line);
    int col = ed_visual_col(ls, ed_cursor);

    int rx = 60;
    rx = ed_write_str(rx, 0, "Ln:", ED_BAR_ATTR);
    rx = ed_write_dec(rx, 0, line + 1, ED_BAR_ATTR);
    rx = ed_write_str(rx, 0, " Col:", ED_BAR_ATTR);
    rx = ed_write_dec(rx, 0, col + 1, ED_BAR_ATTR);
}

static void ed_draw_text()
{
    for (int row = 0; row < ED_TEXT_ROWS; row++)
    {
        int screen_y = row + 1;
        ed_clear_row(screen_y, ED_TEXT_ATTR);

        int line = ed_top_line + row;
        int ls = ed_line_start(line);
        // Don't draw if past end of buffer
        if (line >= ed_total_lines()) continue;

        int len = ed_line_len(ls);
        int x = 0;
        for (int i = 0; i < len && x < ED_COLS; i++)
        {
            char ch = ed_buf[ls + i];
            if (ch == '\t')
            {
                int next = (x / ED_TAB_STOP + 1) * ED_TAB_STOP;
                while (x < next && x < ED_COLS)
                    ed_vga_put(x++, screen_y, ' ', ED_TEXT_ATTR);
            }
            else
            {
                ed_vga_put(x++, screen_y, ch, ED_TEXT_ATTR);
            }
        }
    }
}

static void ed_draw_status()
{
    ed_clear_row(24, ED_BAR_ATTR);
    if (ed_message)
    {
        ed_write_str(1, 24, ed_message, ED_BAR_ATTR);
        ed_message = 0;
    }
    else
    {
        ed_write_str(1, 24, "^X Exit  ^S Save", ED_BAR_ATTR);
    }
}

static void ed_update_cursor()
{
    int line = ed_cursor_line();
    int ls = ed_line_start(line);
    int vcol = ed_visual_col(ls, ed_cursor);
    int screen_row = line - ed_top_line + 1;
    // Clamp to visible area
    if (vcol >= ED_COLS) vcol = ED_COLS - 1;
    if (screen_row < 1) screen_row = 1;
    if (screen_row > ED_TEXT_ROWS) screen_row = ED_TEXT_ROWS;
    ed_vga_cursor(vcol, screen_row);
}

static void ed_redraw()
{
    ed_draw_title();
    ed_draw_text();
    ed_draw_status();
    ed_update_cursor();
}

// ── File I/O ────────────────────────────────────────────────────────

static int ed_load()
{
    int namelen = 0;
    while (ed_filename[namelen]) namelen++;

    int fd = fs_open(ed_filename, namelen, FS_MODE_READ);
    if (fd < 0) return -1; // file doesn't exist, start empty

    int nread = fs_read(fd, (u8int*)ed_buf, ED_BUF_SIZE - 1);
    fs_close(fd);
    if (nread < 0) nread = 0;
    ed_size = nread;
    return 0;
}

static void ed_save()
{
    int namelen = 0;
    while (ed_filename[namelen]) namelen++;

    // Delete old file, create new, write
    fs_delete(ed_filename, namelen);
    if (fs_create(ed_filename, namelen) != 0)
    {
        ed_message = "Error: could not create file";
        return;
    }
    int fd = fs_open(ed_filename, namelen, FS_MODE_WRITE);
    if (fd < 0)
    {
        ed_message = "Error: could not open file";
        return;
    }
    if (ed_size > 0)
        fs_write(fd, (const u8int*)ed_buf, ed_size);
    fs_close(fd);
    fs_flush();
    ed_modified = 0;
    ed_message = "Saved.";
}

// ── Input ───────────────────────────────────────────────────────────

static u8int ed_getkey()
{
    char c;
    do { c = keyboard_getchar(); } while (c == '\0');
    return (u8int)c;
}

// ── Main entry point ────────────────────────────────────────────────

void editor_open(const char* name, int namelen)
{
    // Init state
    ed_size = 0;
    ed_cursor = 0;
    ed_top_line = 0;
    ed_modified = 0;
    ed_message = 0;
    ed_exit_pending = 0;
    ed_target_vcol = -1;

    // Copy filename
    if (namelen > 18) namelen = 18;
    for (int i = 0; i < namelen; i++)
        ed_filename[i] = name[i];
    ed_filename[namelen] = '\0';

    // Zero out buffer
    for (int i = 0; i < ED_BUF_SIZE; i++)
        ed_buf[i] = 0;

    // Try to load existing file
    ed_load();

    // Initial draw
    ed_redraw();

    // Main loop
    int running = 1;
    while (running)
    {
        u8int key = ed_getkey();

        switch (key)
        {
            case 24: // Ctrl+X
                if (ed_modified && !ed_exit_pending)
                {
                    ed_message = "Unsaved changes! ^X again to exit, ^S to save";
                    ed_exit_pending = 1;
                }
                else
                {
                    running = 0;
                }
                break;

            case 19: // Ctrl+S
                ed_save();
                ed_exit_pending = 0;
                break;

            case KEY_UP:
                ed_move_up();
                break;
            case KEY_DOWN:
                ed_move_down();
                break;
            case KEY_LEFT:
                ed_move_left();
                break;
            case KEY_RIGHT:
                ed_move_right();
                break;
            case KEY_HOME:
                ed_move_home();
                break;
            case KEY_END:
                ed_move_end();
                break;
            case KEY_PGUP:
                ed_page_up();
                break;
            case KEY_PGDN:
                ed_page_down();
                break;
            case KEY_DELETE:
                ed_delete_char();
                break;

            case '\b': // Backspace
                ed_backspace();
                break;

            case '\n': // Enter
                ed_insert('\n');
                ed_target_vcol = -1;
                break;

            case '\t': // Tab
                ed_insert('\t');
                ed_target_vcol = -1;
                break;

            default:
                // Only insert printable ASCII
                if (key >= 32 && key < 127)
                {
                    ed_insert(key);
                    ed_target_vcol = -1;
                }
                break;
        }

        ed_ensure_visible();
        ed_redraw();
    }

    // Drain any leftover keys in the buffer
    while (keyboard_getchar() != '\0') {}

    // Restore normal screen
    clrscr();
}
