# Design Document: File Copy Using System Calls

## 1. Program Purpose and Requirements

### Purpose

The `my_copy` program is a file copy utility that copies contents from a source file to a destination file using **only Linux system calls** - no standard C library file I/O functions (`fopen`, `fread`, `fwrite`, etc.).

### Requirements Met

- [x] Accepts 2 command-line arguments (source and destination files)
- [x] Uses only system calls: `open()`, `read()`, `write()`, `close()`, `access()`
- [x] Checks if destination file exists and prompts user for confirmation
- [x] Validates user input (y/n) in a loop
- [x] Efficient buffer-based copying (4 KB chunks)
- [x] Comprehensive error handling for all system calls
- [x] Single-threaded implementation

---

## 2. System Calls Analysis

### System Calls Used

| System Call | Purpose                                                           |
| ----------- | ----------------------------------------------------------------- |
| `access()`  | Check if destination file exists                                  |
| `open()`    | Open source file for reading, create/open destination for writing |
| `read()`    | Read data from source file in chunks                              |
| `write()`   | Write data to destination file, print messages to stdout/stderr   |
| `close()`   | Close file descriptors                                            |

---

### 2.1 `access()` - File Existence Check

```c
if (access(dest_file, F_OK) == 0) {
    // File exists - prompt user
}
```

- **Flag used:** `F_OK` - checks if file exists
- **Return:** 0 if exists, -1 if not
- **Why used:** Need to know if destination exists before prompting user

---

### 2.2 `open()` - Opening/Creating Files

#### For Source File:

```c
int source_fd = open(source_file, O_RDONLY);
```

**Flags:**

- `O_RDONLY` - Open for reading only

**Why:** We only read from source, never write to it.

#### For Destination File:

```c
int dest_fd = open(dest_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
```

**Flags:**

- `O_WRONLY` - Open for writing only (we never read from destination)
- `O_CREAT` - Create file if it doesn't exist
- `O_TRUNC` - Truncate (empty) file if it already exists
    - Safe because we already got user confirmation to overwrite

**Mode (0644):**

- Owner: read + write (6 = rw-)
- Group: read only (4 = r--)
- Others: read only (4 = r--)

**Why these flags:**

- `O_WRONLY` is more efficient than `O_RDWR` when we only write
- `O_CREAT` allows creating new files
- `O_TRUNC` ensures old content is removed (after user confirms)
- `0644` is standard for text files

---

### 2.3 `read()` and `write()` - Data Transfer

```c
while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
    ssize_t bytes_written = write(dest_fd, buffer, bytes_read);
}
```

**`read()` returns:**

- Number of bytes read (may be less than requested)
- 0 when end of file (EOF) is reached
- -1 on error

**`write()` critical detail:**

- Must write exactly `bytes_read` bytes, **not** `BUFFER_SIZE`
- Last chunk may be smaller than buffer size
- Writing `BUFFER_SIZE` would include garbage data at the end

**`write()` for messages:**

- `write(STDOUT_FILENO, ...)` - Success messages
- `write(STDERR_FILENO, ...)` - Error messages

---

### 2.4 `close()` - Closing Files

```c
close(source_fd);
close(dest_fd);
```

**Why it's important:**

- Releases file descriptors (limited system resource)
- Flushes buffered data to disk
- Should be called even if errors occurred earlier

---

## 3. Buffer Management and Efficiency

### 3.1 Buffer Size Choice

**Selected size: 4096 bytes (4 KB)**

```c
#define BUFFER_SIZE 4096
```

### 3.2 Rationale

**1. Linux Page Size**

- Most Linux systems use 4 KB pages
- File operations are optimized for page-sized I/O
- Verification: `getconf PAGESIZE` returns 4096

**2. File System Block Size**

- Modern file systems (ext4, XFS) use 4 KB blocks
- Reading in block-sized chunks is most efficient

**3. System Call Overhead**

- System calls are expensive (context switch to kernel mode)
- Larger buffer = fewer system calls = better performance

**4. Memory Efficiency**

- 4 KB is small enough for stack allocation
- Larger buffers (>64 KB) would require heap allocation
- Fits comfortably in CPU cache

---

### 3.3 Efficiency Proof

**System calls needed for file of size S bytes:**

- Number of `read()` calls: ⌈S / 4096⌉
- Number of `write()` calls: ⌈S / 4096⌉
- **Total:** 2 × ⌈S / 4096⌉

**Examples:**

| File Size | System Calls with 4 KB Buffer | System Calls with 1 Byte Buffer | Improvement   |
| --------- | ----------------------------- | ------------------------------- | ------------- |
| 10 KB     | 6                             | 20,480                          | 3,413× faster |
| 1 MB      | 512                           | 2,097,152                       | 4,096× faster |
| 100 MB    | 51,200                        | 209,715,200                     | 4,096× faster |

