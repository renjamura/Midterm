#include "cpu.h"

#include <iostream>

namespace vm
{
    Registers::Registers()
        : first_reg(0), second_reg(0), third_reg(0), ip(0) {}

    CPU::CPU(Memory &memory, PIC &pic): registers(), _memory(memory), _pic(pic) {}

    CPU::~CPU() {}

    void CPU::Step()
    {
        int ip = registers.ip;

        int instruction = _memory.ram[ip];
        int data = _memory.ram[ip + 1];

        if (instruction == CPU::MOVA_BASE_OPCODE) {
            registers.first_reg = data;
            registers.ip += 2;
        } else if (instruction == CPU::MOVB_BASE_OPCODE) {
            registers.second_reg = data;
            registers.ip += 2;
        } else if (instruction == CPU::MOVC_BASE_OPCODE) {
            registers.third_reg = data;
            registers.ip += 2;
        } else if (instruction == CPU::JMP_BASE_OPCODE) {
            registers.ip += data;
        } else if (instruction == CPU::INT_BASE_OPCODE) {
            _pic.isr_3();
        } else {
            std::cerr << "CPU: invalid opcode data. Skipping..." << std::endl;
            registers.ip += 2;
        }
    }
}
