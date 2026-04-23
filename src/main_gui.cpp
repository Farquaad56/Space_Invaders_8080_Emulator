// Standard library headers FIRST - must come before any third-party headers
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

// Raylib (linked via vcpkg raylib.lib, no RAYLIB_IMPLEMENTATION needed)
#include <raylib.h>

// Raylib audio support
#include <raymath.h>

// Local project headers
#include "../include/emulator.hpp"

// rlImGui integration
#include "rlImGui.h"
#include "rlImGuiColors.h"

// ============================================================
// Constantes et structures globales
// ============================================================
static const ImVec4 PANEL_BG = ImVec4(0.05f, 0.05f, 0.05f, 0.95f); // Noir avec transparence
static const ImVec4 CYAN_COLOR = ImVec4(0.0f, 1.0f, 1.0f, 1.0f);   // Cyan #00FFFF
static const ImVec4 WHITE_TEXT = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
static const ImVec4 CYAN_BTN = ImVec4(0.0f, 0.8f, 0.8f, 1.0f);      // Bleu cyan pour boutons
static const ImVec4 GREEN_BTN = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);
static const ImVec4 RED_BTN = ImVec4(0.7f, 0.2f, 0.2f, 1.0f);
static const ImVec4 YELLOW_BTN = ImVec4(0.7f, 0.6f, 0.1f, 1.0f);
static const ImVec4 BLUE_BTN = ImVec4(0.1f, 0.4f, 0.7f, 1.0f);

// Structure pour les paramètres "Important Stuff"
struct GameParams {
    int credits;
    int coinSwitch;
    int gameMode;
    int suspend;
    int port1;
    int port2;
};

// ============================================================
// Colonne 1 — Éditeur Mémoire (ROM/RAM/VRAM selection) (~25%)
// ============================================================
void DrawMemoryEditorPanel(Emulator& emulator, uint16_t& startAddr) {
    float panelWidth = 2560.0f * 0.25f; // 640px
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 1440.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, PANEL_BG);
    
    if (!ImGui::Begin("Éditeur Mémoire")) {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    // Titre en cyan
    ImGui::TextColored(CYAN_COLOR, "=== MÉMOIRE ÉDITEUR ===");
    ImGui::Separator();

    // --- Sélecteur de type de mémoire ---
    ImGui::TextColored(WHITE_TEXT, "--- Sélection ROM / RAM / VRAM ---");
    
    static int memTypeSelection = 0; // 0=ROM, 1=RAM, 2=VRAM
    if (ImGui::RadioButton("ROM (0x0000-0x1FFF)", memTypeSelection == 0)) {
        memTypeSelection = 0;
        startAddr = 0x0000;
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "■");
    
    if (ImGui::RadioButton("RAM (0x2000-0x23FF)", memTypeSelection == 1)) {
        memTypeSelection = 1;
        startAddr = 0x2000;
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.6f, 1.0f), "■");
    
    if (ImGui::RadioButton("VRAM (0x2400-0x3FFF)", memTypeSelection == 2)) {
        memTypeSelection = 2;
        startAddr = 0x2400;
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.9f, 1.0f), "■");

    ImGui::Separator();

    // Entrée de début d'adresse
    char addrBuf[16];
    snprintf(addrBuf, sizeof(addrBuf), "%04X", startAddr);
    if (ImGui::InputText("Adresse de départ", addrBuf, sizeof(addrBuf), ImGuiInputTextFlags_CharsHexadecimal)) {
        startAddr = static_cast<uint16_t>(strtol(addrBuf, nullptr, 16));
    }

    // Sélecteur de plage mémoire
    ImGui::SameLine();
    if (ImGui::Button("Range")) {
        // Toggle range selector (placeholder)
    }
    ImGui::SameLine();
    ImGui::Text("0000..FFFF");

    ImGui::Separator();

    // Afficher 48 lignes de mémoire hexadécimale (16 bytes par ligne, incrément 0x10) - agrandi
    ImGui::BeginChild("HexMemory", ImVec2(0, 700.0f), true);
    
    for (int i = 0; i < 48; ++i) {
        uint16_t addr = startAddr + static_cast<uint16_t>(i * 16);
        
        // Déterminer le type de mémoire pour coloration
        ImVec4 memColor = WHITE_TEXT;
        if (addr >= 0x0000 && addr <= 0x1FFF) {
            memColor = ImVec4(0.6f, 0.9f, 0.6f, 1.0f); // ROM - vert clair
        } else if (addr >= 0x2000 && addr <= 0x23FF) {
            memColor = ImVec4(0.9f, 0.9f, 0.6f, 1.0f); // RAM - jaune clair
        } else if (addr >= 0x2400 && addr <= 0x3FFF) {
            memColor = ImVec4(0.6f, 0.6f, 0.9f, 1.0f); // VRAM - bleu clair
        }

        char hexBytes[256];
        snprintf(hexBytes, sizeof(hexBytes), 
            "%04X: %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X  %02X %02X %02X %02X",
            addr,
            emulator.getCpu().readMem(addr),
            emulator.getCpu().readMem(addr + 1),
            emulator.getCpu().readMem(addr + 2),
            emulator.getCpu().readMem(addr + 3),
            emulator.getCpu().readMem(addr + 4),
            emulator.getCpu().readMem(addr + 5),
            emulator.getCpu().readMem(addr + 6),
            emulator.getCpu().readMem(addr + 7),
            emulator.getCpu().readMem(addr + 8),
            emulator.getCpu().readMem(addr + 9),
            emulator.getCpu().readMem(addr + 10),
            emulator.getCpu().readMem(addr + 11),
            emulator.getCpu().readMem(addr + 12),
            emulator.getCpu().readMem(addr + 13),
            emulator.getCpu().readMem(addr + 14),
            emulator.getCpu().readMem(addr + 15)
        );
        ImGui::TextColored(memColor, "%s", hexBytes);
    }
    ImGui::EndChild();

    // --- Légende mémoire ---
    ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "■ ROM");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.6f, 1.0f), "■ RAM");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.9f, 1.0f), "■ VRAM");

    // --- Options de plage mémoire ---
    ImGui::Separator();
    ImGui::TextColored(WHITE_TEXT, "--- Options ---");
    
    static int rangeStart = 0x0000;
    static int rangeEnd = 0xFFFE;
    
    if (ImGui::SliderInt("Début range", &rangeStart, 0x0000, 0xFFFF, "%04X")) {
        startAddr = static_cast<uint16_t>(rangeStart);
    }
    if (ImGui::SliderInt("Fin range", &rangeEnd, 0x0000, 0xFFFF, "%04X")) {
        // Range update
    }

    ImGui::PopStyleColor();
    ImGui::End();
}

