#ifndef CPU8080_HPP
#define CPU8080_HPP

#include <cstdint>
#include <array>
#include <functional>
#include <map>
#include "flags.hpp"
#include "alu.hpp"
#include "memory.hpp"

class CPU8080 {
public:
    struct Registers {
        uint8_t a, b, c, d, e, h, l;
    };

    CPU8080();

    void reset();
    void step();
    
    // Getters for debugging/UI
    uint16_t getPC() const { return pc; }
    uint16_t getSP() const { return sp; }
    uint16_t getHL() const { return (regs.h << 8) | regs.l; }
    const Registers& getRegisters() const { return regs; }
    bool isHalted() const { return halted; }

    // Flag accessors for GUI/debugging
    bool getCY() const { return flags.isCarry(); }
    bool getP() const { return flags.isParity(); }
    bool getAC() const { return flags.isAuxiliaryCarry(); }
    bool getZ() const { return flags.isZero(); }
    bool getS() const { return flags.isSign(); }

    // Memory access via CPU
    uint8_t readMem(uint16_t addr) { return memory.read(addr); }
    void writeMem(uint16_t addr, uint8_t val) { memory.write(addr, val); }

    // I/O handling
    void setIOHandler(uint8_t port, std::function<uint8_t(uint8_t)> reader, std::function<void(uint8_t)> writer);
    uint8_t ioTransfer(uint8_t port, uint8_t value, bool isOut);

private:
    Registers regs;
    
    // I/O handler map — membre d'instance (plus de static global)
    std::map<uint8_t, std::pair<std::function<uint8_t(uint8_t)>, std::function<void(uint8_t)>>> ioHandlers_;

    uint16_t pc;
    uint16_t sp;
    uint8_t flagsByte;
    Flags flags;
    Memory memory;

    bool halted;
    bool inte; // Interrupt enable
    bool interruptPending;
    int intLatch; // -1 if none, or vector index

    uint32_t totalCycles;
    uint32_t currentInstructionTStates;

    // Instruction dispatch
    using InstructionFunc = std::function<uint8_t(CPU8080&)>;
    std::array<InstructionFunc, 256> instructions;

    void initInstructions();
    void handleInterrupts();
    
    // Helper functions for instruction implementation
    uint8_t fetch8();
    uint16_t fetch16();
    uint8_t getReg(uint8_t r);
    void setReg(uint8_t r, uint8_t val);
    uint8_t readM() { return memory.read((regs.h << 8) | regs.l); }
    void writeM(uint8_t v) { memory.write((regs.h << 8) | regs.l, v); }
    void push16(uint16_t val);
    uint16_t pop16();

    // Test accessors - allow tests to access private members
    uint8_t& getFlagsRef() { return flagsByte; }
    uint16_t& getPCRef() { return pc; }
    uint16_t& getSPRef() { return sp; }

public:
    // Trigger interrupt from outside (VBLANK)
    void triggerInterrupt(int vector);  // vector = 0-7 for RST n*8
    
    // Accessors needed by Emulator
    uint32_t& getTotalCyclesRef() { return totalCycles; }
    Memory& getMemoryRef() { return memory; }
};

#endif // CPU8080_HPP