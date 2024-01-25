#define STB_IMAGE_IMPLEMENTATION
#include "texture.h"
#include "stb_image.h"
#include "../res/includes/glad/include/glad/glad.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <corecrt_math_defines.h>

// Gaussian Blur
float gaussian(float x, float y, float sigma) {
    return exp(-(x*x + y*y) / (2.0 * sigma * sigma));
}

float gaussianDerivativeX(float x, float y, float sigma) {
    return (-x / (sigma * sigma)) * gaussian(x, y, sigma);
}

static void convolution(const int sqr, unsigned char* inputImage, const float gaussKernel[3][3], float outputImage[256 * 256]) {
    unsigned char** borderImage = new unsigned char*[sqr];
    for (int i = 0; i < sqr; ++i) {
        for (int j = 0; j < sqr; ++j) {
            borderImage[i] = new unsigned char[sqr];
        }
    }

    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < 256; ++j) {
            for (int k = -1; k <= 1; ++k) {
                for (int l = -1; l <= 1; ++l) {
                    if (0 <= i + k <= 255 && 0 <= j + l <= 255) {
                    borderImage[i][j] +=inputImage[(i + k) * 256 + (j + l)] * gaussKernel[k + 1][l + 1];
                    }
                }
            }
        }
    }

    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < 256; ++j) {
            outputImage[i * sqr + j] = borderImage[i][j];
        }
    }
}

// Canny Edge Detection
static unsigned char* canny(unsigned char* data, int width, int height){
    int len = width * height;
    const int sqr = std::sqrt(len);
    
    unsigned char* canniedImage = new unsigned char[len];

    float* outputImage = new float[len];
    float gaussKernel[3][3];
    float sigma = 1.8;

    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            gaussKernel[i+1][j+1] =  (1.0 / (2.0 * M_PI * sigma * sigma)) *
                           exp(-(pow(i / 2, 2) + pow(j / 2, 2)) / (2.0 * sigma * sigma));
        }
    }

    convolution(sqr,data, gaussKernel, outputImage);

    float* output2 = new float[len];
    double* angles = new double[len];

    float gx[3][3] = {{-1, 0, 1},
                      {-1.5, 0, 1.5},
                      {-1, 0, 1}};
    float gy[3][3] = {{1,  1.5,  1},
                      {0,  0,  0},
                      {-1, -1.5, -1}};

    for (int i = 1; i < sqr; ++i) {
        for (int j = 1; j < sqr; ++j) {
            // std::cout << "i = " << i << std::endl;
            // std::cout << "j = " << j << std::endl;

            float newPixelGx = outputImage[(i - 1)*sqr + j - 1] * gx[0][0] +
                               outputImage[(i - 1)*sqr + j + 1] * gx[0][2] +
                               outputImage[(i + 1)*sqr + j - 1] * gx[2][0] +
                               outputImage[(i + 1)*sqr + j + 1] * gx[2][2] +
                               outputImage[i*sqr + j - 1] * gx[1][0] +
                               outputImage[i*sqr + j + 1] * gx[1][2];
            float newPixelGy = outputImage[(i - 1) *sqr + j - 1] * gy[0][0] +
                               outputImage[(i - 1) *sqr + j] * gy[0][1] +
                               outputImage[(i - 1)*sqr + j + 1] * gy[0][2] +
                               outputImage[(i + 1)*sqr + j - 1] * gy[2][0] +
                               outputImage[(i + 1)*sqr + j] * gy[2][1] +
                               outputImage[(i + 1)*sqr + j + 1] * gy[2][2];
            float newPixel = (std::sqrt(newPixelGx * newPixelGx + newPixelGy * newPixelGy));

            // std::cout << "newPixelGx = " << newPixelGx << std::endl;
            // std::cout << "newPixelGy = " << newPixelGy << std::endl;
            // std::cout << "newPixel = " << newPixel << std::endl;

            output2[i* sqr + j] = newPixel;

            float angle = (std::atan2(newPixelGy, newPixelGx));
            // std::cout << "angeld " << angle << std::endl;
            angles[i* sqr + j] = angle; 
        }
    }

    // Non-maximum suppression for Sobel operator
    int ang = 180;
    for (int i = 1; i < sqr - 1; ++i) {
        for (int j = 1; j < sqr - 1; ++j) {
            float p1, p2, curr = output2[i * sqr + j];
            double direction = angles[i * sqr + j];

            if ((0 <= direction && direction < ang / 8) || (15 * ang / 8 <= direction && direction <= 2 * ang)) {
                p1 = output2[i * sqr + j - 1];
                p2 = output2[i * sqr + j + 1];
            } else if ((ang / 8 <= direction && direction < 3 * ang / 8) || (9 * ang / 8 <= direction && direction < 11 * ang / 8)) {
                p1 = output2[(i + 1) * sqr + j - 1];
                p2 = output2[(i - 1) * sqr + j + 1];
            } else if ((3 * ang / 8 <= direction && direction < 5 * ang / 8) || (11 * ang / 8 <= direction && direction < 13 * ang / 8)) {
                p1 = output2[(i - 1) * sqr + j];
                p2 = output2[(i + 1) * sqr + j];
            } else {
                p1 = output2[(i + 1) * sqr + j - 1];
                p2 = output2[(i - 1) * sqr + j + 1];
            }

            if (curr >= p1 && curr >= p2 && curr > 40) {
                canniedImage[i * sqr + j] = 255; // white
            } else {
                canniedImage[i * sqr + j] = 0; // black
            }
        }
    }
    return canniedImage;
}


