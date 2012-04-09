//
// Emulator for Notch's DCPU for his upcoming gmae 0x10c
//

using namespace std;

#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <iomanip>

#include <sys/time.h>

#include "dcpu.h"

bool ishex(char c)
{
    if(isdigit(c))
        return true;
    if(tolower(c) >= 'a' && tolower(c) <= 'f')
        return true;

    return false;
}

Dcpu::Dcpu(ifstream &code, bool debug, bool fast)
{
    DEBUG = debug;
    dont_kill = true;
    wait_cycles = fast;
    dirtyScreen = false;

    registers = new unsigned short[NUM_REG];
    RAM = new unsigned short[RAM_SIZE];
    PC = 0;
    SP = 0xffff; // stack pointer starts at 0xffff and counts downwards
    O = 0;

    literals = new unsigned short[LITERAL_VALUE_HIGH - LITERAL_VALUE_LOW];
    for(int i = 0; i < LITERAL_VALUE_HIGH - LITERAL_VALUE_LOW; i++)
        literals[i] = i;

    skip_next_ins = false;
    cycles_to_wait = 0;

    // read the code into RAM
    unsigned short nextword;
    while(code >> hex >> nextword)
    {
        RAM[PC] = nextword;
        //cout << " " << hex << RAM[PC] << endl;
        PC++;
    }
    PC = 0;
    code.close();

    init_terminal();
}
Dcpu::~Dcpu()
{
    delete [] registers;
    delete [] RAM;
}

// Waits for a given number of cpu cycles defined by CLOCK_SPEED
void Dcpu::wait(int cycles, timeval &start)
{
    int endTime;
    int clk_u = 1000000 / CLOCK_SPEED; // clock speed in microseconds

    endTime = start.tv_usec + cycles * clk_u;

    timeval end;
    gettimeofday(&end, NULL);
    while(end.tv_usec < endTime) { gettimeofday(&end, NULL); }
}

// returns a pointer to a value determined by a
unsigned short * Dcpu::GetValuePtr(int a)
{
    if(a <= REGISTER_HIGH)           // register adress
        return &registers[a];

    if(a <= REGISTER_RAM_HIGH)           // RAM adress of a regitser
    {
        int offs = registers[a - 8];
        return &RAM[offs];
    }

    if(a <= NEXT_WORD_REG_HIGH)           // RAM adress with an offest of register + [++PC]
    {
        int offs = RAM[PC++] + registers[a - 16];
        return &RAM[offs];
    }

    // stack pointer values
    if(a == _POP)
        return &RAM[SP++];  // POP
    if(a == _PEEK)
        return &RAM[SP];    // PEEK
    if(a == _PUSH)
        return &RAM[--SP];  // PUSH

    // stack pointer
    if(a == _SP)
        return &SP;
    if(a == _PC)
        return &PC;
    if(a == _O)
        return &O;
    if(a == NEXT_WORD)
        return &RAM[RAM[PC++]];  // return value at the address in the next word
    if(a == NEXT_WORD_LITERAL)
        return &RAM[PC++];

    return &literals[a - LITERAL_VALUE_LOW];
}

