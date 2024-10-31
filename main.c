#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#define BLOCK_SIZE 512
#define MAX_BLOCKS 1024
#define MAX_FILES 100
#define VAR_NAME_SIZE 100

// Global variable for verbose mode
bool verbose = false;

// ANSI color codes for colored output
#define COLOR_RESET   "\x1b[0m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_RED     "\x1b[31m"

// Function to print verbose messages with color
void vprint(const char *format, ...) {
    if (verbose) {
        va_list args;
        printf(COLOR_GREEN "[INFO] " COLOR_RESET); // Green colored [INFO]
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
}

// Structure for a file entry in the directory
typedef struct {
    char filename[VAR_NAME_SIZE];
    int size;
    int start_block;
} FileEntry;

// Structure for a block in the file system
typedef struct {
    char data[BLOCK_SIZE];
} Block;

// File system structure stored in a file
typedef struct {
    FileEntry directory[MAX_FILES]; // File table
    Block blocks[MAX_BLOCKS]; // Blocks of data
    int used_blocks[MAX_BLOCKS];
} FileSystem;

// Global file system
FileSystem fs;

// Save the file system to a file
void save_fs(const char *fs_file) {
    FILE *file = fopen(fs_file, "wb");
    if (file == NULL) {
        printf(COLOR_RED "Error: Could not open file %s for writing\n" COLOR_RESET, fs_file);
        return;
    }
    fwrite(&fs, sizeof(FileSystem), 1, file);
    fclose(file);
    vprint("File system saved to '%s'\n", fs_file);
}

// Load the file system from a file
void load_fs(const char *fs_file) {
    FILE *file = fopen(fs_file, "rb");
    if (file == NULL) {
        printf(COLOR_RED "File system not found. Initializing a new one...\n" COLOR_RESET);
        memset(&fs, 0, sizeof(FileSystem));
        return;
    }
    fread(&fs, sizeof(FileSystem), 1, file);
    fclose(file);
    vprint("File system loaded from '%s'\n", fs_file);
}

// Create a new file with a name uniqueness check
void create_file(const char *filename, int size) {
    // Check if a file with the same name already exists
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(fs.directory[i].filename, filename) == 0 && fs.directory[i].size > 0) {
            printf(COLOR_RED "Error: A file with the name '%s' already exists.\n" COLOR_RESET, filename);
            return;
        }
    }

    int blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int start_block = -1;

    // Find free blocks
    for (int i = 0; i < MAX_BLOCKS - blocks_needed; i++) {
        int free_blocks = 0;
        for (int j = 0; j < blocks_needed; j++) {
            if (fs.used_blocks[i + j] == 0) {
                free_blocks++;
            }
        }
        if (free_blocks == blocks_needed) {
            start_block = i;
            break;
        }
    }

    if (start_block == -1) {
        printf(COLOR_RED "Error: Not enough space to create the file.\n" COLOR_RESET);
        return;
    }

    // Mark blocks as used
    for (int i = 0; i < blocks_needed; i++) {
        fs.used_blocks[start_block + i] = 1;
    }

    // Add the file to the directory
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.directory[i].size == 0) {  // Look for an empty slot
            strcpy(fs.directory[i].filename, filename);
            fs.directory[i].size = size;
            fs.directory[i].start_block = start_block;
            vprint("File '%s' created with size %d bytes.\n", filename, size);
            return;
        }
    }

    printf(COLOR_RED "Error: Directory full. Cannot create more files.\n" COLOR_RESET);
}