// ============================================================
// Colonne 2 — 20 Dernières Instructions Assembleur Décodées (~20%)
// ============================================================
void DrawInstructionPanel(Emulator& emulator, uint16_t& instrStartAddr) {
    float panelX = 2560.0f * 0.25f; // 640px
    float panelWidth = 2560.0f * 0.20f; // 512px
    
    ImGui::SetNextWindowPos(ImVec2(panelX, 0));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 1440.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, PANEL_BG);

    if (!ImGui::Begin("Instructions Assembleur")) {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    // Titre en cyan
    ImGui::TextColored(CYAN_COLOR, "=== INSTRUCTIONS ===");
    ImGui::Separator();

    // Adresse de départ pour les instructions
    char addrBuf[16];
    snprintf(addrBuf, sizeof(addrBuf), "%04X", instrStartAddr);
    if (ImGui::InputText("Adresse début", addrBuf, sizeof(addrBuf), ImGuiInputTextFlags_CharsHexadecimal)) {
        instrStartAddr = static_cast<uint16_t>(strtol(addrBuf, nullptr, 16));
    }

    ImGui::Separator();
    ImGui::TextColored(WHITE_TEXT, "--- 20 Instructions Décodées ---");

    // Afficher 20 instructions assembleur décodées
    ImGui::BeginChild("AsmInstructions", ImVec2(0, 800.0f), true);
    
    for (int i = 0; i < 20; ++i) {
        uint16_t addr = instrStartAddr + static_cast<uint16_t>(i * 2); // Instructions consécutives (1 opcode + possible operand)
        uint8_t opcode = emulator.getCpu().readMem(addr);
        
        // Décodage complet des instructions 78 instructions 8080
        std::string mnemonic;
        char asmLine[64];
        
        switch (opcode) {
            // Instructions sur les Flags Carry
            case 0x37: mnemonic = "STC"; break;
            case 0x3F: mnemonic = "CMC"; break;

            // NOP et HLT
            case 0x00: mnemonic = "NOP"; break;
            case 0x76: mnemonic = "HLT"; break;

            // Rotations Accumulateur
            case 0x07: mnemonic = "RLC"; break;
            case 0x0F: mnemonic = "RRC"; break;
            case 0x17: mnemonic = "RAL"; break;
            case 0x1F: mnemonic = "RAR"; break;

            // CMA et DAA
            case 0x27: mnemonic = "DAA"; break;
            case 0x2F: mnemonic = "CMA"; break;

            // Instructions sur paires de registres - DAD
            case 0x09: mnemonic = "DAD B"; break;
            case 0x19: mnemonic = "DAD D"; break;
            case 0x29: mnemonic = "DAD H"; break;
            case 0x39: mnemonic = "DAD SP"; break;

            // Instructions sur paires de registres - INX/DCX
            case 0x03: mnemonic = "INX B"; break;
            case 0x13: mnemonic = "INX D"; break;
            case 0x23: mnemonic = "INX H"; break;
            case 0x33: mnemonic = "INX SP"; break;
            case 0x0B: mnemonic = "DCX B"; break;
            case 0x1B: mnemonic = "DCX D"; break;
            case 0x2B: mnemonic = "DCX H"; break;
            case 0x3B: mnemonic = "DCX SP"; break;

            // PUSH / POP
            case 0xC1: mnemonic = "POP B"; break;
            case 0xD1: mnemonic = "POP D"; break;
            case 0xE1: mnemonic = "POP H"; break;
            case 0xF1: mnemonic = "POP PSW"; break;
            case 0xC5: mnemonic = "PUSH B"; break;
            case 0xD5: mnemonic = "PUSH D"; break;
            case 0xE5: mnemonic = "PUSH H"; break;
            case 0xF5: mnemonic = "PUSH PSW"; break;

            // XCHG, XTHL, SPHL
            case 0xEB: mnemonic = "XCHG"; break;
            case 0xE3: mnemonic = "XTHL"; break;
            case 0xF9: mnemonic = "SPHL"; break;

            // PCHL
            case 0xE9: mnemonic = "PCHL"; break;

            // LXI (Load Immediate 16 bits)
            case 0x01: mnemonic = "LXI B"; break;
            case 0x11: mnemonic = "LXI D"; break;
            case 0x21: mnemonic = "LXI H"; break;
            case 0x31: mnemonic = "LXI SP"; break;

            // MVI (Move Immediate 8 bits)
            case 0x06: mnemonic = "MVI B"; break;
            case 0x0E: mnemonic = "MVI C"; break;
            case 0x16: mnemonic = "MVI D"; break;
            case 0x1E: mnemonic = "MVI E"; break;
            case 0x26: mnemonic = "MVI H"; break;
            case 0x2E: mnemonic = "MVI L"; break;
            case 0x36: mnemonic = "MVI M"; break;
            case 0x3E: mnemonic = "MVI A"; break;

            // INR / DCR (Register)
            case 0x04: mnemonic = "INR B"; break;
            case 0x0C: mnemonic = "INR C"; break;
            case 0x14: mnemonic = "INR D"; break;
            case 0x1C: mnemonic = "INR E"; break;
            case 0x24: mnemonic = "INR H"; break;
            case 0x2C: mnemonic = "INR L"; break;
            case 0x34: mnemonic = "INR M"; break;
            case 0x3C: mnemonic = "INR A"; break;
            case 0x05: mnemonic = "DCR B"; break;
            case 0x0D: mnemonic = "DCR C"; break;
            case 0x15: mnemonic = "DCR D"; break;
            case 0x1D: mnemonic = "DCR E"; break;
            case 0x25: mnemonic = "DCR H"; break;
            case 0x2D: mnemonic = "DCR L"; break;
            case 0x35: mnemonic = "DCR M"; break;
            case 0x3D: mnemonic = "DCR A"; break;

            // Opérations Registre/Mémoire → Accumulateur (ADD, ADC, SUB, SBB, ANA, XRA, ORA, CMP)
            case 0x80: mnemonic = "ADD B"; break;
            case 0x81: mnemonic = "ADD C"; break;
            case 0x82: mnemonic = "ADD D"; break;
            case 0x83: mnemonic = "ADD E"; break;
            case 0x84: mnemonic = "ADD H"; break;
            case 0x85: mnemonic = "ADD L"; break;
            case 0x86: mnemonic = "ADD M"; break;
            case 0x87: mnemonic = "ADD A"; break;

            case 0x88: mnemonic = "ADC B"; break;
            case 0x89: mnemonic = "ADC C"; break;
            case 0x8A: mnemonic = "ADC D"; break;
            case 0x8B: mnemonic = "ADC E"; break;
            case 0x8C: mnemonic = "ADC H"; break;
            case 0x8D: mnemonic = "ADC L"; break;
            case 0x8E: mnemonic = "ADC M"; break;
            case 0x8F: mnemonic = "ADC A"; break;

            case 0x90: mnemonic = "SUB B"; break;
            case 0x91: mnemonic = "SUB C"; break;
            case 0x92: mnemonic = "SUB D"; break;
            case 0x93: mnemonic = "SUB E"; break;
            case 0x94: mnemonic = "SUB H"; break;
            case 0x95: mnemonic = "SUB L"; break;
            case 0x96: mnemonic = "SUB M"; break;
            case 0x97: mnemonic = "SUB A"; break;

            case 0x98: mnemonic = "SBB B"; break;
            case 0x99: mnemonic = "SBB C"; break;
            case 0x9A: mnemonic = "SBB D"; break;
            case 0x9B: mnemonic = "SBB E"; break;
            case 0x9C: mnemonic = "SBB H"; break;
            case 0x9D: mnemonic = "SBB L"; break;
            case 0x9E: mnemonic = "SBB M"; break;
            case 0x9F: mnemonic = "SBB A"; break;

            case 0xA0: mnemonic = "ANA B"; break;
            case 0xA1: mnemonic = "ANA C"; break;
            case 0xA2: mnemonic = "ANA D"; break;
            case 0xA3: mnemonic = "ANA E"; break;
            case 0xA4: mnemonic = "ANA H"; break;
            case 0xA5: mnemonic = "ANA L"; break;
            case 0xA6: mnemonic = "ANA M"; break;
            case 0xA7: mnemonic = "ANA A"; break;

            case 0xA8: mnemonic = "XRA B"; break;
            case 0xA9: mnemonic = "XRA C"; break;
            case 0xAA: mnemonic = "XRA D"; break;
            case 0xAB: mnemonic = "XRA E"; break;
            case 0xAC: mnemonic = "XRA H"; break;
            case 0xAD: mnemonic = "XRA L"; break;
            case 0xAE: mnemonic = "XRA M"; break;
            case 0xAF: mnemonic = "XRA A"; break;

            case 0xB0: mnemonic = "ORA B"; break;
            case 0xB1: mnemonic = "ORA C"; break;
            case 0xB2: mnemonic = "ORA D"; break;
            case 0xB3: mnemonic = "ORA E"; break;
            case 0xB4: mnemonic = "ORA H"; break;
            case 0xB5: mnemonic = "ORA L"; break;
            case 0xB6: mnemonic = "ORA M"; break;
            case 0xB7: mnemonic = "ORA A"; break;

            case 0xB8: mnemonic = "CMP B"; break;
            case 0xB9: mnemonic = "CMP C"; break;
            case 0xBA: mnemonic = "CMP D"; break;
            case 0xBB: mnemonic = "CMP E"; break;
            case 0xBC: mnemonic = "CMP H"; break;
            case 0xBD: mnemonic = "CMP L"; break;
            case 0xBE: mnemonic = "CMP M"; break;
            case 0xBF: mnemonic = "CMP A"; break;

            // Instructions immédiates (ADI, ACI, SUI, SBI, ANI, XRI, ORI, CPI)
            case 0xC6: mnemonic = "ADI"; break;
            case 0xCE: mnemonic = "ACI"; break;
            case 0xD6: mnemonic = "SUI"; break;
            case 0xDE: mnemonic = "SBI"; break;
            case 0xE6: mnemonic = "ANI"; break;
            case 0xEE: mnemonic = "XRI"; break;
            case 0xF6: mnemonic = "ORI"; break;
            case 0xFE: mnemonic = "CPI"; break;

            // Adressage direct (STA, LDA, SHLD, LHLD)
            case 0x32: mnemonic = "STA"; break;
            case 0x3A: mnemonic = "LDA"; break;
            case 0x22: mnemonic = "SHLD"; break;
            case 0x2A: mnemonic = "LHLD"; break;

            // Sauts (JUMP)
            case 0xC3: mnemonic = "JMP"; break;
            case 0xDA: mnemonic = "JC"; break;
            case 0xD2: mnemonic = "JNC"; break;
            case 0xCA: mnemonic = "JZ"; break;
            case 0xC2: mnemonic = "JNZ"; break;
            case 0xFA: mnemonic = "JM"; break;
            case 0xF2: mnemonic = "JP"; break;
            case 0xEA: mnemonic = "JPE"; break;
            case 0xE2: mnemonic = "JPO"; break;

            // Appels de sous-routine (CALL)
            case 0xCD: mnemonic = "CALL"; break;
            case 0xDC: mnemonic = "CC"; break;   // CY=1
            case 0xD4: mnemonic = "CNC"; break;  // CY=0
            case 0xCC: mnemonic = "CZ"; break;   // Z=1
            case 0xC4: mnemonic = "CNZ"; break;  // Z=0
            case 0xFC: mnemonic = "CM"; break;   // S=1
            case 0xF4: mnemonic = "CP"; break;   // S=0
            case 0xEC: mnemonic = "CPE"; break;  // P=1
            case 0xE4: mnemonic = "CPO"; break;  // P=0

            // Retours de sous-routine (RETURN)
            case 0xC9: mnemonic = "RET"; break;
            case 0xD8: mnemonic = "RC"; break;   // CY=1
            case 0xD0: mnemonic = "RNC"; break;  // CY=0
            case 0xC8: mnemonic = "RZ"; break;   // Z=1
            case 0xC0: mnemonic = "RNZ"; break;  // Z=0
            case 0xF8: mnemonic = "RM"; break;   // S=1
            case 0xF0: mnemonic = "RP"; break;   // S=0
            case 0xE8: mnemonic = "RPE"; break;  // P=1
            case 0xE0: mnemonic = "RPO"; break;  // P=0

            // RST — Restart (interruption logicielle)
            case 0xC7: mnemonic = "RST 0"; break;
            case 0xCF: mnemonic = "RST 1"; break;
            case 0xD7: mnemonic = "RST 2"; break;
            case 0xDF: mnemonic = "RST 3"; break;
            case 0xE7: mnemonic = "RST 4"; break;
            case 0xEF: mnemonic = "RST 5"; break;
            case 0xF7: mnemonic = "RST 6"; break;
            case 0xFF: mnemonic = "RST 7"; break;

            // Contrôle des interruptions
            case 0xFB: mnemonic = "EI"; break;
            case 0xF3: mnemonic = "DI"; break;

            // Entrées/Sorties
            case 0xDB: mnemonic = "IN"; break;
            case 0xD3: mnemonic = "OUT"; break;

            // Transferts de données (MOV)
            case 0x47: mnemonic = "MOV A,A"; break;
            case 0x40: mnemonic = "MOV B,B"; break;
            case 0x41: mnemonic = "MOV B,C"; break;
            case 0x42: mnemonic = "MOV B,D"; break;
            case 0x43: mnemonic = "MOV B,E"; break;
            case 0x44: mnemonic = "MOV B,H"; break;
            case 0x45: mnemonic = "MOV B,L"; break;
            case 0x46: mnemonic = "MOV B,M"; break;
            case 0x48: mnemonic = "MOV C,B"; break;
            case 0x49: mnemonic = "MOV C,C"; break;
            case 0x4A: mnemonic = "MOV C,D"; break;
            case 0x4B: mnemonic = "MOV C,E"; break;
            case 0x4C: mnemonic = "MOV C,H"; break;
            case 0x4D: mnemonic = "MOV C,L"; break;
            case 0x4E: mnemonic = "MOV C,M"; break;
            case 0x4F: mnemonic = "MOV C,A"; break;
            case 0x50: mnemonic = "MOV D,B"; break;
            case 0x51: mnemonic = "MOV D,C"; break;
            case 0x52: mnemonic = "MOV D,D"; break;
            case 0x53: mnemonic = "MOV D,E"; break;
            case 0x54: mnemonic = "MOV D,H"; break;
            case 0x55: mnemonic = "MOV D,L"; break;
            case 0x56: mnemonic = "MOV D,M"; break;
            case 0x57: mnemonic = "MOV D,A"; break;
            case 0x58: mnemonic = "MOV E,B"; break;
            case 0x59: mnemonic = "MOV E,C"; break;
            case 0x5A: mnemonic = "MOV E,D"; break;
            case 0x5B: mnemonic = "MOV E,E"; break;
            case 0x5C: mnemonic = "MOV E,H"; break;
            case 0x5D: mnemonic = "MOV E,L"; break;
            case 0x5E: mnemonic = "MOV E,M"; break;
            case 0x5F: mnemonic = "MOV E,A"; break;
            case 0x60: mnemonic = "MOV H,B"; break;
            case 0x61: mnemonic = "MOV H,C"; break;
            case 0x62: mnemonic = "MOV H,D"; break;
            case 0x63: mnemonic = "MOV H,E"; break;
            case 0x64: mnemonic = "MOV H,H"; break;
            case 0x65: mnemonic = "MOV H,L"; break;
            case 0x66: mnemonic = "MOV H,M"; break;
            case 0x67: mnemonic = "MOV H,A"; break;
            case 0x68: mnemonic = "MOV L,B"; break;
            case 0x69: mnemonic = "MOV L,C"; break;
            case 0x6A: mnemonic = "MOV L,D"; break;
            case 0x6B: mnemonic = "MOV L,E"; break;
            case 0x6C: mnemonic = "MOV L,H"; break;
            case 0x6D: mnemonic = "MOV L,L"; break;
            case 0x6E: mnemonic = "MOV L,M"; break;
            case 0x6F: mnemonic = "MOV L,A"; break;
            case 0x78: mnemonic = "MOV A,B"; break;
            case 0x79: mnemonic = "MOV A,C"; break;
            case 0x7A: mnemonic = "MOV A,D"; break;
            case 0x7B: mnemonic = "MOV A,E"; break;
            case 0x7C: mnemonic = "MOV A,H"; break;
            case 0x7D: mnemonic = "MOV A,L"; break;
            case 0x7E: mnemonic = "MOV A,M"; break;
            case 0x7F: mnemonic = "MOV A,A"; break;

            // STAX / LDAX
            case 0x02: mnemonic = "STAX B"; break;
            case 0x12: mnemonic = "STAX D"; break;
            case 0x0A: mnemonic = "LDAX B"; break;
            case 0x1A: mnemonic = "LDAX D"; break;

            default: 
                snprintf(asmLine, sizeof(asmLine), "%04X: DB 0x%02X", addr, opcode);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", asmLine);
                continue;
        }
        
        snprintf(asmLine, sizeof(asmLine), "%04X: %-12s ; 0x%02X", addr, mnemonic.c_str(), opcode);
        ImGui::Text("%s", asmLine);
    }
    ImGui::EndChild();

    // --- Légende des instructions ---
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.6f, 1.0f), "78 instructions 8080 supportées");
    ImGui::TextColored(WHITE_TEXT, "Format: Adresse: MNEMONIQUE ; opcode");

    ImGui::PopStyleColor();
    ImGui::End();
}

