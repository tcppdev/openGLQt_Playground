#ifndef MODEL_H
#define MODEL_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.h"
#include "shader.h"

#include <QOpenGLContext>
#ifdef __EMSCRIPTEN__
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <emscripten.h>
#else
#include <QOpenGLFunctions_3_3_Core>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    string directory;
    bool gammaCorrection;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false) : gammaCorrection(gamma)
    {
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('MODEL CONSTRUCTOR DEBUG: Starting Model constructor with path: ' + UTF8ToString($0));
        }, path.c_str());
#endif
        loadModel(path);
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('MODEL CONSTRUCTOR DEBUG: Model constructor completed successfully');
        });
#endif
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('MODEL DRAW DEBUG: Drawing model with ' + $0 + ' meshes');
        }, static_cast<int>(meshes.size()));
#endif
        for(unsigned int i = 0; i < meshes.size(); i++) {
#ifdef __EMSCRIPTEN__
            EM_ASM_({
                console.log('MODEL DRAW DEBUG: Drawing mesh ' + $0);
            }, static_cast<int>(i));
#endif
            meshes[i].Draw(shader);
        }
    }
    
private:
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string path)  // Take by value to ensure string data is preserved in WebAssembly
    {
        // **CRITICAL FIX**: Keep importer alive throughout entire model loading process
        // to prevent scene corruption in WebAssembly
        Assimp::Importer* importer = new Assimp::Importer();
        const aiScene* scene = nullptr;
        
#ifdef __EMSCRIPTEN__
        // WebAssembly Fix: Copy string to prevent corruption during function call
        std::string path_copy = path;  // Explicit copy to maintain string validity
        const char* path_char = path_copy.c_str(); 
        
        // Debug logging in Model constructor
        EM_ASM_({
            console.log('MODEL DEBUG: Original path: ' + UTF8ToString($0));
            console.log('MODEL DEBUG: Path copy: ' + UTF8ToString($1));
            console.log('MODEL DEBUG: Path char: ' + UTF8ToString($2));
        }, path.c_str(), path_copy.c_str(), path_char);
        
        scene = importer->ReadFile(path_char, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
#else
        scene = importer->ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
#endif
        // check for errors
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('SCENE DEBUG: scene pointer = ' + $0);
            console.log('SCENE DEBUG: scene flags = ' + $1);  
            console.log('SCENE DEBUG: rootNode pointer = ' + $2);
        }, (scene ? 1 : 0), (scene ? static_cast<int>(scene->mFlags) : -1), (scene && scene->mRootNode ? 1 : 0));
#endif
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer->GetErrorString() << endl;
#ifdef __EMSCRIPTEN__
            if (!scene) {
                EM_ASM_({ console.log('SCENE DEBUG: scene is null'); });
            }
            if (scene && (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)) {
                EM_ASM_({ console.log('SCENE DEBUG: scene has incomplete flag'); });
            }
            if (scene && !scene->mRootNode) {
                EM_ASM_({ console.log('SCENE DEBUG: rootNode is null'); });
            }
#endif
            delete importer;  // Clean up on error
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively - importer stays alive during this process
        processNode(scene->mRootNode, scene);
        
        // Clean up importer after processing is complete
        delete importer;
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('PROCESS NODE MEMORY DEBUG: aiNode pointer = 0x' + $0.toString(16));
            console.log('PROCESS NODE MEMORY DEBUG: Node has ' + $1 + ' meshes and ' + $2 + ' children');
            console.log('PROCESS NODE MEMORY DEBUG: mMeshes pointer = 0x' + $3.toString(16));
            console.log('PROCESS NODE MEMORY DEBUG: mChildren pointer = 0x' + $4.toString(16));
        }, reinterpret_cast<uintptr_t>(node), 
           static_cast<int>(node->mNumMeshes), 
           static_cast<int>(node->mNumChildren),
           reinterpret_cast<uintptr_t>(node->mMeshes),
           reinterpret_cast<uintptr_t>(node->mChildren));
#endif
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            // Push directly to avoid stack overflow in WebAssembly with large meshes
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('PROCESS MESH DEBUG: Processing mesh with ' + $0 + ' vertices and ' + $1 + ' faces');
        }, static_cast<int>(mesh->mNumVertices), static_cast<int>(mesh->mNumFaces));
#endif

        // Pre-reserve space to avoid reallocations (critical for WebAssembly with large meshes)
        vertices.reserve(mesh->mNumVertices);
        indices.reserve(mesh->mNumFaces * 3); // Assuming triangulated faces

        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            if (mesh->HasNormals())
            {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }

#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('GOT HERE 1');});
#endif
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('GOT HERE 2');});
#endif

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('GOT HERE 3');});
#endif
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('GOT HERE 4');});
#endif
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        
        // Extract material diffuse color for fallback when no textures
        // Use C API to avoid template recursion issues in WebAssembly
        glm::vec3 matColor(0.8f, 0.8f, 0.8f); // default color
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('GOT HERE 5');});
#endif
        
        // Use aiGetMaterialColor C API instead of C++ template Get()
        aiColor4D color4d;
        if (aiGetMaterialColor(material, "$clr.diffuse", 0, 0, &color4d) == 0) {
            matColor = glm::vec3(color4d.r, color4d.g, color4d.b);
        }
        
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('GOT HERE 6');});
#endif
        
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('PROCESS MESH DEBUG: Material color = (' + $0 + ', ' + $1 + ', ' + $2 + ')');
            console.log('PROCESS MESH DEBUG: Diffuse textures = ' + $3);
        }, matColor.r, matColor.g, matColor.b, static_cast<int>(diffuseMaps.size()));
#endif
        
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures, matColor);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};


unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
#ifdef __EMSCRIPTEN__
    QOpenGLExtraFunctions *functions = QOpenGLContext::currentContext()->extraFunctions();
#else
    QOpenGLFunctions_3_3_Core *functions = new QOpenGLFunctions_3_3_Core;
    functions->initializeOpenGLFunctions();
#endif
    
    string filename = string(path);
    filename = directory + '/' + filename;

#ifdef __EMSCRIPTEN__
    EM_ASM_({
        console.log('TEXTURE LOAD DEBUG: Attempting to load texture from: ' + UTF8ToString($0));
    }, filename.c_str());
#endif

    unsigned int textureID;
    functions->glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        functions->glBindTexture(GL_TEXTURE_2D, textureID);
        functions->glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        functions->glGenerateMipmap(GL_TEXTURE_2D);

        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('TEXTURE LOAD DEBUG: Successfully loaded texture - width: ' + $0 + ', height: ' + $1 + ', components: ' + $2);
        }, width, height, nrComponents);
#endif
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        
#ifdef __EMSCRIPTEN__
        EM_ASM_({
            console.log('TEXTURE LOAD DEBUG: Failed to load texture from: ' + UTF8ToString($0));
        }, filename.c_str());
#endif
    }

#ifndef __EMSCRIPTEN__
    delete functions;
#endif
    
    return textureID;
}
#endif