// Write data to a file, writing up to the file's limit if necessary
void write_file(const char *filename, int offset, const char *data) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(fs.directory[i].filename, filename) == 0) {
            int file_size = fs.directory[i].size;
            
            if (offset >= file_size) {
                printf(COLOR_RED "Error: Offset exceeds file size for '%s'.\n" COLOR_RESET, filename);
                return;
            }

            int data_len = strlen(data);
            int write_len = (offset + data_len > file_size) ? file_size - offset : data_len;

            if (write_len < data_len) {
                printf(COLOR_RED "Warning: Only %d bytes written, remaining data exceeds file size.\n" COLOR_RESET, write_len);
            }

            int block_idx = fs.directory[i].start_block + offset / BLOCK_SIZE;
            int block_offset = offset % BLOCK_SIZE;

            // Write the allowed portion of data to the file
            strncpy(fs.blocks[block_idx].data + block_offset, data, write_len);
            vprint("Written to file '%s': %.*s\n", filename, write_len, data);
            return;
        }
    }
    printf(COLOR_RED "Error: File '%s' not found.\n" COLOR_RESET, filename);
}

// Read data from a file, reading up to the file's limit if necessary
void read_file_content(const char *filename, int offset, int size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(fs.directory[i].filename, filename) == 0) {
            int file_size = fs.directory[i].size;

            if (offset >= file_size) {
                printf(COLOR_RED "Error: Offset exceeds file size for '%s'.\n" COLOR_RESET, filename);
                return;
            }

            int read_len = (offset + size > file_size) ? file_size - offset : size;

            if (read_len < size) {
                printf(COLOR_RED "Warning: Only %d bytes read, remaining data exceeds file size.\n" COLOR_RESET, read_len);
            }

            int block_idx = fs.directory[i].start_block + offset / BLOCK_SIZE;
            int block_offset = offset % BLOCK_SIZE;

            // Read the allowed portion of data from the file
            char buffer[BLOCK_SIZE];
            strncpy(buffer, fs.blocks[block_idx].data + block_offset, read_len);
            buffer[read_len] = '\0';
            printf("Read from file '%s': %s\n", filename, buffer);
            return;
        }
    }
    printf(COLOR_RED "Error: File '%s' not found.\n" COLOR_RESET, filename);
}


// Delete a file
void delete_file(const char *filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (strcmp(fs.directory[i].filename, filename) == 0) {
            int blocks_to_free = (fs.directory[i].size + BLOCK_SIZE - 1) / BLOCK_SIZE;

            // Free the blocks
            for (int j = 0; j < blocks_to_free; j++) {
                fs.used_blocks[fs.directory[i].start_block + j] = 0;
            }

            // Remove the file from the directory
            fs.directory[i].size = 0;
            memset(fs.directory[i].filename, 0, sizeof(fs.directory[i].filename));
            vprint("File '%s' deleted.\n", filename);
            return;
        }
    }
    printf(COLOR_RED "Error: File '%s' not found.\n" COLOR_RESET, filename);
}

// List all files with a table format and show free space sections (including internal block space)
void list_files() {
    // Print the table of files
    printf("\n");
    printf("Listing all files in the directory:\n");
    printf("+-----------------------+--------------+------------+\n");
    printf("| Filename              | Size (bytes) | Start Block|\n");
    printf("+-----------------------+--------------+------------+\n");

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.directory[i].size > 0) {
            printf("| %-21s | %-12d | %-10d |\n", fs.directory[i].filename, fs.directory[i].size, fs.directory[i].start_block);
        }
    }

    printf("+-----------------------+--------------+------------+\n");

    // Now we check the free block sections
    printf("\nFree space sections:\n");
    printf("+----------------+----------------+\n");
    printf("| Start Block    | End Block       |\n");
    printf("+----------------+----------------+\n");

    int free_start = -1;
    int free_end = -1;
    bool in_free_section = false;

    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (fs.used_blocks[i] == 0 && !in_free_section) {
            // Start of a new free section
            free_start = i;
            in_free_section = true;
        } else if (fs.used_blocks[i] == 1 && in_free_section) {
            // End of a free section
            free_end = i - 1;
            printf("| %-14d | %-14d |\n", free_start, free_end);
            in_free_section = false;
        }
    }

    // If we ended in a free section, print the remaining part
    if (in_free_section) {
        free_end = MAX_BLOCKS - 1;
        printf("| %-14d | %-14d |\n", free_start, free_end);
    }

    printf("+----------------+----------------+\n");

    // Checking for partially free space within used blocks
    printf("\nPartial free space within used blocks:\n");
    printf("+----------------+--------------------+----------------------+\n");
    printf("| Block          | Used By File       | Free Space (bytes)    |\n");
    printf("+----------------+--------------------+----------------------+\n");

    for (int i = 0; i < MAX_FILES; i++) {
        if (fs.directory[i].size > 0) {
            int last_block_index = fs.directory[i].start_block + (fs.directory[i].size / BLOCK_SIZE);
            int last_block_used_space = fs.directory[i].size % BLOCK_SIZE;
            
            if (last_block_used_space > 0 && last_block_used_space < BLOCK_SIZE) {
                int free_space_in_last_block = BLOCK_SIZE - last_block_used_space;
                printf("| %-14d | %-18s | %-20d |\n", last_block_index, fs.directory[i].filename, free_space_in_last_block);
            }
        }
    }

    printf("+----------------+--------------------+----------------------+\n");
}

