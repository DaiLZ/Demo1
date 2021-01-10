/*#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
void *run(void *ptr){
	int i;
	for( i=0;i<3;i++)
	{
		sleep(1);	
		printf("hello %d\n",i);
	}
	return 0;
}
int main()
{
	pthread_t id;
	int ret=0;
	ret=pthread_create(&id,NULL,run,NULL);
	if(ret){
		printf("create thread failed\n");
	}
	pthread_join(id,NULL);
	return 0;
}*/
#include <time.h>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
// block layout is: {w-1,h-1}{x0,y0}{x1,y1}{x2,y2}{x3,y3} (two bits each)
int x = 431424, y = 598356, r = 427089, px = 247872, py = 799248, pr,
    c = 348480, p = 615696, tick, board[20][10],
    block[7][4] = {{431424, 598356, 431424, 598356},
                   {427089, 615696, 427089, 615696},
                   {348480, 348480, 348480, 348480},
                   {599636, 431376, 598336, 432192},
                   {411985, 610832, 415808, 595540},
                   {247872, 799248, 247872, 799248},
                   {614928, 399424, 615744, 428369}},
    score = -1;
int x1 = 431424, yy1 = 598356, r1 = 427089, px1 = 247872, py1 = 799248, pr1,
    c1 = 348480, p1 = 615696, tick1, board1[20][10],
    score1 = -1;
  pthread_t id;
  pthread_mutex_t mutex;
// extract a 2-bit number from a block entry
int NUM(int x, int y) { return 3 & block[p][x] >> y; }
int NUM1(int x, int y) { return 3 & block[p1][x] >> y; }

// create a new piece, don't remove old one (it has landed and should stick)
void new_piece() {
  y = py = 0;
  p = rand() % 7;
  r = pr = rand() % 4;
  x = px = rand() % (10 - NUM(r, 16));
  score++;
}
void new_piece1() {
  yy1 = py1 = 0;
  p1 = rand() % 7;
  r1 = pr1 = rand() % 4;
  x1 = px1 = rand() % (10 - NUM1(r1, 16));
  score1++;
}
// draw the board and score
void frame() {
 pthread_mutex_lock(&mutex);
  for (int i = 0; i < 20; i++) {
    move(1 + i,1); // otherwise the box won't draw
    for (int j = 0; j < 10; j++) {
      board[i][j] && attron(262176 | board[i][j] << 8);
      printw("  ");
      attroff(262176 | board[i][j] << 8);
    }
  }
  move(22, 5);
  printw("Score: %d", score);
  refresh();
 pthread_mutex_unlock(&mutex);
}
void frame1(){
 pthread_mutex_lock(&mutex);
 for (int i = 0; i < 20; i++) {
    move(1 + i,38+ 1); // otherwise the box won't draw
    for (int j = 0; j < 10; j++) {
      board1[i][j] && attron(262176 | board1[i][j] << 8);
      printw("  ");
      attroff(262176 | board1[i][j] << 8);
    }
  }
  move(22, 38+5);
  printw("Score: %d", score1);
  refresh();
  pthread_mutex_unlock(&mutex);
}

