// credit to this guy
// https://austinmorlan.com/posts/chip8_emulator
// unfinished

#include <iostream>
#include <cstdint>
#include <fstream>

class Chip8 {
public:
	uint8_t registers[16] {};
	uint8_t memory[3096] {};
	uint16_t index {};
	uint16_t pc {};		// program counter
	uint8_t sp {};		// stack pointer
	uint8_t delayTimer {};
	uint8_t soundTimer {};
	uint8_t keypad[16] {};
	uint32_t video[64 * 32] {};
	uint16_t opcode;
};

const unsigned int START_ADDRESS = 0x200;

void Chip8::LoadROM(char const* filename) {
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
		for (long i = 0; i < size; ++i) {
			memory[START_ADDRESS + i] = buffer[i];
		}
		
		// Free the buffer
		delete[] buffer;
	}
}

Chip8::Chip8() {
	// Initialize PC
	pc = START_ADDRESS;
}

int main() {
	std::cout << "Hello, World!" << std::endl;

	return 0;
}