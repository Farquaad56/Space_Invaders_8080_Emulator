#include "emulator.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

Emulator::Emulator() : paused_(false), running_(true), emulation_speed_(1.0f), lastFrameStart_(std::chrono::steady_clock::now()) {
    cpu_.reset();
}

Emulator::~Emulator() {
    // Nettoyage automatique
}

bool Emulator::loadROMs(
    const std::string& pathH,
    const std::string& pathG,
    const std::string& pathF,
    const std::string& pathE
) {
    auto loadROM = [this](const std::string& path, uint16_t address) -> bool {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Erreur: Impossible d'ouvrir " << path << std::endl;
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            std::cerr << "Erreur: Impossible de lire " << path << std::endl;
            return false;
        }

        file.close();

        // Charger dans la mémoire CPU (ROM à l'adresse spécifiée)
        for (size_t i = 0; i < buffer.size(); ++i) {
            if (address + static_cast<uint16_t>(i) < 0x10000) {
                cpu_.getMemoryRef().write(address + static_cast<uint16_t>(i), buffer[i]);
            }
        }

        std::cout << "ROM chargee: " << path << " (taille=" << size << ", addr=0x" 
                  << std::hex << address << std::dec << ")" << std::endl;
        return true;
    };

    bool success = true;
    success &= loadROM(pathH, 0x0000);   // ROM H → adresse 0x0000
    success &= loadROM(pathG, 0x0800);   // ROM G → adresse 0x0800
    success &= loadROM(pathF, 0x1000);   // ROM F → adresse 0x1000
    success &= loadROM(pathE, 0x1800);   // ROM E → adresse 0x1800

    return success;
}

void Emulator::initialize() {
    setupIOHandlers();
    reset();
    
    // Initialiser le système audio au démarrage (chargement des WAV + logs)
    InputSystem::initAudioEarly();
    
    std::cout << "Emulateur Space Invaders initialise (CPU=1.9968 MHz)" << std::endl;
}

void Emulator::reset() {
    cpu_.reset();
    // PC initialisé à 0x0000 (début ROM H)
    // SP initialisé à 0x23FF (Space Invaders standard)
    running_ = true;
    paused_ = false;
    
    // Reset valeurs ports I/O pour edge detection audio
    oldPort3_ = 0x00;
    oldPort5_ = 0x00;
    
    // Reset état audio (arrêter boucles UFO)
    input_.resetAudio();
}

void Emulator::setupIOHandlers() {
    // Créer des lambdas capture pour les handlers I/O
    
    // Handler lecture port 1
    auto readPort1Handler = [this](uint8_t) -> uint8_t {
        return this->ioReadPort1(0);
    };
    
    // Handler lecture port 2
    auto readPort2Handler = [this](uint8_t) -> uint8_t {
        return this->ioReadPort2(0);
    };
    
    // Handler écriture port 2 (shift amount)
    auto writePort2Handler = [this](uint8_t data) {
        this->ioWritePort2(data);
    };
    
    // Handler lecture port 3 (résultat du shift register)
    auto readPort3Handler = [this](uint8_t) -> uint8_t {
        return this->shift_register_.readPort3();
    };

    // Handler écriture port 3 (son banque 1)
    auto writePort3Handler = [this](uint8_t data) {
        this->ioWritePort3(data);
        
        // BUG FIX: Déclencher les sons via InputSystem audio callbacks
        input_.setAudioCallback(0x03, [this](uint8_t soundData) {
            // Banque 1 : UFO loop, tir joueur, mort joueur, mort alien
            if (soundData & 0x01) { /* UFO loop */ }
            if (soundData & 0x02) { /* Player shot */ }
            if (soundData & 0x04) { /* Player death */ }
            if (soundData & 0x08) { /* Alien death */ }
        });
    };
    
    // Handler écriture port 4 (shift data)
    auto writePort4Handler = [this](uint8_t data) {
        this->ioWritePort4(data);
    };
    
    // Handler écriture port 5 (son banque 2)
    auto writePort5Handler = [this](uint8_t data) {
        this->ioWritePort5(data);
        
        // BUG FIX: Déclencher les sons via InputSystem audio callbacks
        input_.setAudioCallback(0x05, [this](uint8_t soundData) {
            // Banque 2 : Pas aliens (4 niveaux), UFO hit
            if (soundData & 0x01) { /* Alien step 1 */ }
            if (soundData & 0x02) { /* Alien step 2 */ }
            if (soundData & 0x04) { /* Alien step 3 */ }
            if (soundData & 0x08) { /* Alien step 4 */ }
            if (soundData & 0x10) { /* UFO hit */ }
        });
    };

    // Handler lecture/écriture port 0 (certaines révisions Space Invaders l'utilisent)
    auto readPort0Handler = [this](uint8_t) -> uint8_t { return 0; };
    auto writePort0Handler = [this](uint8_t data) { (void)data; };

    // Handler écriture port 6 — Graphismes/Shifted Data (Space Invaders)
    auto writePort6Handler = [this](uint8_t data) {
        this->ioWritePort6(data);
    };

    // Connecter les handlers au CPU
    cpu_.setIOHandler(0x00, readPort0Handler, writePort0Handler);  // Port 0 (optionnel)
    cpu_.setIOHandler(0x01, readPort1Handler, nullptr);   // Lecture port 1
    cpu_.setIOHandler(0x02, readPort2Handler, writePort2Handler);  // Lecture/écriture port 2
    cpu_.setIOHandler(0x03, readPort3Handler, writePort3Handler);  // Port 3 (shift register + son)
    cpu_.setIOHandler(0x04, nullptr, writePort4Handler);  // Écriture port 4
    cpu_.setIOHandler(0x05, nullptr, writePort5Handler);  // Écriture port 5
    cpu_.setIOHandler(0x06, nullptr, writePort6Handler);  // Écriture port 6 (graphismes/shifted data)
}

