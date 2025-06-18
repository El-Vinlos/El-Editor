#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>
#include <unistd.h>
#include <errno.h> 
#include <stdbool.h>

#include "terminal.h" 


#define VERSION "0.01"
#define TAP_STOP 2

#define CTRL_KEY(k) ((k) & 0x1f)

/// essentially a c++ vector
typedef struct editor_row{
  int size;
  int render_size;
  char *chars;
  char *render;
} erow;

struct editor_config {
  int caret_x_pos,caret_y_pos;
  int screen_rows;
  int screen_cols;
  int row_offset;
  int col_offset;
  int num_rows;
  erow *row;
  bool need_redrawn;
  struct termios orig_termios;
};

enum editor_key {
  arrow_left = 1001,
  arrow_right ,
  arrow_up ,
  arrow_down ,
  del_key,
  home_key,
  end_key,
  page_up,
  page_down,
};

extern struct editor_config E;

void editor_refresh_screen(void (*draw_fn)(struct abuf *ab));
int editor_read_key(void);
void editor_open(char *file_name);
void editor_process_key(void);
void editor_move_key(char key);
void editor_draw_rows(struct abuf *ab);
void editor_draw_welcome(struct abuf *ab);
void draw_welcome_message(struct abuf *ab);
void editor_draw_erow(struct abuf *ab, int y, int x);
int trim_newline_char(int line_len, char *line);
void editor_append_row(char *s, size_t len);
void editor_update_row(erow *row);
int editor_scroll();

#endif // EDITOR_H
