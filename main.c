#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VAR_NAME_SIZE 256
#define BLOCK_SIZE 512
#define MAX_FILES 100
#define TOTAL_STORAGE (1024 * 1024) // 1 MB of simulated storage
#define MAX_BLOCKS (TOTAL_STORAGE / BLOCK_SIZE)

// Structure to represent a file entry in the file table
typedef struct {
    char name[VAR_NAME_SIZE];
    size_t size;
    int start_block;
    int num_blocks;
} FileEntry;

int verbose = 0;  // Verbose mode flag
FileEntry file_table[MAX_FILES];  // Array to hold file metadata
char storage[TOTAL_STORAGE];  // Simulated storage space
int block_map[MAX_BLOCKS];  // 0: free, 1: occupied for each block
int file_count = 0;  // Tracks the number of files in the system

// Function to create a new file in the system
void create_file(const char *name, size_t size) {
    // Check if file limit has been reached
    if (file_count >= MAX_FILES) {
        fprintf(stderr, "Error: Maximum number of files reached\n");
        return;
    }

    int required_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;  // Calculate needed blocks
    int start_block = -1;
    int found = 0;

    // Search for a sequence of free blocks
    for (int i = 0; i <= MAX_BLOCKS - required_blocks; i++) {
        found = 1;
        for (int j = 0; j < required_blocks; j++) {
            if (block_map[i + j] != 0) {
                found = 0;
                break;
            }
        }
        if (found) {
            start_block = i;
            break;
        }
    }

    // Handle insufficient space
    if (start_block == -1) {
        fprintf(stderr, "Error: Not enough space to create file %s\n", name);
        return;
    }

    // Mark blocks as occupied
    for (int i = 0; i < required_blocks; i++) {
        block_map[start_block + i] = 1;
    }

    // Create a file entry
    strncpy(file_table[file_count].name, name, VAR_NAME_SIZE);
    file_table[file_count].size = size;
    file_table[file_count].start_block = start_block;
    file_table[file_count].num_blocks = required_blocks;
    file_count++;

    if (verbose) {
        printf("CREATE: File %s created with size %lu bytes and %d blocks\n", name, size, required_blocks);
    }
}

// Function to write data to a file at a specific offset
void write_file(const char *name, size_t offset, const char *data) {
    int file_index = -1;

    // Find the file in the file table
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_table[i].name, name) == 0) {
            file_index = i;
            break;
        }
    }

    // Handle file not found error
    if (file_index == -1) {
        fprintf(stderr, "Error: File %s not found\n", name);
        return;
    }

    // Check for write overflow
    if (offset + strlen(data) > file_table[file_index].size) {
        fprintf(stderr, "Error: Write exceeds file size for %s\n", name);
        return;
    }

    // Write data to storage
    int start = file_table[file_index].start_block * BLOCK_SIZE + offset;
    strncpy(&storage[start], data, strlen(data));

    if (verbose) {
        printf("WRITE: Data written to file %s at offset %lu: %s\n", name, offset, data);
    }
}

// Function to read data from a file starting from a specific offset
void read_file_content(const char *name, size_t offset, size_t size) {
    int file_index = -1;

    // Find the file in the file table
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_table[i].name, name) == 0) {
            file_index = i;
            break;
        }
    }

    // Handle file not found error
    if (file_index == -1) {
        fprintf(stderr, "Error: File %s not found\n", name);
        return;
    }

    // Check for read overflow
    if (offset + size > file_table[file_index].size) {
        fprintf(stderr, "Error: Read exceeds file size for %s\n", name);
        return;
    }

    // Read data from storage
    int start = file_table[file_index].start_block * BLOCK_SIZE + offset;
    char buffer[512];
    strncpy(buffer, &storage[start], size);
    buffer[size] = '\0';

    printf("READ: Data read from file %s at offset %lu: %s\n", name, offset, buffer);
}

// Function to delete a file and free its blocks
void delete_file(const char *name) {
    int file_index = -1;

    // Find the file in the file table
    for (int i = 0; i < file_count; i++) {
        if (strcmp(file_table[i].name, name) == 0) {
            file_index = i;
            break;
        }
    }

    // Handle file not found error
    if (file_index == -1) {
        fprintf(stderr, "Error: File %s not found\n", name);
        return;
    }

    int start_block = file_table[file_index].start_block;
    int num_blocks = file_table[file_index].num_blocks;

    // Free occupied blocks
    for (int i = 0; i < num_blocks; i++) {
        block_map[start_block + i] = 0;
    }

    // Remove the file entry by shifting entries in the array
    for (int i = file_index; i < file_count - 1; i++) {
        file_table[i] = file_table[i + 1];
    }
    file_count--;

    if (verbose) {
        printf("DELETE: File %s deleted\n", name);
    }
}

// Function to list all files in the system
void list_files() {
    if (file_count == 0) {
        printf("No files in the system\n");
        return;
    }

    for (int i = 0; i < file_count; i++) {
        printf("%s - %lu bytes\n", file_table[i].name, file_table[i].size);
    }
}

void read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char line[512]; // Buffer for reading lines from the file
    char command[10]; // To store the command name
    char file_name[VAR_NAME_SIZE]; // To store the file name
    size_t size, offset; // Variables for size and offset
    char data[512]; // Buffer for data to be written or read

    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r' || (line[0] == '\r' && line[1] == '\n')) {
            // printf("Skipping comment or empty line\n");
            continue;
        }

        // Parse the command
        if (sscanf(line, "%s", command) != 1) {
            fprintf(stderr, "Error: Invalid instruction format\n");
            continue;
        }

        // Handle each command
        if (strcmp(command, "CREATE") == 0) {
            if (sscanf(line, "%*s %s %lu", file_name, &size) == 2) {
                create_file(file_name, size);
            } else {
                fprintf(stderr, "Error: Invalid CREATE format\n");
            }
        } else if (strcmp(command, "WRITE") == 0) {
            if (sscanf(line, "%*s %s %lu \"%[^\"]\"", file_name, &offset, data) == 3) {
                write_file(file_name, offset, data);
            } else {
                fprintf(stderr, "Error: Invalid WRITE format\n");
            }
        } else if (strcmp(command, "READ") == 0) {
            if (sscanf(line, "%*s %s %lu %lu", file_name, &offset, &size) == 3) {
                read_file_content(file_name, offset, size);
            } else {
                fprintf(stderr, "Error: Invalid READ format\n");
            }
        } else if (strcmp(command, "DELETE") == 0) {
            if (sscanf(line, "%*s %s", file_name) == 1) {
                delete_file(file_name);
            } else {
                fprintf(stderr, "Error: Invalid DELETE format\n");
            }
        } else if (strcmp(command, "LIST") == 0) {
            list_files();
        } else {
            fprintf(stderr, "Error: Unknown command %s\n", command);
        }
    }

    fclose(file);
}


int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <filename> [-v]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 3 && strcmp(argv[2], "-v") == 0) {
        verbose = 1;
    }

    read_file(argv[1]);

    return EXIT_SUCCESS;
}
