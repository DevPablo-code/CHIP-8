
#include <GL/glew.h>

#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>

#include "CHIP-8.h"

CHIP8::CHIP8(GLFWwindow *window):
window(window)
{
	srand(unsigned int(time(NULL)));

	memory = {};
	videoBuffer = {};
	registers = {};
	stack = {};

	DT = 0;
	ST = 0;
	IP = 0;
	SP = 0;
}

void CHIP8::loadProgram(const std::string &path)
{
	std::ifstream file(path, std::ios::binary);

	file.read((char*)&memory[PC], 0x1000 - 0x200);
}

void CHIP8::loadFont()
{
	for (int i = 0; i < 80; i++) 
	{
		memory[i] = FONT[i];
	}
}

void CHIP8::nextInstruction()
{
	Opcode = memory[PC] << 8;
	Opcode |= memory[PC + 1];
	PC += 2;
}

void CHIP8::executeInstruction()
{
	switch (Opcode & 0xF000) {
		case 0x0000: {
			switch (Opcode & 0x00FF) {
				case 0x00E0: {
					LOG("CLS");
					videoBuffer.fill(0);
					glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 32, GL_RED, GL_UNSIGNED_BYTE, (const void*)videoBuffer.data());
					break;
				}
				case 0x00EE: {
					LOG("RET");
					PC = stack[SP];
					SP -= 1;
					break;
				}
			}
			break;
		}
		case 0x1000: {
			LOG("JMP");
			PC = Opcode & 0x0FFF;
			break;
		}
		case 0x2000: {
			LOG("CALL");
			SP += 1;
			stack[SP] = PC;
			PC = Opcode & 0x0FFF;
			break;
		}
		case 0x3000: {
			LOG("SE Vx, byte");
			uint8_t x = ((Opcode & 0x0F00) >> 8);
			if (registers[x] == (Opcode & 0x00FF)) 
			{
				PC += 2;
			}
			break;
		}
		case 0x4000: {
			LOG("SNE Vx, byte");
			uint8_t x = ((Opcode & 0x0F00) >> 8);
			if (registers[x] != (Opcode & 0x00FF))
			{
				PC += 2;
			}
			break;
		}
		case 0x5000: {
			LOG("SE Vx, Vy");
			uint8_t x = ((Opcode & 0x0F00) >> 8);
			uint8_t y = ((Opcode & 0x00F0) >> 4);
			if (registers[x] == registers[y])
			{
				PC += 2;
			}
			break;
		}
		case 0x6000: {
			LOG("LD Vx, byte");
			uint8_t x = ((Opcode & 0x0F00) >> 8);
			registers[x] = (Opcode & 0x00FF);
			break;
		}
		case 0x7000: {
			LOG("ADD Vx, byte");
			uint8_t x = ((Opcode & 0x0F00) >> 8);
			registers[x] += (Opcode & 0x00FF);
			break;
		}
		case 0x8000: {
			switch(Opcode & 0x000F) 
			{
				case 0x0000: {
					LOG("LD Vx, Vy");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					uint8_t y = ((Opcode & 0x00F0) >> 4);
					registers[x] = registers[y];
					break;
				}
				case 0x0001: {
					LOG("OR Vx, Vy");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					uint8_t y = ((Opcode & 0x00F0) >> 4);
					registers[x] |= registers[y];
					break;
				}
				case 0x0002: {
					LOG("AND Vx, Vy");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					uint8_t y = ((Opcode & 0x00F0) >> 4);
					registers[x] &= registers[y];
					break;
				}
				case 0x0003: {
					LOG("XOR Vx, Vy");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					uint8_t y = ((Opcode & 0x00F0) >> 4);
					registers[x] ^= registers[y];
					break;
				}
				case 0x0004: {
					LOG("ADD Vx, Vy");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					uint8_t y = ((Opcode & 0x00F0) >> 4);
					uint16_t result = registers[x] + registers[y];
					registers[0xF] = result > 255;
					registers[x] = uint8_t(result);
					break;
				}
				case 0x0005: {
					LOG("SUB Vx, Vy");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					uint8_t y = ((Opcode & 0x00F0) >> 4);
					registers[0xF] = registers[x] > registers[y];
					registers[x] -= registers[y];
					break;
				}
				case 0x0006: {
					LOG("SHR Vx {, Vy}");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					registers[0xF] = (registers[x] & 1);
					registers[x] >>= 1;
					break;
				}
				case 0x0007: {
					LOG("SUBN Vx, Vy");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					uint8_t y = ((Opcode & 0x00F0) >> 4);
					registers[0xF] = registers[y] > registers[x];
					registers[y] -= registers[x];
					break;
				}
				case 0x000E: {
					LOG("SHL Vx {, Vy}");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					registers[0xF] = (registers[x] >> 7);
					registers[x] <<= 1;
					break;
				}
			}
			break;
		}
		case 0x9000: {
			LOG("SNE Vx, Vy");
			uint8_t x = ((Opcode & 0x0F00) >> 8);
			uint8_t y = ((Opcode & 0x00F0) >> 4);
			if (registers[x] != registers[y]) 
			{
				PC += 2;
			}
			break;
		}
		case 0xA000: {
			LOG("LD I, addr");
			IP = Opcode & 0x0FFF;
			break;
		}
		case 0xB000: {
			LOG("JP V0, addr");
			PC = (Opcode & 0x0FFF) + registers[0x0];
			break;
		}
		case 0xC000: {
			LOG("RND Vx, byte");
			uint8_t x = ((Opcode & 0x0F00) >> 8);
			registers[x] = (rand() % 255 + 1) & (Opcode & 0x00FF);
			break;
		}
		case 0xD000: {
			LOG("DRW Vx, Vy, nibble");
			uint8_t x = ((Opcode & 0x0F00) >> 8);
			uint8_t y = ((Opcode & 0x00F0) >> 4);
			uint8_t n = (Opcode & 0x000F);
			registers[0xF] = 0;
			
			for (int i = 0; i < n; i++) {
				uint8_t sprite = memory[IP + i];
				int32_t row = (registers[y] + i) % 32;
				
				for (int j = 0; j < 8; j++)
				{
					uint8_t b = (sprite & (0x80 >> j));
					int32_t col = (registers[x] + j) % 64;

					if (b) {
						if (videoBuffer[row * 64 + col]) {
							registers[0xF] = 1;
						}
						videoBuffer[row * 64 + col] ^= 1;
						videoBuffer[row * 64 + col] *= -1;
					}
				}
			}
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 32, GL_RED, GL_UNSIGNED_BYTE, (const void*)videoBuffer.data());
			break;
		}
		case 0xE000: {
			switch (Opcode & 0x00FF)
			{
				case 0x009E: {
					LOG("SKP Vx");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					if (glfwGetKey(window, keys.at(registers[x])) == GLFW_PRESS)
					{
						PC += 2;
					}
					break;
				}
				case 0x00A1: {
					LOG("SKNP Vx");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					if (glfwGetKey(window, keys.at(registers[x])) != GLFW_PRESS)
					{
						PC += 2;
					}
					break;
				}
			}
			break;
		}
		case 0xF000: {
			switch (Opcode & 0x00FF)
			{
				case 0x0007: {
					LOG("LD Vx, DT");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					registers[x] = DT;
					break;
				}
				case 0x000A: {
					LOG("LD Vx, K");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					bool keyPressed = false;
					for (int i = 0; i < 16; i++)
					{
						if (glfwGetKey(window, keys.at(i)) == GLFW_PRESS)
						{
							registers[x] = i;
							keyPressed = true;
							break;
						}
					}
					if (!keyPressed) 
					{
						PC -= 2;
					}
					break;
				}
				case 0x0015: {
					LOG("LD DT, Vx");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					DT = registers[x];
					break;
				}
				case 0x0018: {
					LOG("LD ST, Vx");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					ST = registers[x];
					break;
				}
				case 0x001E: {
					LOG("ADD I, Vx");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					if (registers[x] + IP > 0xFFF)
					{
						registers[0xF] = 1;
					}
					else
					{
						registers[0xF] = 0;
					}
					IP += registers[x];
					break;
				}
				case 0x0029: {
					LOG("LD F, Vx");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					IP = registers[x] * 5;
					break;
				}
				case 0x0033: {
					LOG("LD B, Vx");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					memory[IP] = registers[x] / 100;
					memory[IP + 1] = (registers[x] / 10) % 10;
					memory[IP + 2] = (registers[x] % 100) % 10;
					break;
				}
				case 0x0055: {
					LOG("LD [I], Vx");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					for (int i = 0; i <= x; i++)
					{
						memory[IP + i] = registers[i];
					}
					break;
				}
				case 0x0065: {
					LOG("LD Vx, [I]");
					uint8_t x = ((Opcode & 0x0F00) >> 8);
					for (int i = 0; i <= x; i++)
					{
						registers[i] = memory[IP + i];
					}
					break;
				}
			}
			break;
		}
	}

	auto now = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimerTick).count() > 1000 / 60)
	{
		if (DT > 0) {
			DT -= 1;
			lastTimerTick = now;
		}
		if (ST > 0) {
			ST -= 1;
			std::cout << "\a";
			lastTimerTick = now;
		}
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1000 / INSTRCUTIONS_PER_SECOND));
}
