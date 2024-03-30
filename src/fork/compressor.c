#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "../utilities/file_utils.c"
#include "../huffman/huffman.c"
#include "../huffman/freq.c"
#include "file_locks.c"


void encode(char *input_file, char *freq_file, FILE *binary_output){

    // Fill the buffer
    wchar_t *buffer = NULL;
    get_wchars_from_file(input_file, &buffer);

    // Extract the frequencies for each character in the text file
    int freq_table[CHAR_SET_SIZE] = {0};
    char_frequencies(buffer, freq_table);
    
    // Write the wchar and its frequency to the output file
    write_wchars_to_file(freq_file, freq_table);

    // We first calculate the size of the freq table (only the characters' freq > 0)
    int freq_table_size = calculateFreqTableSize(freq_table);

    // Build Huffman Tree from the frequency table
    struct MinHeapNode* huffmanRoot = buildHuffmanTree(freq_table, freq_table_size);

    // Huffman codes 2D array where the first array contains the character and the second array
    // the length of the code and the code for that character
    struct HuffmanCode* huffmanCodesArray[MAX_FREQ_TABLE_SIZE] = {NULL};
    
    // An array where the Huffman code for each character is gonna be stored
    int bits[MAX_CODE_SIZE];

    // Generate Huffman codes for each character in the text file
    generateHuffmanCodes(huffmanRoot, bits, 0, huffmanCodesArray);

    // Write the Huffman Codes to file    
    size_t buffer_size = wcslen(buffer);
    write_encoded_bits_to_file(buffer, buffer_size, input_file, huffmanRoot, huffmanCodesArray, binary_output);
}

int main() {
    pid_t pid;
    int numOfProcess = TOTAL_BOOKS;

    // Folder Paths
    const char* booksFolder = "books";
    const char* out = "out/bin/compressed.bin";

    int runs = TOTAL_BOOKS;

    FILE *binary_output = fopen(out, "wb");
    if (binary_output == NULL) {
        perror("Error opening output binary file");
        exit(EXIT_FAILURE);
    }

    // Set for every book in books folder
    struct EncodeArgs *paths = getAllPaths(booksFolder);

    // Encode
    for (int i = 0; i < numOfProcess; i++) {
        pid = fork();

        // Código específico del proceso hijo
        if (pid == 0) {
            
            // Asegurarse de que solo este proceso escriba
            set_lock(binary_output, F_WRLCK);

            // Escribir
            printf("[PID %d] CODING : %s\n", i+1, paths->books[i]);
            encode(paths->books[i], paths->freqs[i], binary_output);
            printf("\n");

            // Liberar el archivo y evitar que los hijos creen más procesos
            unlock_file(binary_output);
            return 0;
        }
    }

    // El padre espera a todos los hijos
    while (numOfProcess > 0) {
        wait(NULL);
        numOfProcess--;
    }

    fclose(binary_output);
    free(paths);
    
    return EXIT_SUCCESS;
}
