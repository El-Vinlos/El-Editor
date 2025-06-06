#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <stdbool.h>
#include <windows.h>

/*define*/
#define VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

/*struct*/
struct abuf {
  char *buffer;
  int len;
};

struct editorConfig {
  uint16_t caret_x_pos, caret_y_pos;
  uint32_t win_width;
  uint32_t win_height;
  void *org_stdin;
  void *org_stdout;
  unsigned long org_mode_in;
  unsigned long org_mode_out;
};

struct editorConfig E;

/*util*/
uint16_t u16_sat_add(uint16_t a, uint16_t b){
  return (a > 0xFFFF -b) ? 0xFFFF : a + b;
}

uint16_t u16_sat_sub(uint16_t a, uint16_t b){
  return (a < b) ? 0: a -b;
}

/*terminal*/
void panic(const char *s) {
  perror(s);
  exit(1);
}

void reset_mode() { SetConsoleMode(E.org_stdin, E.org_mode_in); SetConsoleMode(E.org_stdout, E.org_mode_out); }

void setup_config(){
  if ((E.org_stdout = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE) {
    panic("Failed to get Output HANDLE");
  }
  if ((E.org_stdin = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE) {
    panic("Failed to get Input HANDLE");
  }
  if ( GetConsoleMode(E.org_stdin, &E.org_mode_in) == 0) {
    panic("Failed to get console mode input");
  }
  if ( GetConsoleMode(E.org_stdout, &E.org_mode_out) == 0) {
    panic("Failed to get console mode output");
  }
}

void setup_console_input() {
  unsigned long mode = E.org_mode_in;
  SetConsoleMode(E.org_stdin, mode);
  mode |= ENABLE_INSERT_MODE | ENABLE_VIRTUAL_TERMINAL_INPUT |
         ENABLE_MOUSE_INPUT ;
  mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
  SetConsoleMode(E.org_stdin, mode);
}

void setup_console_output() {
  unsigned long dwMode = E.org_mode_out;
  if (!GetConsoleMode(E.org_stdout, &dwMode)) {
    panic("Failed to get console mode output");
  }
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN |
           ENABLE_PROCESSED_OUTPUT;
  SetConsoleMode(E.org_stdout, dwMode);
}

void setup_console(){
  setup_console_input();
  setup_console_output();
  atexit(reset_mode);
}

///append data to buffer to write
void ab_append(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->buffer, ab->len + len);

  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->buffer = new;
  ab->len += len;
}

///free buffer
void ab_free(struct abuf *ab) { free(ab->buffer); }

void enter_alternate_screen(struct abuf *ab) {
  ab_append(ab, "\x1b[?1049h", 8);
}

void leave_alternate_screen(struct abuf *ab) { ab_append(ab, "\x1b[?1049l", 8); }

void move_caret_to(unsigned int x, unsigned int y, struct abuf *ab) {
  char ansi_esc_sq[32];
  uint16_t pos_len = snprintf(ansi_esc_sq, sizeof(ansi_esc_sq), "\x1b[%u;%uH", y, x);
  if (pos_len > 0 ) {
    ab_append(ab, ansi_esc_sq, pos_len);
  }
}

void move_caret_home(struct abuf *ab) { ab_append(ab, "\x1b[H", 3); }

void hide_caret(struct abuf *ab) { ab_append(ab, "\x1b[?25l", 6); }

void show_caret(struct abuf *ab) { ab_append(ab, "\x1b[?25h", 6); }

void change_title(const char *s, struct abuf *ab) { 
  char title[80];
  int title_len = snprintf(title, sizeof(title), "\x1b]0;%s\x1b\x5c",s);
  ab_append(ab, title, title_len);
}

void clear_screen(struct abuf *ab) { ab_append(ab, "\x1b[2J", 4); }

void clear_line(struct abuf *ab) { ab_append(ab, "\x1b[K", 3); }

char editor_read_key() {
  int nread;
  char c;
  while ((nread = ReadFile(E.org_stdin, &c, 1, NULL, NULL)) != 1) {
    if (nread == 0)
      panic("read");
  }
  return c;
}

int get_cursor_position(uint32_t *rows, uint32_t *cols) {
  char buf[32];
  unsigned int i = 0;
  unsigned long bytes_written;
  if (WriteFile(E.org_stdout, "\x1b[6n", 4, &bytes_written, NULL) != 1 || bytes_written != 4)
    return -1;

  while (i < sizeof(buf) - 1) {
    if (ReadFile(E.org_stdin, &buf[i], 1, NULL, NULL) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }

  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf_s(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;

  return 0;
}

int get_win_size(uint32_t *cols, uint32_t *rows) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if (GetConsoleScreenBufferInfo(E.org_stdout,&csbi) == 0) {
    // Move cusor to the absolute bottom right
    unsigned long bytes_written;
    if (!WriteFile(E.org_stdout, "\x1b[999C\x1b[999B", 12, &bytes_written, NULL) || bytes_written  != 12)
      return -1;
    return get_cursor_position(rows, cols);
  }
  if ((*cols = csbi.srWindow.Right - csbi.srWindow.Left + 1) == 0)
    return (-2);
  if ((*rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1) == 0)
    return (-2);
  return 0;
}

void init_editor() {
  struct abuf ab = ABUF_INIT;
  setup_config();
  setup_console();
  E.caret_x_pos = 0;
  E.caret_y_pos = 0;
  enter_alternate_screen(&ab);
  change_title("Vinh's Editor",&ab);
  switch (get_win_size(&E.win_width, &E.win_height)) {
    case 0:
      break;
    case -1:
      panic("Can't get window size");
      break;
    case -2:
      panic("Window size is too small");
      break;
    default:
      panic("Unknown error getting window size");
      break;
  }
  WriteFile(E.org_stdout, ab.buffer, ab.len, NULL, NULL);
  ab_free(&ab);
}

void terminate() {
  struct abuf ab = ABUF_INIT;

  clear_screen(&ab);
  leave_alternate_screen(&ab);
  ab_append(&ab, "Goodbye.", 8);
  WriteFile(E.org_stdout, ab.buffer, ab.len, NULL, NULL);
  ab_free(&ab);
  exit(0);
}

/*View*/
/*Input*/

void editor_move_caret(char key){
  switch (key) {
    case ('w'):
      E.caret_y_pos = u16_sat_sub(E.caret_y_pos,1);
      break;
    case ('a'):
      E.caret_x_pos = u16_sat_sub(E.caret_x_pos,1);
      break;
    case ('s'):
      E.caret_y_pos = u16_sat_add(E.caret_y_pos, 1);
      break;
    case ('d'):
      E.caret_x_pos = u16_sat_add(E.caret_x_pos, 1);
      break;
  }
}

void editor_process_keypress() {
  char c = editor_read_key();

  switch (c) {
    case CTRL_KEY('q'):
    terminate();
    break;
    case ('w'):
    case ('a'):
    case ('s'):
    case ('d'):
    editor_move_caret(c);
    break;
  }
}

void draw_welcome_message(struct abuf *ab) {
  char welcome[80];
  uint16_t welcomeLen =
      snprintf(welcome, sizeof(welcome), "El-Editor version %s", VERSION);
  if (welcomeLen > E.win_width)
    welcomeLen = E.win_width;
  int padding = (E.win_width - welcomeLen) / 2;
  if (padding) {
    ab_append(ab, "~", 1);
    padding--;
  }
  while (padding--)
    ab_append(ab, " ", 1);
  ab_append(ab, welcome, welcomeLen);
}

/* Output */
void draw_rows(struct abuf *ab) {
  int vertical_centered = E.win_height / 3;
  uint16_t current_rows;
  for (current_rows = 0; current_rows < E.win_height; current_rows++) {
    clear_line(ab);
    if (current_rows == vertical_centered) {
      draw_welcome_message(ab);
    } else
      ab_append(ab, "~", 1);
    if (current_rows < E.win_height - 1)
      ab_append(ab, "\r\n", 2);
  }
}

void refresh_screen() {
  struct abuf ab = ABUF_INIT;
  clear_screen(&ab);
  hide_caret(&ab);
  move_caret_home(&ab);
  draw_rows(&ab);
  move_caret_to(E.caret_x_pos + 1, E.caret_y_pos + 1, &ab);
  show_caret(&ab);
  WriteFile(E.org_stdout, ab.buffer, ab.len, NULL, NULL);
  ab_free(&ab);
}

int main() {
  init_editor();
  while (1) {
    refresh_screen();
    editor_process_keypress();
  }
  return 0;
}
