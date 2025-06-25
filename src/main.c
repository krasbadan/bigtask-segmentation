#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "lodepng.h"

typedef struct MST_node{
    int count;
    unsigned char R, G, B;
    struct MST_node* parent;
} MST_node;

void MakeBlurredMatrix(int width, int height, unsigned char* img, unsigned char* mat);
void mat_to_img(int width, int height, unsigned char* img, unsigned char* mat);

MST_node MST_node_init();
MST_node* MST_find(MST_node* node);
void MST_merge(MST_node* node1, MST_node* node2);
void FindComponents(int width, int height, unsigned char* mat, MST_node* MST_mat);

int main(int argc, char *argv[]){
    srand(time(NULL));

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
        img[i] = img[i+1] = img[i+2] = avg;
    }

    unsigned char* mat = (unsigned char*)malloc(height * width * sizeof(unsigned char));
    if (!mat) {
        printf("Memory allocation failed for mat\n");
        free(img);
        return 1;
    }
    
    MakeBlurredMatrix(width, height, img, mat);

    MST_node* MST_mat = (MST_node*)malloc(height * width * sizeof(MST_node));
    if (!MST_mat) {
        printf("Memory allocation failed for MST_mat\n");
        free(mat);
        free(img);
        return 1;
    }
    
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            MST_mat[y*width + x] = MST_node_init();
        }
    }

    FindComponents(width, height, mat, MST_mat);

    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            MST_node ptr = *MST_find(&MST_mat[y*width + x]);
            int idx = (y*width + x) * 4;
            img[ idx ] = ptr.R;
            img[ idx + 1 ] = ptr.G;
            img[ idx + 2 ] = ptr.B;
            img[ idx + 3 ] = 255;
        }
    }

    error = lodepng_encode32_file(output_file, img, width, height);
    if( error ) printf("Error %u: %s\n", error, lodepng_error_text(error));

    free(MST_mat);
    free(mat);
    free(img);
    return 0;
}

void MakeBlurredMatrix(int width, int height, unsigned char* img, unsigned char* mat){
    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            int idx = (y * width + x) * 4;
            mat[y*width + x] = img[idx];
        }
    }

    /* Gauss kernel 3x3
    float kernel[3][3] = {
        {1.0/16, 2.0/16, 1.0/16},
        {2.0/16, 4.0/16, 2.0/16},
        {1.0/16, 2.0/16, 1.0/16}
    };
    //*/

    //* Gauss kernel 5x5
    float kernel[5][5] = {
        {1.0/256,  4.0/256,  6.0/256,  4.0/256, 1.0/256},
        {4.0/256, 16.0/256, 24.0/256, 16.0/256, 4.0/256},
        {6.0/256, 24.0/256, 36.0/256, 24.0/256, 6.0/256},
        {4.0/256, 16.0/256, 24.0/256, 16.0/256, 4.0/256},
        {1.0/256,  4.0/256,  6.0/256,  4.0/256, 1.0/256}
    };
    //*/

    int radius = 2;

    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            float sum = 0;

            for(int ky = -radius; ky <= radius; ky++){
                for(int kx = -radius; kx <= radius; kx++){
                    int ny = y + ky;
                    int nx = x + kx;

                    if( ny >= 0 && ny < height && nx >= 0 && nx < width ){
                        sum += mat[ny*width + nx] * kernel[ky+radius][kx+radius];
                    }
                    else{
                        ny = y + ky;
                        nx = x + kx;
                        if(ny < 0) ny = -ny;
                        if(ny >= height) ny = 2*height - ny - 2;
                        if(nx < 0) nx = -nx;
                        if(nx >= width) nx = 2*width - nx - 2;
                        sum += mat[ny*width + nx] * kernel[ky+radius][kx+radius];
                    }
                }
            }
            int round = 32;
            unsigned char col = (unsigned char)sum;
            col /= round; col *= round; col += round - 1;
            mat[y*width + x] = col;
        }
    }
}

MST_node MST_node_init(){
    MST_node new;
    new.count = 1;
    new.parent = NULL;
    new.R = rand() % 255;
    new.G = rand() % 255;
    new.B = rand() % 255;
    return new;
}

MST_node* MST_find(MST_node* node){
    if( node->parent == NULL ) return node;
    
    node->parent = MST_find(node->parent);
    return node->parent;
}

void MST_merge(MST_node* node1, MST_node* node2){
    MST_node* root1 = MST_find(node1);
    MST_node* root2 = MST_find(node2);

    if( root1 == root2 ) return;

    if( root1->count < root2->count ){
        root1->parent = root2;
        root2->count += root1->count;
    }else{
        root2->parent = root1;
        root1->count += root2->count;
    }
}

void FindComponents(int width, int height, unsigned char* mat, MST_node* MST_mat){
    //* Neighborhood kernel 5x5
    float kernel[5][5] = {
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 0, 1, 1},
        {1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1}
    };
    //*/

    int radius = 2;

    for(int y = 0; y < height; y++){
        for(int x = 0; x < width; x++){
            for(int ky = -radius; ky <= radius; ky++){
                for(int kx = -radius; kx <= radius; kx++){
                    if( kernel[ky+radius][kx+radius] ){
                        int ny = y + ky;
                        int nx = x + kx;

                        if( ny >= 0 && ny < height && nx >= 0 && nx < width ){
                            if( mat[y*width + x] == mat[ny*width + nx] ) 
                                MST_merge( &(MST_mat[y*width + x]), &(MST_mat[ny*width + nx]) );
                        }
                        else{
                            ny = y + ky;
                            nx = x + kx;
                            if(ny < 0) ny = -ny;
                            if(ny >= height) ny = 2*height - ny - 2;
                            if(nx < 0) nx = -nx;
                            if(nx >= width) nx = 2*width - nx - 2;
                            if( mat[y*width + x] == mat[ny*width + nx] ) 
                                MST_merge( &(MST_mat[y*width + x]), &(MST_mat[ny*width + nx]) );
                        }
                    }
                }
            }
        }
    }
}