#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

int main(int argc, char *argv[]) {
    if(argc < 3) {
        printf("Usage: %s input.png output.png\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    unsigned error;
    unsigned char* image;
    unsigned width, height;

    error = lodepng_decode32_file(&image, &width, &height, input_file);
    if( error ){ printf("Error %u: %s\n", error, lodepng_error_text(error)); return 1; }
    
    // Первичная версия просто инвертирует картинку и делает тёмные части прозрачными
    for(unsigned i = 0; i < width*height*4; i += 4){
        image[i] = 255 - image[i];
        image[i+1] = 255 - image[i+1];
        image[i+2] = (255 - image[i+2])*0;
        if( image[i]+image[i+1]+image[i+2] < 30 ) image[i+3] = 0;
    }

    error = lodepng_encode32_file(output_file, image, width, height);
    if( error ) printf("Error %u: %s\n", error, lodepng_error_text(error));

    free(image);
    return 0;
}