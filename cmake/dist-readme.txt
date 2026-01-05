Bestow Game Engine - Windows x64 Distribution
=============================================

QUICK START
-----------
1. Extract this folder anywhere
2. Open a command prompt in this folder
3. Set the library path and run the example:

   set BESTOW_LIBRARY_PATH=%CD%\library
   bin\bestow.exe run examples\snake-lua\main.lua

   Or add 'bin' to your PATH for easier access.


FOLDER STRUCTURE
----------------
bin/          - Bestow executable and required DLLs
library/      - Engine assets (shaders, default textures, sounds)
template/     - Template files for 'bestow init' command
examples/     - Example games
  snake-lua/  - A complete snake game example


REQUIREMENTS
------------
- Windows 10/11 x64
- Vulkan-capable GPU with up-to-date drivers
- Visual C++ Redistributable 2022 (if not already installed)
  Download: https://aka.ms/vs/17/release/vc_redist.x64.exe


ENVIRONMENT VARIABLES
---------------------
BESTOW_LIBRARY_PATH  - Path to the library/ folder (shaders, default assets)
                       Set this before running if not in the distribution root.


COMMANDS
--------
bestow run <path/to/main.lua>   Run a game (include path to main.lua)
bestow init                     Initialize current directory as a new project
bestow new <name>               Create a new project in a new directory
bestow help                     Show all commands


RUNNING YOUR OWN GAMES
----------------------
1. Create a folder for your game with a main.lua
2. Run from the distribution root:

   set BESTOW_LIBRARY_PATH=%CD%\library
   bin\bestow.exe run path\to\your-game\main.lua


DOCUMENTATION
-------------
See template/CLAUDE.md for the full Lua API documentation and examples.


SUPPORT
-------
https://github.com/radical-beard/bestow
