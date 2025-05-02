# Linux System-Wide FD Table Monitor

## 📌 Overview
This C-based Linux utility explores how the operating system tracks open files. It extracts and displays various views of file descriptor data using the `/proc` virtual filesystem. Users can generate per-process, system-wide, vnode-based, and composite FD tables, save them to text or binary files, and evaluate performance based on output format.

## 👤 Author
**Abd-Ur-Rehman**  

## 🎯 Features
- View per-process FD tables
- Generate system-wide FD tables with filenames
- Display vnode-based tables using inode numbers
- Combine all views into a composite table
- Show summary of FD counts per process
- Highlight processes exceeding FD threshold
- Save composite table to:
  - Plain text file
  - Binary file with custom serialization
- Performance comparison between output formats

## 🧠 Approach
- **Command-line interface** to select views and output options
- Reads from `/proc/[pid]/fd` to extract FD metadata
- Uses:
  - `readlink()` for filename resolution
  - `stat()` for inode retrieval
  - `opendir()` and `readdir()` for directory traversal
- Modular function design for each table and file type
- Composite view combines PID, FD, filename, and inode data
- Output can be redirected to screen, text file, or binary file


## 🛠️ Compilation Instructions
Run on a Linux system using:

```bash
gcc -Wall -Wextra -std=c99 -Werror -g -o [FILENAME] [FILENAME].c
```

## 🚩 Supported Command-Line Arguments

### 🔹 Flagged Arguments

- `--per-process`: Show only the process FD table.
- `--systemWide`: Show only the system-wide FD table.
- `--Vnodes`: Show only the Vnodes FD table.
- `--composite`: Show only the composed FD table.
- `--summary`: Show a summary of the number of FDs open per process.
- `--threshold=X`: Show only processes with more than `X` FDs open.

## 📋 Command-Line Usage Examples

| Command | Description |
|--------|-------------|
| `./A2` | Displays the composite table by default |
| `./A2 1234 --per-process` | Shows FD table for PID 1234 |
| `./A2 --systemWide` | Displays system-wide FD table |
| `./A2 --threshold=2` | Lists processes with >2 FDs |
| `./A2 --output_TXT` | Saves composite table to `compositeTable.txt` |
| `./A2 1234 --output_binary` | Saves PID 1234’s composite table to `compositeTable.bin` |

## 🧪 Testing
Tested various combinations of command-line arguments including:
- Valid/invalid PIDs
- All flags together
- File-saving with invalid file paths
- Thresholds <= 0 and > 0

## 📊 Performance Evaluation (Bonus)
| Case | Avg Time (s) | Text Size (bytes) | Binary Size (bytes) |
|------|--------------|-------------------|----------------------|
| All Processes | 0.023 | 11262 | 9238 |
| Single PID | 0.0166 | 2122.8 | 1595.8 |

### Observations
- **Binary output** saves more space than text
- **Processing time** is dependent on data size, not output format
- Output is **consistent and reliable**

## ⚠️ Disclaimers
- Table formats follow Prof. Marcelo’s demo specifications
- Tables for specific PIDs exclude row indices
- Composite and threshold tables maintain consistent output order
- `#define _GNU_SOURCE` is used to access `DT_LNK` and `DT_DIR`

## 📚 References
- [snprintf - GeeksforGeeks](https://www.geeksforgeeks.org/snprintf-c-library/)
- [define _GNU_SOURCE - StackOverflow](https://stackoverflow.com/questions/5582211/what-does-define-gnu-source-imply)
- [PATH_MAX in Linux - StackOverflow](https://stackoverflow.com/questions/9449241/where-is-path-max-defined-in-linux)
- [Standard Deviation Calculator](https://www.calculator.net/standard-deviation-calculator.html)

