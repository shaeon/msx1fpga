/*

MSX1 FPGA project

Copyright (c) 2016 Fabio Belavenuto

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Using Avelino Herreras Morales C library
http://msx.atlantes.org/index_en.html#sdccmsxdos

*/

#include <stdlib.h>
#include "hardware.h"
#include "bios.h"
#include "conio.h"
#include "msxdos.h"
#include "getopt.h"

/* Defines */
typedef unsigned char bool;
#define false 0
#define true 1

/* Constants */
const unsigned char REGS[] = {REG_MAPPER, REG_TURBO, REG_VOLBEEP, REG_VOLEAR,
							 REG_VOLPSG, REG_VOLSCC, REG_VOLOPLL, REG_VOLAUX1};
const unsigned char *ONOFFSTR[] = {"OFF", "ON"};
const unsigned char SCCMAPTYPEVAL[] = {0, 1, 3};
const unsigned char *SCCMAPTYPESTR[] = {"SCC-I", "ASCII 8", "ASCII 16"};

/* Structures */
struct tRegValPair {
	unsigned char reg;
	unsigned char value;
};

/* Global vars */
unsigned char hwid, hwversion, hwtxt[20], hwmemcfg, hwds;
unsigned char c, options;
unsigned int  i;
int fhandle;
struct tRegValPair rvp;
bool reset = false;
bool chg50 = false, chg60 = false;
bool saveregs = false;
char *filename = NULL;
bool loadregs = false;
bool chgmapper = false;
unsigned char newmapper;
bool chgturbo = false;
unsigned char newturbo;
bool chgsline = false;
unsigned char newsline;
bool chgsdoubler = false;
unsigned char newsdoubler;
bool chgvolbeep = false;
unsigned char newvolbeep;
bool chgvolear = false;
unsigned char newvolear;
bool chgvolpsg = false;
unsigned char newvolpsg;
bool chgvolscc = false;
unsigned char newvolscc;
bool chgvolopll = false;
unsigned char newvolopll;
bool chgvolaux = false;
unsigned char newvolaux;

/******************************************************************************/
void use()
{
	//             1111111111222222222233333333334 
	//    1234567890123456789012345678901234567890
	puts("Use:\r\n");
	puts("\r\n");
	//             1111111111222222222233333333334 
	//    1234567890123456789012345678901234567890
	puts("MSXCTRL -r -[5|6] -m<0-2> -t<0-1>\r\n");
	puts("        [-g<filename> | -l<filename>]\r\n");
	puts("        -b<0-255> -e<0-255> -p<0-255>\r\n");
	puts("        -s<0-255> -o<0-255> -a<0-255>\r\n");
	puts(" -r       Resets the machine\r\n");
	puts(" -5       Enable 50 Hz\r\n");
	puts(" -6       Enable 60 Hz\r\n");
	puts(" -g fn    Save the all registers to\r\n");
	puts("          file <fn>\r\n");
	puts(" -l fn    Restore the all registers\r\n");
	puts("          from file <fn>\r\n");
	puts(" -m 0-2   ESCCI Mapper type (0=SCCI,\r\n");
	puts("          1=ASCII8, 2=ASCII16)\r\n");
	//             1111111111222222222233333333334 
	//    1234567890123456789012345678901234567890
	puts(" -c 0-1   Scanlines (0=OFF, 1=ON)\r\n");
	puts(" -d 0-1   Scandoubler (0=OFF, 1=ON)\r\n");
	puts(" -t 0-1   Turbo (0=OFF, 1=ON)\r\n");
	puts(" -b 0-255 Keyboard Beep volume (0-255)\r\n");
	puts(" -e 0-255 EAR feedback volume (0-255)\r\n");
	puts(" -p 0-255 PSG volume (0-255)\r\n");
	puts(" -s 0-255 SCC volume (0-255)\r\n");
	puts(" -o 0-255 OPLL volume (0-255)\r\n");
	puts(" -a 0-255 AUX1 volume (0-255)\r\n");
	exit(1);
}

