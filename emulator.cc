#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

unsigned int text[2*1024 / 4]; //word addressable
unsigned int staticData[4*1024]; //byte addressable
unsigned int stack[2*1024]; //byte addressable
int registers[32];

int pc; //Program counter
int hireg;
int loreg;

/**
 * Takes a byte-address in order to access the byte-addressable stack and static-data 
 * segments of memory, except for text segment which is word addressable
 */
int getAddress(unsigned int address) {
	if (address>=0x7fffeffc && address < 0x00400000) {
		return stack[address - 0x7fffeffc];
	}

	if (address>=0x00400000 && address < 0x10010000) {
		return text[address - 0x00400000];
	}

	if (address >= 0x10010000) {
		return staticData[address - 0x10010000];
	}
}

/**
 * takes a byte-address in order to access the byte-addressable stack and 
 * static-data segments of memory, except for text segment which is word addressable
 */
int storeAddress(unsigned int address, int wordToStore) {
	if (address>=0x7fffeffc && address < 0x00400000) {
		return stack[address - 0x7fffeffc] = wordToStore;
	}
	if (address>=0x00400000 && address < 0x10010000) {
		return text[address - 0x00400000] = wordToStore;
	}

	if (address >= 0x10010000) {
		return staticData[address - 0x10010000] = wordToStore;
	}
}

//LB ra, b(rc)
void lb(int a, int b, int c) {
	registers[a] = (signed int)(signed char)(getAddress(b+registers[c]) & 0xFF);
}

//LBU ra, b(rc)
void lbu(int a, unsigned int b, int c) {
	registers[a] = getAddress(b+registers[c]);
}

//LW ra, b(rc)
void lw(int a, int b, int c) {
	registers[a] = (getAddress(b+registers[c] + 3)) + (getAddress(b
			+registers[c]+2)<< 8) + (getAddress(b+registers[c]+1) << 16)
			+ (getAddress(b+registers[c]) << 24);
}

void sb(int a, int b, int c) {
	storeAddress(b+registers[c], registers[a] & 0xFF); //0xFF = 8 one's in a row to get first byte
}

void sw(int a, int b, int c) {
	storeAddress(b+registers[c] + 3, (registers[a] & 0xFF));
	storeAddress(b+registers[c] + 2, (((registers[a] & 0xFF00) >> 8)));
	storeAddress(b+registers[c] + 1, (((registers[a] & 0xFF0000) >> 16)));
	storeAddress(b+registers[c], (((registers[a] & 0xFF000000) >> 24)));
}

void lui(int a, unsigned short b) {
	unsigned int bval = b;
	registers[a] = bval << 16;
}
//ADD dreg, ra, rb
void add(int dreg, int a, int b) {
	registers[dreg] = registers[a] + registers[b];
}

//ADDI dreg, ra, c
void addi(int dreg, int a, signed int c) {
	registers[dreg] = registers[a] + c;
}

//convert regular ints to unsigned ints
//ADDU dreg, ra, rb
void addu(int dreg, int a, int b) {
	unsigned int unsA = registers[a];
	unsigned int unsB = registers[b];
	registers[dreg] = unsA + unsB;
}

//ADDIU dreg, ra, c
void addiu(int dreg, int a, int c) {
	unsigned int unsA = registers[a];
	unsigned int unsC = c;
	registers[dreg] = unsA + unsC;
}

//AND dreg, ra, rb
void andfunc(int dreg, int a, int b) {
	registers[dreg] = registers[a] & registers[b];
}

//MULT ra, rb
//Product goes into HI and LO registers
void mult(int a, int b) {
	long longa = registers[a];
	long longb = registers[b];
	unsigned long long product = longa * longb;
	loreg = (int)((product << 32) >> 32);
	hireg = (int)(product >> 32);
}

//MULTU ra, rb
void multu(int a, int b) {
	unsigned int unsA = registers[a];
	unsigned int unsB = registers[b];
	long long product = unsA * unsB;
	hireg = product >> 32;
	loreg = (product << 32) >> 32;
}