// Halfton Pattern
static unsigned char* halftone(unsigned char* data, int width, int height) {
    int len = width * height;
    int sqr = std::sqrt(len);
    unsigned char* outputImage = new unsigned char[len];
    
    for (int i = 0; i< sqr; i+=2){
        for (size_t j = 0; j < sqr; j+=2){
            int pix1 = data[i*sqr + j];
            int pix2 = data[i*sqr + j+1];
            int pix3 = data[(i+1)*sqr + j];
            int pix4 = data[(i+1)*sqr + j+1];
            int avg = (pix1 + pix2 + pix3 + pix4) / 4;

            if (avg < 50) {
                outputImage[i*sqr + j] = 0;
                outputImage[i*sqr + j+1] = 0;
                outputImage[(i+1)*sqr + j] = 0;
                outputImage[(i+1)*sqr + j+1] = 0;
            } else if (avg < 100) {
                outputImage[i*sqr + j] = 0;
                outputImage[i*sqr + j+1] = 0;
                outputImage[(i+1)*sqr + j] = 0;
                outputImage[(i+1)*sqr + j+1] =  255;
            } else if (avg < 150) {
                outputImage[i*sqr + j] = 0;
                outputImage[i*sqr + j+1] = 255;
                outputImage[(i+1)*sqr + j] = 0;
                outputImage[(i+1)*sqr + j+1] =  255;
            } else if (avg < 200) {
                outputImage[i*sqr + j] = 0;
                outputImage[i*sqr + j+1] = 255;
                outputImage[(i+1)*sqr + j] = 255;
                outputImage[(i+1)*sqr + j+1] = 255;
            }else {
                outputImage[i*sqr + j] = 255;
                outputImage[i*sqr + j+1] = 255;
                outputImage[(i+1)*sqr + j] = 255;
                outputImage[(i+1)*sqr + j+1] = 255;
            }
        }
    }
    return outputImage;
}


// Floyd-Steinberg Algorithm
static unsigned char* floyd(unsigned char* data, int width, int height) {
    int len = width * height;
    unsigned char* outputImage = new unsigned char[len];
    for (int i = 0; i< width; i++){
        for (size_t j = 0; j < height; j++){
            int oldPixel = data[i*height + j];
            int newPixel = (oldPixel / 16) * 16;
            outputImage[i*height + j] = newPixel;

            int quantError = oldPixel - newPixel;
            if (j < height - 1){
                outputImage[i*height + j + 1] += quantError * 7 / 16;
            }

            if (i < width - 1){
                if (j > 0){
                    outputImage[i + 1*height + j - 1] += quantError * 3 / 16;
                }

                outputImage[i + 1*height + j] += quantError * 5 / 16;

                if (j < width - 1) {
                    outputImage[(i + 1)*width + j + 1] += quantError * 1 / 16;
                }
            }
        }
    }
    return outputImage;  
}


// Convert to GreyScale
static unsigned char* convertImageToGreyScale(unsigned char* data, int width, int height){
    int len = width * height;
    int sqr = std::sqrt(len);
    unsigned char* outputImage = new unsigned char[len];

    for (int i = 0; i < len; ++i){
        int r = data[i * 4 + 0];
        int g = data[i * 4 + 1];
        int b = data[i * 4 + 2];

        outputImage[i] = (r + g + b) / 3;
    }
    return outputImage;
}

