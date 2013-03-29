#ifndef CPU_H
#define CPU_H

#include "memory.h"
#include "pic.h"

namespace vm
{
    struct Registers
    {
        int first_reg;
        int second_reg;
        int third_reg;

        unsigned int ip;
        

        Registers();
    };

    class CPU
    {
    public:
        static const int MOVA_BASE_OPCODE = 0x10;
        static const int MOVB_BASE_OPCODE = 0x10 + 1;
        static const int MOVC_BASE_OPCODE = 0x10 + 2;

        static const int JMP_BASE_OPCODE = 0x20;

        static const int INT_BASE_OPCODE = 0x30;

        Registers registers;

        CPU(Memory &memory, PIC &pic);
        virtual ~CPU();

        void Step();

    private:
        Memory &_memory;
        PIC &_pic;
    };
}

#endif
