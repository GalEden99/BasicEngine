//#define GLEW_STATIC
// #define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#include "texture.h"
#include "stb_image.h"
#include "../res/includes/glad/include/glad/glad.h"
#include <iostream>
#include <fstream>

static unsigned char *sobel(unsigned char *data) {
    int len = strlen((char *) data) / 4;
    int sqr = sqrt(len);

    double mat[sqr + 2][sqr + 2];
    for (int i = 1; i < sqr + 1; i++) {
        for (int j = 1; j < sqr + 1; j++) {
            mat[i][j] = (data[4 * (i * sqr + j)] + data[4 * (i * sqr + j) + 1] + data[4 * (i * sqr + j) + 2]) / 3.0;
        }
    }

    float mat2[sqr + 2][sqr + 2];


    ///// insert gaussian

    float gaussKernel[3][3] = {{1, 2, 1},
                               {2, 4, 2},
                               {1, 2, 1}};

    for (int i = 1; i < sqr + 2; i++) {
        for (int j = 1; j < sqr + 2; j++) {
            float newPixel = mat[i - 1][j - 1] * gaussKernel[0][0] +
                             mat[i - 1][j] * gaussKernel[0][1] +
                             mat[i - 1][j + 1] * gaussKernel[0][2] +
                             mat[i][j - 1] * gaussKernel[1][0] +
                             mat[i][j] * gaussKernel[1][1] +
                             mat[i][j + 1] * gaussKernel[1][2] +
                             mat[i + 1][j - 1] * gaussKernel[2][0] +
                             mat[i + 1][j] * gaussKernel[2][1] +
                             mat[i + 1][j + 1] * gaussKernel[2][2];

            mat2[i][j] = newPixel / 16.0;
        }
    }

    //// sobel

    float gx[3][3] = {{-1, 0, 1},
                      {-2, 0, 2},
                      {-1, 0, 1}};
    float gy[3][3] = {{1,  2,  1},
                      {0,  0,  0},
                      {-1, -2, -1}};


    float mat3[sqr + 2][sqr + 2];
    double angles[sqr + 2][sqr + 2];

    for (int i = 1; i < sqr + 2; i++) {
        for (int j = 1; j < sqr + 2; j++) {
            float newPixelGx = mat2[i - 1][j - 1] * gx[0][0] +
                               mat2[i - 1][j + 1] * gx[0][2] +
                               mat2[i + 1][j - 1] * gx[2][0] +
                               mat2[i + 1][j + 1] * gx[2][2] +
                               mat2[i][j - 1] * gx[1][0] +
                               mat2[i][j + 1] * gx[1][2];
            float newPixelGy = mat2[i - 1][j - 1] * gy[0][0] +
                               mat2[i - 1][j] * gy[0][1] +
                               mat2[i - 1][j + 1] * gy[0][2] +
                               mat2[i + 1][j - 1] * gy[2][0] +
                               mat2[i + 1][j] * gy[2][1] +
                               mat2[i + 1][j + 1] * gy[2][2];
            float newPixel = (std::sqrt(newPixelGx * newPixelGx + newPixelGy * newPixelGy));

                mat3[i][j] = newPixel;

                float angle = (std::atan2(newPixelGy, newPixelGx));
                angles[i][j] = angle;

        }
    }

    //// non max

    float mat4[sqr + 2][sqr + 2];
    int ang = 180;

    for (int i = 1; i < sqr - 1; i++) {
        for (int j = 1; j < sqr - 1; j++) {
            float p1;
            float p2;
            float curr = mat3[i][j];
            double direction = angles[i][j];
            if ((0 <= direction < ang/8) || (15 * ang/8 <= direction <= 2 * ang)) {
                p1 = mat3[i][j - 1];
                p2 = mat3[i][j + 1];
            } else if ((ang/8 <= direction * 3 * ang/8) || (9 * ang / 8 <= direction < 11 * ang/8)) {
                p1 = mat3[i + 1][j - 1];
                p2 = mat3[i - 1][j + 1];
            } else if ((3 * ang/8 <= direction < 5 * ang/8) || (11 * ang / 8 <= direction < 13 * ang/8)) {
                p1 = mat3[i - 1][j];
                p2 = mat3[i + 1][j];
            } else {
                p1 = mat3[i + 1][j - 1];
                p2 = mat3[i - 1][j + 1];
            }
            if (curr >= p1 && curr >= p2) {
                mat4[i][j] = curr;
            } else {
                mat4[i][j] = 0;
            }
        }
    }

    float highThreshold = 180;
    float lowThreshold = 120;

    for (int i = 0; i < sqr + 1; i++) {
        for (int j = 0; j < sqr + 1; j++) {
            if (mat4[i][j] > highThreshold) {
                mat4[i][j] = 255;
            } else if (mat4[i][j] > lowThreshold) {
                mat4[i][j] = 25;
            } else {
                mat4[i][j] = 0;
            }
        }
    }


    for (int i = 0; i < sqr + 1; i++) {
        for (int j = 0; j < sqr + 1; j++) {
            if (mat4[i][j] == 25) {
                if ((mat4[i + 1][j - 1] == 255) || (mat4[i + 1][j] == 255) || (mat4[i + 1][j + 1] == 255)
                    || (mat4[i][j - 1] == 255) || (mat4[i][j + 1] == 255)
                    || (mat4[i - 1][j - 1] == 255) || (mat4[i - 1][j] == 255) || (mat4[i - 1][j + 1] == 255))
                    mat4[i][j] = 255;
                else
                    mat4[i][j] = 0;
            }
        }
    }

    unsigned char *data2 = new unsigned char[len * 4];
    int index = 0;
    for (int i = 1; i < sqr + 1; i++) {
        for (int j = 1; j < sqr + 1; j++) {
            data2[index] = mat4[i][j];
            data2[index + 1] = mat4[i][j];
            data2[index + 2] = mat4[i][j];
            data2[index + 3] = data[4 * (i * sqr + j) + 3];
            index += 4;
        }
    }


    return data2;
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


std::vector<unsigned char> GetEdgesImage(const unsigned char* data, int width, int height)
{
    std::vector<unsigned char> edgesImage(width * height, 0);

    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int gx = data[(y - 1) * width + (x + 1)] + 2 * data[y * width + (x + 1)] + data[(y + 1) * width + (x + 1)]
                - data[(y - 1) * width + (x - 1)] - 2 * data[y * width + (x - 1)] - data[(y + 1) * width + (x - 1)];

            int gy = data[(y + 1) * width + (x - 1)] + 2 * data[(y + 1) * width + x] + data[(y + 1) * width + (x + 1)]
                - data[(y - 1) * width + (x - 1)] - 2 * data[(y - 1) * width + x] - data[(y - 1) * width + (x + 1)];

            int gradient = std::sqrt(gx * gx + gy * gy);

            if (gradient > 128)
            {
                edgesImage[y * width + x] = 255;
            }
        }
    }

    return edgesImage;
}


Texture::Texture(const std::string& fileName,bool for2D,int textureIndx)
{
    int width, height, numComponents;
    unsigned char* data = stbi_load((fileName).c_str(), &width, &height, &numComponents, 4);

    switch(textureIndx){
        case 0:
        std::printf("Texture 0\n");
            break;
        case 1:
        std::printf("Texture 1\n");
        data = sobel(data);
            break;
        case 2:
        std::printf("Texture 2\n");
            break;
        case 3:
        std::printf("Texture 3\n");
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