// ============================================================
// Colonne 3 — État CPU & Contrôles (~20%)
// ============================================================
void DrawCPUStatePanel(Emulator& emulator, GameParams& gameParams) {
    float panelX = 2560.0f * 0.45f; // 1152px
    float panelWidth = 2560.0f * 0.20f; // 512px
    
    ImGui::SetNextWindowPos(ImVec2(panelX, 0));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 1440.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, PANEL_BG);

    if (!ImGui::Begin("État CPU & Contrôles")) {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    auto& cpu = emulator.getCpu();
    auto regs = cpu.getRegisters();

    // --- État du Processeur ---
    ImGui::TextColored(CYAN_COLOR, "=== ÉTAT CPU ===");
    ImGui::Separator();

    uint8_t opcode = cpu.readMem(cpu.getPC());
    
    ImGui::Text("Instruction: %02X", opcode);

    // Décodage instruction courante
    std::string currentInstr;
    switch (opcode) {
        case 0xA7: currentInstr = "ANA A"; break;
        case 0x87: currentInstr = "ADD A"; break;
        case 0xBF: currentInstr = "CMP A"; break;
        case 0xAF: currentInstr = "XRA A"; break;
        default: currentInstr = "???"; break;
    }
    ImGui::TextColored(CYAN_COLOR, "%s", currentInstr.c_str());

    ImGui::Separator();
    ImGui::TextColored(WHITE_TEXT, "--- Registres 8 bits ---");

    // Grille de registres en 2 colonnes
    ImGui::Columns(2);
    
    char regBuf[16];
    snprintf(regBuf, sizeof(regBuf), "%02Xh", regs.a);
    ImGui::Text("A: %s", regBuf);
    ImGui::NextColumn();
    snprintf(regBuf, sizeof(regBuf), "%02Xh", regs.b);
    ImGui::Text("B: %s", regBuf);
    ImGui::NextColumn();
    snprintf(regBuf, sizeof(regBuf), "%02Xh", regs.c);
    ImGui::Text("C: %s", regBuf);
    ImGui::NextColumn();
    snprintf(regBuf, sizeof(regBuf), "%02Xh", regs.d);
    ImGui::Text("D: %s", regBuf);
    ImGui::NextColumn();
    snprintf(regBuf, sizeof(regBuf), "%02Xh", regs.e);
    ImGui::Text("E: %s", regBuf);
    ImGui::NextColumn();
    snprintf(regBuf, sizeof(regBuf), "%02Xh", regs.h);
    ImGui::Text("H: %s", regBuf);
    ImGui::NextColumn();
    snprintf(regBuf, sizeof(regBuf), "%02Xh", regs.l);
    ImGui::Text("L: %s", regBuf);
    ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::TextColored(WHITE_TEXT, "--- Registres 16 bits ---");
    
    snprintf(regBuf, sizeof(regBuf), "%04Xh", cpu.getSP());
    ImGui::Text("SP: %s", regBuf);
    snprintf(regBuf, sizeof(regBuf), "%04Xh", cpu.getPC());
    ImGui::Text("PC: %s", regBuf);
    snprintf(regBuf, sizeof(regBuf), "%04Xh", (regs.h << 8) | regs.l);
    ImGui::Text("HL: %s", regBuf);
    snprintf(regBuf, sizeof(regBuf), "%04Xh", (regs.b << 8) | regs.c);
    ImGui::Text("BC: %s", regBuf);
    snprintf(regBuf, sizeof(regBuf), "%04Xh", (regs.d << 8) | regs.e);
    ImGui::Text("DE: %s", regBuf);

    ImGui::Separator();
    ImGui::TextColored(WHITE_TEXT, "--- Flags de statut ---");

    bool cy = cpu.getCY();
    bool z = cpu.getZ();
    bool s = cpu.getS();
    bool p = cpu.getP();
    bool ac = cpu.getAC();

    ImVec4 flagOn = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
    ImVec4 flagOff = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);

    // Format: S Z AC P CY avec valeurs
    std::string statusStr = "";
    statusStr += s ? "1" : "0";
    statusStr += "S ";
    statusStr += z ? "1" : "0";
    statusStr += "Z ";
    statusStr += ac ? "1" : "0";
    statusStr += "AC ";
    statusStr += p ? "1" : "0";
    statusStr += "P ";
    statusStr += cy ? "1" : "0";
    statusStr += "CY";
    
    ImGui::TextColored(CYAN_COLOR, "%s", statusStr.c_str());

    // --- Shift Register (MB14241) ---
    ImGui::Separator();
    ImGui::TextColored(CYAN_COLOR, "--- Shift Register (MB14241) ---");
    
    auto& sr = emulator.getShiftRegister();
    ImGui::Text("Offset: %d", sr.getOffset());
    
    char srBuf[32];
    snprintf(srBuf, sizeof(srBuf), "Value: %04X", sr.getValue());
    ImGui::Text("%s", srBuf);
    
    snprintf(srBuf, sizeof(srBuf), "Result: %02X", sr.readPort3());
    ImGui::Text("%s", srBuf);

    // --- Contrôles d'émulation ---
    ImGui::Separator();
    ImGui::TextColored(CYAN_COLOR, "--- CONTRÔLES ÉMULATION ---");

    float btnWidth = (panelWidth - 40.0f) / 3.0f;
    float btnHeight = 35.0f;

    // Ligne 1 : Run/Pause + Reset
    ImGui::PushStyleColor(ImGuiCol_Button, GREEN_BTN);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    if (!emulator.isPaused()) {
        if (ImGui::Button("Pause", ImVec2(btnWidth * 2.0f - 5.0f, btnHeight))) {
            emulator.setPaused(true);
        }
    } else {
        if (ImGui::Button("Run", ImVec2(btnWidth * 2.0f - 5.0f, btnHeight))) {
            emulator.setPaused(false);
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, RED_BTN);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Reset", ImVec2(btnWidth, btnHeight))) {
        emulator.reset();
    }
    ImGui::PopStyleColor(2);

    // Ligne 2 : Vitesse d'émulation X0.5 / X1 / X2
    ImGui::Separator();
    ImGui::TextColored(WHITE_TEXT, "--- Vitesse ---");

    float speed = emulator.getEmulationSpeed();
    
    ImGui::Columns(3);
    
    ImGui::PushStyleColor(ImGuiCol_Button, BLUE_BTN);
    if (ImGui::Button("X0.5", ImVec2(btnWidth - 5.0f, btnHeight))) {
        emulator.setEmulationSpeed(0.5f);
    }
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::PushStyleColor(ImGuiCol_Button, GREEN_BTN);
    if (ImGui::Button("X1", ImVec2(btnWidth - 5.0f, btnHeight))) {
        emulator.setEmulationSpeed(1.0f);
    }
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::PushStyleColor(ImGuiCol_Button, YELLOW_BTN);
    if (ImGui::Button("X2", ImVec2(btnWidth - 5.0f, btnHeight))) {
        emulator.setEmulationSpeed(2.0f);
    }
    ImGui::PopStyleColor();
    ImGui::NextColumn();

    ImGui::Columns(1);

    // Afficher la vitesse actuelle
    char speedBuf[32];
    snprintf(speedBuf, sizeof(speedBuf), "Actuelle: X%.1f", speed);
    ImGui::TextColored(CYAN_COLOR, "%s", speedBuf);

    // Ligne 3 : Next Code + Breakpoint
    ImGui::Separator();
    
    ImGui::PushStyleColor(ImGuiCol_Button, BLUE_BTN);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
    if (ImGui::Button("Next Instruction", ImVec2(btnWidth * 2.0f - 5.0f, btnHeight))) {
        if (!emulator.isPaused()) emulator.setPaused(true);
        emulator.runFrame();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button, CYAN_BTN);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 1.0f, 1.0f, 1.0f));
    if (ImGui::Button("Break", ImVec2(btnWidth, btnHeight))) {
        // Toggle breakpoint
    }
    ImGui::PopStyleColor(2);

    // --- Section DIP SWITCHES (Important Stuff) ---
    ImGui::Separator();
    ImGui::TextColored(CYAN_COLOR, "--- DIP SWITCHES ---");

    // DIP Switch 1 : Coin Switch (Bit 7 Port 2)
    static bool dipCoinSwitch = false;
    if (ImGui::Checkbox("Coin Insert Enabled", &dipCoinSwitch)) {
        gameParams.coinSwitch = dipCoinSwitch ? 0x80 : 0x00;
    }

    // DIP Switch 2 : Bonus Score (Bit 3 Port 2)
    static bool dipBonusScore = true;
    if (ImGui::Checkbox("Bonus 1500pts", &dipBonusScore)) {
        gameParams.port2 = dipBonusScore ? 0x08 : 0x00;
    }

    // DIP Switch 3 : Vies initiales (Bit 0-1 Port 2)
    static int dipInitialLives = 0; // 0=3, 1=4, 2=5, 3=6
    if (ImGui::Combo("Initial Lives", &dipInitialLives, "3\04\05\06")) {
        gameParams.port2 = (gameParams.port2 & 0xFC) | (dipInitialLives & 0x03);
    }

    // DIP Switch 4 : Game Mode
    static int dipGameMode = 1;
    if (ImGui::Combo("Game Mode", &dipGameMode, "Standard\0Test\0Demo\0Custom")) {
        gameParams.gameMode = dipGameMode;
    }

    // DIP Switch 5 : Difficulty
    static int dipDifficulty = 0;
    if (ImGui::Combo("Difficulty", &dipDifficulty, "Easy\0Normal\0Hard\0Very Hard")) {
        // Sera utilisé par le jeu pour ajuster la vitesse des aliens
    }

    ImGui::Separator();

    // --- Section Credits ---
    ImGui::TextColored(WHITE_TEXT, "--- CREDITS ---");
    
    if (ImGui::Button("-")) {
        if (gameParams.credits > 0) gameParams.credits--;
    }
    ImGui::SameLine();
    char creditDisplay[64];
    snprintf(creditDisplay, sizeof(creditDisplay), "Credits: %02d", gameParams.credits);
    ImGui::TextColored(CYAN_COLOR, "%s", creditDisplay);
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        if (gameParams.credits < 99) gameParams.credits++;
    }

    // Appliquer les crédits à l'émulateur
    emulator.setCredits(static_cast<uint8_t>(gameParams.credits));

    ImGui::Separator();

    // --- Section Frame Counter (debug) ---
    static int frameCount = 0;
    static float lastTime = 0.0f;
    float currentTime = GetTime();
    if (currentTime - lastTime >= 1.0f) {
        // Afficher FPS
        char fpsBuf[64];
        snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d | Frames: %d", GetFPS(), frameCount);
        ImGui::TextColored(GREEN_BTN, "%s", fpsBuf);
        lastTime = currentTime;
        frameCount = 0;
    }
    frameCount++;

    // --- Section Status ---
    ImGui::Separator();
    bool isRunning = emulator.isRunning();
    bool isPaused = emulator.isPaused();
    
    ImVec4 statusColor = isRunning ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
    ImGui::TextColored(statusColor, "Status: %s", isRunning ? "RUNNING" : "STOPPED");
    
    if (isPaused) {
        ImGui::TextColored(YELLOW_BTN, "(PAUSED)");
    }

    ImGui::PopStyleColor();
    ImGui::End();
}

