#include "wManager.h"
#include <stdlib.h>
#include "main.h"

void* runWindowThread(void* winsPtr)
{
  initScreen(winsPtr);
  sem_post(&((winStruct*)winsPtr)->win_sem);
  parseInput(winsPtr);

  return 0;
}

void initScreen(winStruct* wins)
{
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  getmaxyx(stdscr, wins->max_row, wins->max_col);
  clear();

  int max_row = wins->max_row;
  int max_col = wins->max_col;
  wins->arp_left_cont = setupArea(max_row - 3, max_col / 2, 2, 0);
  wborder(wins->arp_left_cont->win, ' ', ACS_VLINE, ' ', ' ', ' ', ACS_VLINE, ' ', ' ');
  wins->arp_left = setupArea(max_row - 3, max_col / 2 - 1, 2, 0);
  wins->arp_right = setupArea(max_row - 3, max_col / 2 - 1, 2, max_col / 2 + 1);

  scrollok(wins->arp_right->win, 1);
  idlok(wins->arp_right->win, 1);

  wins->tab_bar = setupArea(2, max_col, 0, 0);
  wborder(wins->tab_bar->win, ' ', ' ', ' ', ACS_HLINE, ' ', ' ', ' ', ' ');
  wins->bot_bar = setupArea(1, max_col, LINES - 1, 0);
  /* help should be on top of the panel stack */
  wins->help_win = setupArea(max_row, max_col, 0, 0);

  printTabBar(wins, 0);
  printBottomBar(wins);
  printHelp(wins);
  hide_panel(wins->help_win->panel);
  wins->help_win->shown = 0;
  update_panels();
  doupdate();
  
}

void printTabBar(winStruct* wins, int highlight)
{
  wmove(wins->tab_bar->win, 0, 1);
  char* tab_names[] = TAB_NAMES;
  for(int i = 0; i < TABS_SIZE; i++)
  {
    if(i == highlight)
      wattron(wins->tab_bar->win, A_REVERSE);

    wprintw(wins->tab_bar->win, "   %s   ", tab_names[i]);

    if(i == highlight)
      wattroff(wins->tab_bar->win, A_REVERSE);
  }
  update_panels();
}

void printBottomBar(winStruct* wins)
{
  wmove(wins->bot_bar->win, 0, 0);
  char* bot_cmd[] = BOT_CMD;
  char* bot_cmd_key[] = BOT_CMD_KEY;
  for(int i =  0; i < CMD_SIZE; i++)
  {
    wattron(wins->bot_bar->win, A_REVERSE);
    wprintw(wins->bot_bar->win, "%s", bot_cmd_key[i]);
    wattroff(wins->bot_bar->win, A_REVERSE);
    wprintw(wins->bot_bar->win, " %s ", bot_cmd[i]);
  }
}

void parseInput(winStruct* wins)
{
  short int ch;
  char quit = 0;
  int currentTab = 0;
  area_info* active_win = wins->arp_left;
  set_panel_userptr(active_win->panel, 0);
  while(!quit)
  {
    mvprintw(LINES - 2, 0, "Got ");
    ch = getch();
    switch(ch)
    {
      /* next tab */
      case '\t':
        currentTab = (currentTab + 1) % TABS_SIZE;
        printTabBar(wins, currentTab);
        break;
      /* prev tab */
      case KEY_BTAB:
        currentTab = ((!currentTab ? TABS_SIZE : currentTab) - 1) % TABS_SIZE;
        printTabBar(wins, currentTab);
        break;
      case KEY_F(1):
        showHelp(wins);
        break;
      /* quit */
      case KEY_F(10):
        clear();
        quit = 1;
        break;
      case KEY_DOWN:
        selectNextLine(active_win);
        break;
      case KEY_UP:
        selectPrevLine(active_win);
        break;
      default:
        printw("%c", ch);
    }
  }
  exit(1);
}

void printHelp(winStruct* wins)
{
  wmove(wins->help_win->win, 0, 0);
  wprintw(wins->help_win->win, "This is the help page\n\n");
  wprintw(wins->help_win->win, "Press any key to return.\n");
}

void showHelp(winStruct* wins)
{
  show_panel(wins->help_win->panel);
  wgetch(wins->help_win->win);
  hide_panel(wins->help_win->panel);
  update_panels();
}

area_info* setupArea(int num_row, int num_col, int starty, int startx)
{
  area_info* new_area = (area_info*)calloc(1, sizeof(area_info));
  new_area->num_col = num_col;
  new_area->num_row = num_row;
  new_area->startx = startx;
  new_area->starty = starty;
  new_area->win = newwin(num_row, num_col, starty, startx);
  new_area->panel = new_panel(new_area->win);
  return new_area;
}

void selectNextLine(area_info* active_win)
{
  arp_resp_info* resp_info = panel_userptr(active_win->panel);
  if(!resp_info)
    return;
  pthread_mutex_lock(&resp_info->lock);
  if(resp_info->selected == -1 || resp_info->selected == resp_info->size - 1)
  {
    pthread_mutex_unlock(&resp_info->lock);
    return;
  }
  
  int old_col, old_row;
  getyx(active_win->win, old_col, old_row);
  mvwchgat(active_win->win, resp_info->selected++ + 2, 0, -1, A_NORMAL, 1, NULL);
  mvwchgat(active_win->win, resp_info->selected + 2, 0, -1, A_REVERSE, 1, NULL);
  wmove(active_win->win, old_row, old_col);
  update_panels();
  pthread_mutex_unlock(&resp_info->lock);
}

void selectPrevLine(area_info* active_win)
{
  arp_resp_info* resp_info = panel_userptr(active_win->panel);
  if(!resp_info)
    return;
  pthread_mutex_lock(&resp_info->lock);
  if(resp_info->selected == -1 || resp_info->selected == 0)
  {
    pthread_mutex_unlock(&resp_info->lock);
    return;
  }
  
  int old_col, old_row;
  getyx(active_win->win, old_col, old_row);
  mvwchgat(active_win->win, resp_info->selected-- + 2, 0, -1, A_NORMAL, 1, NULL);
  mvwchgat(active_win->win, resp_info->selected + 2, 0, -1, A_REVERSE, 1, NULL);
  wmove(active_win->win, old_row, old_col);
  update_panels();
  pthread_mutex_unlock(&resp_info->lock);
}
