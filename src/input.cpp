#include "input.hpp"
#include <raylib.h>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// ====== Mapping clavier par défaut (Raylib keys) ======
static const int KEY_P1_FIRE = KEY_SPACE;
static const int KEY_P1_LEFT = KEY_A;
static const int KEY_P1_RIGHT = KEY_D;
static const int KEY_P2_FIRE = KEY_ENTER;
static const int KEY_P2_LEFT = KEY_KP_4;
static const int KEY_P2_RIGHT = KEY_KP_6;

// ====== Audio singleton pour Space Invaders ======
static bool audioInitialized = false;
static Sound sounds[9];  // 9 fichiers WAV
static std::map<std::string, int> soundIndex;

// États des sons en boucle (UFO)
static bool ufoLoopPlaying = false;

// Chemin de base trouvé pour les fichiers WAV
static std::string audioBasePath = "";

// Fonction interne d'initialisation audio (appelée par initAudioEarly ou lazy-load)
static void initAudioInternal() {
    if (audioInitialized) return;
    
    // Initialiser explicitement le device audio Raylib avec des paramètres standards
    // Cela contourne le bug de format WAV non-standard (11025 Hz au lieu de 44100 Hz)
    InitAudioDevice();                              // Ouvrir le device audio
    
    // Configurer les buffers par défaut pour éviter les erreurs de conversion
    SetAudioStreamBufferSizeDefault(512);           // Buffer plus petit pour WAV simples
    
    std::cout << "[AUDIO] Device audio initialisé" << std::endl;
    
    // Chemins possibles pour les fichiers WAV
    // Le working directory dépend de comment l'exécutable est lancé:
    // - Depuis CLion/build/ : cmake-build-debug/ → "../assets/sound/"
    // - Depuis racine projet : H:/__Emulator__/... → "assets/sound/"
    std::vector<std::string> testPaths = {
        "assets/sound/",      // Working directory = racine du projet
        "../assets/sound/",   // Working directory = cmake-build-debug/
        "../../assets/sound/",// Working directory = sous-dossier build imbriqué
        ""                    // executable directory (fallback)
    };
    
    struct SoundFile {
        std::string name;
        int index;
    };
    
    SoundFile files[] = {
        {"ufo.wav", 0},
        {"shot.wav", 1},
        {"playerdied.wav", 2},
        {"invaderkilled.wav", 3},
        {"invadermove1.wav", 4},
        {"invadermove2.wav", 5},
        {"invadermove3.wav", 6},
        {"invadermove4.wav", 7},
        {"ufohit.wav", 8}
    };
    
    // Trouver le chemin qui fonctionne pour TOUS les fichiers
    // Utiliser ifstream pour vérifier l'existence réelle du fichier
    for (auto& path : testPaths) {
        bool allFound = true;
        for (auto& f : files) {
            std::string fullPath = path + f.name;
            std::ifstream testFile(fullPath, std::ios::binary);
            if (!testFile.good()) {
                allFound = false;
                break;
            }
        }
        
        if (allFound) {
            audioBasePath = path;
            std::cout << "[AUDIO] Chemin trouvé: '" << audioBasePath << "' (tous les WAV accessibles)" << std::endl;
            break;
        } else {
            std::cout << "[AUDIO] Chemin échoué: '" << path << "'" << std::endl;
        }
    }
    
    if (audioBasePath.empty()) {
        std::cerr << "[AUDIO] ERREUR: Aucun chemin valide trouvé pour les fichiers WAV!" << std::endl;
        std::cerr << "[AUDIO] Vérifiez que les fichiers existent dans assets/sound/" << std::endl;
    }
    
    // Charger tous les sons une fois le chemin trouvé
    int loadedCount = 0;
    for (auto& f : files) {
        soundIndex[f.name] = f.index;
        std::string fullPath = audioBasePath + f.name;
        
        if (!audioBasePath.empty()) {
            // Utiliser LoadSound directement (plus simple que LoadWave+LoadSoundFromWave)
            sounds[f.index] = LoadSound(fullPath.c_str());
            
            // Vérifier si le son a été chargé correctement
            if (sounds[f.index].stream.buffer != nullptr) {
                std::cout << "[AUDIO] OK: " << fullPath 
                          << " (" << sounds[f.index].stream.sampleRate << " Hz)" << std::endl;
                loadedCount++;
            } else {
                // Essayer avec LoadWave + LoadSoundFromWave comme fallback
                Wave wave = LoadWave(fullPath.c_str());
                if (wave.data != nullptr) {
                    sounds[f.index] = LoadSoundFromWave(wave);
                    UnloadWave(wave);
                    
                    if (sounds[f.index].stream.buffer != nullptr) {
                        std::cout << "[AUDIO] OK (fallback): " << fullPath 
                                  << " (" << sounds[f.index].stream.sampleRate << " Hz)" << std::endl;
                        loadedCount++;
                    } else {
                        std::cerr << "[AUDIO] FAIL: " << fullPath << std::endl;
                    }
                } else {
                    std::cerr << "[AUDIO] FAIL (wave null): " << fullPath << std::endl;
                }
            }
        } else {
            std::cerr << "[AUDIO] SKIP (chemin vide): " << f.name << std::endl;
        }
    }
    
    std::cout << "[AUDIO] " << loadedCount << "/9 fichiers WAV chargés avec succès" << std::endl;
    
    audioInitialized = true;
}

