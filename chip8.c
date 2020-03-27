/* José Guilherme de C. Rodrigues - 03/2020 */
#include "chip8.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

/* 
TODO: Some instructions were different in a few sites, for those, it would be a good idea to add a flag
      to select a version to use when executing the code.
      Maybe add step-by-step execution?
*/

uint8_t chip8_characters[0x50] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x60, 0x20, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

chip8 *create_chip8(bool debug) {
	chip8 *c = malloc(sizeof(chip8));

	c->opcode = 0x0000;
	memset(c->memory, 0x00, sizeof(c->memory));

	// Place the characters in memory.
	for (size_t s = 0; s < sizeof(chip8_characters); ++s)
		c->memory[s] = chip8_characters[s];

	memset(c->regs.v, 0x00, sizeof(c->regs.v));

	c->regs.i = 0x0000;
	c->regs.sound_timer = 0x00;
	c->regs.delay_timer = 0x00;
	c->regs.pc = 0x0200;
	c->regs.sp = 0x00;

	memset(c->stack, 0x0000, sizeof(c->stack));
	
	for (uint8_t y = 0; y < DISPLAY_HEIGHT; ++y)
		for (uint8_t x = 0; x < DISPLAY_WIDTH; ++x)
			c->display[y][x] = 0;

	memset(c->keyboard, false, sizeof(c->keyboard));
	c->status.need_redraw = false;
	c->status.need_sound = false;
	c->status.need_keystroke = false;
	c->status.debug = debug;

	// We should also initialize the random number generator with a random seed.
	// CHIP8 uses in one of it's instruction.
	srand(time(NULL));

	return c;
}

void delete_chip8(chip8 *chip8) {
	if (chip8) {
		free(chip8);
		chip8 = NULL;
	}
}

bool load_program(chip8 *chip8, const char *filename) {
	bool success = false;

	FILE *file = fopen(filename, "rb");
	if (file) {
		// Get file size.
		fseek(file, 0, SEEK_END);
		long size = ftell(file);
		fseek(file, 0, SEEK_SET);

		// If it can fit in the program memory.
		if (size <= sizeof(chip8->memory) - 0x200) {		
			fread(chip8->memory + 0x200, sizeof(uint8_t), size, file);

			if(chip8->status.debug) {
				printf("Loaded program!\n");
				print_memory_in_range(chip8, 0x200, 0x200 + size);
			}

			success = true;
		} else {
			printf("\"%s\" is not a valid chip8 program.", filename);
		}

		fclose(file);
	}

	return success;
}

bool check_key(chip8 *chip8, uint8_t key) {
	return chip8->keyboard[key];
}

void change_key(chip8 *chip8, uint8_t key, bool active) {
	chip8->keyboard[key] = active;

	// In this case, we are probably inside a loop in the extern code waiting for a key to be pressed.
	// As soon as it's pressed, this function is called, and therefore, we need to put the pressed key
	// inside the register Vx as the last instruction told us to do.
	if (active && chip8->status.need_keystroke) {
		// Be aware, this opcode is the opcode of the last function executed.
		// If it isn't ld_vx_k/Fx0A something is wrong. Probably because the extern code isn't waiting for a key press.
		chip8->regs.v[HB_LN(chip8->opcode)] = key;
		chip8->status.need_keystroke = false;
	}
}

void tick(chip8 *chip8) {
	if (chip8) {
		// Fetch
		fetch_instruction(chip8);

		// Decode
		infn_ptr instruction = decode_instruction(chip8);

		// Execute
		if (instruction)
			(*instruction)(chip8);
		
		// Update timers
		if (chip8->regs.delay_timer != 0)
			--chip8->regs.delay_timer;

		if (chip8->regs.sound_timer != 0) {
			// Play sound
			chip8->status.need_sound = true;
			--chip8->regs.sound_timer;
		}
	}
}

void fetch_instruction(chip8 *chip8) {
	// All instructions are 2 byte long and the most significant byte is stored first.
	chip8->opcode = 0x0000;
	// Fetch the instruction (the first part and shift 8 bytes left and then the second part) and put it
	// in the opcode.
	chip8->opcode = (chip8->memory[chip8->regs.pc] << 8);
	chip8->opcode |= chip8->memory[++chip8->regs.pc];
	// Increment pc so it's on the next instruction to be executed.
	++chip8->regs.pc;
}

