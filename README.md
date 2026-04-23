# Space Invaders 8080 Emulator

Un émulateur de la machine arcade Space Invaders (1978 Taito) basé sur le CPU Intel 8080, développé en C++ avec Raylib et ImGui.

## Fonctionnalités

- **Émulation CPU Intel 8080** complète avec ALU, registres, flags
- **Shift Register MB14241** pour les sprites des aliens
- **Affichage vidéo** avec rotation 90° CCW (512×128 → 256×224)
- **Son** avec 9 fichiers WAV (ufo, shot, playerdied, invaderkilled, invadermove×4, ufohit)
- **Interface GUI** avec 4 colonnes :
  - Éditeur mémoire (ROM/RAM/VRAM)
  - 20 dernières instructions assembleur décodées
  - État CPU, registres, flags, contrôles d'émulation
  - Affichage du jeu

## Prérequis

- **CMake** 3.16+
- **Compilateur C++17** (MSVC, GCC, ou Clang)
- **Raylib** (géré automatiquement via vcpkg)

## Build

```bash
# Créer le dossier de build
mkdir build && cd build

# Configurer avec CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Compiler
cmake --build . --config Debug

# Lancer l'émulateur
./space_invaders_gui.exe
```

### Avec CLion

1. Ouvrir le projet dans CLion
2. CLion détectera automatiquement le CMakeLists.txt
3. Build et lancer depuis l'IDE

## ROMs Space Invaders

Les ROMs **ne sont pas incluses** dans ce repository pour des raisons de licences.

Vous devez placer les 4 fichiers ROM suivants dans `assets/roms/` :

- `invaders.e`
- `invaders.f`
- `invaders.g`
- `invaders.h`

Ces fichiers sont extraits de la machine arcade originale Space Invaders de Taito (1978).

## Contrôles

| Touche | Action |
|--------|--------|
| **A / ←** | Déplacer gauche (Joueur 1) |
| **D / →** | Déplacer droite (Joueur 1) |
| **Espace / W** | Tir (Joueur 1) |
| **Enter / R** | Start P1 |
| **C** | Ajouter un credit |
| **Pause / Run** | Pause/Reprendre l'émulation |
| **Reset** | Réinitialiser l'émulateur |

## Structure du projet

```
├── CMakeLists.txt          # Configuration du build
├── .gitignore              # Fichiers exclus du repository
├── include/                # Headers
│   ├── alu.hpp             # Unité arithmétique logique
│   ├── cpu8080.hpp         # CPU Intel 8080
│   ├── emulator.hpp        # Emulateur principal
│   ├── flags.hpp           # Flags de statut
│   ├── input.hpp           # Système d'input
│   ├── memory.hpp          # Gestion mémoire
│   ├── shift_register.hpp  # Shift Register MB14241
│   └── video.hpp           # Affichage vidéo
├── src/                    # Sources
│   ├── main_gui.cpp        # Interface GUI principale
│   ├── cpu8080.cpp         # Implémentation CPU
│   ├── emulator.cpp        # Implémentation emu
│   ├── input.cpp           # Système d'input
│   ├── shift_register.cpp  # Shift Register
│   └── video.cpp           # Affichage vidéo
├── gui/                    # Extensions GUI
│   └── imgui_main.cpp      # Point d'entrée alternatif
├── assets/
│   ├── roms/               # ROMs (NON INCLUS - à ajouter localement)
│   └── sound/              # Sons WAV (INCLUS)
├── third_party/            # Bibliothèques tierces
│   ├── imgui/              # Dear ImGui
│   ├── raylib/             # Raylib headers
│   └── rlImGui/            # Integration Raylib + ImGui
└── tests/                  # Tests unitaires
    └── test_cpu8080.cpp
```

## Architecture technique

- **CPU 8080** : Implémentation complète avec 78 instructions décodées
- **Mémoire** : 64KB adressable (ROM, RAM, VRAM)
- **Video** : Buffer 512×128 pixels avec rotation 90° CCW
- **Audio** : Raylib audio avec edge detection pour sons impulsifs
- **GUI** : ImGui + Raylib GLFW window

## Licence

Ce projet est un émulateur éducatif. Les ROMs Space Invaders sont la propriété de Taito Corporation et ne sont pas incluses dans ce repository.

## Crédits

Développé en C++ avec Raylib, Dear ImGui, et CMake.