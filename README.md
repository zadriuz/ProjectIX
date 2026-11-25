# ProjectIX

Top-down horror maze-chase game built with Godot Engine 4, C++, and custom GDShaders under **Parabyte**, inspired by *Dark Deception*.

> **Asset Notice:** Source code, scenes, shaders, and audio bus configurations are public for reference. Licensed third-party sprites, tilemaps, and audio files (`assets/`) are excluded from this repository.

<!--
### Gameplay & Visuals

| Title Screen | Maze & Chase Gameplay | Shader & Minimap Effects |
| :---: | :---: | :---: |
| ![Title](screenshots/title.png) | ![Gameplay](screenshots/gameplay.png) | ![Shaders](screenshots/shaders.png) |
| *Custom title shader and menu navigation* | *Multi-tier maze navigation and enemy AI* | *Minimap vignette and atmospheric shaders* |
-->

---

### Role & Context

- **Team / Studio:** Parabyte
- **Genre:** Horror / Maze Chase (inspired by *Dark Deception*)
- **Engine:** Godot Engine 4

---

### Gameplay & Technical Features

- **Multi-Tier Maze Progression:** Three escalating difficulty stages (`EasyLevel`, `MediumLevel`, `HardLevel`) featuring collectible capsules (`capsule.cpp`) and stage exit portals (`portal.cpp`).
- **Distinct Enemy AI Archetypes:** Three custom C++ enemy behaviors—**The Follower** (`follower.cpp`), **The Charger** (`charger.cpp`), and **The Cloner** (`cloner.cpp`)—pursuing **Subject IX** (`subject_ix.cpp`).
- **Tactical Power-Ups:** Collectible **Immunity Shield** (`immunity_shield.cpp`) and **Speed Boost** (`speedboost.cpp`) pickups to evade pursuit.
- **Custom Visual Shaders (`GDShader`):** Dedicated shaders for the title screen (`gameTitle.gdshader`), minimap framing (`miniMapVignette.gdshader`), and world atmosphere (`project_ix.gdshader`).
- **Complete Narrative & UI Flow:** Includes an in-game dialogue system (`dialogue_manager.cpp`), HUD, pause/game-over states, ending cutscene, and credits sequence.

---

### Tech Stack & Repository Structure

| Component | Technology |
| :--- | :--- |
| **Game Engine** | Godot Engine 4 |
| **Gameplay & AI** | C++ (`96.8%`) |
| **Visual Effects** | GDShader (`3.2%`) |
| **Scenes & Audio** | Godot Scene Tree (`.tscn`), Audio Bus Layout (`.tres`) |

```text
ProjectIX/
├── scenes/                  # Level, entity, HUD, menu, and cutscene .tscn files
├── scripts/                 # C++ gameplay/AI classes and custom .gdshader files
├── default_bus_layout.tres  # Master, BGM, and SFX audio bus configuration
├── export_presets.cfg       # Windows Desktop export preset configuration
└── project.godot            # Godot 4 project configuration
# Note: assets/ is gitignored to protect licensed third-party assets.
```

---

### Running Locally (Reference)

1. Install **Godot Engine 4** and a compatible C++ compiler toolchain (such as MSVC on Windows or GCC/Clang on Linux/macOS) to compile the C++ extension binaries.
2. Clone this repository and place placeholder sprites, tilemaps, fonts, and audio files inside a local `assets/` directory matching the scene resource paths.
3. Open `project.godot` in the Godot Project Manager, build the C++ library target, and press **F5** to launch.
