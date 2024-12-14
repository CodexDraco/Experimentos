#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

#define POINTS_COUNT 32

static SDL_FPoint points[POINTS_COUNT];

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  SDL_SetAppMetadata("Try SDL3", "0.1", "com.dexco.trysdl3");

  if(!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  if(!SDL_CreateWindowAndRenderer("dexco/trysdl3", 1280, 720, 0, &window, &renderer)) {
    SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
    return SDL_APP_FAILURE;
  }

  for(int i=0; i < POINTS_COUNT; i++) {
    float theta = (float)i / POINTS_COUNT * 2 * SDL_PI_F;
    points[i].x = 640 + 350 * SDL_cos(theta);
    points[i].y = 360 + 350 * SDL_sin(theta);
  }

  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  if(event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
  const float red = 0.5;
  const float green = 0.3;
  const float blue = 0.7;

  SDL_SetRenderDrawColorFloat(renderer, red, green, blue, SDL_ALPHA_OPAQUE_FLOAT);

  SDL_RenderClear(renderer);

  SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);
  SDL_RenderLines(renderer, points, POINTS_COUNT);
  SDL_RenderLine(renderer, points[POINTS_COUNT - 1].x, points[POINTS_COUNT - 1].y, points[0].x, points[0].y);

  SDL_RenderPresent(renderer);

  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

}

