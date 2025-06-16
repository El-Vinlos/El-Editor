//
// Created by El-Vinlos on 16/6/25.
//
#include "editor.h"

int main(int argc, char *argv[])
{
    terminal_init();
    if (argc >= 2) {
        editor_open(argv[1]);
    } else {
        editor_refresh_screen(editor_draw_welcome);
    }
    while (1) {
        editor_refresh_screen(editor_draw_rows);
        editor_process_key();
    }
}
