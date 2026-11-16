# Deviations from the SRS and SADS

**Project:** Snake Game · UE23CS341A Software Engineering, PES University
**Compared against:** SRS v1.0 (27/09/2026), Software Architecture and Design Specification v1.0 (28/09/2026)
**Implementation:** `features/US19_validate_portability_and_architecture.cpp` (complete Sprint 2 build, US-01 to US-19)

This document lists every place where the finished game differs from the original requirements and design documents, and why. Most entries resolve an open issue from SADS Appendix B; the rest are decisions taken during Sprint 2 that meet a requirement better. None removes a required behaviour.

| # | Original document said | Implemented | Reason | Reference |
|---|---|---|---|---|
| 1 | SFML 2.6 or later | SFML 3.x | "2.6 or later" permits it. SFML 3 is the current release and its event API (`std::optional`-based) is safer. | SRS 2.5, 2.7 |
| 2 | Window 640×480 | Window 640×620 | A 20×20 board of 24 px cells already takes 480 px; the SCORE / BEST cards need space above it. SRS updated to v2.0. | SRS 3.1 · DEF-03 |
| 3 | GameEngine calls HighScoreManager (High Score Service required by GameEngine) | GameController calls HighScoreManager; GameEngine contains no file code | Keeps the domain layer free of I/O so the game rules can be tested without touching disk. File access still happens only at session boundaries and only through HighScoreManager. | SADS 2.2, Fig. 1 |
| 4 | Session states RUNNING, PAUSED, GAME_OVER | INTRO state added | The snake must not move while the 3-second control instructions are on screen. | SADS Appendix B #2 · NFR-5 |
| 5 | Exit only from Game Over or main menu, with Esc or Q | Esc or window close button exits from any state; Q not used | Blocking Esc mid-game is poor usability. The exit hook saves the high score first, so nothing is lost. Q is next to W/A/S/D and risks accidental quits. | FR-12 · SADS 3.3.2 · Appendix B #3, #6 |
| 6 | Restart only from the Game Over screen | Restart also allowed while paused | Common player expectation. Still blocked while running, so a stray R cannot wipe a run; the abandoned run's score is saved first. | FR-11 |
| 7 | While paused, only the pause key is processed | While paused: pause, restart, help and exit keys work; direction keys are ignored | Keeps the intent (no gameplay input while paused) while letting the player quit or read help. | FR-10 · SRS 5.5 |
| 8 | Main menu referenced but not specified | No main menu; intro screen at launch and a help overlay (H / F1) | Meets NFR-5 without inventing an unspecified menu. | SADS Appendix B #1 |
| 9 | Behaviour on a full board undefined | Session ends with a "YOU WIN" screen | The only sensible outcome; verified by the US-16 autopilot test that fills all 400 cells. | SADS 3.4 · Appendix B #4 |
| 10 | High score updated at Game Over | High score finalised at the end of every session: game over, restart from pause, and exit | FR-14 says "each session"; a run ended by quitting is still a session. | FR-14 |
| 11 | `TickResult` struct returned by `tick()`; `getState()` | `tick()` returns `bool` (session ended); state read through `GameSnapshot` / `world()` | Same information through a simpler interface. SADS 3.3 allows types to change provided behaviour is unchanged. | SADS 3.3.1, 3.3.4 |
| 12 | Reject a reversing direction | Reject reversals and also queue up to 3 direction presses | Quick taps inside one 150 ms tick were being lost. The bounded queue keeps them; each is still checked against the previous move. | FR-4 · SEC-3 · US-17 |
| 13 | Logic, rendering and input in separate modules | Separate classes (GameEngine, Renderer, InputHandler, GameController, HighScoreManager) inside one file per user story | Every story file must compile on its own for the per-story demos. Separation is by class and layer; splitting into header/source files is a mechanical next step. | NFR-7 |
| 14 | Not specified | Added: help overlay (H/F1), performance panel (F3), "NEW!" badge on the BEST card, auto-pause when the window loses focus | Support NFR-5 (usability) and give live evidence for NFR-1/NFR-2. No required behaviour changed. | NFR-1, NFR-2, NFR-5 |
| 15 | High score is any non-negative integer up to 999,999 | Values with more than 6 characters (e.g. `0000001`) are rejected | The game never writes leading zeros; a stricter check is safer against tampering. | SEC-2 · DEF-02 |
| 16 | Tick interval and grid size "adjustable / configurable" | Adjustable in code only (`TickTimer::setInterval`, clamped to 80–300 ms; `GameGrid(cols, rows)`); the game uses 20×20 and 150 ms | Matches the SRS wording "adjustable in code"; no run-time setting was required. | FR-1, FR-5 · DEF-04, DEF-05 |

## Open SADS issues and how they were closed

| SADS Appendix B issue | Decision |
|---|---|
| 1. Main menu | Not built; intro screen + help overlay instead (#8) |
| 2. Snake movement during the 3-second instructions | Snake waits; INTRO state (#4) |
| 3. Exit during play / close button | Allowed from any state, high score saved first (#5) |
| 4. Full board | Player wins, session ends (#9) |
| 5. High score file format and location | Plain-text integer in `highscore.dat` in the working directory; warnings to stderr (as proposed) |
| 6. Final key bindings | Arrows/WASD move, P/Space pause, R restart, H/F1 help, F3 performance, Esc quit |