/******************************************************************************/
void readRegs()
{
	// Try open file
	fhandle = open(filename, O_RDONLY);
	if (fhandle == -1) {
		puts("Error opening file '");
		puts(filename);
		puts("'.\r\n");
		exit(2);
	}
	for (i = 0; i < sizeof(REGS); i++) {
		if (-1 == read(fhandle, &rvp, 2)) {
			if (last_error == EEOF) {
				puts("End of file!\r\n");
				close(fhandle);
				return;
			} else {
				puts("Reading error: ");
				puthex8(last_error);
				puts("!\r\n");
				close(fhandle);
				return;
			}
		}
		SWIOP_REGNUM = rvp.reg;
		SWIOP_REGVAL = rvp.value;
	}
	puts("Regs load sucessful!\r\n");
	close(fhandle);
}

/******************************************************************************/
void writeRegs()
{
	// write
	// Try open file
	fhandle = creat(filename, O_WRONLY, ATTR_NONE);
	if (fhandle == -1) {
		puts("Error opening file '");
		puts(filename);
		puts("'.\r\n");
		exit(2);
	}
	for (i = 0; i < sizeof(REGS); i++) {
		rvp.reg = REGS[i];
		SWIOP_REGNUM = rvp.reg;
		rvp.value = SWIOP_REGVAL;
		if (-1 == write(fhandle, &rvp, sizeof(rvp))) {
			puts("Error writing file.\r\n");
			close(fhandle);
			exit(4);
		}
	}
	puts("Regs write sucessful!\r\n");
	close(fhandle);
}