//OR dreg, ra , rb
void orfunc(int dreg, int a, int b) {
	registers[dreg] = registers[a] | registers[b];
}

//ORI rd, ra, c
void ori(int dreg, int a, unsigned short c) {
	unsigned int shift = (unsigned short) c;
	registers[dreg] = registers[a] | shift;
}

//XOR dreg, r1, r2
void xorfunc(int dreg, int reg1, int reg2) {
	registers[dreg] = registers[reg1]^registers[reg2];
}

//SLL dreg, ra, c
void sll(int dreg, int a, unsigned int c) {
	registers[dreg] = registers[a] << c;
}

//SRA dreg, ra, c
void sra(int dreg, int a, signed int c) {
	registers[dreg] = (signed int)registers[a] >> c;
}

//SRL dreg, ra, c
void srl(int dreg, int a, unsigned int c) {
	registers[dreg] = registers[a] >> c;
}

void sub(int dreg, int a, int b) {
	registers[dreg] = registers[a] - registers[b];
}

//SUBU rd, ra, rb
void subu(int dreg, int a, int b) {
	unsigned int unsA = registers[a];
	unsigned int unsB = registers[b];
	registers[dreg] = unsA - unsB;
}

void slt(int dreg, int a, int b) {
	if (registers[a] < registers[b])
		registers[dreg] = 1;
	else
		registers[dreg] = 0;
}

void slti(int dreg, int a, int c) {
	if (registers[a] < c)
		registers[dreg] = 1;
	else
		registers[dreg] = 0;
}

void sltu(int dreg, int a, int b) {
	unsigned int unsA = registers[a];
	unsigned int unsB = registers[b];
	if (unsA < unsB)
		registers[dreg] = 1;
	else
		registers[dreg] = 0;
}

void sltiu(int dreg, int a, int c) {
	unsigned int unsA = registers[a];
	unsigned int unsC = c;
	if (unsA < unsC)
		registers[dreg] = 1;
	else
		registers[dreg] = 0;
}

void beq(int a, int b, int c) {
	if (registers[a] == registers[b])
		pc += ((c & 0xFFFFF) - 1);
}

void bgez(int a, int c) {
	if (registers[a] >= 0)
		pc += (c & 0xFFFFF) - 1;
}

void bgtz(int a, int c) {
	if (registers[a] > 0)
		pc += (c & 0xFFFFF) - 1;
}

void blez(int a, int c) {
	if (registers[a] <= 0)
		pc += (c & 0xFFFFF) - 1;
}

void bltz(int a, int c) {
	if (registers[a] < 0)
		pc += (c & 0xFFFFF) - 1;
}

void bne(int a, int b, int c) {
	if (registers[a] != registers[b])
		pc += (c & 0xFFFFF) - 1;
}

void jump(int c) {
	pc = (pc & 0xF0000000) | (c & 0xFFFF);
}

void jal(int c) {
	registers[31] = pc;
	pc = (pc & 0xF0000000) | (c & 0xFFFF);
}

void jr(int a) {
	// need to divide by 4 because address given as byte addressable
	if (a == 31) {
		pc = (pc & 0xF0000000) | (registers[a] & 0xFFFF);
	} else
		pc = (pc & 0xF0000000) | ((registers[a] / 4) & 0xFFFF);
}

void mfhi(int a) {
	registers[a] = hireg;
}
void mflo(int a) {
	registers[a] = loreg;
}