// ============================================================
// Colonne 4 — Affichage du Jeu (~35%)
// ============================================================
void DrawGameScreen(Emulator& emulator) {
    float panelX = 2560.0f * 0.65f; // 1664px
    float panelWidth = 2560.0f - panelX; // 896px
    
    ImGui::SetNextWindowPos(ImVec2(panelX, 0));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 1440.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, PANEL_BG);

    if (!ImGui::Begin("Affichage du Jeu")) {
        ImGui::PopStyleColor();
        ImGui::End();
        return;
    }

    // Titre en cyan
    ImGui::TextColored(CYAN_COLOR, "=== SPACE INVADERS ===");
    ImGui::Separator();

    // Dimensions après rotation 90° CCW = 224x256
    int width = 224;
    int height = 256;

    // Générer le framebuffer RGBA
    std::vector<uint8_t> framebuffer(width * height * 4);
    emulator.generateFramebuffer(framebuffer.data(), width, height);

    // Calculer la taille d'affichage avec aspect ratio
    float displayWidth = panelWidth - 40.0f;
    float displayHeight = static_cast<float>(displayWidth * static_cast<float>(height) / width);
    
    // Centrer verticalement si nécessaire
    float offsetY = std::max(10.0f, (1440.0f - displayHeight) / 2.0f);

    // --- Dessiner le jeu pixel par pixel avec AddRectFilled ---
    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.y += offsetY;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    float pxWidth = displayWidth / static_cast<float>(width);
    float pxHeight = displayHeight / static_cast<float>(height);

    // Dessiner chaque pixel individuellement avec la bonne couleur
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = (y * width + x) * 4;
            uint8_t r = framebuffer[idx];
            uint8_t g = framebuffer[idx + 1];
            uint8_t b = framebuffer[idx + 2];
            
            // Si pixel "éteint" (noir), ne pas dessiner (fond déjà noir)
            if (r == 0 && g == 0 && b == 0) continue;
            
            ImU32 color = IM_COL32(r, g, b, 255);
            
            float pxPosx = pos.x + x * pxWidth;
            float pxPosY = pos.y + y * pxHeight;
            
            draw_list->AddRectFilled(
                ImVec2(pxPosx, pxPosY),
                ImVec2(pxPosx + pxWidth + 0.5f, pxPosY + pxHeight + 0.5f),
                color
            );
        }
    }

    ImGui::Dummy(ImVec2(displayWidth, displayHeight));

    // Espacement en bas
    ImGui::Dummy(ImVec2(displayWidth, 10.0f));

    ImGui::PopStyleColor();
    ImGui::End();
}

