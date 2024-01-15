//#define GLEW_STATIC
// #define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "texture.h"
#include "stb_image.h"
#include "../res/includes/glad/include/glad/glad.h"
#include <iostream>
#include <fstream>


static unsigned char* sobel(unsigned char* data, int width, int height)
{
    int len = width * height;
    int sqr = std::sqrt(len);
    
    int sobelX[] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
    int sobelY[] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};
    unsigned char* sobeldImage = new unsigned char[len];
    int kernelSize = 3;

    for (int y = 1; y < sqr - 1; ++y) {
        for (int x = 1; x < sqr - 1; ++x) {
            int sumX = 0;
            int sumY = 0;

            for (int ky = 0; ky < kernelSize; ++ky) {
                for (int kx = 0; kx < kernelSize; ++kx) {
                    int offsetX = x + kx - 1;
                    int offsetY = y + ky - 1;
                    int pixelValue = data[offsetY * sqr + offsetX];

                    sumX += pixelValue * sobelX[ky * kernelSize + kx];
                    sumY += pixelValue * sobelY[ky * kernelSize + kx];
                }
            }

            int magnitude = std::sqrt(sumX * sumX + sumY * sumY);
            unsigned char edgePixel = (magnitude > 128) ? 255 : 0;
            sobeldImage[y * sqr + x] = edgePixel;
        }
    }
    return sobeldImage;
}

static unsigned char* halftone(unsigned char* data, int width, int height) {
    int len = width * height;
    int sqr = std::sqrt(len);
    unsigned char* outputImage = new unsigned char[len];
    
    for (int i = 0; i< sqr; i+=2){
        for (size_t j = 0; j < sqr; j+=2)
        {
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

static unsigned char* floyd(unsigned char* data, int width, int height) {
    int len = width * height;
    unsigned char* outputImage = new unsigned char[len];
    for (int i = 0; i< width; i++){
        for (size_t j = 0; j < height; j++)
        {
            int oldPixel = data[i*height + j];
            int newPixel = (oldPixel / 16) * 16;
            outputImage[i*height + j] = newPixel;

            int quantError = oldPixel - newPixel;
            if (j < height - 1) {
                outputImage[i*height + j + 1] += quantError * 7 / 16;
            }
            if (i < width - 1) {
                if (j > 0) {
                    outputImage[(i + 1)*height + j - 1] += quantError * 3 / 16;
                }
                outputImage[(i + 1)*height + j] += quantError * 5 / 16;
                if (j < width - 1) {
                    outputImage[(i + 1)*width + j + 1] += quantError * 1 / 16;
                }
            }
        }
    }
    return outputImage;
    
}

static unsigned char* convertImageToGreyScale(unsigned char* data,int width,int height) {
    int len = width * height;
    int sqr = std::sqrt(len);
    unsigned char* outputImage = new unsigned char[len];

    for (int i = 0; i < len; ++i) {
        int r = data[i * 4 + 0];
        int g = data[i * 4 + 1];
        int b = data[i * 4 + 2];

        outputImage[i] = (r + g + b) / 3;
    }
    return outputImage;
}

static unsigned char* convertGrayScaleToColor(unsigned char* greyedImage,unsigned char* orginalData,int width,int height ) {
    int orginalLen = width * height * 4;
    int len = width * height;
    int sqr = std::sqrt(len);
    unsigned char* outputImage = new unsigned char[orginalLen];
    int index = 0;
    for (int i = 1; i < sqr + 1; i++) {
        for (int j = 1; j < sqr + 1; j++) {
            outputImage[index] = greyedImage[i*sqr + j];
            outputImage[index + 1] = greyedImage[i*sqr + j];
            outputImage[index + 2] = greyedImage[i*sqr + j];
            outputImage[index + 3] = orginalData[4 * (i * sqr + j) + 3];
            index += 4;
        }
    }
    return outputImage;
}  

void printToFileGreyScale (char* fileName ,unsigned char* data,int width,int height) {
        std::ofstream outputFile(fileName);
        if (outputFile.is_open())
        {
            printf("file opened\n");
            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    outputFile << data[i * height + j] / 17 << ",";
                }
            }
            outputFile.close();
        }
        else {
            printf("Unable to open file");
        }
}

void printToFileBlackWhitevoid (char* fileName ,unsigned char* data,int width,int height) {
        std::ofstream outputFile(fileName);
        if (outputFile.is_open())
        {
            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    int value = data[i * height + j] == 255 ? 1 : 0;
                    outputFile << value << ",";
                }
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
        printf("\n");
    }
        printf("len: %d\n", len);
}

Texture::Texture(const std::string& fileName)
{
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
    unsigned char* sobeled;

    switch(textureIndx){
        case 0:
            greyed = convertImageToGreyScale(data,width,height);
            sobeled = halftone(greyed,width,height);
            data = convertGrayScaleToColor(sobeled, data,width,height);
            printToFileBlackWhitevoid("img5.txt",sobeled,width,height);
            break;
        case 1:
            break;
        case 2:
            greyed = convertImageToGreyScale(data,width,height);
            sobeled = floyd(greyed,width,height);
            data = convertGrayScaleToColor(sobeled, data,width,height);
            printToFileGreyScale("img6.txt",sobeled,width,height);
            break;
        case 3:
            greyed = convertImageToGreyScale(data,width,height);
            sobeled = sobel(greyed,width,height);
            data = convertGrayScaleToColor(sobeled, data,width,height);
            printToFileBlackWhitevoid("img4.txt",sobeled,width,height);
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

