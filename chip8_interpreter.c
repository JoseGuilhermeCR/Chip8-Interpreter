/* José Guilherme de C. Rodrigues - 03/2020 */
#include "chip8_interpreter.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

int main(int argc, char **argv) {
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "--help") == 0) {
			show_help();
			return 0;
		}
	}

	if (argc >= 2) {
		program_struct ps;

		// Default values for args that weren't given.
		bool debug = false;
		uint8_t scale = 10;
		uint32_t cycle_ms = 16;

		if (argc >= 3) {
			debug = (strcmp(argv[2], "true") == 0) ? true : false;

			if (argc >= 4) {
				scale = atoi(argv[3]);

				if (argc == 5)
					cycle_ms = atoi(argv[4]);
			}
		}
		
		
		initialize(&ps, argv[1], debug, scale, cycle_ms);

		while (ps.running) {
			SDL_Delay(ps.cycle_ms);
			update(&ps);
			if (ps.chip->status.need_redraw)
				render(&ps);
		}

		destroy(&ps);
	} else {
		show_help();
	}

	return 0;
}

void initialize(program_struct *ps, const char* program, bool debug, uint8_t scale, uint32_t cycle_ms) {
	ps->running = false;
	ps->paused = false;

	SDL_Init(SDL_INIT_VIDEO);

	// Window size
	const uint16_t window_width = DISPLAY_WIDTH * scale;
	const uint16_t window_height = DISPLAY_HEIGHT * scale;

	ps->window = SDL_CreateWindow(
        	"CHIP 8 Interpreter - José Guilherme de C. Rodrigues",	// Window Title
        	SDL_WINDOWPOS_UNDEFINED,           			// Initial X position
        	SDL_WINDOWPOS_UNDEFINED,           			// Initial Y position
        	window_width,                               		// WIDTH
        	window_height,                               		// HEIGHT
        	SDL_WINDOW_OPENGL                  			// FLAGS
	);

	// If window successfully created.
	if (ps->window) {
		ps->renderer = SDL_CreateRenderer(ps->window, -1, 0);

		// If renderer successfully created.
		if (ps->renderer) {
			// Set the size of the pixel_rect.
			ps->pixel_rect.w = scale;
			ps->pixel_rect.h = scale;

			ps->chip = create_chip8(debug);
			if (load_program(ps->chip, program)) {
				ps->running = true;
				ps->cycle_ms = cycle_ms;
			}
		} else {
			fprintf(stderr, "An error occurred when creating the renderer. %s\n", SDL_GetError());
		}
	} else {
		fprintf(stderr, "An error occurred when creating the Window. %s\n", SDL_GetError());
	}
}

void update(program_struct *ps) {
	// Process SDL events.
	while (SDL_PollEvent(&ps->event)) {
		if (ps->event.type == SDL_QUIT) {
			ps->running = false;
		} else if (ps->event.type == SDL_KEYDOWN) { // Key down event.
			process_interpreter_key_event(ps, &ps->event.key.keysym);
			process_key_event(ps, &ps->event.key.keysym, true);
		} else if (ps->event.type == SDL_KEYUP) { // Key up event.
			// The key should not be active in chip8.
			process_key_event(ps, &ps->event.key.keysym, false);
		}
	}

	if (!ps->paused) {
		// Simulates a chip8 cycle.
		tick(ps->chip);

		// Will wait for a keystroke if the chip8 status need_keystroke is set. (Must be done if using chip8.h)
		wait_for_keystroke(ps);

		if (ps->chip->status.need_sound) {
			// TODO: Play a sound.
			ps->chip->status.need_sound = false;
		}
	}
}