// ============================================================
// Point d'entrée principal GUI Raylib + rlImGui
// ============================================================
int main_gui(int argc, char* argv[]) {
    // Initialisation Raylib - Fenêtre QHD (2560×1440) pour meilleur affichage
    const int windowWidth = 2560;
    const int windowHeight = 1440;
    InitWindow(windowWidth, windowHeight, "Space Invaders Emulator — Interface d'Émulateur Arcade");
    SetTargetFPS(60);

    // Initialisation rlImGui
    rlImGuiSetup(true);

    // Créer l'emulateur
    Emulator emulator;

    // Charger les ROMs
    std::string romH = "../assets/roms/invaders.h";
    std::string romG = "../assets/roms/invaders.g";
    std::string romF = "../assets/roms/invaders.f";
    std::string romE = "../assets/roms/invaders.e";

    bool romsLoaded = emulator.loadROMs(romH, romG, romF, romE);
    if (!romsLoaded) {
        std::cerr << "ERREUR: Impossible de charger les ROMs Space Invaders!" << std::endl;
        std::cerr << "Vérifiez que les fichiers existent dans: assets/roms/" << std::endl;
        std::cerr << "Fichiers attendus: invaders.h, invaders.g, invaders.f, invaders.e" << std::endl;
    }

    emulator.initialize();
    
    // Paramètres initiaux du jeu — DIP Switches par défaut Space Invaders Taito 1978
    GameParams gameParams;
    gameParams.credits = 0;           // Démarrer avec 0 credits (insérer pièce pour jouer)
    gameParams.coinSwitch = 0x80;     // Bit 7 = Coin Insert Enabled (DIP Switch par défaut)
    gameParams.gameMode = 1;          // Standard mode
    gameParams.suspend = 0;           // Non suspendu
    gameParams.port1 = 0;             // Port 1 = inputs joueur
    gameParams.port2 = 0x08;          // Bit 3 = Bonus 1500pts (DIP Switch par défaut)
    
    emulator.setCredits(static_cast<uint8_t>(gameParams.credits));

    // Adresses de départ pour les éditeurs
    uint16_t memStartAddr = 0x29F0;     // Pour l'éditeur mémoire
    uint16_t instrStartAddr = 0x0000;   // Pour le panneau instructions

    // Flag pour reset du Start P1 (impulsion)
    bool p1_start_pressed_ = false;

    // Boucle principale
    while (!WindowShouldClose()) {
        // ====== MAPPING CLAVIER DIRECT — Joueur 1 ======
        // Port 1 bits: Bit2=P1Start, Bit4=P1Tir, Bit5=P1Gauche, Bit6=P1Droite
        if (romsLoaded) {
            // A ou Flèche GAUCHE → Gauche P1 (bit 5, key 0x05)
            emulator.setInputKeyState(0x05, IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
            
            // D ou Flèche DROITE → Droite P1 (bit 6, key 0x06)
            emulator.setInputKeyState(0x06, IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT));
            
            // Espace ou W → Tir P1 (bit 4, key 0x04)
            emulator.setInputKeyState(0x04, IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_W));
            
            // Enter ou R → Start P1 (impulsion)
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) {
                emulator.triggerStartP1();
            }
            
            // C → Ajouter un credit (impulsion)
            if (IsKeyPressed(KEY_C)) {
                int currentCredits = gameParams.credits + 1;
                if (currentCredits > 99) currentCredits = 99;
                gameParams.credits = currentCredits;
                emulator.setCredits(static_cast<uint8_t>(gameParams.credits));
            }
        }

        // Exécuter l'émulation si pas en pause ET ROMs chargées
        if (!emulator.isPaused() && romsLoaded) {
            emulator.runFrame();
            
            // Reset du Start P1 après exécution (impulsion)
            emulator.setInputKeyState(0x02, false);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        rlImGuiBegin();

        // 1. Colonne 1 — Éditeur Mémoire (~25%)
        DrawMemoryEditorPanel(emulator, memStartAddr);

        // 2. Colonne 2 — Instructions Assembleur (~15%)
        DrawInstructionPanel(emulator, instrStartAddr);

        // 3. Colonne 3 — État CPU & Contrôles (~25%)
        DrawCPUStatePanel(emulator, gameParams);

        // 4. Colonne 4 — Affichage du Jeu (~35%)
        DrawGameScreen(emulator);

        // Affichage avertissement si ROMs non chargées
        if (!romsLoaded) {
            ImGui::SetNextWindowPos(ImVec2(2560.0f * 0.65f, 1440.0f - 100.0f));
            ImGui::SetNextWindowSize(ImVec2(2560.0f - 2560.0f * 0.65f, 100.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.8f, 0.2f, 0.2f, 0.9f));
            if (ImGui::Begin("Avertissement", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), 
                    "⚠ ROMs Space Invaders non trouvées!");
                ImGui::Text("Placez invaders.h/g/f/e dans assets/roms/ et relancez l'émulateur.");
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                    "L'émulation est en pause pour éviter un comportement indéfini.");
            }
            ImGui::End();
            ImGui::PopStyleColor();
        }

        rlImGuiEnd();

        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();
    return 0;
}

// Alias pour compatibilité
int main(int argc, char* argv[]) {
    return main_gui(argc, argv);
}