void syscall() {
	int v0 = registers[2]; // register 2 is v0
	switch (v0) {
	case 1:
		// printf int
		printf("%d", registers[4]); //registers 4-7 are a0-a3
		break;
	case 4:
		// printf string
		char toPrint [80];
		int stringIndex;
		stringIndex = 0;
		while (true) {
			char ch = (char)(getAddress(registers[4] + stringIndex));
			if (ch == 0)
				break;
			toPrint [stringIndex] = ch;
			stringIndex++;
		}
		toPrint [stringIndex] = 0x0; // null at the end of the string
		printf("%s", &toPrint[0]); //registers 4-7 are a0-a3
		break;
	case 5:
		// scanf int
		scanf("%d", &registers[2]);
		break;
	case 8:
		// scanf string
		char str [80];
		//		a0 = registers[4];
		//		a1 = registers[5];
		scanf("%s", str);
		registers[4] = (unsigned int) &str;
		
		int strIndex;
		strIndex = 0;
		while (true) {
			char ch = str[strIndex];
			if (ch == 0)
				break;
			strIndex++;
		}
		registers[5] = strIndex;
//		registers[5] = sizeof(str)/sizeof(char) + 1;
		break;
	case 10:
		// exit program
		exit(1);
		break;
	}
}

/**
 * Takes an instruction, decodes it, and executes appropriate command
 */
void parseLine(int instruction) {
	// increment program pointer
	pc += 1;
	registers[0] = 0;

	//parse registry code
	int opcode = (instruction & 0xFC000000) >> 26;
	// R-type
	int rs = (instruction & 0x3E00000) >> 21;
	int rt = (instruction & 0x1F0000) >> 16;
	int rd = (instruction & 0xF800) >> 11;
	int shift = (instruction & 0x7C0) >> 6;
	int funct = instruction & 0x3F;

	// I-type
	short imm = instruction & 0xFFFF;
	// J-type
	int address = instruction & 0x3FFFFFF;

	switch (opcode) {
	case 0x0:
		// R type
		switch (funct) {
		case 0x20:
			add(rd, rs, rt);
			break;
		case 0x21:
			addu(rd, rs, rt);
			break;
		case 0x24:
			andfunc(rd, rs, rt); // need to call it something else
			break;
		case 0x8:
			jr(rs);
			break;
		case 0x10:
			mfhi(rd);
			break;
		case 0x12:
			mflo(rd);
			break;
		case 0x18:
			mult(rs, rt);
			break;
		case 0x19:
			multu(rs, rt);
			break;
		case 0x25:
			orfunc(rd, rs, rt);
			break;
		case 0x0:
			sll(rd, rt, shift);
			break;
		case 42:
			slt(rd, rs, rt);
			break;
		case 43:
			sltu(rd, rs, rt);
			break;
		case 0x3:
			sra(rd, rt, shift);
			break;
		case 0x2:
			srl(rd, rt, shift);
			break;
		case 0x22:
			sub(rd, rs, rt);
			break;
		case 0x23:
			subu(rd, rs, rt);
			break;
		case 0xC:
			syscall();
			break;
		case 0x26:
			xorfunc(rd, rs, rt);
			break;
		}
		break;
	// I-type
	case 0x08:
		addi(rt, rs, imm);
		break;
	case 0x09:
		addiu(rt, rs, imm);
		break;
	case 0x4:
		beq(rs, rt, imm);
		break;
	case 0x1:
		switch (rt) {
		case 1:
			bgez(rs, imm);
			break;
		case 0:
			bltz(rs, imm);
			break;
		}
		break;
	case 0x7:
		if (rt == 0)
			bgtz(rs, imm);
		break;
	case 0x6:
		if (rt == 0)
			blez(rs, imm);
		break;
	case 0x5:
		bne(rs, rt, imm);
		break;
	case 0x20:
		lb(rt, imm, rs);
		break;
	case 0x24:
		lbu(rt, imm, rs);
		break;
	case 0xF:
		lui(rt, imm);
		break;
	case 0x23:
		lw(rt, imm, rs);
		break;
	case 0xD:
		ori(rt, rs, imm);
		break;
	case 0x28:
		sb(rt, imm, rs);
		break;
	case 0xA:
		slti(rt, rs, imm);
		break;
	case 0xB:
		sltiu(rt, rs, imm);
		break;
	case 0x2B:
		sw(rt, imm, rs);
		break;
	// J-type
	case 0x2:
		jump(address);
		break;
	case 0x3:
		jal(address);
		break;
	default:
		cout << "not a valid instruction" << endl;
		break;
	}
}

/**
 * Takes the source file and reads each line into an array
 */
