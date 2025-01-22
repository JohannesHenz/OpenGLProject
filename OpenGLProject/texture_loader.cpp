#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Einbinden der stb_image-Bibliothek

#include <GL/glew.h>
#include <iostream>
#include <string>

class TextureLoader {
public:
    static unsigned int loadTexture(const std::string& path);
};

unsigned int TextureLoader::loadTexture(const std::string& path) {
    int width, height, nrChannels;

    // Lade das Bild mit stbi_load
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Texture-Parameter setzen
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Textur aus den geladenen Bilddaten erstellen
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Daten freigeben
        stbi_image_free(data);

        return textureID;
    }
    else {
        std::cerr << "Fehler beim Laden der Textur: " << path << std::endl;
        return 0;
    }
}
