#include <iostream>
#include "cpu8080.hpp"

int main() {
    std::cout << "--- Intel 8080 Emulator Demo ---" << std::endl;

    CPU8080 cpu;
    cpu.reset();

    // Test: Load a simple program into memory
    // Opcode 0x3E is MVI A, imm (load byte into A)
    // Opcode 0x00 is NOP
    // Opcode 0x76 is HLT
    cpu.writeMem(0x0000, 0x3E); // MVI A opcode
    cpu.writeMem(0x0001, 0x05); // Value 5 into A
    cpu.writeMem(0x0002, 0x00); // NOP
    cpu.writeMem(0x0003, 0x76); // HLT

    std::cout << "Starting execution..." << std::endl;

    int max_steps = 100;
    int steps = 0;

    while (!cpu.isHalted() && steps < max_steps) {
        cpu.step();
        steps++;
    }

    std::cout << "Execution finished after " << steps << " steps." << std::endl;
    std::cout << "Final PC: 0x" << std::hex << cpu.getPC() << std::dec << std::endl;
    std::cout << "Final A: 0x" << std::hex << (int)cpu.getRegisters().a << std::dec << std::endl;

    if (cpu.isHalted()) {
        std::cout << "CPU Halted correctly." << std::endl;
    } else {
        std::cout << "CPU reached max steps without halting." << std::endl;
    }

    return 0;
}