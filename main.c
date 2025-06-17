//
// Created by El-Vinlos on 11/6/25.
//
#include "editor.h"

int main(int argc, char *argv[])
{
    init_terminal();
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