void render(program_struct *ps) {
	// Clear the screen with the black color.
	SDL_SetRenderDrawColor(ps->renderer, 0x0, 0x0, 0x0, 0xFF);
	SDL_RenderClear(ps->renderer);

	// Set the color to white to render the active pixels.
	SDL_SetRenderDrawColor(ps->renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	for (uint8_t y = 0; y < DISPLAY_HEIGHT; ++y) {
		for (uint8_t x = 0; x < DISPLAY_WIDTH; ++x) {
			if (ps->chip->display[y][x]) {
				ps->pixel_rect.x = x * ps->pixel_rect.w;
				ps->pixel_rect.y = y * ps->pixel_rect.h;
				SDL_RenderFillRect(ps->renderer, &ps->pixel_rect);
			}
		}	
	}

	// Shows the rendered screen.
	SDL_RenderPresent(ps->renderer);
	// Disable chip8 need_redraw status. (Should be done if using chip8.h)
	ps->chip->status.need_redraw = false;
}

// TODO: Maybe put this functionality inside the event loop in update...
// This function will wait until a key is pressed before letting the interpreter continue.
void wait_for_keystroke(program_struct *ps) {
	if (!ps->paused) {
		while (ps->running && ps->chip->status.need_keystroke) {
			while (SDL_PollEvent(&ps->event)) {
				if (ps->event.type == SDL_QUIT) {
					ps->running = false;
				} else if (ps->event.type == SDL_KEYDOWN /*&& ps->event.key.repeat == 0 Don't need to activate key in case it's a repeated signal */) {			
					process_key_event(ps, &ps->event.key.keysym, true);
				}
			}
		}
	}
}

void process_interpreter_key_event(program_struct *ps, SDL_Keysym *keysim) {
	if (ps->event.key.keysym.sym == SDLK_UP) {
		INTERPRETER_LOG("Increasing Cycle Ms: {%u}.\n", ++ps->cycle_ms);
	} else if (ps->event.key.keysym.sym == SDLK_DOWN) {
		if (ps->cycle_ms != 0)
			INTERPRETER_LOG("Decreasing Cycle Ms: {%u}.\n", --ps->cycle_ms);
		else
			INTERPRETER_LOG("Minimum Cycle Ms is 0.\n");
	} else if (ps->event.key.keysym.sym == SDLK_SPACE) {
		ps->chip->status.debug = !ps->chip->status.debug;
		INTERPRETER_LOG("Debug %s.\n", (ps->chip->status.debug) ? "enabled" : "disabled");
	} else if (ps->event.key.keysym.sym == SDLK_p) {
		ps->paused = !ps->paused;
		INTERPRETER_LOG("Pause %s.\n", (ps->paused) ? "enabled" : "disabled");
	} else if (ps->event.key.keysym.sym == SDLK_i) {
		print_registers(ps->chip);
	} else if (ps->event.key.keysym.sym == SDLK_k) {
		print_keyboard(ps->chip);
	} else if (ps->event.key.keysym.sym == SDLK_m) {
		print_memory_in_range(ps->chip, 0x0000, 0xfff);
	}
}

/*
			MAPS INTO 
   CHIP 8 Keyboard	    |	QWERTY Keyboard
   1	2	3	C   |	1	2	3	4
   4	5	6	D   |	Q	W	E	R
   7	8	9	E   |	A	S	D	F
   A	0	B	F   | 	Z	X	C	V
*/
void process_key_event(program_struct *ps, SDL_Keysym *keysim, bool value) {
	if (!ps->paused) {
		uint8_t key = 0x10;

		// Maps each key to it's respective chip8 keyboard key.
		if (keysim->sym == SDLK_1)
			key = K_1;
		else if (keysim->sym == SDLK_2)
			key = K_2;
		else if (keysim->sym == SDLK_3)
			key = K_3;
		else if (keysim->sym == SDLK_4)
			key = K_C;
		else if (keysim->sym == SDLK_q)
			key = K_4;
		else if (keysim->sym == SDLK_w)
			key = K_5;
		else if (keysim->sym == SDLK_e)
			key = K_6;
		else if (keysim->sym == SDLK_r)
			key = K_D;
		else if (keysim->sym == SDLK_a)
			key = K_7;
		else if (keysim->sym == SDLK_s)
			key = K_8;
		else if (keysim->sym == SDLK_d)
			key = K_9;
		else if (keysim->sym == SDLK_f)
			key = K_E;
		else if (keysim->sym == SDLK_z)
			key = K_A;
		else if (keysim->sym == SDLK_x)
			key = K_0;
		else if (keysim->sym == SDLK_c)
			key = K_B;
		else if (keysim->sym == SDLK_v)
			key = K_F;
		
		if (key < 0x10)
			change_key(ps->chip, key, value);
	}
}

void destroy(program_struct *ps) {
	delete_chip8(ps->chip);
	SDL_DestroyRenderer(ps->renderer);
	SDL_DestroyWindow(ps->window);
	SDL_Quit();
}

void show_help() {
	puts(
		"chip8_interpreter program.ch8 <debug> <scale> <cycle_ms>\n"
		"Parameters in <> are optional, but if they are given, the order must be followed.\n"
		"debug = true to enable or anything else to disable (shows debug information while running).\n"
		"scale = int8_t (will scale the dimensions of the original 64x32 chip8 display).\n"
		"cycle_ms = int32_t (the amount of time the interpreter will sleep between each cycle).\n"
		"--help will show this message and exit the program.\n"
		"                        MAPS INTO\n"
   		"CHIP 8 Keyboard             |   QWERTY Keyboard\n"
   		"1	2	3	C   |	1	2	3	4\n"
		"4	5	6	D   |	Q	W	E	R\n"
   		"7	8	9	E   |	A	S	D	F\n"
   		"A	0	B	F   | 	Z	X	C	V\n"
		"Interpreter keys:\n"
		"Up will increase cycle_ms\n"
		"Down will decrease cycle_ms\n"
		"Space will enable/disable debug\n"
		"P will pause the interpreter\n"
		"I will print registers state\n"
		"K will print keyboard state\n"
		"M will print the whole memory (this might be really big for your console's window)\n"
		"Made by Jose Guilherme de C. Rodrigues 03/2020."
	);
}