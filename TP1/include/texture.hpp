#pragma once

#include <GLFW/glfw3.h>
#include <vector>

namespace Texture{
    struct tex {
        char *path, *uniformName;
        GLuint id, uniformLocation;
        GLenum activationInt;

        void activate(){
            glActiveTexture(activationInt);
        }
    };
    class TextureManager {
        public:
        static tex loadTexture(char * path, char *uniformName, GLuint programID);

        static void render(GLuint programID);
    };
}