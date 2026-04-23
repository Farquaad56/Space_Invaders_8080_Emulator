#include "../include/emulator.hpp"
#include <iostream>
#include <string>

/**
 * Point d'entrée principal pour le GUI Space Invaders
 * 
 * Ce fichier est un STUB — il nécessite Raylib + Dear ImGui + rlImGui compilés.
 * Pour l'instant, c'est une démo console qui montre la structure du GUI.
 * 
 * Quand Raylib sera installé :
 * - Remplacer cette fonction main() par une boucle Raylib complète
 * - Inclure "raylib.h", "imgui.h", "imgui_impl_raylib.h"
 * - Utiliser les panneaux ImGui pour l'éditeur mémoire, registres, contrôles
 */

int main_gui(int argc, char* argv[]) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Space Invaders Emulator — GUI Mode" << std::endl;
    std::cout << "(STUB — nécessite Raylib + Dear ImGui)" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Créer l'émulateur
    Emulator emulator;

    // Charger les ROMs
    std::string romH = "../assets/roms/invaders.h";
    std::string romG = "../assets/roms/invaders.g";
    std::string romF = "../assets/roms/invaders.f";
    std::string romE = "../assets/roms/invaders.e";

    if (!emulator.loadROMs(romH, romG, romF, romE)) {
        std::cerr << "Erreur: Impossible de charger les ROMs." << std::endl;
        return 1;
    }

    emulator.initialize();
    emulator.setCredits(2);

    // ============================================================
    // STRUCTURE DU GUI (à implémenter avec Raylib + ImGui)
    // ============================================================
    /*
    // === Initialisation Raylib ===
    InitWindow(1280, 720, "Space Invaders Emulator");
    InitImGui();  // rlImGui

    // === Layout des panneaux ===
    // Zone gauche (30%) : Éditeur mémoire hexadécimal
    // Zone centre (35%) : Panneau registre CPU + contrôles
    // Zone droite (35%) : Écran de jeu Space Invaders

    // === Boucle principale ===
    while (!WindowShouldClose()) {
        // 1. Exécuter une instruction CPU
        emulator.runFrame();

        // 2. Générer le framebuffer vidéo
        uint8_t framebuffer[256 * 224 * 4];
        int width, height;
        emulator.generateFramebuffer(framebuffer, width, height);

        // 3. Dessiner les panneaux ImGui
        BeginImGui();

        // Panneau Mémoire Éditeur (gauche)
        DrawMemoryPanel(emulator);

        // Panneau Registre Info + Contrôles (centre)
        DrawRegisterPanel(emulator);
        DrawControlButtons(emulator);

        // Écran de Jeu (droite)
        DrawGameScreen(framebuffer, width, height);

        EndImGui();

        // 4. Afficher l'écran Raylib
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawTextureEx(videoTexture, {0, 0}, 0, 2.0f, WHITE);
        EndDrawing();
    }

    CloseWindow();
    */

    // ============================================================
    // DEMONSTRATION CONSOLE (structure des panneaux)
    // ============================================================
    std::cout << "Structure du GUI prévue :\n" << std::endl;

    std::cout << "--- Panneau Mémoire Éditeur (gauche 30%) ---\n";
    std::cout << "  Adresse    Données hexadécimales\n";
    std::cout << "  29F0: 00 38 00 0E 00 0E 00 00 ...\n";
    std::cout << "  2A00: FF 00 FF 00 FF 00 FF 00 ...\n";
    std::cout << "  [Options] Range 0000..FFFE\n" << std::endl;

    std::cout << "--- Panneau Registre Info (centre 35%) ---\n";
    auto& cpu = emulator.getCpu();
    std::cout << "  Current Instruction: ";
    uint8_t opcode = cpu.readMem(cpu.getPC());
    printf("0x%02X\n", opcode);
    std::cout << "  Next Byte: ";
    uint8_t nextByte = cpu.readMem(cpu.getPC() + 1);
    printf("0x%02X\n", nextByte);
    std::cout << "  Reg A : 0x" << std::hex << (int)cpu.getRegisters().a << "\n";
    std::cout << "  Reg B : 0x" << std::hex << (int)cpu.getRegisters().b << "\n";
    std::cout << "  Reg C : 0x" << std::hex << (int)cpu.getRegisters().c << "\n";
    std::cout << "  Reg D : 0x" << std::hex << (int)cpu.getRegisters().d << "\n";
    std::cout << "  Reg E : 0x" << std::hex << (int)cpu.getRegisters().e << "\n";
    std::cout << "  Reg H : 0x" << std::hex << (int)cpu.getRegisters().h << "\n";
    std::cout << "  Reg L : 0x" << std::hex << (int)cpu.getRegisters().l << "\n";
    std::cout << "  Reg SP: 0x" << std::hex << cpu.getSP() << "\n";
    std::cout << "  Reg PC: 0x" << std::hex << cpu.getPC() << "\n" << std::dec;
    std::cout << "  [Breakpoint] [Reset] [Start/Pause] [ISR Enable]\n" << std::endl;

    std::cout << "--- Écran de Jeu (droite 35%) ---\n";
    std::cout << "  Résolution: 256x224 pixels\n";
    std::cout << "  Rotation: 90° anti-horaire\n";
    std::cout << "  Couleurs: Cyan (#00FFFF) sur noir\n" << std::endl;

    std::cout << "--- Section Inférieure ---\n";
    std::cout << "  Credits: 02\n";
    std::cout << "  CoinSwitch: 00\n";
    std::cout << "  GameMode: 01\n" << std::endl;

    return 0;
}

// Alias pour compatibilité avec le nom de fonction attendu
int main(int argc, char* argv[]) {
    return main_gui(argc, argv);
}