// Version publique statique — appelée au démarrage de l'émulateur
void InputSystem::initAudioEarly() {
    initAudioInternal();
}

// Jouer un son one-shot (déclenché sur front montant)
static void playOneShot(int index, const std::string& name) {
    if (!audioInitialized) initAudioInternal();  // lazy-load si nécessaire
    if (index >= 0 && index < 9) {
        PlaySound(sounds[index]);
        std::cout << "[AUDIO] OneShot: " << name << " (idx=" << index << ")" << std::endl;
    } else {
        std::cerr << "[AUDIO] Index invalide: " << index << " pour " << name << std::endl;
    }
}

// Démarrer/arrêter la boucle UFO
static void startUfoLoop() {
    if (!ufoLoopPlaying) {
        playOneShot(0, "ufo.wav");  // Raylib joue en boucle si redémarré rapidement
        ufoLoopPlaying = true;
        std::cout << "[AUDIO] UFO Loop START" << std::endl;
    }
}

static void stopUfoLoop() {
    if (ufoLoopPlaying) {
        // Arrêter le son UFO
        StopSound(sounds[0]);
        ufoLoopPlaying = false;
        std::cout << "[AUDIO] UFO Loop STOP" << std::endl;
    }
}

// Reset de l'état audio (pour reset émulateur)
static void resetAudioState() {
    stopUfoLoop();
}

// Fermer le device audio à la fin
static void shutdownAudio() {
    UnloadSound(sounds[0]);
    UnloadSound(sounds[1]);
    UnloadSound(sounds[2]);
    UnloadSound(sounds[3]);
    UnloadSound(sounds[4]);
    UnloadSound(sounds[5]);
    UnloadSound(sounds[6]);
    UnloadSound(sounds[7]);
    UnloadSound(sounds[8]);
    CloseAudioDevice();
    std::cout << "[AUDIO] Device audio fermé" << std::endl;
}

InputSystem::InputSystem() {
    // Initialisation EXPLICITE de tous les membres (évite memory garbage)
    p1_fire_ = nullptr;
    p1_left_ = nullptr;
    p1_right_ = nullptr;
    p2_fire_ = nullptr;
    p2_left_ = nullptr;
    p2_right_ = nullptr;
    p1_start_ = nullptr;
    p2_start_ = nullptr;
    credits_ = 0;
    vies_ = 3;
    bonus_ = false;
    p1_fire_state_ = false;
    p1_left_state_ = false;
    p1_right_state_ = false;
    p2_fire_state_ = false;
    p2_left_state_ = false;
    p2_right_state_ = false;
    p1_start_state_ = false;
    p2_start_state_ = false;
    
    std::cout << "[DEBUG] InputSystem constructor: credits_=0, vies_=3" << std::endl;
}