void Dcpu::NonBasic(unsigned short o, unsigned short _a)
{
    // aaaa aaoo oooo 0000
    switch(o)
    {
/*JSR*/ case 0x01:
            //cout << " jsr\n";
            unsigned short *a = GetValuePtr(_a);
            cycles_to_wait = 2;
            RAM[--SP] = PC;
            PC = *a;
            break;
    }
}
void Dcpu::SET(unsigned short _a, unsigned short _b)
{
    //cout << " set\n";
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    if(a >= &RAM[SCREEN_BUFFER] || a < &RAM[SCREEN_BUFFER + (TERMINAL_WIDTH * TERMINAL_HEIGHT)])
        dirtyScreen = true;
    if(a == &PC && *b == old_PC)       // exit condition, eg, :halt    SET PC, halt
    {
        kill();
        cout << "killing...\n";
    }   

    cycles_to_wait = 1;
    *a = *b;
}
void Dcpu::ADD(unsigned short _a, unsigned short _b)
{
    //cout << PC << " add\n";
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);

    cycles_to_wait = 2;
    *a = *a + *b;

    if(*a < *b)
        O = 0x0001;
    else
        O = 0x0;
}
void Dcpu::SUB(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << " sub\n";
    cycles_to_wait = 2;
    *a = *a - *b;

    if(*a > *b)
        O = 0xffff;
    else
        O = 0x0;
}
void Dcpu::MUL(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "mul\n";
    cycles_to_wait = 2;
    *a = (*a) * (*b); // that looks so confusing...

    O = (*a >> 16) & 0xffff;
}
void Dcpu::DIV(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "div\n";
    cycles_to_wait = 3;
    O = ((*a << 16) / *b) & 0xffff;
    *a = *a / *b;
}
void Dcpu::MOD(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "mod\n";
    cycles_to_wait = 3;
    if(*b == 0)
        *a = 0;
    else
        *a = *a % *b;
}
void Dcpu::SHL(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << " shl\n";
    cycles_to_wait = 2;
    *a = *a << *b;

    O = (*a >> 16) & 0xffff;
}
void Dcpu::SHR(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "shr\n";
    cycles_to_wait = 2;
    *a = *a >> *b;

    O = (*a << 16) & 0xffff;
}
void Dcpu::AND(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "and\n";
    cycles_to_wait = 1;
    *a = *a & *b;
}
void Dcpu::BOR(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "bor\n";
    cycles_to_wait = 1;
    *a = *a | *b;
}
void Dcpu::XOR(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "xor\n";
    cycles_to_wait = 1;
    *a = *a ^ *b;
}
void Dcpu::IFE(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "ife\n";
    cycles_to_wait = 3;
    if(*a != *b)
        skip_next_ins = true;
    else
        cycles_to_wait = 2;
}
void Dcpu::IFN(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "ifn\n";
    cycles_to_wait = 3;
    if(*a == *b)
        skip_next_ins = true;
    else
        cycles_to_wait = 2;
}
void Dcpu::IFG(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "ifg\n";
    cycles_to_wait = 3;
    if(*a < *b)
        skip_next_ins = true;
    else
        cycles_to_wait = 2;
}
void Dcpu::IFB(unsigned short _a, unsigned short _b)
{
    unsigned short *a = GetValuePtr(_a);
    unsigned short *b = GetValuePtr(_b);
    //cout << "ifb\n";
    cycles_to_wait = 3;
    if((*a & *b) == 0)
        skip_next_ins = true;
    else
        cycles_to_wait = 2;
}
void Dcpu::kill()
{
	dont_kill = false;
}
void Dcpu::MemoryDump()
{
	ofstream outFile("mem.dump");
    if(outFile.good())
    {
        outFile << "**** Memory Dump ****\n\n";
        
        // dump registers

        char r_names[] = {'A', 'B', 'C', 'X', 'Y', 'Z', 'I', 'J'};

        outFile << setfill('0');
        for(int r = 0; r < NUM_REG; r++)
            outFile << r_names[r] <<": " << setw(4) << hex << registers[r] << " ";
        outFile << "\nPC: " << setw(4) << PC << " SP: " << setw(4) << SP << endl;

        // dump the memory if the set of 8 has atleast one number thats non 0
        for(int addr = 0; addr < RAM_SIZE; addr += 8)
        {
            if(OctetNonZero(addr))
            {
                outFile << setw(4) << hex << addr << ": ";
                for(int i = addr; i < addr + 8; i++)
                    outFile << setw(4) << hex << RAM[i] << " ";

                outFile << endl;
            }
        }
    }

}

bool Dcpu::OctetNonZero(int index)
{
    for(int i = index; i < index + 8; i++)
    {
        if(RAM[i] != 0)
            return true;
    }
    return false;
}

