#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

typedef struct {
    char filename[256];
    long filesize;
} FileInfo;

// Function to create directories for a given path
void create_directories(const char *path) {
    char temp_path[512];
    char *p = NULL;
    
    // Copy the path to a temporary buffer
    strncpy(temp_path, path, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';
    
    // Process each directory level
    for (p = temp_path + 1; *p; p++) {
        if (*p == '/') {
            // Temporarily terminate the string at the slash
            *p = '\0';
            
            // Create directory if it doesn't exist
            #ifdef _WIN32
            if (mkdir(temp_path) != 0 && errno != EEXIST) {
            #else
            if (mkdir(temp_path, 0755) != 0 && errno != EEXIST) {
            #endif
                // Only warn if it's not "File exists" error
                if (errno != EEXIST) {
                    fprintf(stderr, "Warning: Failed to create directory '%s': %s\n", temp_path, strerror(errno));
                }
            }
            
            // Restore the slash
            *p = '/';
        }
    }
}

// Helper function to collect all files in a directory recursively
int collect_files_recursive(const char *path, char ***file_list, int *file_count) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return -1;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            return -1;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            if (collect_files_recursive(full_path, file_list, file_count) != 0) {
                closedir(dir);
                return -1;
            }
        }

        closedir(dir);
        return 0;
    } else {
        // It's a file, add it to the list
        *file_list = realloc(*file_list, (*file_count + 1) * sizeof(char *));
        if (!*file_list) {
            return -1;
        }

        (*file_list)[*file_count] = strdup(path);
        if (!(*file_list)[*file_count]) {
            return -1;
        }

        (*file_count)++;
        return 0;
    }
}

void create_archive(const char *archive_name, char *input_paths[], int path_count) {
    // Collect all files from the input paths
    char **file_list = NULL;
    int file_count = 0;

    for (int i = 0; i < path_count; i++) {
        if (collect_files_recursive(input_paths[i], &file_list, &file_count) != 0) {
            fprintf(stderr, "Warning: Failed to collect files from '%s'\n", input_paths[i]);
        }
    }

    if (file_count == 0) {
        fprintf(stderr, "Error: No files to archive\n");
        return;
    }

    FILE *archive = fopen(archive_name, "wb");
    if (!archive) {
        fprintf(stderr, "Error: Failed to create archive '%s': %s\n", archive_name, strerror(errno));
        // Free collected file list
        for (int i = 0; i < file_count; i++) {
            free(file_list[i]);
        }
        free(file_list);
        return;
    }

    // Write the number of files
    if (fwrite(&file_count, sizeof(int), 1, archive) != 1) {
        fprintf(stderr, "Error: Failed to write file count to archive\n");
        fclose(archive);
        // Free collected file list
        for (int i = 0; i < file_count; i++) {
            free(file_list[i]);
        }
        free(file_list);
        return;
    }

    int valid_files = 0;

    // Write information about files and contents
    for (int i = 0; i < file_count; i++) {
        FILE *input = fopen(file_list[i], "rb");
        if (!input) {
            fprintf(stderr, "Warning: Failed to open file '%s': %s\n", file_list[i], strerror(errno));
            continue;
        }

        // Get file size
        fseek(input, 0, SEEK_END);
        long filesize = ftell(input);
        fseek(input, 0, SEEK_SET);

        // Write file info
        FileInfo file_info;
        // Store the full path in the archive
        strncpy(file_info.filename, file_list[i], sizeof(file_info.filename) - 1);
        file_info.filename[sizeof(file_info.filename) - 1] = '\0';
        file_info.filesize = filesize;

        if (fwrite(&file_info, sizeof(FileInfo), 1, archive) != 1) {
            fprintf(stderr, "Error: Failed to write file info for '%s'\n", file_list[i]);
            fclose(input);
            continue;
        }

        // Read and write the file contents
        char *buffer = malloc(filesize);
        if (!buffer) {
            fprintf(stderr, "Error: Memory allocation failed for file '%s'\n", file_list[i]);
            fclose(input);
            continue;
        }

        if (fread(buffer, 1, filesize, input) != (size_t)filesize) {
            fprintf(stderr, "Error: Failed to read file '%s': %s\n", file_list[i], strerror(errno));
            free(buffer);
            fclose(input);
            continue;
        }

        if (fwrite(buffer, 1, filesize, archive) != (size_t)filesize) {
            fprintf(stderr, "Error: Failed to write file content for '%s': %s\n", file_list[i], strerror(errno));
            free(buffer);
            fclose(input);
            continue;
        }

        free(buffer);
        fclose(input);
        valid_files++;
    }

    fclose(archive);

    // Free collected file list
    for (int i = 0; i < file_count; i++) {
        free(file_list[i]);
    }
    free(file_list);

    if (valid_files == 0) {
        fprintf(stderr, "Error: No valid files to archive\n");
        // Remove the empty archive
        unlink(archive_name);
    } else {
        printf("Successfully created archive '%s' with %d files\n", archive_name, valid_files);
    }
}

