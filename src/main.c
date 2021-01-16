#include <time.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
typedef struct {
	int id;//start from 0
	int x,y;
	int shape;//r
	int px,py;
	int pshape;//pr
	int c;
	int kind;//p
	int tick;
	int score;
	int board[20][10];
	int tipy;
}GameUser;
int block[7][4] = { {431424, 598356, 431424, 598356},//Z1
				   {427089, 615696, 427089, 615696},//Z2 
				   {348480, 348480, 348480, 348480},
				   {599636, 431376, 598336, 432192},
				   {411985, 610832, 415808, 595540},
				   {247872, 799248, 247872, 799248},//|
				   {614928, 399424, 615744, 428369} };
pthread_t id, id_key;
pthread_mutex_t mutex;
GameUser user1, user2;
// extract a 2-bit number from a block entry
int NUM(int x, int y, int p) { return 3 & block[p][x] >> y; }
// create a new piece, don't remove old one (it has landed and should stick)
void new_piece(GameUser *user) {
	(*user).tipy=(*user).y = (*user).py = 0;
	(*user).kind = rand() % 7; // 7 kinds
	(*user).shape = (*user).pshape = rand() % 4;  // 4 shapes
	(*user).x = (*user).px = rand() % (10 - NUM((*user).shape, 16, (*user).kind));
	(*user).score++;
}
// draw the board and score
void frame(GameUser *user) {
	pthread_mutex_lock(&mutex);
	for (int i = 0; i < 20; i++) {
		move(1 + i, (*user).id * 38 + 1); // otherwise the box won't draw
		for (int j = 0; j < 10; j++) {
			(*user).board[i][j] && attron(262176 | (*user).board[i][j] << 8);
			printw("  ");
			attroff(262176 | (*user).board[i][j] << 8);
		}
	}
	move(22, (*user).id * 38 + 5);
	printw("Score: %d", (*user).score);
	refresh();
	pthread_mutex_unlock(&mutex);
}

// set the value fo the board for a particular (x,y,r) piece
void set_piece(int x, int y, int shape,GameUser *user, int v) {
	for (int i = 0; i < 8; i += 2) {
		(*user).board[NUM(shape, i * 2, (*user).kind) + y][NUM(shape, (i * 2) + 2, (*user).kind) + x] = v;
	}
}
int check_hit1(int x, int y, int shape,GameUser *user) {
	int c = 0;
	if (y + NUM(shape, 18, (*user).kind) > 19) {
		return 1;
	}
	for (int i = 0; i < 8; i += 2) {
		if ((*user).board[y + NUM(shape, i * 2, (*user).kind)][x + NUM(shape, (i * 2) + 2, (*user).kind)] == 8) continue;
		(*user).board[y + NUM(shape, i * 2, (*user).kind)][x + NUM(shape, (i * 2) + 2, (*user).kind)] && c++;
	}
	return c;
}
// move a piece from old (p*) coords to new
void update_piece(GameUser *user) {
	set_piece((*user).px, (*user).py,(*user).pshape,user, 0);
	{
		set_piece((*user).px,(*user).tipy,(*user).pshape, user, 0);
		(*user).tipy=(*user).y;
		while (!check_hit1((*user).x,(*user).tipy + 1,(*user).shape, user)) (*user).tipy++;
		set_piece((*user).x, (*user).tipy,(*user).shape, user, 8);
	}
	set_piece((*user).px = (*user).x, (*user).py = (*user).y,(*user).pshape=(*user).shape, user, (*user).kind + 1);
}
// remove line(s) from the board if they're full
void remove_line(GameUser *user) {
	int c;
	for (int row = (*user).y; row <= (*user).y + NUM((*user).shape, 18, (*user).kind); row++) {
		c = 1;
		for (int i = 0; i < 10; i++) {
			if ((*user).board[row][i] == 8) continue;
			c *= (*user).board[row][i];
		}
		if (!c) continue;
		for (int i = row - 1; i > 0; i--) {
			memcpy(&(*user).board[i + 1][0], &(*user).board[i][0], 40);
		}
		memset(&(*user).board[0][0], 0, 10);
		(*user).score += 10;
	}
}

