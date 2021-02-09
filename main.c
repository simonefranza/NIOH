#include <curses.h>

int main(int argc, char* argv[])
{
  initscr();
  cbreak();
  noecho();

  clear();

  addstr("Hi, welcome to NIOH");

  getch();
  endwin();

  return 0;
}