void extract_archive(const char *archive_name) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        fprintf(stderr, "Error: Failed to open archive '%s': %s\n", archive_name, strerror(errno));
        return;
    }
    
    // Read the number of files in the archive
    int file_count;
    if (fread(&file_count, sizeof(int), 1, archive) != 1) {
        fprintf(stderr, "Error: Failed to read file count from archive: %s\n", strerror(errno));
        fclose(archive);
        return;
    }
    
    printf("Extracting %d files from archive...\n", file_count);
    
    // Read each file's information and contents
    for (int i = 0; i < file_count; i++) {
        FileInfo file_info;
        if (fread(&file_info, sizeof(FileInfo), 1, archive) != 1) {
            fprintf(stderr, "Error: Failed to read file info for entry %d: %s\n", i, strerror(errno));
            break;
        }
        
        // Create directories for the file path
        create_directories(file_info.filename);
        
        // Open the output file
        FILE *file = fopen(file_info.filename, "wb");
        if (!file) {
            fprintf(stderr, "Error: Failed to create file '%s': %s\n", file_info.filename, strerror(errno));
            // Skip file contents
            fseek(archive, file_info.filesize, SEEK_CUR);
            continue;
        }
        
        // Read and write the file contents
        char *buffer = malloc(file_info.filesize);
        if (!buffer) {
            fprintf(stderr, "Error: Memory allocation failed for file '%s'\n", file_info.filename);
            fclose(file);
            // Skip file contents
            fseek(archive, file_info.filesize, SEEK_CUR);
            continue;
        }
        
        if (fread(buffer, 1, file_info.filesize, archive) != (size_t)file_info.filesize) {
            fprintf(stderr, "Error: Failed to read file contents for '%s': %s\n", file_info.filename, strerror(errno));
            free(buffer);
            fclose(file);
            continue;
        }
        
        if (fwrite(buffer, 1, file_info.filesize, file) != (size_t)file_info.filesize) {
            fprintf(stderr, "Error: Failed to write file contents for '%s': %s\n", file_info.filename, strerror(errno));
            free(buffer);
            fclose(file);
            continue;
        }
        
        free(buffer);
        fclose(file);
        printf("Extracted: %s\n", file_info.filename);
    }
    
    fclose(archive);
    printf("Archive extraction completed successfully\n");
}

void printhelp(void) {
    printf("Copyright(c) 2025 diabloblanko\nUsage:\n"
           "  sac -c, --create, --make <archive> <files...>  Create an archive\n"
           "  sac -u, --unpack <archive>                     Extract an archive\n"
           "  sac -v                                         Print version\n"
           "  sac -h                                         Print this help\n"
           "\n"
           "Example:\n"
           "  sac -c archive.sac file1 file2 folder/\n"
           "  sac -u archive.sac\n");
}

void printver(void) {
    printf(" ____    _    ____\n"
           "/ ___|  / \\  / ___| Release: SacV2.0\n"
           "\\___ \\ / _ \\| |\n"
           " ___) / ___ \\ |___  Build: sacv2-cherryos\n"
           "|____/_/   \\_\\____|\n");
}

int main(int argc, char *argv[]) {
    int opt;
    int create_mode = 0;
    int unpack_mode = 0;
    const char *archive_name = NULL;
    
    // Parse short options
    while ((opt = getopt(argc, argv, "hvc:u:")) != -1) {
        switch (opt) {
            case 'h':
                printhelp();
                return 0;
            case 'v':
                printver();
                return 0;
            case 'c':
                create_mode = 1;
                archive_name = optarg;
                break;
            case 'u':
                unpack_mode = 1;
                archive_name = optarg;
                break;
            default:
                printhelp();
                return 1;
        }
    }
    
    // Check for long options
    if (!create_mode && !unpack_mode && optind < argc) {
        if (strcmp(argv[optind], "--create") == 0 || strcmp(argv[optind], "--make") == 0) {
            create_mode = 1;
            if (optind + 1 < argc) {
                archive_name = argv[optind + 1];
                optind += 2;
            } else {
                fprintf(stderr, "Error: No archive name specified\n");
                printhelp();
                return 1;
            }
        } else if (strcmp(argv[optind], "--unpack") == 0) {
            unpack_mode = 1;
            if (optind + 1 < argc) {
                archive_name = argv[optind + 1];
                optind += 2;
            } else {
                fprintf(stderr, "Error: No archive name specified\n");
                printhelp();
                return 1;
            }
        }
    }
    
    // Handle create mode
    if (create_mode) {
        if (!archive_name) {
            fprintf(stderr, "Error: Archive name not specified\n");
            printhelp();
            return 1;
        }
        
        if (optind >= argc) {
            fprintf(stderr, "Error: No files specified for archiving\n");
            printhelp();
            return 1;
        }
        
        char **file_list = &argv[optind];
        int file_count = argc - optind;
        
        create_archive(archive_name, file_list, file_count);
        return 0;
    }
    
    // Handle unpack mode
    if (unpack_mode) {
        if (!archive_name) {
            fprintf(stderr, "Error: Archive name not specified\n");
            printhelp();
            return 1;
        }
        
        extract_archive(archive_name);
        return 0;
    }
    
    // If we get here, it's an error
    fprintf(stderr, "Error: No operation specified\n");
    printhelp();
    return 1;
}