**Conclusion:** 4 KB buffer reduces system calls by approximately **4,000 times** compared to byte-by-byte copying.

---

### 3.4 Comparison with Other Sizes

| Buffer Size | Pros                | Cons                          |
| ----------- | ------------------- | ----------------------------- |
| 512 B       | Low memory          | Too many system calls         |
| **4 KB**    | **Optimal balance** | **None**                      |
| 64 KB       | Fewer syscalls      | Wastes memory for small files |
| 1 MB        | Minimal syscalls    | Excessive memory usage        |

**4 KB is the sweet spot** for general-purpose file copying on Linux.

---

## 4. Error Handling

### 4.1 Error Detection Strategy

**Every system call is checked for errors** - all system calls return -1 on failure.

### 4.2 Errors Handled

**1. Invalid arguments:**

```c
if (argc != 3) {
    write(STDERR_FILENO, "Usage: ./my_copy <source> <destination>\n", ...);
    return 1;
}
```

**2. Cannot open source file:**

```c
if (source_fd == -1) {
    write(STDERR_FILENO, "Error: Cannot open source file\n", ...);
    return 1;
}
```

Causes: File doesn't exist, no read permission, path is directory

**3. Cannot create destination file:**

```c
if (dest_fd == -1) {
    write(STDERR_FILENO, "Error: Cannot create destination file\n", ...);
    close(source_fd);  // Clean up!
    return 1;
}
```

Causes: No write permission, disk full, read-only filesystem

**4. Read failure:**

```c
if (bytes_read == -1) {
    write(STDERR_FILENO, "Error: Failed to read from source\n", ...);
    close(source_fd);
    close(dest_fd);
    return 1;
}
```

Causes: I/O error, file deleted during copy

**5. Write failure:**

```c
if (bytes_written == -1) {
    write(STDERR_FILENO, "Error: Failed to write to destination\n", ...);
    close(source_fd);
    close(dest_fd);
    return 1;
}
```

Causes: **Disk full** (most common), I/O error, quota exceeded

**6. Incomplete write:**

```c
if (bytes_written != bytes_read) {
    write(STDERR_FILENO, "Error: Incomplete write\n", ...);
    close(source_fd);
    close(dest_fd);
    return 1;
}
```

Rare but must be checked for correctness.

**7. Close failures:**

```c
if (close(source_fd) == -1) { ... }
if (close(dest_fd) == -1) { ... }
```

Ensures buffered data was flushed to disk.

---

### 4.3 Error Handling Principles

1. **Check every system call** - No assumptions
2. **Clear error messages** - Include filename when relevant
3. **Write to stderr** - Errors go to stderr, not stdout
4. **Clean up resources** - Close open files before exiting
5. **Return error codes** - Exit with code 1 on errors, 0 on success

---

## 5. Design Decisions

### 5.1 Check Destination Before Opening Source

**Decision:** Use `access()` to check destination **before** opening source.

**Rationale:**

- If user will cancel, no point wasting a file descriptor on source
- Faster user experience - fail early if needed

---

### 5.2 Input Validation Loop

**Decision:** Keep asking until valid input ('y' or 'n').

```c
while (!valid_input) {
    // Read input
    if (response == 'y' || response == 'Y') { ... }
    else if (response == 'n' || response == 'N') { ... }
    else { /* Ask again */ }
}
```

**Rationale:**

- User-friendly: accepts both uppercase and lowercase
- Robust: doesn't crash on invalid input
- Prevents accidental overwrites

---

### 5.3 Custom `string_length()` Function

**Decision:** Implement our own string length function instead of using `strlen()`.

**Rationale:**

- Cannot use `strlen()` - it's from standard library (`<string.h>`)
- Assignment requires system calls only
- Needed for writing variable-length filenames in error messages

---

## 6. Testing

### Test Cases Verified

| Test                            | Expected Result         |
| ------------------------------- | ----------------------- |
| Normal copy to new file         | Success                 |
| Overwrite existing (answer 'y') | File overwritten        |
| Overwrite existing (answer 'n') | Copy cancelled          |
| Invalid input then 'y'          | Asks again, then copies |
| Missing source file             | Error message           |
| No arguments                    | Usage message           |
| Wrong number of arguments       | Usage message           |

---

## 7. Conclusion

The `my_copy` program successfully demonstrates efficient file copying using only Linux system calls. Key achievements:

- **Efficiency:** 4 KB buffer reduces system calls by ~4,000× compared to byte-by-byte copying
- **Safety:** User confirmation prevents accidental data loss
- **Robustness:** Comprehensive error handling for all edge cases
- **Compliance:** Uses only system calls as required

The implementation balances performance, code clarity, and user safety while meeting all assignment requirements.

