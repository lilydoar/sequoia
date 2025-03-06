# Game Engine Development Curriculum

## Foundation: Pong
Pong is an excellent starting point. It covers fundamental systems while remaining manageable.

**Core Engine Components:**
- Window creation and management
- Game loop architecture 
- Input handling
- Simple collision detection
- Basic 2D rendering
- Audio playback
- Game state management

## Project 2: Breakout
This builds on Pong while introducing new challenges.

**New Engine Components:**
- Particle systems (for brick destruction effects)
- More complex collision detection
- Power-up system (foundation for item/buff systems)
- Simple UI for score/lives display
- Level loading from data files

## Project 3: Tetris
Introduces grid-based gameplay and more complex state management.

**New Engine Components:**
- Grid-based collision systems
- More advanced input handling (rotation, hold mechanics)
- Ghost piece preview (rendering overlays)
- Score/level progression systems
- Menu system improvements
- Game state serialization (high scores)

## Project 4: Platformer (Mario-like)
Now we're moving to more complex game mechanics and world representation.

**New Engine Components:**
- Tile-based world representation
- Camera systems (scrolling)
- Sprite animation system
- Platformer physics (jumping, gravity)
- Enemy AI pathfinding
- Level editor foundations

## Project 5: Simple RPG (Zelda-like)
Introduces more complex world interaction and saving/loading.

**New Engine Components:**
- Expanded world representation (multi-room/screen)
- Inventory systems
- Dialog system
- NPC interaction
- Save/load game state
- Resource management
- Quest tracking systems

## Project 6: Turn-based Strategy (Advance Wars-like)
Challenges you with more complex UI and systemic interactions.

**New Engine Components:**
- Advanced grid systems (terrain effects, movement costs)
- Turn-based mechanics
- Unit management
- Tactical AI
- Complex UI layouts
- Multi-state game flows
- Improved editor tools for map creation

## Project 7: Procedural Roguelike
Introduce procedural generation and complex interconnected systems.

New Engine Components:
- Procedural level generation
- Dungeon connectivity algorithms
- Advanced AI with behavior trees
- Item/equipment system with procedural properties
- Permadeath and persistent progression systems
- Complex interaction between multiple systems (environment, items, enemies)
- Lighting and fog-of-war systems

## Implementation Strategy

For each project:
1. Research similar games to understand core mechanics
2. Define minimal feature set to create a playable prototype
3. Implement game-specific features
4. Refactor reusable components into your engine
5. Document API and usage patterns
6. Review and optimize before moving to next project

## Focus Areas Across Projects

1. **UI Layout & Interaction**
   - Start with simple menus in Pong
   - Add complexity in Tetris (game over screens, setup screens)
   - Fully develop in the Strategy game (unit info panels, contextual menus)

2. **Saving/Loading**
   - Begin with high scores in Tetris
   - Full game state in the RPG
   - Complex multi-save systems in the Strategy game

3. **Text Rendering**
   - Score display in early games
   - Dialog system in RPG
   - Advanced formatting in Strategy game UI

4. **Sprite Animation**
   - Introduce in Platformer
   - Expand in RPG with more complex character animations
   - Potentially transition to 3D models in final project

5. **Dialogue Systems**
   - Simple tutorial text in early games
   - NPC conversations in RPG
   - Mission briefings in Strategy game

6. **Editors/Tools**
   - Level design tools for Platformer
   - World editor for RPG
   - Map editor with terrain and unit placement for Strategy game


1. **Procedural Content Generation**
   - Simple random level elements in Breakout
   - Random enemy placement in Platformer
   - Procedural item stats in RPG
   - Full procedural dungeons in Roguelike

1. **Performance Optimization**
   - Sprite batching in Platformer
   - Spatial partitioning in RPG
   - Entity component system refinement in Strategy game
   - Advanced memory management in Roguelike

2. **Debugging Tools**
   - Simple FPS counter and memory usage in early games
   - Visual collision debugging in Platformer
   - State inspection for complex entities in RPG
   - Full suite of visualization tools for AI and procedural generation in later projects

4. **Data-Driven Design**
   - Basic configuration files in early projects
   - Data-defined entities and behaviors in RPG
   - Complete separation of data and logic in Strategy game
   - Modding capabilities in Roguelike

5. **Advanced Physics**
   - Simple collision response in early games
   - More realistic physics in Platformer
   - Environmental interaction in RPG (pushing blocks, etc.)
   - Advanced physics for projectiles, explosions in later games

6. **Networking Fundamentals** (optional stretch goal)
   - Add simple leaderboards to earlier games
   - Basic multiplayer capabilities to Strategy game
   - Co-op mode for Roguelike

7. **Engine Architecture Refinement**
   - Begin with simple architecture
   - Gradually introduce better patterns (component systems, event handling)
   - Refine into a data-oriented design as you progress
   - Create proper API documentation and examples

8. **Portability**
   - Abstract platform-specific code early
   - Test on different platforms as you progress
   - Potentially target web export (via WebAssembly) for easier sharing
