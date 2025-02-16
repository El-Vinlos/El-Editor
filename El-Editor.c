#include <stdlib.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#include <stdbool.h>

/*define*/
#define CTRL_KEY(k) ((k) & 0x1f)

/*struct*/
struct editorConfig {
  int win_width;
  int win_height;
  HANDLE org_stdin;
  DWORD org_mode;
};

struct editorConfig E;

/*terminal*/
void panic(const char *s) {
  perror(s);
  exit(1);
}

void resetMode() {
  SetConsoleMode(E.org_stdin, E.org_mode);
}

void setConsole() {
  E.org_stdin = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(E.org_stdin, &E.org_mode);
  DWORD mode = 0;
  atexit(resetMode);
  SetConsoleMode(E.org_stdin,mode);
  mode = ENABLE_INSERT_MODE | ENABLE_VIRTUAL_TERMINAL_INPUT 
        | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT ;
  SetConsoleMode(E.org_stdin,mode);
}

void enableVT () {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut == INVALID_HANDLE_VALUE)
  {
    panic("Failed to get Output HANDLE");
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
  while ((nread = ReadFile(E.org_stdin, &c, 1,NULL,NULL)) != 1) {
    if (nread == -1) panic("read");
  }
  return c;
}

int getCursorPosition(int *rows, int *cols){
  char buf[32];
  unsigned int i = 0;
  printf("\x1b[6n");

  while(i < sizeof(buf) - 1){
    if (ReadFile(E.org_stdin, &buf[i], 1, NULL, NULL) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  
  buf[i] = '\0';
  printf("\n&buf[1]: '%s'\n", &buf[1]);

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf_s(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}

int getWinsize(int *cols, int *rows){
  CONSOLE_SCREEN_BUFFER_INFO csbi;


  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi) == 0){
    printf("\x1b[999C\x1b[999B"); //Move cusor to the bottom right
    return getCursorPosition(rows,cols);
    
  }
  if ((*cols = csbi.srWindow.Right - csbi.srWindow.Left + 1) == 0)
    panic("0 cols");
  *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  return 0;
}

void initEditor() {
  enableVT();
  setConsole();
  enterAlternateScreen();
  changeTitle("Vinh's Editor");
  if (getWinsize(&E.win_height, &E.win_width) == -1) panic("getWindowSize");
}

void terminate() {
  clearScreen();
  LeaveAlternateScreen();
  printf("Goodbye\n");
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
  for (y = 0; y < E.win_height; y++) {
    printf("~");

    if(y < E.win_height - 1)
      printf("\n");
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