infn_ptr decode_instruction(chip8 *chip8) {
	infn_ptr infn = NULL;

	// I would really like to get rid of this if/else.
	uint8_t hb_hn = HB_HN(chip8->opcode);
	if (hb_hn == 0x00) {
		uint8_t lb = LB(chip8->opcode);
		if (lb == 0xE0)
			infn = &cls;
		else if (lb == 0xEE)
			infn = &ret;
	} else if (hb_hn == 0x01) {
		infn = &jp_addr;
	} else if (hb_hn == 0x02) {
		infn = &call_addr;
	} else if (hb_hn == 0x03) {
		infn = &se_vx_byte;
	} else if (hb_hn == 0x04) {
		infn = &sne_vx_byte;
	} else if (hb_hn == 0x05) {
		infn = &se_vx_vy;
	} else if (hb_hn == 0x06) {
		infn = &ld_vx_byte;
	} else if (hb_hn == 0x07) {
		infn = &add_vx_byte;
	} else if (hb_hn == 0x08) {
		uint8_t lb_ln = LB_LN(chip8->opcode);
		if (lb_ln == 0x00) {
			infn = &ld_vx_vy;
		} else if (lb_ln == 0x01) {
			infn = &or_vx_vy;
		} else if (lb_ln == 0x02) {
			infn = &and_vx_vy;
		} else if (lb_ln == 0x03) {
			infn = &xor_vx_vy;
		} else if (lb_ln == 0x04) {
			infn = &add_vx_vy;
		} else if (lb_ln == 0x05) {
			infn = &sub_vx_vy;
		} else if (lb_ln == 0x06) {
			infn = &shr_vx_vy;
		} else if (lb_ln == 0x07) {
			infn = &subn_vx_vy;
		} else if (lb_ln == 0x0E) {
			infn = &shl_vx_vy;
		}
	} else if (hb_hn == 0x09) {
		infn = &sne_vx_vy;
	} else if (hb_hn == 0x0A) {
		infn = &ld_i_addr;
	} else if (hb_hn == 0x0B) {
		infn = &jp_v0_addr;
	} else if (hb_hn == 0x0C) {
		infn = &rnd_vx_byte;
	} else if (hb_hn == 0x0D) {
		infn = &drw_vx_vy_nibble;
	} else if (hb_hn == 0x0E) {
		uint8_t lb = LB(chip8->opcode);
		if (lb == 0x9E)
			infn = &skp_vx;
		else if (lb == 0xA1)
			infn = &sknp_vx;
	} else if (hb_hn == 0x0F) {
		uint8_t lb = LB(chip8->opcode);
		if (lb == 0x07)
			infn = &ld_vx_dt;
		else if (lb == 0x0A)
			infn = &ld_vx_k;
		else if (lb == 0x15)
			infn = &ld_dt_vx;
		else if (lb == 0x18)
			infn = &ld_st_vx;
		else if (lb == 0x1E)
			infn = &add_i_vx;
		else if (lb == 0x29)
			infn = &ld_f_vx;
		else if (lb == 0x33)
			infn = &ld_b_vx;
		else if (lb == 0x55)
			infn = &ld_at_i_vx;
		else if (lb == 0x65)
			infn = &ld_vx_at_i;
	}

	return infn;
}

INFN(cls) {
	DEBUG_INSTRUCTION_LOG("cls");
	// 00E0: Clears the screen.
	memset(chip8->display, 0x0, sizeof(chip8->display));
}

INFN(ret) {
	DEBUG_INSTRUCTION_LOG("ret");
	// 00EE: Return from subroutine.
	chip8->regs.pc = chip8->stack[chip8->regs.sp--];
}

INFN(jp_addr) {
	DEBUG_INSTRUCTION_LOG("jp_addr");
	// 1nnn: Jump to location at nnn.
	// SHOULD THE FUNCTION BE ABLE TO JUMP TO MEMORY LOWER THAN 0x200?
	chip8->regs.pc = chip8->opcode & 0x0FFF;
}

INFN(call_addr) {
	DEBUG_INSTRUCTION_LOG("call_addr");
	// 2nnn: Call subroutine at nnn.
	// SHOULD THE FUNCTION BE ABLE TO JUMP TO MEMORY LOWER THAN 0x200?
	chip8->stack[++chip8->regs.sp] = chip8->regs.pc;
	chip8->regs.pc = chip8->opcode & 0x0FFF;
}