void readFile(string filename) {
	string line;
	ifstream myfile(filename.c_str());
	vector<string> entireFile(1000000);
	if (myfile.is_open()) {

		int i=0;
		while (!myfile.eof() ) {
			getline(myfile, line);
			cout << "line is " << line << endl;
			entireFile[i]=line;
			i++;
		}
		myfile.close();
	}

	int textSize;
	unsigned int j;
	for (j=0; j<entireFile.size(); j++) {

		if (entireFile[j] == "DATA SEGMENT") {
			textSize = j;
			cout << "found data segment at " << textSize << endl;
			break;
		}

		cout << "string is: " << entireFile[j] << endl;
		sscanf(entireFile[j].c_str(), "%x", &text[j]);
		cout << "text at " << dec << j << " is " << hex << text[j] << endl;
	}

	cout << "out of first loop" << endl;
	unsigned int k;
	for (k = textSize + 1; k < entireFile.size(); k++) {
		string first =entireFile[k];
		if (first.empty())
			break;
		cout << "gonna split string: " << first << endl;
		string firstStr=first.substr(0, 10);
		string secondStr=first.substr(11, 10);
		int firstInt;
		int secondInt;
		sscanf(firstStr.c_str(), "%x", &firstInt);
		sscanf(secondStr.c_str(), "%x", &secondInt);
		// store word equivalent to achieve byte alignment
		storeAddress(firstInt + 3, (secondInt & 0xFF));
		storeAddress(firstInt + 2, (((secondInt & 0xFF00) >> 8)));
		storeAddress(firstInt + 1, (((secondInt & 0xFF0000) >> 16)));
		storeAddress(firstInt, (((secondInt & 0xFF000000) >> 24)));
	}

}

int main(int argc, char* argv[]) {
	registers[29] = 0x7fffeffc;
	pc = 0;

	cout << "argc = " << argc << endl;
	//    string fileName = "./sum.o";
	//	cout << fileName << endl;
	string fileName;
	//	cout << "Enter name of instruction file: ";
	//	cin >> fileName;
	fileName = "torture.o";
	//	cout << "Choose Mode (0:Run to completion; 1:Single step): ";
	int mode;
	//	cin >> mode;
	mode = 0;
	//mode 0 = run to completion
	//mode 1 = step through program
	readFile(fileName);
	if (mode == 0) { //if user passes run to completion mode
		cout << "run to completion mode------" << endl;

		while (true) { // infinite loop because we expect user to have a syscall exit
			parseLine(text[pc]);
		}
	} else if (mode == 1) { //single step through program
		cout << "single step mode------" << endl;

		while (1) {
			string input;
			cin >> input;

			//  p_reg print a specific register (e.g., p 4, prints 
			//      the contents in hex of register 4)
			//	p_all print the contents of all registers, including the PC, HI, & LO in hex
			//	d_addr print the contents of memory location addr in hex, 
			//		assume addr is a word address in hex.
			//	s_n execute the next n instructions and stop (should print each 
			//		instruction executed), then wait for the user to input another command

			if (input.substr(0, input.length()) == "p_all") {
				for (int i=0; i<32; i++) {
					cout << "register " << dec << i << ": "<< hex
							<< registers[i] << endl;
				}
			} else if (input.at(0) == 'p') {
				int registerNum;
				sscanf(input.substr(2, input.size()-2).c_str(), "%d", &registerNum);
				cout << "register " << dec << registerNum << ": " << hex
						<< registers[registerNum] << endl;
			}

			if (input.at(0) == 'd') {
				int addr;
				sscanf(input.substr(2).c_str(), "%x", &addr);
				cout << hex << getAddress(addr) << endl;
			}

			if (input.at(0) == 's') {
				int numSkip;
				sscanf(input.substr(2, input.size()-2).c_str(), "%d", &numSkip);
				int i;
				int instr;
				for (i = 0; i < numSkip; i++) {
					instr = text[pc];
					cout << "Instruction: " << hex << instr << endl;
					parseLine(instr);
				}

			}

		}
	}
	return 0;
}
