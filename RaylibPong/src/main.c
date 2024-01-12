#include "raylib.h"

#include <stdio.h>

#define PADDLE_W  16
#define PADDLE_H 128

#define SCREEN_W 1280
#define SCREEN_H  720

#define PADDLE_CENTER_SCREEN (int)(SCREEN_H / 2 - PADDLE_H / 2)

#define BALL_R 8

#define BALL_CENTER_SCREEN_X ((int)(SCREEN_W / 2) - BALL_R)
#define BALL_CENTER_SCREEN_Y ((int)(SCREEN_H / 2) - BALL_R)

#define PADDING 16

typedef struct Paddle_t {
  float x;
  float y;
  int score;
} Paddle;

typedef struct Ball_t {
  float x;
  float y;
  Vector2 speed;
} Ball;

int main() {
  Paddle red = {16, PADDLE_CENTER_SCREEN, 0};
  Paddle blue = {SCREEN_W - PADDLE_W - 16, PADDLE_CENTER_SCREEN, 0};
  Ball ball = {BALL_CENTER_SCREEN_X, BALL_CENTER_SCREEN_Y, {-0.01, -0.01}};

  char red_score[3] = "0";
  char blue_score[3] = "0";
  
  InitWindow(SCREEN_W, SCREEN_H, "raylib Pong");

  while(!WindowShouldClose())
  {
    if(IsKeyDown(KEY_W)) {
      red.y -= 0.05;
      if(red.y < PADDING) red.y = PADDING;
    } else if(IsKeyDown(KEY_S)) {
      red.y += 0.05;
      if(red.y > SCREEN_H - PADDLE_H - PADDING) red.y = SCREEN_H - PADDLE_H - PADDING;
    }
    if(IsKeyDown(KEY_UP)) {
      blue.y -= 0.05;
      if(blue.y < PADDING) blue.y = PADDING;
    } else if(IsKeyDown(KEY_DOWN)) {
      blue.y += 0.05;
      if(blue.y > SCREEN_H - PADDLE_H - PADDING) blue.y = SCREEN_H - PADDLE_H - PADDING;
    }

    ball.x += ball.speed.x;
    ball.y += ball.speed.y;
    if(ball.x < 16) {
      ball.x = BALL_CENTER_SCREEN_X;
      ball.y = BALL_CENTER_SCREEN_Y;
      blue.score += 1;
      snprintf(blue_score, 3, "%d", blue.score);
    }
    if(ball.x > SCREEN_W - 16 - BALL_R) {
      ball.x = BALL_CENTER_SCREEN_X;
      ball.y = BALL_CENTER_SCREEN_Y;
      red.score += 1;
      snprintf(red_score, 3, "%d", red.score);
    }
    if(ball.y < PADDING) {
      ball.speed.y *= -1;
      ball.y = PADDING;
    }
    if(ball.y > SCREEN_H - PADDING - BALL_R) {
      ball.speed.y *= -1;
      ball.y = SCREEN_H - PADDING - BALL_R;
    }

    if(ball.x < red.x + BALL_R + PADDLE_W && red.y <= ball.y && ball.y <= red.y + PADDLE_H ) {
      ball.speed.x *= -1;
    }

    if(ball.x > blue.x - BALL_R && blue.y <= ball.y && ball.y <= blue.y + PADDLE_H){
      ball.speed.x *= -1;
    }
    
    BeginDrawing();
    ClearBackground(DARKGRAY);
    DrawRectangle(red.x, red.y, PADDLE_W, PADDLE_H, RED);
    DrawRectangle(blue.x, blue.y, PADDLE_W, PADDLE_H, BLUE);
    DrawCircle(ball.x, ball.y, BALL_R, WHITE);
    DrawText(red_score, 32, SCREEN_H - 32, 16, GREEN);
    DrawText(blue_score, SCREEN_W - 128, SCREEN_H - 32, 16, GREEN);
    EndDrawing();
  }

  CloseWindow();

  return 0;
}