INFN(se_vx_byte) {
	DEBUG_INSTRUCTION_LOG("se_vx_byte");
	// 3xkk: Skip next instruction if Vx = k.
	if (chip8->regs.v[HB_LN(chip8->opcode)] == LB(chip8->opcode))
		chip8->regs.pc += 2;
}

INFN(sne_vx_byte) {
	DEBUG_INSTRUCTION_LOG("sne_vx_byte");
	// 4xkk: Skip next instruction if Vx != k.
	if (chip8->regs.v[HB_LN(chip8->opcode)] != LB(chip8->opcode))
		chip8->regs.pc += 2;
}

INFN(se_vx_vy) {
	DEBUG_INSTRUCTION_LOG("se_vx_vy");
	// 5xy0: Skip next instruction if Vx = Vy.
	if (chip8->regs.v[HB_LN(chip8->opcode)] == chip8->regs.v[LB_HN(chip8->opcode)])
		chip8->regs.pc += 2;
}

INFN(ld_vx_byte) {
	DEBUG_INSTRUCTION_LOG("ld_vx_byte");
	// 6xkk: Set Vx = kk.
	chip8->regs.v[HB_LN(chip8->opcode)] = LB(chip8->opcode);
}

INFN(add_vx_byte) {
	DEBUG_INSTRUCTION_LOG("add_vx_byte");
	// 7xkk: Add the value kk to Vx then stores the result in Vx.
	chip8->regs.v[HB_LN(chip8->opcode)] += LB(chip8->opcode);
}

INFN(ld_vx_vy) {
	DEBUG_INSTRUCTION_LOG("ld_vx_vy");
	// 8xy0: Set Vx = Vy.
	chip8->regs.v[HB_LN(chip8->opcode)] = chip8->regs.v[LB_HN(chip8->opcode)];
}

INFN(or_vx_vy) {
	DEBUG_INSTRUCTION_LOG("or_vx_vy");
	// 8xy1: Vx |= Vy.
	chip8->regs.v[HB_LN(chip8->opcode)] |= chip8->regs.v[LB_HN(chip8->opcode)];
}

INFN(and_vx_vy) {
	DEBUG_INSTRUCTION_LOG("and_vx_vy");
	// 8xy2: Vx &= Vy.
	chip8->regs.v[HB_LN(chip8->opcode)] &= chip8->regs.v[LB_HN(chip8->opcode)];
}

INFN(xor_vx_vy) {
	DEBUG_INSTRUCTION_LOG("xor_vx_vy");
	// 8xy3: Vx ^= Vy.
	chip8->regs.v[HB_LN(chip8->opcode)] ^= chip8->regs.v[LB_HN(chip8->opcode)];
}

INFN(add_vx_vy) {
	DEBUG_INSTRUCTION_LOG("add_vx_vy");
	// 8xy4: Set Vx += Vy, VF = carry
	if (chip8->regs.v[HB_LN(chip8->opcode)] + chip8->regs.v[LB_HN(chip8->opcode)] <= 255)
		chip8->regs.v[0xF] = 0x0;
	else
		chip8->regs.v[0xF] = 0x1;

	chip8->regs.v[HB_LN(chip8->opcode)] += chip8->regs.v[LB_HN(chip8->opcode)];
}

INFN(sub_vx_vy) {
	DEBUG_INSTRUCTION_LOG("sub_vx_vy");
	// 8xy5: Set Vx -= Vy, VF = not borrow
	if (chip8->regs.v[HB_LN(chip8->opcode)] > chip8->regs.v[LB_HN(chip8->opcode)])
		chip8->regs.v[0xF] = 0x1;
	else
		chip8->regs.v[0xF] = 0x0;

	chip8->regs.v[HB_LN(chip8->opcode)] -= chip8->regs.v[LB_HN(chip8->opcode)];
}

INFN(shr_vx_vy) {
	DEBUG_INSTRUCTION_LOG("shr_vx_vy");
	// 8xy6: Set Vx = Vy >> 1, VF = least significant bit prior shift.
	// 8 bit register, to get least significant bit just & 0x01
	chip8->regs.v[0xF] = chip8->regs.v[LB_HN(chip8->opcode)] & 0x01;
	chip8->regs.v[HB_LN(chip8->opcode)] = chip8->regs.v[LB_HN(chip8->opcode)] >> 1;
}

