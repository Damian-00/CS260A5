#include "texture.hpp"

#include "opengl.hpp"

#include <lodepng.h>
#include <sstream>

namespace {
    /**
     * @brief 
     * 
     * @param filename 
     * @param out_width 
     * @param out_height 
     * @param out_buffer 
     */
    void load_png(char const* filename, unsigned* out_width, unsigned* out_height, std::vector<unsigned char>* out_buffer)
    {
        int error = lodepng::decode(*out_buffer, *out_width, *out_height, filename);
        if (error != 0) {
            std::stringstream ss;
            ss << "Could not load " << filename << ": " << lodepng_error_text(error);
            throw std::runtime_error(ss.str());
        }
        // Flip!
        int w = *out_width;
        int h = *out_height;

        uint8_t* pData = out_buffer->data();
		const uint32_t uBytesPerLine = w * 4;

		std::vector<uint8_t> line(w * 4);
		uint8_t* pLine = line.data();
		for (uint32_t j = 0; j < h / 2; ++j){
			memcpy(pLine, pData + j * uBytesPerLine, uBytesPerLine);
			memcpy(pData + j * uBytesPerLine, pData + ((h - j - 1) * uBytesPerLine), uBytesPerLine);
			memcpy(pData + ((h - j - 1) * uBytesPerLine), pLine, uBytesPerLine);
		}
    }
}

namespace engine {
    /**
     * @brief Destroy the texture::texture object
     * 
     */
    texture::~texture()
    {
        if (m_id) {
            glDeleteTextures(1, &m_id);
            m_id = 0;
        }
    }

    /**
     * @brief 
     * 
     * @param filename 
     */
    void texture::create(char const* filename)
    {
        std::vector<unsigned char> data;
        load_png(filename, &m_width, &m_height, &data);

        // Load to GPU
        glGenTextures(1, &m_id);
        glBindTexture(GL_TEXTURE_2D, m_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        // Filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /**
     * @brief 
     * 
     */
    void texture::destroy()
    {
        if (m_id) {
            glDeleteTextures(1, &m_id);
            m_id = 0;
        }
    }

    /**
     * @brief 
     * 
     * @param slot 
     */
    void texture::activate(unsigned slot)
    {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }

}
