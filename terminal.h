#ifndef TERMINAL_H
#define TERMINAL_H

#include <termios.h>
/*
 * Terminal utility functions for screen control and raw mode configuration.
 * These functions interact directly with the terminal via ANSI escape codes
 * and POSIX terminal attributes.
 */

/// append buffer
struct abuf{
  char *buffer;
  int len;
};

#define ABUF_INIT {NULL,0}

void ab_append(struct abuf *ab, const char *s, int len);

void ab_free(const struct abuf *ab);

void clear_screen(struct abuf *ab);

void clear_line(struct abuf *ab);

void move_caret_home(struct abuf *ab);

void move_caret_to(unsigned int x, unsigned int y, struct abuf *ab);

int get_caret_position(int *rows, int *cols);

void hide_caret(struct abuf *ab);

void show_caret(struct abuf *ab);

void change_title_to(const char *s, struct abuf *ab);

void panic(const char *s);

void enable_raw_mode(void);

void disable_raw_mode(void);

void enter_alternate_screen(struct abuf *ab);

void leave_alternate_screen(struct abuf *ab);

int get_win_size(int *rows, int *cols);

void terminate(void);

void init_terminal(void);

#endif // TERMINAL_H
