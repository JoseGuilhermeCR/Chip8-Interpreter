/* José Guilherme de C. Rodrigues - 03/2020 */
#ifndef __CHIP8_H__
#define __CHIP8_H__

#include <stdint.h>
#include <stdbool.h>

#define DEBUG_INSTRUCTION_LOG(x) if (chip8->status.debug) \
					printf("[CHIP8 - DEBUG] " x " 0x%X\n", chip8->opcode)

// Some macros to make 2 byte variables manipulation easier.
#define HB_HN(x) (x & 0xF000) >> 12	// HIGH BYTE HIGH NIBBLE
#define HB_LN(x) (x & 0x0F00) >> 8	// HIGH BYTE LOW NIBBLE
#define LB_HN(x) (x & 0x00F0) >> 4	// LOW BYTE HIGH NIBBLE
#define LB_LN(x) (x & 0x000F)		// LOW BYTE LOW NIBBLE
#define HB(x) 	 (x & 0xFF00) >> 8	// HIGH BYTE
#define LB(x)	 (x & 0x00FF)		// LOW BYTE

#define K_0 0x0
#define K_1 0x1
#define K_2 0x2
#define K_3 0x3
#define K_4 0x4
#define K_5 0x5
#define K_6 0x6
#define K_7 0x7
#define K_8 0x8
#define K_9 0x9
#define K_A 0xA
#define K_B 0xB
#define K_C 0xC
#define K_D 0xD
#define K_E 0xE
#define K_F 0xF

#define DISPLAY_WIDTH 0x40
#define DISPLAY_HEIGHT 0x20

typedef struct {
	bool need_redraw;
	bool need_sound;
	bool need_keystroke;
	bool debug;
} chip8_status;

typedef struct {
	uint8_t v[0x10];	// 16 8-bit general purpose registers.
	uint16_t i;		// Generally used to store memory addresses.
	uint8_t sound_timer;	// Sound timer.
	uint8_t delay_timer;	// Delay timer.
	uint16_t pc;		// Program counter (currently executing address).
	uint8_t	sp;		// Stack pointer (topmost level of the stack).
} chip8_regs;

typedef struct {
	uint16_t opcode;
	uint8_t memory[0x1000];					// 4096 Bytes of memory. (4KB)
	chip8_regs regs;
	uint16_t stack[0x10];					// 16 16-bit levels of stack (used to store addresses to return when coming back from subroutines).
	uint8_t display[DISPLAY_HEIGHT][DISPLAY_WIDTH]; 	// 64x32 pixel monochrome display.
	bool keyboard[0x10];					// 16 key keyboard each part position indicates a key state.
	chip8_status status;
} chip8;

// Default CHIP8 sprites. Should be store from (0x000 to 0x1FF)
extern uint8_t chip8_characters[0x50];
// Memory from 0x000 to 0x1FF is reserved for interpreter.
// Most programs start at 0x200.
// Program memory is from 0x200 to 0xFFF.

// The decode function will use the type infn_ptr to return a function that will execute the instruction.
typedef void(*infn_ptr)(chip8 *);

// A macro for declaring instruction functions
#define INFN(x) void x(chip8 *chip8)

chip8 *create_chip8(bool debug);
void delete_chip8(chip8 *chip8);
bool load_program(chip8 *chip8, const char *filename);
bool check_key(chip8 *chip8, uint8_t key);
void change_key(chip8 *chip8, uint8_t key, bool active);

void tick(chip8 *chip8); //  A tick will go through every step needed in a cycle.
void fetch_instruction(chip8 *chip8);
infn_ptr decode_instruction(chip8 *chip8);

// All 35 instructions the normal chip8 uses.
// Instruction SYS addr is not implemented by modern interpreters.
INFN(cls);
INFN(ret);
INFN(jp_addr);
INFN(call_addr);
INFN(se_vx_byte);
INFN(sne_vx_byte);
INFN(se_vx_vy);
INFN(ld_vx_byte);
INFN(add_vx_byte);
INFN(ld_vx_byte);
INFN(add_vx_byte);
INFN(ld_vx_vy);
INFN(or_vx_vy);
INFN(and_vx_vy);
INFN(xor_vx_vy);
INFN(add_vx_vy);
INFN(sub_vx_vy);
INFN(shr_vx_vy);
INFN(subn_vx_vy);
INFN(shl_vx_vy);
INFN(sne_vx_vy);
INFN(ld_i_addr);
INFN(jp_v0_addr);
INFN(rnd_vx_byte);
INFN(drw_vx_vy_nibble);
INFN(skp_vx);
INFN(sknp_vx);
INFN(ld_vx_dt);
INFN(ld_vx_k);
INFN(ld_dt_vx);
INFN(ld_st_vx);
INFN(add_i_vx);
INFN(ld_f_vx);
INFN(ld_b_vx);
INFN(ld_at_i_vx);
INFN(ld_vx_at_i);

// Functions that print the component's state.
void print_registers(chip8 *chip8);
void print_memory_in_range(chip8 *chip8, uint16_t start, uint16_t end);
void print_keyboard(chip8 *chip8);

#endif

/* The instructions were taken from:
 * https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Instruction-Set#notes and http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.0 
 * I tried to use both of them. Some instructions, for example, ld_at_i_vx may differ in these sites, when that happened, I tried to take the more
 * "complete" instruction. */
