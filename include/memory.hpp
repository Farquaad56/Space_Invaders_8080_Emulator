#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <vector>

class Memory {
public:
    static constexpr uint32_t MEM_SIZE = 65536;

    Memory() : data(MEM_SIZE, 0) {}

    uint8_t read(uint16_t address) const {
        // BUG #4 FIX: Space Invaders mappe 0x4000-0xFFFF sur 0x0000-0x3FFF (mirroir complet)
        uint16_t mirroredAddr = (address >= 0x4000) ? (address & 0x3FFF) : address;
        return data[mirroredAddr];
    }

    void write(uint16_t address, uint8_t value) {
        // BUG #4 FIX: Space Invaders mappe 0x4000-0xFFFF sur 0x0000-0x3FFF (mirroir complet)
        uint16_t mirroredAddr = (address >= 0x4000) ? (address & 0x3FFF) : address;
        data[mirroredAddr] = value;
    }

    // For Space Invaders specific logic if needed later
    void writeRange(uint16_t start, uint16_t end, const std::vector<uint8_t>& values) {
        for (size_t i = 0; i < values.size() && (start + i) < MEM_SIZE; ++i) {
            data[start + i] = values[i];
        }
    }

private:
    std::vector<uint8_t> data;
};

#endif // MEMORY_HPP