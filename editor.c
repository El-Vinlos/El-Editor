#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "terminal.h"
#include "editor.h"
#include "utils.h"


#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/
struct editor_config E;

void editor_open(char *file_name){
  FILE *fp = fopen(file_name, "r");
  if (!fp) {
    panic("fopen failed");
  }
  
  char *line = NULL;
  size_t line_cap = 0;
  ssize_t line_len;
  while ((line_len = getline(&line, &line_cap, fp)) != -1) {
    line_len = trim_newline_char(line_len,line);
    editor_append_row(line, line_len);
  }

  free(line);
  fclose(fp);
}

void editor_append_row(char *s, size_t len){
  E.row = realloc(E.row, sizeof(erow) * (E.num_rows + 1));

  int at = E.num_rows;
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  E.num_rows++;
}

int trim_newline_char(int line_len, char *line){
    while (line_len > 0 && ((line[line_len-1] == '\n') || 
      line[line_len-1] == '\r')) {
      line_len--;
    }
  return line_len;
}

/*** input ***/
int editor_read_key(void){
  int nread;
  char key;
  while ((nread = read(STDIN_FILENO, &key, 1)) != 1) {
    if ((nread == -1) && errno != EAGAIN ){
      panic("read failed");
    }
  }

  if (key != '\x1b') return key;

  char seq[3];
  if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
  if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

  if (seq[0] != '[') return '\x1b';

  if (seq[1] >= '0' && seq[1] <= '9'){
    if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
    if (seq[2] != '~') return '\x1b';

    switch (seq[1]) {
      case '1': return home_key;
      case '3': return del_key;
      case '4': return end_key;
      case '5': return page_up;
      case '6': return page_down;
      case '7': return home_key;
      case '8': return end_key;
    }
  }

  switch (seq[1]) {
    case 'A': return arrow_up;
    case 'B': return arrow_down;
    case 'C': return arrow_right;
    case 'D': return arrow_left;
    case 'H': return home_key;
    case 'F': return end_key;
  }

  if (seq[0] == '0') {
    switch (seq[1]) {
      case 'H': return home_key;
      case 'F': return end_key;
    }
  }

  return '\x1b';
}

void editor_move_caret(int key){
  erow *row = (E.caret_y_pos >= E.num_rows) ? NULL : &E.row[E.caret_y_pos];

  switch (key) {
    case arrow_up:
    E.caret_y_pos = sat_sub(E.caret_y_pos, 1);
    break;
    case arrow_left:
      if (E.caret_x_pos != 0) {
        E.caret_x_pos--;
      } else if (E.caret_y_pos > 0) {
        E.caret_y_pos--;
        E.caret_x_pos = E.row[E.caret_y_pos].size;
      }
    break;
    case arrow_down:
      if (E.caret_y_pos < E.num_rows) {
        E.caret_y_pos = sat_add(E.caret_y_pos, 1);
      }
    break;
    case arrow_right:
      if (row && E.caret_x_pos < row->size) {
        E.caret_x_pos = sat_add(E.caret_x_pos,1);
      } else if (row && E.caret_x_pos == row->size) {
        E.caret_y_pos = sat_add(E.caret_y_pos, 1);
        E.caret_x_pos = 0;
      }
  } 
  row = (E.caret_y_pos >= E.num_rows) ? NULL : &E.row[E.caret_y_pos];
  int row_len = row ? row->size : 0;
  if (E.caret_x_pos > row_len) {
    E.caret_x_pos = row_len;
  }
}

void editor_process_key(void){
  int key = editor_read_key();

  switch (key) {
    case CTRL_KEY('q'):
      exit(0);

    case page_up:
    case page_down:
      {
        int times = E.screen_rows;
        while (times--)
          editor_move_caret(key == page_up ? arrow_up : arrow_down);
      }
    break;

    case home_key:
    E.caret_x_pos = 0;
    break;

    case end_key:
      E.caret_x_pos = E.row[E.caret_y_pos].size;
    break;


    case arrow_right:
    case arrow_up:
    case arrow_down:
    case arrow_left:
      editor_move_caret(key);
    break;
  }
}

/*** output ***/

void editor_draw_welcome(struct abuf *ab){
  int vertical_center = E.screen_rows / 3;
  for (int y = 0; y < E.screen_rows; y++) {
    if (y == vertical_center) {
      draw_welcome_message(ab);
    } else {
      ab_append(ab, "~", 1);
    }

    clear_line(ab);

    if (y < E.screen_rows - 1 ) {
      ab_append(ab, "\r\n", 2);
    }
  }
}

void editor_draw_rows(struct abuf *ab){
  for (int y = 0; y < E.screen_rows; y++) {
    int file_row = y + E.row_offset;
    if (file_row < E.num_rows) {
      editor_draw_erow(ab,file_row, E.col_offset);
    } else {
      ab_append(ab, "~", 1);
    }

    clear_line(ab);

    if (y < E.screen_rows - 1 ) {
      ab_append(ab, "\r\n", 2);
    }
  }
}

void editor_draw_erow(struct abuf *ab, int y, int x){
  unsigned int len = sat_sub(E.row[y].size, x);
  if (len > (unsigned int)E.screen_cols) len = E.screen_cols;
  ab_append(ab, &E.row[y].chars[x], len);
}

void draw_welcome_message(struct abuf *ab){
  char welcome_message[80];
  int welcome_len = snprintf(welcome_message, sizeof(welcome_message), "El_Editor -- version %s", VERSION);
  if (welcome_len > E.screen_cols) {
    welcome_len = E.screen_cols;
  }

  ab_append(ab, "~", 1);
  int padding = (E.screen_cols - welcome_len - 1) / 2;
  if (padding) {
    while (padding-- > 0) {
      ab_append(ab, " ", 1);
    }
  }
  ab_append(ab, welcome_message, welcome_len);
}

int editor_scroll(){
  if (E.caret_y_pos < E.row_offset) {
    E.row_offset = E.caret_y_pos;
    return 1;
  }
  if (E.caret_y_pos >= E.row_offset + E.screen_rows) {
    E.row_offset = E.caret_y_pos - E.screen_rows + 1;
    return 1;
  }

  if (E.caret_x_pos < E.col_offset) {
    E.col_offset = E.caret_x_pos;
    return 1;
  }
  if (E.caret_x_pos >= E.col_offset + E.screen_cols) {
    E.col_offset = E.caret_x_pos - E.screen_cols + 1;
    return 1;
  }
  return 0;
}

void editor_refresh_screen(void (*draw_fn)(struct abuf *ab)){
  if (editor_scroll()) {
    E.need_redrawn = true;
  }
  struct abuf ab = ABUF_INIT;
  if (E.need_redrawn) {
    hide_caret(&ab);
    move_caret_home(&ab);
    draw_fn(&ab);
    show_caret(&ab);
  }
  move_caret_to( (E.caret_x_pos - E.col_offset + 1), (E.caret_y_pos - E.row_offset + 1), &ab);
  E.need_redrawn = false;
  write(STDOUT_FILENO, ab.buffer, ab.len);
  ab_free(&ab);
}

