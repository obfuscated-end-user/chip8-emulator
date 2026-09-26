// credit to this guy
// https://austinmorlan.com/posts/chip8_emulator
// unfinished, doesn't even compile
// get SDL first
// g++ main.cpp -o main.exe ;; .\main.exe

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>

const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;

uint8_t fontset[FONTSET_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
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

class Chip8 {
public:
	uint8_t registers[16] {};
	uint8_t memory[4096] {};
	uint16_t index {};
	uint16_t pc {};		// program counter
	uint16_t stack[16] {};
	uint8_t sp {};		// stack pointer
	uint8_t delayTimer {};
	uint8_t soundTimer {};
	uint8_t keypad[16] {};
	uint32_t video[64 * 32] {};
	uint16_t opcode;
	// void LoadROM(char const* filename);
	std::default_random_engine randGen;
	std::uniform_int_distribution<uint8_t> randByte;

	Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
		// Initialize PC
		pc = START_ADDRESS;
		// Load fonts into memory
		for (unsigned int i = 0; i < FONTSET_SIZE; ++i)
			memory[FONTSET_START_ADDRESS + i] = fontset[i];
		// Initialize RNG
		randByte = std::uniform_int_distribution<uint8_t>(0, 255U);
	}

	void LoadROM(char const* filename) {
		// Open the file as a stream of binary and move the file pointer to the end
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (file.is_open()) {
			// Get size of file and allocate a buffer to hold the contents
			std::streampos size = file.tellg();
			char* buffer = new char[size];
			// Go back to the beginning of the file and fill the buffer
			file.seekg(0, std::ios::beg);
			file.read(buffer, size);
			file.close();
			// Load the ROM contents into the Chip8's memory, starting at 0x200
			for (long i = 0; i < size; ++i)
				Chip8::memory[START_ADDRESS + i] = buffer[i];
			// Free the buffer
			delete[] buffer;
		}
	}

	// SUBROUTINES
	void OP_00EO() {
		/**
		 * CLS
		 * Clear the display.
		 */
		memset(video, 0, sizeof(video));
	}

	void OP_00EE() {
		/**
		 * RET
		 * Return from a subroutine.
		 */
		--sp;
		pc = stack[sp];
	}

	void OP_1nnn() {
		/**
		 * JP addr
		 * Jump to location nnn.
		 * The interpreter sets the program counter to nnn.
		 */
		uint16_t address = opcode & 0x0FFFu;
		pc = address;
	}

	void OP_2nnn() {
		/**
		 * CALL addr
		 * Call subroutine at nnn.
		 */
		uint16_t address = opcode & 0x0FFFu;
		stack[sp] = pc;
		++sp;
		pc = address;
	}

	void OP_3xkk() {
		/**
		 * SE Vx, byte
		 * Skip next instruction if Vx = kk.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;
		if (registers[Vx] == byte) pc += 2;
	}

	void OP_4xkk() {
		/**
		 * SNE Vx, byte
		 * Skip next instruction if Vx != kk.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;
		if (registers[Vx] != byte) pc += 2;
	}

	void OP_5xy0() {
		/**
		 * SE Vx, Vy
		 * Skip next instruction if Vx = Vy.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		if (registers[Vx] == registers[Vy]) pc += 2;
	}

	void OP_6xkk() {
		/**
		 * LD Vx, byte
		 * Set Vx = kk.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;
		registers[Vx] = byte;
	}

	void OP_7xkk() {
		/**
		 * ADD Vx, byte
		 * Set Vx = Vx + kk.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;
		registers[Vx] += byte;
	}

	void OP_8xy0() {
		/**
		 * LD Vx, Vy
		 * Set Vx = Vy.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		registers[Vx] = registers[Vy];
	}

	void OP_8xy1() {
		/**
		 * OR Vx, Vy
		 * Set Vx OR Vy.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		registers[Vx] |= registers[Vy];
	}

	void OP_8xy2() {
		/**
		 * AND Vx, Vy
		 * Set Vx AND Vy.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		registers[Vx] &= registers[Vy];
	}

	void OP_8xy3() {
		/**
		 * XOR Vx, Vy
		 * Set Vx XOR Vy.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		registers[Vx] ^= registers[Vy];
	}

	void OP_8xy4() {
		/**
		 * ADD Vx, Vy
		 * Set Vx + Vy, set VF = carry.
		 * The values of Vx and Vy are added together. If the result is greater
		 * than 8 bits (i.e., > 255), VF is set to 1, otherwise 0. Only the
		 * lowest 8 bits of the result are kept, and stored in Vx.
		 * 
		 * This is an ADD with an overflow flag. If the sum is greater than what
		 * can fit into a byte (255), register VF will be set to 1 as a flag.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		uint16_t sum = registers[Vx] + registers[Vy];
		if (sum > 255U) registers[0xF] = 1;
		else registers[0xF] = 0;
		registers[Vx] = sum & 0xFFu;
	}

	void OP_8xy5() {
		/**
		 * SUB Vx, Vy
		 * Set Vx = Vx - Vy, set VF = NOT borrow.
		 * If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted
		 * from Vx, and the results stored in Vx.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		if (registers[Vx] > registers[Vy]) registers[0xF] = 1;
		else registers[0xF] = 0;
		registers[Vx] -= registers[Vy];
	}

	void OP_8xy6() {
		/**
		 * 8xy6 - SHR Vx
		 * Set Vx = Vx SHR 1.
		 * If the least-significant bit of Vx is 1, then VF is set to 1,
		 * otherwise 0. Then Vx is divided by 2.
		 * A right shift is performed (division by 2), and the least significant
		 * bit is saved in Register VF.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		// Save LSB (least significant bit) in VF
		registers[0xF] = (registers[Vx] & 0x1u);
		registers[Vx] >>= 1;
	}

	void OP_8xy7() {
		/**
		 * 8xy7 - SUBN Vx, Vy
		 * Set Vx = Vy - Vx, set VF = NOT borrow.
		 * If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted
		 * from Vy, and the results stored in Vx.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		if (registers[Vy] > registers[Vx]) registers[0xF] = 1;
		else registers[0xF] = 0;
		registers[Vx] = registers[Vy] - registers[Vx];
	}

	void OP_8xy7E() {
		/**
		 * 8xyE - SHL Vx {, Vy}
		 * Set Vx = Vx SHL 1.
		 * If the most-significant bit of Vx is 1, then VF is set to 1,
		 * otherwise to 0. Then Vx is multiplied by 2.
		 * A left shift is performed (multiplication by 2), and the most
		 * significant bit is saved in Register VF.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		// Save MSB (most significant bit) in VF
		registers[0xF] = (registers[Vx] & 0x80u) >> 7u;
		registers[Vx] <<= 1;
	}

	void OP_9xy0() {
		/**
		 * 9xy0 - SNE Vx, Vy
		 * Skip next instruction if Vx != Vy.
		 * Since our PC has already been incremented by 2 in `Cycle()`, we can
		 * just increment by 2 again to skip the next instruction.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t Vy = (opcode & 0x00F0u) >> 4u;
		if (registers[Vx] != registers[Vy]) pc += 2;
	}

	void OP_Annn() {
		/**
		 * Annn - LD I, addr
		 * Set I = nnn.
		 */
		uint16_t address = opcode & 0x0FFFu;
		index = address;
	}

	void OP_Bnnn() {
		/**
		 * Bnnn - JP V0, addr
		 * Jump to location nnn + V0.
		 */
		uint16_t address = opcode & 0x0FFFu;
		pc = registers[0] + address;
	}

	void OP_Cxkk() {
		/**
		 * Cxkk - RND Vx, byte
		 * Set Vx = random byte AND kk.
		 */
		uint8_t Vx = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;
		registers[Vx] = randByte(randGen) & byte;
	}
};

int main() {
	std::cout << "Hello, world!" << std::endl;
	std::cout << "Currently work in progress, check back later." << std::endl;

	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<double> dist(1.0, 10.0);
	for (int i = 0; i < 10; ++i) std::cout << dist(mt) << "\n";

	// world's longest error message
	Chip8 chip {};
	chip.OP_9xy0();
	// std::cout << chip << std::endl;

	return 0;
}