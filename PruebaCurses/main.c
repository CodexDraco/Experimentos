#include <ncurses.h>

int main()
{
	int cols, rows;
	initscr();
	cbreak();
	noecho();
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);

	attron(COLOR_PAIR(1));
	getmaxyx(stdscr, rows, cols);
	mvprintw(rows / 2, cols / 2 - 6, "Hola mundo!!");
	attroff(COLOR_PAIR(1));

	refresh();
	getch();
	endwin();

	return 0;
}
