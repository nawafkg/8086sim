#include <stdio.h>
#include <stdlib.h>
#include "decoder.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.bin> <output.asm>\n", argv[0]);
        return 1;
    }

    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        perror("Failed to open input file");
        return 1;
    }

    FILE *out = fopen(argv[2], "w");
    if (!out) {
        perror("Failed to open output file");
        fclose(in);
        return 1;
    }

    decode_file(in, out);

    fclose(in);
    fclose(out);

    return 0;
}
