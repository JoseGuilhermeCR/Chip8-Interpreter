/* José Guilherme de C. Rodrigues - 03/2020 */
#ifndef __CHIP8_INTERPRETER_H__
#define __CHIP8_INTERPRETER_H__

#include "chip8.h"
#include "SDL2/SDL.h"

#define INTERPRETER_LOG(...) printf("[INTERPRETER] " __VA_ARGS__)

typedef struct  {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Rect pixel_rect;
	SDL_Event event;
	bool running;
	bool paused;
	uint32_t cycle_ms;
	chip8 *chip;
} program_struct;

void initialize(program_struct *ps, const char* program, bool debug, uint8_t scale, uint32_t cycle_ms);
void update(program_struct *ps);
void render(program_struct *ps);
void wait_for_key_stroke(program_struct *ps);
void process_key_event(program_struct *ps, SDL_Keysym *keysim, bool value);
void process_interpreter_key_event(program_struct *ps, SDL_Keysym *keysim);
void destroy(program_struct *ps);
void show_help();

#endif