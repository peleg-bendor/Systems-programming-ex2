# My Copy - File Copy Using System Calls

A file copy utility written in C that uses **only Linux system calls** (no standard C library file functions).

---

## Description

This program copies the contents of a source file to a destination file using low-level Linux system calls (`open`, `read`, `write`, `close`, `access`).

**Key Features:**
- [x] Pure system calls - no `fopen()`, `fread()`, `fwrite()`, etc.
- [x] Efficient buffer-based copying (4 KB chunks)
- [x] Checks if destination file exists before overwriting
- [x] Comprehensive error handling
- [x] User confirmation for overwrite operations

---

## Compilation

### Using Make (recommended):
```bash
make
```

### Manual compilation:
```bash
gcc -Wall -Wextra -Werror -std=c99 my_copy.c -o my_copy
```

---

## Usage

```bash
./my_copy <source_file> <destination_file>
```

### Examples:

**Copy a file:**
```bash
./my_copy document.txt backup.txt
```

**If destination exists, you'll be prompted:**
```
Destination file 'backup.txt' already exists. Copying will overwrite it. Continue? (y/n): 
```
- Type `y` to proceed with overwrite
- Type `n` to cancel the operation

---

## System Calls Used

| System Call | Purpose |
|-------------|---------|
| `open()` | Open/create files |
| `read()` | Read data from source file |
| `write()` | Write data to destination file, output messages |
| `close()` | Close file descriptors |
| `access()` | Check if destination file exists |

**No standard library file I/O functions are used.**

---

## Testing

### Test 1: Basic copy
```bash
echo "Hello World" > test.txt
./my_copy test.txt copy.txt
cat copy.txt
# Output: Hello World
```

### Test 2: Overwrite existing file (say yes)
```bash
echo "Old content" > existing.txt
./my_copy test.txt existing.txt
# Type 'y' when prompted
cat existing.txt
# Output: Hello World
```

### Test 3: Cancel overwrite (say no)
```bash
./my_copy test.txt existing.txt
# Type 'n' when prompted
# Copy cancelled by user.
```

### Test 4: Error handling - missing source
```bash
./my_copy nonexistent.txt output.txt
# Error: Cannot open source file 'nonexistent.txt'
```

### Test 5: Error handling - wrong arguments
```bash
./my_copy
# Usage: ./my_copy <source_file> <destination_file>
```

---

## Technical Details

### Buffer Size
- **4096 bytes (4 KB)**
- Chosen because it matches the typical page size in Linux systems
- Balances memory usage with number of system calls
- Efficient for both small and large files

### Error Handling
Every system call is checked for errors (`return -1`). The program provides clear error messages to `stderr` and exits with appropriate error codes.

### File Permissions
New files are created with permissions `0644` (rw-r--r--):
- Owner: read + write
- Group: read only
- Others: read only

---

## Clean Up

Remove compiled files:
```bash
make clean
```

---

## Project Structure

```
ex2/
├── my_copy.c      # Source code (heavily commented)
├── Makefile       # Build configuration
├── README.md      # This file
└── my_copy        # Compiled executable (created by make)
```

---

## Assignment Requirements Met

- [x] Uses only system calls (no standard library file I/O)
- [x] Accepts 2 command-line arguments (source, destination)
- [x] Validates input arguments
- [x] Checks if source file exists and is readable
- [x] Checks if destination file exists
- [x] Prompts user before overwriting existing files
- [x] Validates user input (y/n)
- [x] Efficient buffer-based copying (4 KB)
- [x] Comprehensive error handling
- [x] Extensive code comments
- [x] Working Makefile
- [x] Complete documentation

---

## Repository

GitHub: https://github.com/peleg-bendor/Systems-programming-ex2
