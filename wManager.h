#pragma once

#include <curses.h>
#include <panel.h>
#include <semaphore.h>
#include <pthread.h>

#define TAB_NAMES {"ARP", "EVIL TWIN", "OTHER"}
#define TABS_SIZE 3
#define BOT_CMD_KEY {"F1", "F10"}
#define BOT_CMD {"Help", "Quit"}
#define CMD_SIZE 2

typedef struct area_info_
{
  WINDOW* win;
  PANEL* panel;
  int shown;
  int num_col;
  int num_row;
  int startx;
  int starty;
  pthread_mutex_t lock;
}area_info;

typedef struct windows
{
  int max_row;
  int max_col;
  sem_t win_sem;
  area_info* tab_bar;
  area_info* cmd_bar;
  area_info* bot_bar;
  area_info* help_win;
  area_info* arp_left;
  area_info* arp_left_cont;
  area_info* arp_right;
} winStruct;


void* runWindowThread(void* winsPtr);
void initScreen(winStruct* wins);
void printTabBar(winStruct* wins, int highlight);
void printBottomBar(winStruct* wins);
void printHelp(winStruct* wins);
void parseInput(winStruct* wins);
void showHelp(winStruct* wins);
area_info* setupArea(int num_row, int num_col, int starty, int startx);
void selectNextLine(area_info* active_win);
void selectPrevLine(area_info* active_win);
void parseCommand(winStruct* wins, char* buffer);
