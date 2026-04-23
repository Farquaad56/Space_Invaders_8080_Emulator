#include "emulator.hpp"
#include <raylib.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>

/**
 * Programme de démonstration pour l'émulateur Space Invaders
 * 
 * Ce programme charge les ROMs et exécute l'émulation CPU avec boucle temps réel.
 * Contrôles :
 *   - Espace = Tir joueur 1
 *   - A/D = Gauche/Droite joueur 1
 *   - C = Insérer une pièce (Coin)
 *   - Entrée = Start joueur 1
 *   - Echap = Quitter
 */

volatile bool running = true;

void signalHandler(int) {
    running = false;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "Space Invaders Emulator Demo (Taito, 1978)" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nContrôles:" << std::endl;
    std::cout << "  [Espace] = Tir P1" << std::endl;
    std::cout << "  [A/D] = Gauche/Droite P1" << std::endl;
    std::cout << "  [C] = Insérer une pièce (Coin)" << std::endl;
    std::cout << "  [Entree] = Start P1" << std::endl;
    std::cout << "  [Echap] = Quitter" << std::endl;

    // Créer l'émulateur
    Emulator emulator;

    // Chemins des ROMs par défaut
    std::string romH = "../assets/roms/invaders.h";
    std::string romG = "../assets/roms/invaders.g";
    std::string romF = "../assets/roms/invaders.f";
    std::string romE = "../assets/roms/invaders.e";

    // Charger les ROMs
    std::cout << "\nChargement des ROMs Space Invaders..." << std::endl;
    if (!emulator.loadROMs(romH, romG, romF, romE)) {
        std::cerr << "Erreur: Impossible de charger les ROMs." << std::endl;
        std::cerr << "Vérifiez que les fichiers existent dans assets/roms/" << std::endl;
        
        // Continuer quand même pour démonstration (mémoire vide)
        std::cout << "Mode demonstration: memoire vide" << std::endl;
    }

    // Initialiser l'émulateur (connecter les handlers I/O)
    emulator.initialize();

    // Configurer les crédits à 0 — l'utilisateur doit insérer une pièce avec 'C'
    emulator.setCredits(0);
    std::cout << "Credits: 0 (appuyer sur [C] pour insérer une pièce)" << std::endl;

    auto& cpu = emulator.getCpu();
    
    // Capturer le signal SIGINT pour quitter proprement
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Boucle principale temps réel — exécuter des frames en continu
    std::cout << "\n=== Démarrage de l'émulation ===" << std::endl;
    
    int frameCount = 0;
    auto lastTime = std::chrono::high_resolution_clock::now();
    int vramLogInterval = 300;  // Logger VRAM toutes les 300 frames (~5 secondes)

    while (running) {
        emulator.runFrame();
        frameCount++;

        // Logger état CPU toutes les N frames (debug)
        if (frameCount % vramLogInterval == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
            lastTime = now;
            
            std::cout << "[FRAME " << frameCount 
                      << "] " << ms << "ms/frame | PC=0x" << std::hex << cpu.getPC() 
                      << std::dec << std::endl;
        }

        // Gérer les inputs clavier — utiliser Raylib si disponible, sinon polling console
        if (GetKeyPressed() == KEY_ESCAPE) {
            running = false;
            break;
        }
        
        // Mapper les touches aux inputs Space Invaders
        if (IsKeyDown(KEY_SPACE)) {
            emulator.setInputKeyState(0x04, true);  // P1 Fire
        } else {
            emulator.setInputKeyState(0x04, false);
        }
        
        if (IsKeyDown(KEY_A)) {
            emulator.setInputKeyState(0x05, true);   // P1 Left
        } else {
            emulator.setInputKeyState(0x05, false);
        }
        
        if (IsKeyDown(KEY_D)) {
            emulator.setInputKeyState(0x06, true);   // P1 Right
        } else {
            emulator.setInputKeyState(0x06, false);
        }

        // Touch unique: C = Coin (incrémente credits), Enter = Start
        if (IsKeyPressed(KEY_C)) {
            uint8_t oldCredits = emulator.getCredits();
            uint8_t newCredits = oldCredits + 1;
            emulator.setCredits(newCredits);
            std::cout << "[DEBUG] KEY_C pressed! Credits: " << (int)oldCredits 
                      << " -> " << (int)newCredits << std::endl;
        }
        
        if (IsKeyPressed(KEY_ENTER)) {
            emulator.triggerStartP1();
            std::cout << "[DEBUG] KEY_ENTER pressed! triggerStartP1() called" << std::endl;
        }

        // Throttle pour ~60 FPS
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - lastTime).count();
        if (elapsed < 17) {  // ~60 FPS = 16.67ms per frame
            std::this_thread::sleep_for(std::chrono::milliseconds(17 - elapsed));
        }
    }

    std::cout << "\n=== Emulation arretee ===" << std::endl;
    std::cout << "Frames executees: " << frameCount << std::endl;

    return 0;
}
