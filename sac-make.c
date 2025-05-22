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

void create_archive(const char *archive_name, char *file_list[], int file_count) {
    FILE *archive = fopen(archive_name, "wb");
    if (!archive) {
   //     error(1);
   	perror("Guess that file is already exists");
        //return 1;
    }

    // Writing amount of files.
    fwrite(&file_count, sizeof(int), 1, archive);
    
    FileInfo *file_info = malloc(file_count * sizeof(FileInfo));

    // Writing information about files and contains
    for (int i = 0; i < file_count; i++) {
        FILE *input = fopen(file_list[i], "rb");
        if (!input) {
            perror("Failed to open file");
            continue;
        }

        // Getting information about file
        strncpy(file_info[i].filename, file_list[i], sizeof(file_info[i].filename) -1);
	file_info[i].filename[sizeof(file_info[i].filename) -1] = '\0';
        fseek(input, 0, SEEK_END);
        file_info[i].filesize = ftell(input);
        fseek(input, 0, SEEK_SET);

        // Writing information about file to archive 
        fwrite(&file_info[i], sizeof(FileInfo), 1, archive);
        
        // Записываем содержимое файла в архив
        char *buffer = malloc(file_info[i].filesize);
        fread(buffer, 1, file_info[i].filesize, input);
        fwrite(buffer, 1, file_info[i].filesize, archive);
        
        free(buffer);
        fclose(input);
    }

    free(file_info);
    fclose(archive);
}

void printhelp(void) {
	printf("Usage:\n"
	       "-v -> Print version\n"
	       "-h -> Print this\n"
	       "<filename> -> Create an archive. I reccomend use the .sac file extension\n"); }

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
    const char *archive_name = argv[optind];
    char **file_list = &argv[optind + 1];
    int file_count = argc - optind  - 1;
    if (optind >= argc) {
	    fprintf(stderr, "[ERR] No archive name or file specified\n");
	    printhelp();
	    return 1; }
    if (argc < 2) { printhelp(); return 1; }
    if (file_count == 0) { fprintf(stderr, "[ERR] No files specified\n"); printhelp(); return 1;}

    create_archive(argv[1], &argv[2], argc - 2);
    return 0;

}
