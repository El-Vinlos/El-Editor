#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "editor.h"

/*** struct ***/
struct termios orig_termios;


void ab_append(struct abuf *ab, const char *s, int len){
  char *new = realloc(ab->buffer, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->buffer = new;
  ab->len += len;
}

void ab_free(const struct abuf *ab){
  free(ab->buffer);
}

void clear_line(struct abuf *ab){
  ab_append(ab, "\x1b[K", 3);
}

void clear_screen(struct abuf *ab){
  ab_append(ab, "\x1b[2J", 4);
}

void move_caret_home(struct abuf *ab){
  ab_append(ab, "\x1b[H", 3);
}

void move_caret_to(unsigned int x, unsigned int y, struct abuf *ab){
  char ansi_esc_sq[32];
  uint16_t pos_len = snprintf(ansi_esc_sq, sizeof(ansi_esc_sq), "\x1b[%u;%uH",y,x);
  ab_append(ab, ansi_esc_sq, pos_len);
}

int get_caret_position(int *rows, int *cols){
  char buf[32];
  unsigned int i = 0;
  
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }

  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}

void hide_caret(struct abuf *ab){
  ab_append(ab, "\x1b[?25l", 6);
}

void show_caret(struct abuf *ab){
  ab_append(ab, "\x1b[?25h", 6);
}

void change_title_to(const char *s, struct abuf *ab){
  char title[80];
  int title_len = snprintf(title, sizeof(title), "\x1b]0;%s\x1b\x5c",s);
  ab_append(ab, title, title_len);
}


void panic(const char *s){
  struct abuf ab = ABUF_INIT;
  clear_screen(&ab);
  move_caret_home(&ab);
  write(STDOUT_FILENO, ab.buffer, ab.len);
  ab_free(&ab);

  perror(s);
  exit(1);
}
/*** terminal ***/

void disable_raw_mode(void){
   if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios)) {
    panic("can't reset terminal");
   }
}

void enable_raw_mode(void){
  tcgetattr(STDIN_FILENO, &E.orig_termios);
  atexit(disable_raw_mode);

  struct termios raw = E.orig_termios;

  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw)) {
    panic("can't set up terminal");
  }
}

void enter_alternate_screen(struct abuf *ab){
  ab_append(ab, "\x1b[?1049h", 8);
}

void leave_alternate_screen(struct abuf *ab){
  ab_append(ab, "\x1b[?1049l", 8);
}

int get_win_size(int *rows, int *cols){
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDIN_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return get_caret_position(rows, cols);
  }

  *rows = ws.ws_row;
  *cols = ws.ws_col;
  return 0;
}

void terminate(void){
  struct abuf ab = ABUF_INIT;
  disable_raw_mode();
  clear_screen(&ab);
  leave_alternate_screen(&ab);
  move_caret_home(&ab);
  write(STDOUT_FILENO, ab.buffer, ab.len);
  ab_free(&ab);
}

void init_terminal(void){
  struct abuf ab = ABUF_INIT;
  E.caret_x_pos = 0;
  E.caret_y_pos = 0;
  E.num_rows = 0;
  E.row_offset = 0;
  E.col_offset = 0;
  E.row = NULL;
  E.need_redrawn = true;
  enter_alternate_screen(&ab);
  enable_raw_mode();
  atexit(terminate);
  move_caret_home(&ab);
  if (get_win_size(&E.screen_rows, &E.screen_cols)) panic("can't get windows size");
  change_title_to("Vinh's Editor",&ab);
  write(STDOUT_FILENO, ab.buffer, ab.len);
  ab_free(&ab);
}