// Convert to Color
static unsigned char* convertGrayScaleToColor(unsigned char* greyedImage, unsigned char* orginalData, int width, int height){
    int orginalLen = width*height*4;
    int len = width*height;
    int sqr = std::sqrt(len);
    unsigned char* outputImage = new unsigned char[orginalLen];
    int index = 0;
    for (int i = 1; i < sqr + 1; i++){
        for (int j = 1; j < sqr + 1; j++){
            outputImage[index] = greyedImage[i*sqr + j];
            outputImage[index + 1] = greyedImage[i*sqr + j];
            outputImage[index + 2] = greyedImage[i*sqr + j];
            outputImage[index + 3] = orginalData[4 * (i * sqr + j) + 3];
            index += 4;
        }
    }
    return outputImage;
}  

// Print to txt file
void printToFileGreyScale (char* fileName, unsigned char* data, int width, int height){
        std::ofstream outputFile(fileName);
        if (outputFile.is_open()){
            printf("file opened\n");
            for (int i = 0; i < width; i++){
                for (int j = 0; j < height; j++){
                    outputFile << data[i * height + j] / 17 << ",";
                }
                outputFile << "\n";
            }
            outputFile.close();
        }
        else{
            printf("Unable to open file");
        }
}

// Print to txt file
void printToFileBlackWhitevoid (char* fileName ,unsigned char* data, int width, int height){
    std::ofstream outputFile(fileName);
    if (outputFile.is_open()){
        for (int i = 0; i < width; i++){
            for (int j = 0; j < height; j++){
                int value = data[i * height + j] == 255 ? 1 : 0;
                outputFile << value << ",";
            }
            outputFile << "\n";
        }
        outputFile.close();
    }
}

static void printImage(unsigned char* data) {
    int len = strlen((char *) data);
    int sqr = std::sqrt(len);
    for (int i = 0; i < sqr; i++) {
        for (int j = 0; j < sqr; j++) {
            printf("%d ", data[i*sqr + j]);
        }
    }
}

Texture::Texture(const std::string& fileName){
	int width, height, numComponents;
    unsigned char* data = stbi_load((fileName).c_str(), &width, &height, &numComponents, 4);
	
    if(data == NULL)
		std::cerr << "Unable to load texture: " << fileName << std::endl;
        
    glGenTextures(1, &m_texture);
    Bind(m_texture);
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_LOD_BIAS,-0.4f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
}


Texture::Texture(const std::string& fileName,bool for2D,int textureIndx)
{
    int width, height, numComponents;
    unsigned char* data = stbi_load((fileName).c_str(), &width, &height, &numComponents, 4);

    unsigned char* greyed;
    unsigned char* halftoned;
    unsigned char* floyded;
    unsigned char* cannied;

    
    switch(textureIndx){
        case 0: // Halftone Pattern
            greyed = convertImageToGreyScale(data,width,height);
            halftoned = halftone(greyed,width,height);
            data = convertGrayScaleToColor(halftoned, data,width,height);
            printToFileBlackWhitevoid("img5.txt",halftoned,width,height);
            break;
        case 1: // GreyScale
            break;
        case 2: // Floyd-Steinberg Algorithm
            greyed = convertImageToGreyScale(data,width,height);
            floyded = floyd(greyed,width,height);
            data = convertGrayScaleToColor(floyded, data,width,height);
            printToFileGreyScale("img6.txt",floyded,width,height);
            break;
        case 3: // Canny
            greyed = convertImageToGreyScale(data,width,height);
            cannied = canny(greyed,width,height);
            data = convertGrayScaleToColor(cannied,data,width,height);
            printToFileBlackWhitevoid("img4.txt",cannied,width,height);
            break;
    }
    
    if(data == NULL)
        std::cerr << "Unable to load texture: " << fileName << std::endl;
        
    glGenTextures(1, &m_texture);
    Bind(m_texture);
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_LOD_BIAS,-0.4f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    // stbi_image_free(data);
}
 


Texture::Texture(int width,int height,unsigned char *data)
{
    glGenTextures(1, &m_texture);
    Bind(m_texture);
        
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
}

Texture::~Texture()
{
	glDeleteTextures(1, &m_texture);
}

void Texture::Bind(int slot)
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_texture);
}

