// Definitions for various aspects of the DCPU-16

using namespace std;

#include <sys/time.h>

#define RAM_SIZE 0x10000 // 64k words, 128k bytes
#define NUM_REG 8
#define CLOCK_SPEED 100000 // 100k Hz 

// 0x8000 is the start of the terminal buffer
// the terminal is 32 chars by 16 chars (32 columns, 16 rows)
// this means 0x8000 - 0x81ff is the entire screen buffer
// # + 48 = ascii representation for that digit
// newline = 12


#define SCREEN_BUFFER 0x8000
#define TERMINAL_WIDTH 32
#define TERMINAL_HEIGHT 16
#define NUM_COLORS 0x1ff                // 9 bit color

#define INPUT_BUFFER 0x9000
// 0x9000 is the input buffer.
// make sure to set 0x9000 to 0 when you are done with it, or don't want it
// Many people impliment a 16 char ring buffer at 0x9000, I did 1 char. Easier to deal with


#define NOB_ 0
#define SET_ 1
#define ADD_ 2
#define SUB_ 3
#define MUL_ 4
#define DIV_ 5
#define MOD_ 6
#define SHL_ 7
#define SHR_ 8
#define AND_ 9
#define BOR_ 10
#define XOR_ 11
#define IFE_ 12
#define IFN_ 13
#define IFG_ 14
#define IFB_ 15


// Map for what values point to what registers/addresses/literals
#define REGISTER_LOW 0
#define REGISTER_HIGH 7
#define REGISTER_RAM_LOW 8
#define REGISTER_RAM_HIGH 15
#define NEXT_WORD_REG_LOW 16
#define NEXT_WORD_REG_HIGH 23
#define _POP 24
#define _PEEK 25
#define _PUSH 26
#define _SP 27
#define _PC 28
#define _O 29
#define NEXT_WORD 30
#define NEXT_WORD_LITERAL 31
#define LITERAL_VALUE_LOW 32
#define LITERAL_VALUE_HIGH 63

#define _A 0
#define _B 1
#define _C 2
#define _X 3
#define _Y 4
#define _Z 5
#define _I 6
#define _J 7

#ifndef DCPU_H
#define DCPU_H

class Dcpu
{
    public:
        Dcpu(ifstream &code, bool debug, bool fast);
        ~Dcpu();

        void run();
        void MemoryDump();
	unsigned short *GetScreenBuffer();
	void kill();
        void PushInBuff(char c);
        bool isKilled();
    private:
        bool DEBUG;
        bool dont_kill;
        bool wait_cycles;

        unsigned short *registers;  // pointer to the registers, 8 total
        unsigned short *RAM;        // pointer to the RAM, 0x10000 words total, 128K 
        unsigned short PC;          // program counter, points to RAM address of current instruciton
        unsigned short old_PC;      // the program counter from the start of the operation
        unsigned short SP;          // points to the top of the stack
        unsigned short O;           // register for overflow detection
        unsigned short *literals;
        bool skip_next_ins;
        int cycles_to_wait;

        void wait(int cycles, timeval &start);
        void init_terminal();
        void UpdateScreen();
        bool OctetNonZero(int index);

        unsigned short *GetValuePtr(int a); // gets a pointer to a value defined by an instruction

/*0x0*/ void NonBasic(unsigned short _a, unsigned short _b);
/*0x1*/ void SET(unsigned short _a, unsigned short _b); // a = b;
/*0x2*/ void ADD(unsigned short _a, unsigned short _b); // a = a + b; O = 0x0001 if overflow, otherwise 0x0
/*0x3*/ void SUB(unsigned short _a, unsigned short _b); // a = a - b; O = 0xfff is underflow, otherwise 0x0
/*0x4*/ void MUL(unsigned short _a, unsigned short _b); // a = a * b; O = ((a * b) >> 16) & 0xffff
/*0x5*/ void DIV(unsigned short _a, unsigned short _b); // a = a / b; O = ((a << 16) / b ) & 0xffff
/*0x6*/ void MOD(unsigned short _a, unsigned short _b); // a = a % b; if b == 0, a = 0
/*0x7*/ void SHL(unsigned short _a, unsigned short _b); // a = a << b; O = ((a << b)>>16) & 0xffff
/*0x8*/ void SHR(unsigned short _a, unsigned short _b); // a = a >> b; O = ((a >> b)<<16) & 0xffff 
/*0x9*/ void AND(unsigned short _a, unsigned short _b); // a = a & b;
/*0xA*/ void BOR(unsigned short _a, unsigned short _b); // a = a | b;
/*0xB*/ void XOR(unsigned short _a, unsigned short _b); // a = a ^ b;
/*0xC*/ void IFE(unsigned short _a, unsigned short _b); // perform next instruction if a == b
/*0xD*/ void IFN(unsigned short _a, unsigned short _b); // perform next instruction if a != b
/*0xE*/ void IFG(unsigned short _a, unsigned short _b); // perform next instruction if a > b
/*0xF*/ void IFB(unsigned short _a, unsigned short _b); // perform next instruction if (a & b) != 0
};


#endif

