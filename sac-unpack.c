#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
typedef struct {
    char filename[256];
    long filesize;
} FileInfo;

void extract_archive(const char *archive_name) {
    FILE *archive = fopen(archive_name, "rb");
    if (!archive) {
        perror("Failed to open archive");
        return;
    }

    // Read the number of files in the archive
    int file_count;
    if (fread(&file_count, sizeof(int), 1, archive) != 1) {
        perror("Failed to read file count");
        fclose(archive);
        return;
    }

    // Read each file's information and contents
    for (int i = 0; i < file_count; i++) {
        FileInfo file_info;
        if (fread(&file_info, sizeof(FileInfo), 1, archive) != 1) {
            perror("Failed to read file info");
            break;
        }

        // Open the output file
        FILE *file = fopen(file_info.filename, "wb");
        if (!file) {
            perror("Failed to create file");
            continue;
        }

        // Read and write the file contents
        char *buffer = malloc(file_info.filesize);
        if (fread(buffer, 1, file_info.filesize, archive) != file_info.filesize) {
            perror("Failed to read file contents");
            free(buffer);
            fclose(file);
            continue;
        }

        fwrite(buffer, 1, file_info.filesize, file);
        free(buffer);
        fclose(file);
    }

    fclose(archive);
}
void printhelp(void) {
    printf("Usage:\n"
    "-v -> Print version\n"
    "-h -> Print this\n"
    "<filename> -> Open archive\n"); }

    void printver(void) {
        printf(" ____    _    ____\n"
        "/ ___|  / \\  / ___| Release: SacV1.1\n"
        "\\___ \\ / _ \\| |\n"
        " ___) / ___ \\ |___  Made with Love, ViM and Clang.\n"
        "|____/_/   \\_\\____|\n"); }

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "hv")) != -1) {
        switch (opt) {
            case 'h':
                printhelp(); return 0;
            case 'v':
                printver();
                return 0;
            default:
                printhelp();
                return 1; } }
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <archive_name>\n", argv[0]);
        return 1;
    }

    extract_archive(argv[1]);
    return 0;
}
