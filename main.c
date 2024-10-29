#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VAR_NAME_SIZE 256
int verbose = 0;

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
        if (line[0] == '#' || line[0] == '\n') {
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
                /* create_file(file_name, size); */
                if (verbose) {
                    printf("CREATE: File %s created with size %lu\n", file_name, size);
                }
            } else {
                fprintf(stderr, "Error: Invalid CREATE format\n");
            }
        } else if (strcmp(command, "WRITE") == 0) {
            if (sscanf(line, "%*s %s %lu %s", file_name, &offset, data) == 3) {
                /* write_file(file_name, offset, data); */
                if (verbose) {
                    printf("WRITE: %s written to file %s at offset %lu\n", data, file_name, offset);
                }
            } else {
                fprintf(stderr, "Error: Invalid WRITE format\n");
            }
        } else if (strcmp(command, "READ") == 0) {
            if (sscanf(line, "%*s %s %lu %lu", file_name, &offset, &size) == 3) {
                /* read_file_content(file_name, offset, size); */
                if (verbose) {
                    printf("READ: %lu bytes read from file %s starting at offset %lu\n", size, file_name, offset);
                }
            } else {
                fprintf(stderr, "Error: Invalid READ format\n");
            }
        } else if (strcmp(command, "DELETE") == 0) {
            if (sscanf(line, "%*s %s", file_name) == 1) {
                /* delete_file(file_name); */
                if (verbose) {
                    printf("DELETE: File %s deleted\n", file_name);
                }
            } else {
                fprintf(stderr, "Error: Invalid DELETE format\n");
            }
        } else if (strcmp(command, "LIST") == 0) {
            /* list_files(); */
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