uint8_t InputSystem::readPort1() {
    uint8_t value = 0;
    
    // Bit 0 : P2 Credit / Coin Insert (1 = credit disponible)
    if (credits_ > 0) {
        value |= (1 << 0);
    }
    
    // Bit 1 : P2 Start — utiliser state direct ou callback
    if (p2_start_state_ || (p2_start_ && p2_start_())) {
        value |= (1 << 1);
    }
    
    // Bit 2 : P1 Start — utiliser state direct ou callback
    if (p1_start_state_ || (p1_start_ && p1_start_())) {
        value |= (1 << 2);
    }
    
    // Bit 3 : Toujours 1
    value |= (1 << 3);
    
    // Bit 4 : P1 Tir — utiliser state direct ou callback
    if (p1_fire_state_ || (p1_fire_ && p1_fire_())) {
        value |= (1 << 4);
    }
    
    // Bit 5 : P1 Gauche — utiliser state direct ou callback
    if (p1_left_state_ || (p1_left_ && p1_left_())) {
        value |= (1 << 5);
    }
    
    // Bit 6 : P1 Droite — utiliser state direct ou callback
    if (p1_right_state_ || (p1_right_ && p1_right_())) {
        value |= (1 << 6);
    }
    
    // DEBUG: Logger chaque lecture CPU du port 1
    std::cout << "[DEBUG] readPort1: credits=" << (int)credits_ 
              << " p1_start=" << (p1_start_state_ ? 1 : 0)
              << " fire=" << (p1_fire_state_ ? 1 : 0)
              << " left=" << (p1_left_state_ ? 1 : 0)
              << " right=" << (p1_right_state_ ? 1 : 0)
              << " value=0x" << std::hex << (int)value << std::dec 
              << " (binary: " << (int)value << ")" << std::endl;
    
    return value;
}

uint8_t InputSystem::readPort2() {
    uint8_t value = 0;
    
    // Bit 0-1 : Vies (00=3, 01=4, 10=5, 11=6) — par défaut 3 vies = 00
    if (vies_ < 4) {
        value |= (vies_ & 0x03);
    }
    
    // Bit 2 : Tilt (non implémenté par défaut = 0)
    // Bit 3 : Bonus (0=1500 pts, 1=1000 pts) — par défaut 1500 = 0
    if (bonus_) {
        value |= (1 << 3);
    }
    
    // Bit 4-6 : P2 Tir/Gauche/Droite (optionnels)
    if (p2_fire_ && p2_fire_()) {
        value |= (1 << 4);
    }
    if (p2_left_ && p2_left_()) {
        value |= (1 << 5);
    }
    if (p2_right_ && p2_right_()) {
        value |= (1 << 6);
    }
    
    // Bit 7 : Coin Info (DIP Switch) — par défaut = 0
    // Si coin switch enabled, bit 7 = 1
    // (géré par le DIP switch dans GUI)
    
    return value;
}

void InputSystem::writePort2(uint8_t data) {
    // Shift Amount (définit l'offset du shift register)
    // Seul les 3 premiers bits sont utilisés
    uint8_t offset = data & 0x07;
    (void)offset;  // Sera utilisé par le shift register dans emulator.cpp
}

void InputSystem::writePort3(uint8_t data, uint8_t oldData) {
    // Son Banque 1 — Space Invaders (Taito, 1978)
    // Détection de front montant (rising edge) : bit passe de 0 à 1
    
    // Bit 0 : UFO loop sound (boucle)
    if ((data & 0x01) && !(oldData & 0x01)) {
        startUfoLoop();
    } else if (!(data & 0x01) && (oldData & 0x01)) {
        stopUfoLoop();
    }
    
    // Bit 1 : Player shot sound (one-shot)
    if ((data & 0x02) && !(oldData & 0x02)) {
        playOneShot(1, "shot.wav");
    }
    
    // Bit 2 : Player death sound (one-shot)
    if ((data & 0x04) && !(oldData & 0x04)) {
        playOneShot(2, "playerdied.wav");
    }
    
    // Bit 3 : Alien death sound (one-shot)
    if ((data & 0x08) && !(oldData & 0x08)) {
        playOneShot(3, "invaderkilled.wav");
    }
}