void Dcpu::init_terminal()
{
    cout << "\033[2J";  // clear the screen
    for(int h = 0; h < TERMINAL_HEIGHT; h++)
    {
        for(int w = 0; w < TERMINAL_WIDTH; w++)
            cout << " ";
        cout << endl;
    }

    //initialize the color table
    /*
    ColorTable = new short[NUM_COLORS];
    // 512 colors total
    // 3 bits for each r, g, b
    for(int r = 0; r < 8; r++)
        for(int g = 0; g < 8; g++)
            for(int b = 0; b < 8; b++)
                ColorTable[(r * 8 * 8) + (g * 8) + b] = (r << 6 | g << 2 | b);
    */
}

void Dcpu::UpdateScreen()
{
    cout << "\033[" << 0 << ";" << 0 << "H"; // reset cursor to 0, 0                     
    
    for(int h = 0; h < TERMINAL_HEIGHT; h++)
    {
        for(int w = 0; w < TERMINAL_WIDTH; w++)
        {
            unsigned short word = RAM[SCREEN_BUFFER + (h * TERMINAL_WIDTH) + w];
            //unsigned short color = word >> 7; // top 9 bits are the color
            char c = (char) word;
            if(c == 0)
                cout << " ";
            else if(c == '\n')          // outputing \n will cause weird things to happen, so we don't do it
                for(int i = w; i < TERMINAL_WIDTH; i++)
                {
                    cout << " ";
                    break;
                }
            else
                cout << c;
        }
        cout << '\n';
    }
    dirtyScreen = false;
}

unsigned short *Dcpu::GetScreenBuffer()
{
	return &RAM[SCREEN_BUFFER];
}

void Dcpu::PushInBuff(char c)
{
    // will not push to buffer if there is alread yachar in the buffer
    if(RAM[INPUT_BUFFER] == 0)
        RAM[INPUT_BUFFER] = c;
}

void Dcpu::run()
{
    timeval start;
    while(dont_kill)
    {
        gettimeofday(&start, NULL);
        old_PC = PC;
        unsigned short ins;
        unsigned short a;
        unsigned short b;
        if(skip_next_ins)
        {
            // look at next instruction but do not exectute it
            ins = RAM[PC++];
            a = (ins & 0x03f0) >> 4;
            b = (ins & 0xfc00) >> 10;

            if((ins & 0x000f) == 0)
                GetValuePtr(b);
            else
            {
                if(!(a == _POP || a == _PEEK || a == _PUSH))
                    GetValuePtr(a);
                if(!(b == _POP || b == _PEEK || b == _PUSH))
                    GetValuePtr(b);
            }

            skip_next_ins = false;
            continue;
        }
        //if(PC != 26)
          //  cout << PC;
        //cout << hex << PC;

        // bbbb bbaa aaaa oooo
        ins = RAM[PC++];

        a = (ins & 0x03f0) >> 4;

        b = (ins & 0xfc00) >> 10;
   
        //cout << "a=" << hex << *a << " b=" << hex << *b << endl;
        int opcode = ins & 0x000f;
        //if(opcode == 0)
            //cout << a << " " << b << endl;
        switch(opcode)
        {
            case NOB_: NonBasic(a, b); break;
            case SET_: SET(a, b); break;
            case ADD_: ADD(a, b); break;
            case SUB_: SUB(a, b); break;
            case MUL_: MUL(a, b); break;
            case DIV_: DIV(a, b); break;
            case MOD_: MOD(a, b); break;
            case SHL_: SHL(a, b); break;
            case SHR_: SHR(a, b); break;
            case AND_: AND(a, b); break;
            case BOR_: BOR(a, b); break;
            case XOR_: XOR(a, b); break;
            case IFE_: IFE(a, b); break;
            case IFN_: IFN(a, b); break;
            case IFG_: IFG(a, b); break;
            case IFB_: IFB(a, b); break;
        }
        //cout << PC;
        if(dirtyScreen)
            //UpdateScreen();
        if(!wait_cycles)
            wait(cycles_to_wait, start);
    }
    cout << "done\n";
    if(DEBUG)
        MemoryDump();
}
