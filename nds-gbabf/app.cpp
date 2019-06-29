#include "app.h"


static PrintConsole topScreen;
static PrintConsole bottomScreen;


u16 swapBits(u16 n)
{
	u16 p1 = 0;
	u16 p2 = 1;
	u16 bit1 = (n >> p1) & 1;
	u16 bit2 = (n >> p2) & 1;
	u16 x = (bit1 ^ bit2);
	x = (x << p1) | (x << p2);
	u16 result = n ^ x;
	return result;
}

u16 read_word_rom(unsigned long address) {
	return GBAROM[address];
}
u16 read_swapped_word_rom(unsigned long address) {
	return swapBits(read_word_rom(address));
}

void write_word_rom(unsigned long address, u16 byte) {
	GBA_BUS[address] = byte;
	swiDelay(10);
}
void write_swapped_word_rom(unsigned long address, u16 byte) {
	write_word_rom(address, swapBits(byte));
}

void write_programming_cycle() {
	write_swapped_word_rom(0x555, 0xAA);
	write_swapped_word_rom(0x2AA, 0x55);
	write_swapped_word_rom(0x555, 0xA0);
}

void reset_MSP55LV128() {
	write_swapped_word_rom(0x0, 0xF0);
}

void erase_MSP55LV128() {

	iprintf("Erasing...");

	write_swapped_word_rom(0x555, 0xAA);
	write_swapped_word_rom(0x2AA, 0x55);
	write_swapped_word_rom(0x555, 0x80);
	write_swapped_word_rom(0x555, 0xAA);
	write_swapped_word_rom(0x2AA, 0x55);
	write_swapped_word_rom(0x555, 0x10);

	u16 status = 0x0000;
	do {
		status = read_swapped_word_rom(0x0000);
		iprintf("Status: %04x\n", status);
	} while ((status | 0xFF7F) != 0xFFFF);

	iprintf("Done!\n");
}
void write_MSP55LV128() {

	reset_MSP55LV128();

	if (fatInitDefault()) {

		FILE* fd = fopen("fat:/ZELDA.gba", "rb");

		if (fd == NULL) {
			iprintf("Error opening file");
			return;
		}

		unsigned long fileSize = 8388608;

		u16 currWord = 0x1234;
		u8 w0 = 0xA0;
		u8 w1 = 0xAA;

		for (unsigned long currAddress = 0, currByte = 0; currByte < fileSize; currAddress += 1, currByte += 2) {

			fread(&w1, 1, 1, fd);
			fread(&w0, 1, 1, fd);

			currWord = ((w0 & 0xFF) << 8) | (w1 & 0xFF);

			//iprintf("Word: %02x %02x %04x\n", w0, w1, currWord);

			write_programming_cycle();

			write_word_rom(currAddress, currWord);


			u16 status = 0x0000;
			do {
				status = read_word_rom(currAddress);
				iprintf("[%u] Status: %04x\n", currAddress, status);
			} while ((status | 0xFF7F) != (currWord | 0xFF7F));

		}

		iprintf("Done!\n");

	}
	else {
		iprintf("fatInitDefault failure: terminating\n");
	}




}

void write_buffered_MSP55LV128() {

	reset_MSP55LV128();

	if (fatInitDefault()) {

		FILE* fd = fopen("fat:/Nelson/POKEMON_RUBY_AXVE01.gba", "rb");

		if (fd == NULL) {
			iprintf("Error opening file");
			return;
		}


		unsigned long currSector = 0x00000;

		unsigned long fileSize = 0x10000 * 1;



		u8* buffer = (u8*)malloc(sizeof(u8) * 512);
		bzero(buffer, sizeof(u8) * 512);


		for (unsigned long currBuffer = 0; currBuffer < 0x10000; currBuffer += 512) {

			fread(buffer, 1, 512, fd);
			for (int currFlashBuffer = 0; currFlashBuffer < 512; currFlashBuffer += 32) {

				write_swapped_word_rom(0x555, 0xAA);
				write_swapped_word_rom(0x2AA, 0x55);
				write_swapped_word_rom(currSector, 0x25);
				write_swapped_word_rom(currSector, 0xF);

				u16 currWord = 0x0000;
				for (int currByte = 0, currWordOffset = 0; currByte < 32; currByte += 2, currWordOffset++) {

					u8 b0 = buffer[currFlashBuffer + currByte + 1];
					u8 b1 = buffer[currFlashBuffer + currByte];

					currWord = ((b0 & 0xFF) << 8) | (b1 & 0xFF);
					iprintf("Word: %04x\n", currWord);
					write_word_rom(currSector + currBuffer + currFlashBuffer + currWordOffset, currWord);
				}

				write_swapped_word_rom(currSector, 0x29);

				u16 status = read_swapped_word_rom(currSector + currBuffer + currBuffer + 30);
				iprintf("Status: %04x\n", status);
				while ((status | 0xFF7F) != (currWord | 0xFF7F)) {
					status = read_swapped_word_rom(currSector + currBuffer + currBuffer + 30);
					if (status == 0xffff) {
						currFlashBuffer -= 32;
					}
				}
			}

		}


		fclose(fd);
		iprintf("Done!");

	}
	else {
		iprintf("fatInitDefault failure: terminating\n");
	}




}

void read_MSP55LV128() {

	for (unsigned int address = 0; address < 10; address++) {


		u16 statusAddr = read_word_rom(address);

		iprintf("[%u] Data: %04x\n", address, statusAddr);
	}

}



int main() {



	// Initialize consoles
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	consoleInit(&bottomScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

	// Enable accessing slot 2:
	sysSetCartOwner(true);

	while (true) {


		while (true) {

			swiWaitForVBlank();

			scanKeys();
			uint32 input = keysDownRepeat();

			if (input & KEY_B) {
				write_MSP55LV128();
				break;
			}
			if (input & KEY_RIGHT) {
				erase_MSP55LV128();
				break;
			}
			if (input & KEY_DOWN) {
				read_MSP55LV128();
				break;
			}



		}


	}







	return 0;
}