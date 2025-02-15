#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#include <stdbool.h>

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
  /*atexit(returnMode);*/
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

void changeTitle(const char *s) {
  printf("\x1b]0;%s\x1b\x5c",s);
}

void initEditor() {
  enableVT();
  setMode();
  enterAlternateScreen();
  changeTitle("Vinh's Editor");
  moveCursorto(0,0);
  fflush(stdout);
}

int main() {
  initEditor();
  char input;
  while (read(STDIN_FILENO, &input, 1) == 1){
    if (GetKeyState(VK_CONTROL) & 0x80)
      printf("%d\n",input);
    else 
      printf("%d (%c)\n",input,input);

    if (input == 'q')
      break;
    
  }
  LeaveAlternateScreen();
  return 0;
}