// check if placing p at (x,y,r) will be a collision
int check_hit(int x, int y, GameUser *user) {
	int c = 0;
	if (y + NUM((*user).shape, 18, (*user).kind) > 19) {
		return 1;
	}
	set_piece((*user).px, (*user).py,(*user).pshape, user, 0);
	for (int i = 0; i < 8; i += 2) {
		if ((*user).board[y + NUM((*user).shape, i * 2, (*user).kind)][x + NUM((*user).shape, (i * 2) + 2, (*user).kind)] == 8) continue;
		(*user).board[y + NUM((*user).shape, i * 2, (*user).kind)][x + NUM((*user).shape, (i * 2) + 2, (*user).kind)] && c++;
	}
	set_piece((*user).px, (*user).py,(*user).pshape, user, (*user).kind + 1);
	return c;
}
// slowly tick the piece y position down so the piece falls
int do_tick(GameUser *user) {
	if (++(*user).tick > 30) {
		(*user).tick = 0;
		if (check_hit((*user).x, (*user).y + 1, user)) {
			if (!(*user).y) {
				return 0;
			}
			remove_line(user);
			new_piece(user);
		}
		else {
			(*user).y++;
			update_piece(user);
		}
	}
	return 1;
}

// main game loop with wasd input checking
void runloop(GameUser *user) {
	char left, right, down, swap;
	if ((*user).id == user1.id)
	{
		left = 'a'; right = 'd'; down = 's'; swap = 'w';
	}
	else {
		left = 'j'; right = 'l'; down = 'k'; swap = 'i';
	}
	while (do_tick(&user1) && do_tick(&user2)) {
		usleep(50000);
		if ((*user).c == left && (*user).x > 0 && !check_hit((*user).x - 1, (*user).y, user)) {
			(*user).x--;
		}
		if ((*user).c == right && (*user).x + NUM((*user).shape, 16, (*user).kind) < 9 && !check_hit((*user).x + 1, (*user).y, user)) {
			(*user).x++;
		}
		if ((*user).c == down) {
			while (!check_hit((*user).x, (*user).y + 1, user)) {
				(*user).y++;
				update_piece(user);
			}
			remove_line(user);
			new_piece(user);
		}
		if ((*user).c == swap) {
			++(*user).shape; (*user).shape %= 4;
			while ((*user).x + NUM((*user).shape, 16, (*user).kind) > 9) {
				(*user).x--;
			}
			if (check_hit((*user).x, (*user).y, user)) {
				(*user).x = (*user).px; (*user).shape = (*user).pshape;
			}
		}
		if ((*user).c == 'q') return;
		update_piece(user);
		frame(user);
	}
}

void *run(void *ptr)
{
	new_piece(&user2);
	runloop(&user2);
	return 0;
}
void *run_key(void *ptr) {
	while (do_tick(&user1)&& do_tick(&user2)) {
		usleep(50000);
		user1.c = user2.c = getch();
		if (user1.c == 'q')break;
	}
	return 0;
}
// init curses and start runloop
int main() {
	WINDOW *user1win;
	WINDOW *user2win;
	user1.score = user2.score = -1;
	user1.id = 0;
	user2.id = 1;
	// WINDOW *result;
	srand(time(0));
	initscr();
	start_color();
	// colours indexed by their position in the block
	// add 1 lighter colors for tips
	//init_color(8, 200, 200, 200);
	for (int i = 1; i < 9; i++) {
		init_pair(i, i, 0);
	}

	new_piece(&user1);
	resizeterm(23, 60);
	noecho();
	timeout(0);
	curs_set(0);
	user1win = subwin(stdscr, 22, 22, 0, 0);
	user2win = subwin(stdscr, 22, 22, 0, 38);
	box(user1win, 0, 0);
	box(user2win, 0, 0);
	pthread_create(&id, NULL, run, NULL);
	pthread_create(&id_key, NULL, run_key, NULL);
	runloop(&user1);
	pthread_join(id, NULL);
	pthread_join(id_key, NULL);
	// result= subwin(stdscr,8,30,7,15);
	// box(result,0,0);
   //   if(score>score1){
   //     result.addstr(11,25,"the left win");
   //   }else if(score1>score){
   //     result.addstr(11,25,"the right win");
   //   }
   //   else result.addstr(11,25,"tie");
   //   result.box();
   //   result.refresh();

   //     move(8, 22); // otherwise the box won't draw
   //     printw("----------------");
   //     move(15, 22); // otherwise the box won't draw
   //     printw("----------------");
   //   move(11,25);
   //   if(score>score1){
   //     printw("the left win");
   //   }else if(score1>score){
   //     printw("the right win");
   //   }
   //   else printw("tie");
   //   refresh();
	while (getch() != 'q') {};
	endwin();
}