// Function to process file system commands from an input file
void process_commands_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, COLOR_RED "Error: Could not open file %s\n" COLOR_RESET, filename);
        exit(EXIT_FAILURE);
    }

    char line[512];         // Buffer for reading lines from the file
    char command[10];       // To store the command name
    char file_name[VAR_NAME_SIZE]; // To store the file name
    size_t size, offset;    // Variables for size and offset
    char data[512];         // Buffer for data to be written or read

    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r' || (line[0] == '\r' && line[1] == '\n')) {
            continue;
        }

        // Parse the command
        if (sscanf(line, "%s", command) != 1) {
            fprintf(stderr, COLOR_RED "Error: Invalid instruction format\n" COLOR_RESET);
            continue;
        }

        // Handle each command
        if (strcmp(command, "CREATE") == 0) {
            if (sscanf(line, "%*s %s %lu", file_name, &size) == 2) {
                create_file(file_name, size);
            } else {
                fprintf(stderr, COLOR_RED "Error: Invalid CREATE format\n" COLOR_RESET);
            }
        } else if (strcmp(command, "WRITE") == 0) {
            if (sscanf(line, "%*s %s %lu \"%[^\"]\"", file_name, &offset, data) == 3) {
                write_file(file_name, offset, data);
            } else {
                fprintf(stderr, COLOR_RED "Error: Invalid WRITE format\n" COLOR_RESET);
            }
        } else if (strcmp(command, "READ") == 0) {
            if (sscanf(line, "%*s %s %lu %lu", file_name, &offset, &size) == 3) {
                read_file_content(file_name, offset, size);
            } else {
                fprintf(stderr, COLOR_RED "Error: Invalid READ format\n" COLOR_RESET);
            }
        } else if (strcmp(command, "DELETE") == 0) {
            if (sscanf(line, "%*s %s", file_name) == 1) {
                delete_file(file_name);
            } else {
                fprintf(stderr, COLOR_RED "Error: Invalid DELETE format\n" COLOR_RESET);
            }
        } else if (strcmp(command, "LIST") == 0) {
            list_files();
        } else {
            fprintf(stderr, COLOR_RED "Error: Unknown command %s\n" COLOR_RESET, command);
        }
    }

    fclose(file);
}

// Main function to handle command-line arguments and initiate the file system
int main(int argc, char *argv[]) {
    const char *fs_file = "filesystem.bin";  // File system storage file
    const char *commands_file = NULL;        // Command file to be passed via command line

    // Check for the -v flag for verbose mode and the command file
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
            vprint("Verbose mode enabled.\n");
        } else {
            commands_file = argv[i];  // Treat any non-flag argument as the command file
        }
    }

    // Ensure a command file is provided
    if (commands_file == NULL) {
        printf(COLOR_RED "Error: No command file specified. Usage: ./program [-v] <commands_file>\n" COLOR_RESET);
        return 1;
    }

    // Load file system from file
    load_fs(fs_file);

    // Simulate file system operations from the specified command file
    process_commands_from_file(commands_file);

    // Save file system to file
    save_fs(fs_file);

    return 0;
}

