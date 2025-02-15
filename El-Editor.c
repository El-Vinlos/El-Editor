#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#include <stdbool.h>

/*define*/
#define CTRL_KEY(k) ((k) & 0x1f)

/*struct*/
struct editorConfig {
  int width;
  int height;
};

struct editorConfig E;

/*terminal*/
void panic(const char *s) {
  perror(s);
  exit(1);
}

void setMode() {
  HANDLE hStdin;
  hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD mode = 0;
  SetConsoleMode(hStdin,mode);
  mode = ENABLE_INSERT_MODE | ENABLE_VIRTUAL_TERMINAL_INPUT 
        | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT ;
  SetConsoleMode(hStdin,mode);
}

void enableVT () {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE)
  {
    panic("Failed to get HANDLE");
  }

  DWORD dwMode = 0;
  if (!GetConsoleMode(hOut, &dwMode))
  {
    panic("Failed to get console mode output");
  }
  SetConsoleMode(hOut,dwMode);

  dwMode = ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN | ENABLE_PROCESSED_OUTPUT;
  SetConsoleMode(hOut,dwMode);
}

void enterAlternateScreen(){
  printf("\x1b[?1049h");
}

void LeaveAlternateScreen(){
  printf("\x1b[?1049l");
}

void moveCursorto(unsigned x, unsigned y){
  printf("\x1b[%d;%dH",x,y);
}

void moveCursorHome(){
  printf("\x1b[H");
}

void changeTitle(const char *s) {
  printf("\x1b]0;%s\x1b\x5c",s);
}

void clearScreen(){
  printf("\x1b[2j");
  moveCursorHome();
}

char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1) panic("read");
  }
  return c;
}

int GetCursorPosition(int *rows, int *cols){
  printf("\x1b[6n");

  printf("\r\n");
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1) {
    if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
  }

  editorReadKey();

  return -1;
}

int getWinsize(int *cols, int *rows){
  CONSOLE_SCREEN_BUFFER_INFO csbi;


  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)){
    printf("\x1b[999C\x1b[999B");
    return GetCursorPosition(rows,cols);
    
  }
  if ((*cols = csbi.srWindow.Right - csbi.srWindow.Left + 1) == 0)
    panic("0 cols");
  *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  return 0;
}

void initEditor() {
  enableVT();
  setMode();
  enterAlternateScreen();
  changeTitle("Vinh's Editor");
  if (getWinsize(&E.height, &E.width) == -1) panic("getWindowSize");
}

void terminate() {
  clearScreen();
  LeaveAlternateScreen();
  printf("Goodbye\r\n");
  exit(0);
}

/*View*/
/*Input*/

void editorProcessKeypress() {
  char c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
    terminate();
    break;
  }
}

/*Output*/
void drawRows(){
  int y;
  for (y = 0; y < E.height; y++) {
    printf("~\r\n");
  }
}

void refreshScreen(){
  clearScreen();
  drawRows();
  moveCursorHome();
}

int main() {
  initEditor();
  while (1){
    refreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