void InputSystem::writePort5(uint8_t data, uint8_t oldData) {
    // Son Banque 2 — Space Invaders (Taito, 1978)
    // Détection de front montant (rising edge) : bit passe de 0 à 1
    
    // Bit 0-3 : Alien step sounds (4 niveaux de vitesse, one-shot)
    if ((data & 0x01) && !(oldData & 0x01)) {
        playOneShot(4, "invadermove1.wav");
    }
    if ((data & 0x02) && !(oldData & 0x02)) {
        playOneShot(5, "invadermove2.wav");
    }
    if ((data & 0x04) && !(oldData & 0x04)) {
        playOneShot(6, "invadermove3.wav");
    }
    if ((data & 0x08) && !(oldData & 0x08)) {
        playOneShot(7, "invadermove4.wav");
    }
    
    // Bit 4 : UFO hit sound (one-shot)
    if ((data & 0x10) && !(oldData & 0x10)) {
        playOneShot(8, "ufohit.wav");
    }
}

void InputSystem::resetAudio() {
    resetAudioState();
}

// Fonction appelée à la fermeture de l'émulateur
static void shutdownAudioEarly() {
    shutdownAudio();
}

void InputSystem::setInputKeyState(uint8_t key, bool state) {
    setKeyState(key, state);
}

void InputSystem::setKeyboardMapping(
    std::function<bool()> p1_fire,
    std::function<bool()> p1_left,
    std::function<bool()> p1_right,
    std::function<bool()> p2_fire,
    std::function<bool()> p2_left,
    std::function<bool()> p2_right) {
    
    p1_fire_ = p1_fire;
    p1_left_ = p1_left;
    p1_right_ = p1_right;
    p2_fire_ = p2_fire;
    p2_left_ = p2_left;
    p2_right_ = p2_right;
}

void InputSystem::setStartMapping(
    std::function<bool()> p1_start,
    std::function<bool()> p2_start) {
    
    p1_start_ = p1_start;
    p2_start_ = p2_start;
}

void InputSystem::update() {
    // Mise à jour des entrées (à appeler dans la boucle principale)
    // Les fonctions std::function sont appelées dans readPort1/readPort2
}

// ==================== CONTRÔLES DIRECTS ====================

void InputSystem::setKeyState(uint8_t key, bool state) {
    switch(key) {
        case 0x04: p1_fire_state_ = state; break;   // P1 Fire (bit 4)
        case 0x05: p1_left_state_ = state; break;   // P1 Left (bit 5)
        case 0x06: p1_right_state_ = state; break;  // P1 Right (bit 6)
        case 0x02: p1_start_state_ = state; break;  // P1 Start (bit 2)
    }
}

void InputSystem::triggerStartP1() {
    p1_start_state_ = true;
    // Reset après un frame (géré par update())
}

void InputSystem::setAudioCallback(uint8_t port, AudioCallback cb) {
    if (port == 0x03) {
        audio_callback_3_ = cb;
    } else if (port == 0x05) {
        audio_callback_5_ = cb;
    }
}

void InputSystem::setCredits(uint8_t credits) {
    uint8_t old = credits_;
    credits_ = credits;
    std::cout << "[DEBUG] setCredits: " << (int)old << " -> " << (int)credits << std::endl;
}

void InputSystem::setVies(uint8_t vies) {
    if (vies < 4) {
        vies_ = vies;
    }
}

void InputSystem::setBonus(bool bonus) {
    bonus_ = bonus;
}