/******************************************************************************/
int main(char *argv[], int argc)
{
	puts("MSXCTRL.COM - Utility to manipulate\r\nMSX1FPGA core.\r\n");
/*
	// Init SWIO
	SWIOP_MKID = mymkid;
	if ((unsigned char)SWIOP_MKID != (unsigned char)~mymkid) {
		puts("MSX1FPGA core needed!\r\n");
		return 1;
	}
	// Read Hardware ID
	SWIOP_REGNUM = REG_HWID;
	hwid = SWIOP_REGVAL;
	SWIOP_REGNUM = REG_HWTXT;
	for (i = 0; i < 20; i++) {
		hwtxt[i] = SWIOP_REGVAL;
		if (hwtxt[i] == 0) {
			break;
		}
	}
	SWIOP_REGNUM = REG_HWVER;
	hwversion = SWIOP_REGVAL;
	SWIOP_REGNUM = REG_HWMEMCFG;
	hwmemcfg = SWIOP_REGVAL;
	SWIOP_REGNUM = REG_HWFLAGS;
	hwds = SWIOP_REGVAL & 0x01;
	puts("HW ID = ");
	puthex8(hwid);
	puts(" - ");
	puts(hwtxt);
	puts("\r\nVersion ");
	putdec8(hwversion >> 4);
	putchar('.');
	putdec8(hwversion & 0x0F);
	puts("\r\nMem config = ");
	puthex8(hwmemcfg);
	puts("\r\nHas HWDS = ");
	puthex8(hwds);
	puts("\r\n\r\n");
*/
	if (argc < 1) {
		use();
	}

	while ((c = getopt(argc, argv, "r56g:l:m:c:d:t:b:e:p:s:o:a:")) != 255) {
		switch (c) {
			case 'r':
				reset = true;
			break;

			case '5':
				chg50 = true;
			break;

			case '6':
				chg60 = true;
			break;

			case 'g':
				saveregs = true;
				filename = optarg;
			break;

			case 'l':
				loadregs = true;
				filename = optarg;
			break;

			case 'm':
				chgmapper = true;
				newmapper = atoi(optarg);
				if (newmapper > 2) {
					puts("Mapper type unknown.\r\n");
					use();
				}
			break;

			case 'c':
				chgsline = true;
				newsline = atoi(optarg);
				if (newsline > 1) {
					puts("Scanlines unknown.\r\n");
					use();
				}
			break;

			case 'd':
				chgsdoubler = true;
				newsdoubler = atoi(optarg);
				if (newsdoubler > 1) {
					puts("Scandoubler unknown.\r\n");
					use();
				}
			break;

			case 't':
				chgturbo = true;
				newturbo = atoi(optarg);
				if (newturbo > 1) {
					puts("Turbo unknown.\r\n");
					use();
				}
			break;

			case 'b':
				chgvolbeep = true;
				newvolbeep = atoi(optarg);
			break;

			case 'e':
				chgvolear = true;
				newvolear = atoi(optarg);
			break;

			case 'p':
				chgvolpsg = true;
				newvolpsg = atoi(optarg);
			break;

			case 's':
				chgvolscc = true;
				newvolscc = atoi(optarg);
			break;

			case 'o':
				chgvolopll = true;
				newvolopll = atoi(optarg);
			break;

			case 'a':
				chgvolaux = true;
				newvolaux = atoi(optarg);
			break;

			default:
				puts("Error in parameters.\r\n");
				use();
		}
	}

	if (chg50 && chg60) {
		puts("Choice only one vertical frequency!.");
		return 1;
	}
	if (saveregs && loadregs) {
		puts("Choice only save or load regs!.");
		return 1;
	}

	// Detects MSXDOS version
	msxdos_init();

	if (reset) {
		SWIOP_REGNUM = REG_RESET;
		SWIOP_REGVAL = 1;
	}

	if (chg50) {
		VDP_CMD = VDP_PAL;
		VDP_CMD = 0x89;
		puts("50 Hz selected.\r\n");
	} else if (chg60) {
		VDP_CMD = VDP_NTSC;
		VDP_CMD = 0x89;
		puts("60 Hz selected.\r\n");
	}

	if (loadregs) {
		readRegs();
	} else if (saveregs) {
		writeRegs();
	}

	if (chgmapper) {
		SWIOP_REGNUM = REG_MAPPER;
		SWIOP_REGVAL = SCCMAPTYPEVAL[newmapper];
		puts("New SCCI mapper type: ");
		puts(SCCMAPTYPESTR[newmapper]);
		puts("\r\n");
	}

	// OPTIONS
	SWIOP_REGNUM = REG_OPTIONS;
	options = SWIOP_REGVAL;

	if (chgsline) {
		if (newsline == 0) {
			options &= ~CFG_SCANLINES;
		} else {
			options |= CFG_SCANLINES;
		}
		puts("Scanlines ");
		puts(ONOFFSTR[newsline]);
		puts("\r\n");
	}

	if (chgsdoubler) {
		if (newsdoubler == 0) {
			options &= ~CFG_SCANDBL;
		} else {
			options |= CFG_SCANDBL;
		}
		puts("Scandoubler ");
		puts(ONOFFSTR[newsdoubler]);
		puts("\r\n");
	}

	SWIOP_REGVAL = options;

	// TURBO
	if (chgturbo) {
		SWIOP_REGNUM = REG_TURBO;
		SWIOP_REGVAL = newturbo;
		puts("Turbo ");
		puts(ONOFFSTR[newturbo]);
		puts("\r\n");
	}

	if (chgvolbeep) {
		SWIOP_REGNUM = REG_VOLBEEP;
		SWIOP_REGVAL = newvolbeep;
	}

	if (chgvolear) {
		SWIOP_REGNUM = REG_VOLEAR;
		SWIOP_REGVAL = newvolear;
	}

	if (chgvolpsg) {
		SWIOP_REGNUM = REG_VOLPSG;
		SWIOP_REGVAL = newvolpsg;
	}

	if (chgvolscc) {
		SWIOP_REGNUM = REG_VOLSCC;
		SWIOP_REGVAL = newvolscc;
	}

	if (chgvolopll) {
		SWIOP_REGNUM = REG_VOLOPLL;
		SWIOP_REGVAL = newvolopll;
	}

	if (chgvolaux) {
		SWIOP_REGNUM = REG_VOLAUX1;
		SWIOP_REGVAL = newvolaux;
	}

	// Volume return message
	if (chgvolbeep || chgvolear || chgvolpsg || chgvolscc || chgvolopll || chgvolaux) {
		puts("Volume changed.\r\n");
	}

	return 0;
}