INFN(subn_vx_vy) {
	DEBUG_INSTRUCTION_LOG("subn_vx_vy");
	// 8xy7: Sev Vx = Vy - Vx, VF = not borrow
	if (chip8->regs.v[LB_HN(chip8->opcode)] > chip8->regs.v[HB_LN(chip8->opcode)])
		chip8->regs.v[0xF] = 0x1;
	else
		chip8->regs.v[0xF] = 0x0;
	
	chip8->regs.v[HB_LN(chip8->opcode)] = chip8->regs.v[LB_HN(chip8->opcode)] - chip8->regs.v[HB_LN(chip8->opcode)];
}

INFN(shl_vx_vy) {
	DEBUG_INSTRUCTION_LOG("shl_vx_vy");
	// 8xyE: Set Vx = Vy << 1, VF = most significant bit prior shift.
	// 8 bit register, to get most significant bit just >> 7 (no need to do an and since every bit to the left becomes 0).
	chip8->regs.v[0xF] = chip8->regs.v[LB_HN(chip8->opcode)] >> 7;
	chip8->regs.v[HB_LN(chip8->opcode)] = chip8->regs.v[LB_HN(chip8->opcode)] << 1;
}

INFN(sne_vx_vy) {
	DEBUG_INSTRUCTION_LOG("sne_vx_vy");
	// 9xy0: Skip next instruction if Vx != Vy.
	if (chip8->regs.v[HB_LN(chip8->opcode)] != chip8->regs.v[LB(chip8->opcode)])
		chip8->regs.pc += 2;
}

INFN(ld_i_addr) {
	DEBUG_INSTRUCTION_LOG("ld_i_addr");
	// Annn: Set I = nnn.
	chip8->regs.i = chip8->opcode & 0x0FFF;
}

INFN(jp_v0_addr) {
	DEBUG_INSTRUCTION_LOG("jp_v0_addr");
	// Bnnn: Jump to location nnn + V0.
	chip8->regs.pc = (chip8->opcode & 0x0FFF) + chip8->regs.v[0];
}

INFN(rnd_vx_byte) {
	DEBUG_INSTRUCTION_LOG("rnd_vx_byte");
	// Cxkk: random byte (0 - 255) and kk
	chip8->regs.v[HB_LN(chip8->opcode)] = (rand() % 256) & LB(chip8->opcode);
}

INFN(drw_vx_vy_nibble) {
	DEBUG_INSTRUCTION_LOG("drw_vx_vy_nibble");
	// Dxyn: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = Collision.
	// All sprites are 8xn pixels in size, where n can go up to 15.
	uint8_t vx = chip8->regs.v[HB_LN(chip8->opcode)], vy = chip8->regs.v[LB_HN(chip8->opcode)], n = LB_LN(chip8->opcode);
	
	for (uint16_t y = 0; y < n; ++y) {
		uint8_t sprite = chip8->memory[chip8->regs.i + y];
		for (uint16_t x = 0; x < 8; ++x) {
			if (chip8->display[(vy + y) % DISPLAY_HEIGHT][(vx + x) % DISPLAY_WIDTH] == 1 && ((sprite >> (7 - x)) & 0x01) == 1)
				chip8->regs.v[0xF] = 1;
			chip8->display[(vy + y) % DISPLAY_HEIGHT][(vx + x) % DISPLAY_WIDTH] ^= ((sprite >> (7 - x)) & 0x01);
		}
	}

	chip8->status.need_redraw = true;
}

INFN(skp_vx) {
	DEBUG_INSTRUCTION_LOG("skp_vx");
	// Ex9E: Skip next instruction if key with the value in Vx is pressed.
	if (check_key(chip8, chip8->regs.v[HB_LN(chip8->opcode)]))
		chip8->regs.pc += 2;
}

INFN(sknp_vx) {
	DEBUG_INSTRUCTION_LOG("sknp_vx");
	// ExA1: Skip next instruction if key with the value in Vx is not pressed.
	if (!check_key(chip8, chip8->regs.v[HB_LN(chip8->opcode)]))
		chip8->regs.pc += 2;
}

INFN(ld_vx_dt) {
	DEBUG_INSTRUCTION_LOG("ld_vx_dt");
	// Fx07: Set Vx = delay_timer.
	chip8->regs.v[HB_LN(chip8->opcode)] = chip8->regs.delay_timer;
}