uint8_t Emulator::ioReadPort1(uint8_t) {
    return input_.readPort1();
}

uint8_t Emulator::ioReadPort2(uint8_t) {
    return input_.readPort2();
}

void Emulator::ioWritePort2(uint8_t data) {
    // Écrire dans le shift register (offset de décalage)
    shift_register_.writePort2(data);
    input_.writePort2(data);
}

void Emulator::ioWritePort3(uint8_t data) {
    // Son Banque 1 - Space Invaders (Taito, 1978)
    input_.writePort3(data, oldPort3_);
    oldPort3_ = data;
}

void Emulator::ioWritePort4(uint8_t data) {
    // Écrire dans le shift register (données 16 bits)
    shift_register_.writePort4(data);
}

void Emulator::ioWritePort5(uint8_t data) {
    // Son Banque 2 - Space Invaders (Taito, 1978)
    input_.writePort5(data, oldPort5_);
    oldPort5_ = data;
}

void Emulator::ioWritePort6(uint8_t data) {
    // Port 6 — Watchdog Timer (Space Invaders Taito, 1978)
    // La ROM écrit périodiquement pour éviter le reset hardware
    oldPort6_ = data;
}

bool Emulator::runFrame() {
    if (paused_) return true;

    auto frameStart = std::chrono::steady_clock::now();

    // Exécuter jusqu'à ce que les cycles CPU correspondent à une frame (ajusté par vitesse)
    uint32_t targetCycles = static_cast<uint32_t>(TARGET_CYCLES_PER_FRAME * emulation_speed_);
    
    cpu_.getTotalCyclesRef() = 0;
    
    // BUG #3 FIX: stepCount séparé de totalCycles pour éviter boucle infinie sur HALT
    uint64_t stepCount = 0;
    const uint64_t MAX_STEPS = 500000;
    
    while (cpu_.getTotalCyclesRef() < targetCycles && running_ && stepCount < MAX_STEPS) {
        cpu_.step();
        stepCount++;
        
        // Déclencher VBLANK automatiquement
        if (cpu_.getTotalCyclesRef() >= targetCycles / 2 && !vblank1Triggered_) {
            vblank1Triggered_ = true;
            cpu_.triggerInterrupt(1);  // RST 1 à mi-frame (~16 640 cycles)
        }
        if (cpu_.getTotalCyclesRef() >= targetCycles && !vblank2Triggered_) {
            vblank2Triggered_ = true;
            cpu_.triggerInterrupt(2);  // RST 2 en fin de frame (~33 280 cycles)
        }
    }
    
    // BUG #6 FIX: Déclencher RST 2 juste après la boucle si pas encore fait
    if (!vblank2Triggered_ && running_) {
        vblank2Triggered_ = true;
        cpu_.triggerInterrupt(2);
    }
    
    // Reset flags VBLANK pour la prochaine frame
    if (stepCount > 0) {
        vblank1Triggered_ = false;
        vblank2Triggered_ = false;
    }
    
    // Synchroniser VRAM : copier la zone 0x2400-0x3FFF depuis la mémoire CPU vers VideoSystem
    static constexpr uint16_t VRAM_START = 0x2400;
    static constexpr size_t VRAM_SIZE = 0x4000 - VRAM_START;  // 7168 bytes
    
    auto& cpuMem = cpu_.getMemoryRef();
    for (size_t i = 0; i < VRAM_SIZE; ++i) {
        uint16_t addr = static_cast<uint16_t>(VRAM_START + i);
        uint8_t val = cpuMem.read(addr);
        video_.writeVRAM(addr, val);
    }

    // Synchronisation temps réel 60 Hz : throttle pour maintenir ~60 FPS
    auto frameEnd = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - lastFrameStart_).count();
    long long remainingMs = TARGET_FRAME_MS - elapsedMs;
    if (remainingMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(remainingMs));
    }
    lastFrameStart_ = std::chrono::steady_clock::now();

    // Note: UpdateAudioDevice() peut être nécessaire selon la version de Raylib
    // Si identificateur introuvable, commenter cette ligne
    
    return running_;
}

void Emulator::generateFramebuffer(uint8_t* framebuffer, int& width, int& height) {
    video_.generateFramebuffer(framebuffer, width, height);
}

void Emulator::triggerVBlankInterrupt(int vector) {
    if (vector >= 0 && vector <= 7) {
        cpu_.triggerInterrupt(vector);
    }
}