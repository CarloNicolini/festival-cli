# Festival Sokoban solver

This project is a modern reimplementation and fork of the original [Festival solver](https://festival-solver.site/), with added support for Sokoban level descriptions in LURD (Left, Up, Right, Down) format.
It saves the solution file in json format as

```json
{
  "filename": "microban_I_000.sok",
  "map": "####\n# .#\n#  ###\n#*@  #\n#  $ #\n#  ###\n####",
  "lurd": "rdddlUrrrdLullddrUluRuulDrddrruLdlUU",
  "elapsed": "0.01"
}
```

---

## What is LURD Notation?

**LURD** is a standard notation used in Sokoban solvers and level editors to represent the sequence of moves made by the player to solve a puzzle. The acronym stands for the four possible movement directions:

- **L** = Left
- **U** = Up
- **R** = Right
- **D** = Down

### How LURD is Used

- Each letter represents a single step or action by the player.
- **Lowercase** letters (`l`, `u`, `r`, `d`) indicate a move where the player walks without pushing a box.
- **Uppercase** letters (`L`, `U`, `R`, `D`) indicate a move where the player pushes a box in that direction.

### Example

Suppose the solution string is: `uurUUL`

- `u` = move up (no push)
- `u` = move up (no push)
- `r` = move right (no push)
- `U` = push box up
- `U` = push box up
- `L` = push box left

### In the Context of Festival/fess-cli

When you run `fess-cli` and it outputs a solution like `uuuuUUUUU`, it means:
- The player moves up 4 times (`uuuu`) to get behind the box.
- Then pushes the box up 5 times (`UUUUU`) to the goal.

This notation is widely used because it's compact, human-readable, and easy to parse for automated tools and level editors.

**Summary:**  
LURD is a string of letters (L, U, R, D, and their lowercase forms) that encodes the sequence of player moves and pushes in a Sokoban solution.

---

## Compilation

To compile the Festival solver, you'll need CMake and a C++ compiler (like GCC or Clang).

```bash
mkdir build
cd build
cmake ..
make
```

This will create two executables in the `build` directory:
- `fess`: The original Festival solver with its GUI (if supported by your system).
- `fess-cli`: A command-line interface for the solver, suitable for systems without a GUI.

## Usage

### `fess-cli` (Command-Line Interface)

The `fess-cli` executable allows you to solve Sokoban levels from the command line.

**Basic Usage:**

```bash
./build/fess-cli <input_file> [options]
```

**Options:**

- `-t <seconds>`: Set the time limit for the solver in seconds (default: 60 seconds).
- `-v`: Enable verbose output, showing more details about the solving process.
- `-o <file>`: Save the solution to a specified output file.

**Example:**

To solve a level from `my_level.sok` with a 30-second time limit and verbose output:

```bash
./build/fess-cli -t 30 -v my_level.sok
```

The solution will be printed to the console.