INFN(ld_vx_k) {
	DEBUG_INSTRUCTION_LOG("ld_vx_k");
	// Fx0A: Wait for a key press, store the value of the key in Vx.
	// TODO: I really don't want this to be here, but it's currently the least invasive way
	// 	 of implementing this function without needing to call extern code from here that I see.
	chip8->status.need_keystroke = true;
}

INFN(ld_dt_vx) {
	DEBUG_INSTRUCTION_LOG("ld_dt_vx");
	// Fx15: Set delay timer = Vx.
	chip8->regs.delay_timer = chip8->regs.v[HB_LN(chip8->opcode)];
}

INFN(ld_st_vx) {
	DEBUG_INSTRUCTION_LOG("ld_st_vx");
	// Fx18: Set sound timer = Vx.
	chip8->regs.sound_timer = chip8->regs.v[HB_LN(chip8->opcode)];
}

INFN(add_i_vx) {
	DEBUG_INSTRUCTION_LOG("add_i_vx");
	// Fx1E: Set I += Vx.
	chip8->regs.i += chip8->regs.v[HB_LN(chip8->opcode)];
}

INFN(ld_f_vx) {
	DEBUG_INSTRUCTION_LOG("ld_f_vx");
	// Fx29: Set I = location of sprite for hex digit in Vx.
	// The characters are stored from position 0 and onwards.
	// Each character is 5 bytes long.
	chip8->regs.i = chip8->regs.v[HB_LN(chip8->opcode)] * 5;
}

INFN(ld_b_vx) {
	DEBUG_INSTRUCTION_LOG("ld_b_vx");
	// Fx33: Store BCD rep. of digit in Vx in memory locations I, I + 1, and I +2.
	uint8_t vx_value = chip8->regs.v[HB_LN(chip8->opcode)];
	chip8->memory[chip8->regs.i] = vx_value / 100;
	chip8->memory[chip8->regs.i + 1] = floor((vx_value % 100) / 10);
	chip8->memory[chip8->regs.i + 2] = vx_value % 10;
}

INFN(ld_at_i_vx) {
	DEBUG_INSTRUCTION_LOG("ld_at_i_vx");
	// Fx55: Store registers V0 through Vx in memory starting at location I.
	// I is set to I + X + 1 after the operation.
	uint8_t x = HB_LN(chip8->opcode);
	for (uint8_t u = 0; u <= x; ++u)
		chip8->memory[chip8->regs.i + u] = chip8->regs.v[u];
	chip8->regs.i += x + 1;
}

INFN(ld_vx_at_i) {
	DEBUG_INSTRUCTION_LOG("ld_vx_at_i");
	// Fx65: Read registers V0 through Vx from memory starting at location I.
	// I is set to I + X + 1 after operation.
	uint8_t x = HB_LN(chip8->opcode);
	for (uint8_t u = 0; u <= x; ++u)
		chip8->regs.v[u] = chip8->memory[chip8->regs.i + u];
	chip8->regs.i += x + 1;
}

// Functions that print the component's state.
void print_registers(chip8 *chip8) {
	if (chip8) {
		for (uint8_t u = 0; u < 0x10; ++u) {
			if (u % 8 == 0)
				printf("\n");

			printf("V[%u]: 0x%X ", u, chip8->regs.v[u]);
		}

		printf(
			"\nI: 0x%X"
			"\tSoundTimer: %u"
			"\tDelayTimer: %u"
			"\tPC: %u"
			"\tSP: %u\n",
			chip8->regs.i, chip8->regs.sound_timer, chip8->regs.delay_timer, chip8->regs.pc, chip8->regs.sp
		);
	}
}

void print_memory_in_range(chip8 *chip8, uint16_t start, uint16_t end) {
	if (chip8) {
		printf("Memory in range [%u, %u]", start, end);
		for (uint16_t u = start; u < end; ++u) {
			if (u % 8 == 0)
				printf("\n[%u/0x%X]", u, u);

			printf("\t0x%X", chip8->memory[u]);
		}
		printf("\n");
	}
}

void print_keyboard(chip8 *chip8) {
	if (chip8) { 
		static char keys[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
		for (uint8_t u = 0; u < 0x10; ++u) {
			printf("[%c]: %u ", keys[u], chip8->keyboard[u]);
		}
		printf("\n");
	}
}