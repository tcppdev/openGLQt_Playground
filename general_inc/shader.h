#ifndef SHADER_H
#define SHADER_H

#include <QOpenGLContext> 
#ifdef __EMSCRIPTEN__
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#else
#include <QOpenGLFunctions_3_3_Core>
#endif
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#ifdef __EMSCRIPTEN__
#include <cstdio>  // for FILE*, fopen, fread, etc.
#endif

#ifdef __EMSCRIPTEN__
class Shader: protected QOpenGLExtraFunctions
#else
class Shader: protected QOpenGLFunctions_3_3_Core
#endif
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        initializeOpenGLFunctions();   // Initialise current context

        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try 
        {
#ifdef __EMSCRIPTEN__
            // WebAssembly: Use FILE* for preloaded virtual filesystem
            FILE* vFile = fopen(vertexPath, "r");
            FILE* fFile = fopen(fragmentPath, "r");
            
            if (vFile == nullptr) {
                std::cout << "ERROR::SHADER::VERTEX_FILE_NOT_FOUND: " << vertexPath << std::endl;
                return;
            }
            if (fFile == nullptr) {
                std::cout << "ERROR::SHADER::FRAGMENT_FILE_NOT_FOUND: " << fragmentPath << std::endl;
                fclose(vFile);
                return;
            }
            
            // Read vertex shader
            fseek(vFile, 0, SEEK_END);
            long vSize = ftell(vFile);
            fseek(vFile, 0, SEEK_SET);
            vertexCode.resize(vSize);
            fread(&vertexCode[0], 1, vSize, vFile);
            fclose(vFile);
            
            // Read fragment shader  
            fseek(fFile, 0, SEEK_END);
            long fSize = ftell(fFile);
            fseek(fFile, 0, SEEK_SET);
            fragmentCode.resize(fSize);
            fread(&fragmentCode[0], 1, fSize, fFile);
            fclose(fFile);
            
            // Read geometry shader if provided (but warn it's not supported)
            if(geometryPath != nullptr)
            {
                std::cout << "WARNING: Geometry shaders not supported in WebGL - ignoring: " << geometryPath << std::endl;
            }
#else
            // Native: Use std::ifstream
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();		
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();			
            // if geometry shader path is present, also load a geometry shader
            if(geometryPath != nullptr)
            {
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
#endif
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char * fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // if geometry shader is given, compile geometry shader
        unsigned int geometry;
        if(geometryPath != nullptr)
        {
#ifndef __EMSCRIPTEN__
            const char * gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
#else
            // Geometry shaders not supported in WebGL
            std::cout << "WARNING: Geometry shaders not supported in WebGL - ignoring" << std::endl;
#endif
        }
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
#ifndef __EMSCRIPTEN__
        if(geometryPath != nullptr)
            glAttachShader(ID, geometry);
#endif
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
#ifndef __EMSCRIPTEN__
        if(geometryPath != nullptr)
            glDeleteShader(geometry);
#endif

    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use() 
    { 
        glUseProgram(ID); 
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value)
    {         
        GLint uniform_loc = glGetUniformLocation(ID, name.c_str());
        glUniform1i(uniform_loc, (int)value); 
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value)
    { 
        GLint uniform_loc = glGetUniformLocation(ID, name.c_str());
        glUniform1i(uniform_loc, value); 
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value)
    { 
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value)
    { 
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec2(const std::string &name, float x, float y)
    { 
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value)
    { 
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z)
    { 
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value)
    { 
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) 
    { 
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat)
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat)
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat)
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if(type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if(!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif
