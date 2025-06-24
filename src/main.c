#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

void Gauss(int width, int height, unsigned char* img, unsigned char mat[height][width]);
void mat_to_img(int width, int height, unsigned char* img, unsigned char mat[height][width]);

int main(int argc, char *argv[]){
    if(argc < 3){
        printf("Usage: %s input.png output.png\n", argv[0]);
        return 1;
    }

    const char* input_file = argv[1];
    const char* output_file = argv[2];

    unsigned error;
    unsigned char* img;
    unsigned width, height;

    error = lodepng_decode32_file(&img, &width, &height, input_file);
    if( error ){ printf("Error %u: %s\n", error, lodepng_error_text(error)); return 1; }
    
    // BW
    for(int i = 0; i < width*height*4; i += 4){
        int avg = (img[i]+img[i+1]+img[i+2])/3;
        // avg /= 32; avg *= 32; avg += 31;
        img[i] = img[i+1] = img[i+2] = avg;
    }

    unsigned char mat[height][width];
    Gauss(width, height, img, mat);

    mat_to_img(width, height, img, mat);

    error = lodepng_encode32_file(output_file, img, width, height);
    if( error ) printf("Error %u: %s\n", error, lodepng_error_text(error));

    free(img);
    return 0;
}

void Gauss(int width, int height, unsigned char* img, unsigned char mat[height][width]){
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            int idx = (y * width + x) * 4;
            mat[y][x] = img[idx];
        }
    }

    /* Гауссово ядро 3х3 (радиус 1)
    float kernel[3][3] = {
        {1.0/16.0, 2.0/16.0, 1.0/16.0},
        {2.0/16.0, 4.0/16.0, 2.0/16.0},
        {1.0/16.0, 2.0/16.0, 1.0/16.0}
    };
    //*/

    //* Гауссово ядро 5х5 (радиус 2)
    float kernel[5][5] = {
        {1.0/256,  4.0/256,  6.0/256,  4.0/256, 1.0/256},
        {4.0/256, 16.0/256, 24.0/256, 16.0/256, 4.0/256},
        {6.0/256, 24.0/256, 36.0/256, 24.0/256, 6.0/256},
        {4.0/256, 16.0/256, 24.0/256, 16.0/256, 4.0/256},
        {1.0/256,  4.0/256,  6.0/256,  4.0/256, 1.0/256}
    };
    //*/

    int A = 2;

    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            float sum = 0;
            
            for(int ky = -A; ky <= A; ky++){
                for(int kx = -A; kx <= A; kx++){
                    int ny = y + ky;
                    int nx = x + kx;
                    
                    if( ny >= 0 && ny < height && nx >= 0 && nx < width ){
                        sum += mat[ny][nx] * kernel[ky+A][kx+A];
                    }
                    else{
                        ny = y + ky;
                        nx = x + kx;
                        if(ny < 0) ny = -ny;
                        if(ny >= height) ny = 2*height - ny - 2;
                        if(nx < 0) nx = -nx;
                        if(nx >= width) nx = 2*width - nx - 2;
                        sum += mat[ny][nx] * kernel[ky+A][kx+A];
                    }
                }
            }
            
            mat[y][x] = (char)(sum + 0.5);
        }
    }
}

void mat_to_img(int width, int height, unsigned char* img, unsigned char mat[height][width]){
    for(int i = 0; i < width*height*4; i++){
        if( i%4 == 3 ) continue;
        int y = (i/4)/width;
        int x = (i/4) - y*width;

        // Color rounds up
        int round = 32;
        int col = mat[y][x];
        col /= round; col *= round; col += round-1;
        img[i] = col;
    }
}