// set the value fo the board for a particular (x,y,r) piece
void set_piece(int x, int y, int r, int v) {
  for (int i = 0; i < 8; i += 2) {
    board[NUM(r, i * 2) + y][NUM(r, (i * 2) + 2) + x] = v;
  }
}
void set_piece1(int x, int y, int r, int v) {
  for (int i = 0; i < 8; i += 2) {
    board1[NUM1(r, i * 2) + y][NUM1(r, (i * 2) + 2) + x] = v;
  }
}
// move a piece from old (p*) coords to new
int update_piece() {
  set_piece(px, py, pr, 0);
  set_piece(px = x, py = y, pr = r, p + 1);
}
int update_piece1() {
  set_piece1(px1, py1, pr1, 0);
  set_piece1(px1 = x1, py1 = yy1, pr1 = r1, p1 + 1);
}
// remove line(s) from the board if they're full
void remove_line() {
  for (int row = y; row <= y + NUM(r, 18); row++) {
    c = 1;
    for (int i = 0; i < 10; i++) {
      c *= board[row][i];
    }
    if (!c) {
      continue;
    }
    for (int i = row - 1; i > 0; i--) {
      memcpy(&board[i + 1][0], &board[i][0], 40);
    }
    memset(&board[0][0], 0, 10);
    score+=10;
  }
}
void remove_line1() {
  for (int row = yy1; row <= yy1 + NUM1(r1, 18); row++) {
    c1 = 1;
    for (int i = 0; i < 10; i++) {
      c1 *= board1[row][i];
    }
    if (!c1) {
      continue;
    }
    for (int i = row - 1; i > 0; i--) {
      memcpy(&board1[i + 1][0], &board1[i][0], 40);
    }
    memset(&board1[0][0], 0, 10);
    score1+=10;
  }
}
// check if placing p at (x,y,r) will be a collision
int check_hit(int x, int y, int r) {
  if (y + NUM(r, 18) > 19) {
    return 1;
  }
  set_piece(px, py, pr, 0);
  c = 0;
  for (int i = 0; i < 8; i += 2) {
    board[y + NUM(r, i * 2)][x + NUM(r, (i * 2) + 2)] && c++;
  }
  set_piece(px, py, pr, p + 1);
  return c;
}
int check_hit1(int x, int y, int r) {
  if (y + NUM1(r, 18) > 19) {
    return 1;
  }
  set_piece1(px1, py1, pr1, 0);
  c1 = 0;
  for (int i = 0; i < 8; i += 2) {
    board1[y + NUM1(r, i * 2)][x + NUM1(r, (i * 2) + 2)] && c1++;
  }
  set_piece1(px1, py1, pr1, p1 + 1);
  return c1;
}
// slowly tick the piece y position down so the piece falls
int do_tick() {
  if (++tick > 30) {
    tick = 0;
    if (check_hit(x, y + 1, r)) {
      if (!y) {
        return 0;
      }
      remove_line();
      new_piece();
    } else {
      y++;
      update_piece();
    }
  }
  return 1;
}
int do_tick1() {
  if (++tick1 > 30) {
    tick1 = 0;
    if (check_hit1(x1, yy1 + 1, r1)) {
      if (!yy1) {
        return 0;
      }
      remove_line1();
      new_piece1();
    } else {
      yy1++;
      update_piece1();
    }
  }
  return 1;
}
// main game loop with wasd input checking
void runloop() {
  while (do_tick()) {
    usleep(20000);
 c=c1=getch();
    if (c == 'a' && x > 0 && !check_hit(x - 1, y, r)) {
      x--;
    }
    if (c == 'd' && x + NUM(r, 16) < 9 && !check_hit(x + 1, y, r)) {
      x++;
    }
    if (c == 's') {
      while (!check_hit(x, y + 1, r)) {
        y++;
        update_piece();
      }
      remove_line();
      new_piece();
    }
    if (c == 'w') {
      ++r;
      r %= 4;
      while (x + NUM(r, 16) > 9) {
        x--;
      }
      if (check_hit(x, y, r)) {
        x = px;
        r = pr;
      }
    }
    if (c == 'q') {
      return;
    }
    update_piece();
    frame();
  }
}

void runloop1() {
  while (do_tick1()) {
    usleep(20000);
 c=c1=getch();
    if (c1 == 'j' && x1 > 0 && !check_hit1(x1 - 1, yy1, r1)) {
      x1--;
    }
    if (c1 == 'l' && x1 + NUM1(r1, 16) < 9 && !check_hit1(x1 + 1, yy1, r1)) {
      x1++;
    }
    if (c1 == 'k') {
      while (!check_hit1(x1, yy1 + 1, r1)) {
        yy1++;
        update_piece1();
      }
      remove_line1();
      new_piece1();
    }
    if (c1 == 'i') {
      ++r1;
	r1%=4;
      while (x1 + NUM1(r1, 16) > 9) {
        x1--;
      }
      if (check_hit1(x1, yy1, r1)) {
        x1 = px1;
        r1 = pr1;
      }
    }
    if (c1 == 'q') {
      return;
    }
    update_piece1();
    frame1();
  }
}
void *run(void *ptr)
{
  new_piece1();
  runloop1();
}
// init curses and start runloop
int main() {
  WINDOW *user1;
  WINDOW *user2;
  srand(time(0));
  initscr();
  start_color();
  // colours indexed by their position in the block
  for (int i = 1; i < 8; i++) {
    init_pair(i, i, 0);
  }
  new_piece();
  resizeterm(23,60);
  noecho();
  timeout(0);
  curs_set(0);
  user1= subwin(stdscr,22,22,0,0);
  user2= subwin(stdscr,22,22,0,38);
  box(user1,0 , 0);
  box(user2,0 , 0);
  //runloop();
  pthread_create(&id,NULL,run,NULL);
  runloop();
  pthread_join(id,NULL);
  endwin();
}

