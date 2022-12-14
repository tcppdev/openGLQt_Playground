#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QOpenGLContext> 
#include <QOpenGLFunctions_3_3_Core>

#include <general_inc/shader.h>

#include <ft2build.h>
#include FT_FREETYPE_H  

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::string NEWLINE_CHARACTER = "\\";

class Text3D: protected QOpenGLFunctions_3_3_Core
{
public:
    
    Text3D(std::string text_to_write, float x, float y, float z, float scale, float offset_x_screen = 0, float offset_y_screen = 0)
    {
        m_text = text_to_write; 
        m_scale = scale;
        m_x = x;
        m_y = y;
        m_z = z;
        
        // Offset of text in clip/screen space (xpos + m_offset_x_screen and ypos + m_offset_y_screen must be between -1 and 1)
        m_offset_x_screen = offset_x_screen;
        m_offset_y_screen = offset_y_screen;

        initializeOpenGLFunctions();   // Initialise current context  (required)

        // Text shaders
        const char* vertex_text = "/home/t.clar/Repos/openGLQt/shaders/text.vs";
        const char* fragment_text = "/home/t.clar/Repos/openGLQt/shaders/text.fs";
        m_text_shader = new Shader(vertex_text, fragment_text);

        // Front 
        std::string font_path = "/home/t.clar/Repos/openGLQt/resources/fonts/Antonio-Bold.ttf";
        
        setup(font_path);
    }

    ~Text3D() {
        delete m_text_shader;
    }

    void setup(std::string font_path)
    {
        // find path to font
        if (font_path.empty())
        {
            std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        }

        // FreeType
        // --------
        FT_Library ft;
        // All functions return a value different than 0 whenever an error occurred
        if (FT_Init_FreeType(&ft))
        {
            std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        }

        // load font as face
        FT_Face face;
        if (FT_New_Face(ft, font_path.c_str(), 0, &face)) {
            std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        }
        else {
            // set size to load glyphs as
            FT_Set_Pixel_Sizes(face, 0, 48);

            // disable byte-alignment restriction
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // load first 128 characters of ASCII set
            for (unsigned char c = 0; c < 128; c++)
            {
                // Load character glyph 
                if (FT_Load_Char(face, c, FT_LOAD_RENDER))
                {
                    std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                    continue;
                }
                // generate texture
                unsigned int texture;
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);  // Bind texture
                glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    GL_RED,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    0,
                    GL_RED,
                    GL_UNSIGNED_BYTE,
                    face->glyph->bitmap.buffer
                );
                // set texture options
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                // now store character for later use
                Character character = {
                    texture,
                    glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                    glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                    static_cast<unsigned int>(face->glyph->advance.x)
                };
                m_characters.insert(std::pair<char, Character>(c, character));
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        // destroy FreeType once we're finished
        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        // configure VAO/VBO for (text) texture quads
        // -----------------------------------
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 5, NULL, GL_DYNAMIC_DRAW);

        // Positions:
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);

        // Texture coordinates:
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        
    }

    void update_position(float x, float y, float z) 
    {
        m_x = x;
        m_y = y;
        m_z = z;
    }

    void draw(glm::mat4 view_matrix, 
              glm::mat4 projection_matrix,
              bool fixed_size = true)
    {
        // OpenGL state
        // ------------
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);  // enabling blending 
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // setting blending function

        // activate corresponding render state	
        m_text_shader->use();

		// http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/billboards/
		m_text_shader->setVec3("camera_right_worldspace", view_matrix[0][0], view_matrix[1][0], view_matrix[2][0]);
		m_text_shader->setVec3("camera_up_worldspace", view_matrix[0][1], view_matrix[1][1], view_matrix[2][1]);
        m_text_shader->setVec3("text_position", m_x, m_y, m_z);
        m_text_shader->setBool("fixed_size", fixed_size);

        m_text_shader->setMat4("projection", projection_matrix);
        m_text_shader->setMat4("view", view_matrix);
        m_text_shader->setVec3("textColor", m_color); // Set uniform
        glActiveTexture(GL_TEXTURE0);  // set texture slot to 0th 
        glBindVertexArray(vao_);

        // iterate through all characters
        float start_x = 0.0;
        float start_y = 0.0;
        float start_z = 0.0;

        std::string::const_iterator c;
        for (c = m_text.begin(); c != m_text.end(); c++) 
        {
            Character ch = m_characters[*c];

            float xpos = start_x + ch.Bearing.x * m_scale + m_offset_x_screen;
            float ypos = start_y - (ch.Size.y - ch.Bearing.y) * m_scale + m_offset_y_screen;

            
            if (NEWLINE_CHARACTER.find(*c) != std::string::npos)
            {
                start_x = 0.0; // Reset to start of line
                start_y -= 1.1*ch.Size.y * m_scale;
            }
            else {
                float w = ch.Size.x * m_scale;
                float h = ch.Size.y * m_scale;
                // update VBO for each character
                float vertices[6][5] = {
                    { xpos,     ypos + h,  start_z,    0.0f, 0.0f },            
                    { xpos,     ypos,      start_z,    0.0f, 1.0f },
                    { xpos + w, ypos,      start_z,    1.0f, 1.0f },

                    { xpos,     ypos + h,  start_z,    0.0f, 0.0f },
                    { xpos + w, ypos,      start_z,    1.0f, 1.0f },
                    { xpos + w, ypos + h,  start_z,    1.0f, 0.0f }           
                };
                // render glyph texture over quad
                glBindTexture(GL_TEXTURE_2D, ch.TextureID);  // bind the correct texture object to active texture slot (0th)
                // update content of VBO memory
                glBindBuffer(GL_ARRAY_BUFFER, vbo_);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);  // sets data within a specified region
                // of the currently bound buffer object. 
                // be sure to use glBufferSubData and not glBufferData

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                // render quad
                glDrawArrays(GL_TRIANGLES, 0, 6);
                // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
                start_x += (ch.Advance >> 6) * m_scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
            }
        }

        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        

    }

private:

    std::map<GLchar, Character> m_characters;
    Shader* m_text_shader;
    unsigned int vao_, vbo_;

    std::string m_text;
    float m_scale;
    float m_x;
    float m_y;
    float m_z;
    glm::vec3 m_color = {0.0, 0.0, 1.0}; // blue

    // Offsets
    float m_offset_x_screen;
    float m_offset_y_screen;
};