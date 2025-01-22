#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include <GL/glew.h>
#include <string>

class TextureLoader {
public:
    // Lädt eine Textur aus einer Datei
    static GLuint loadTexture(const std::string& path);

    // Lädt eine Normalmap aus einer Datei
    static GLuint loadNormalMap(const std::string& path);
};

#endif
