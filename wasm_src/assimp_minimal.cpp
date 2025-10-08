// Minimal Assimp implementation for WebAssembly
// This provides basic OBJ file loading without the full Assimp library

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <cstring>
#include <array>
#include <map>

#ifdef __EMSCRIPTEN__

// Add Emscripten console logging for debugging
#include <emscripten.h>

// Assimp constants that Model.h expects
enum aiTextureType {
    aiTextureType_DIFFUSE = 1,
    aiTextureType_SPECULAR = 2,
    aiTextureType_AMBIENT = 3,
    aiTextureType_HEIGHT = 5
};

// Process flags
const unsigned int aiProcess_Triangulate = 0x8;
const unsigned int aiProcess_GenSmoothNormals = 0x40;
const unsigned int aiProcess_FlipUVs = 0x800000;
const unsigned int aiProcess_CalcTangentSpace = 0x1;

// Scene flags  
const unsigned int AI_SCENE_FLAGS_INCOMPLETE = 0x1;

// Forward declarations
struct aiScene;
struct aiMesh;
struct aiMaterial;
struct aiNode;
struct aiFace;
struct aiVector3D;
struct aiString;

// Minimal Assimp namespace and classes for WebAssembly  
namespace Assimp {

class Importer {
public:
    Importer();
    ~Importer();
    
    const aiScene* ReadFile(const char* path, unsigned int flags);
    const char* GetErrorString() const;
    
private:
    aiScene* m_scene;
    std::string m_error;
    bool LoadOBJ(const std::string& path);
    bool LoadMTL(const std::string& mtlPath, std::vector<aiMaterial*>& materials, std::map<std::string, int>& materialMap);
    std::string GetDirectory(const std::string& path);
};

} // namespace Assimp

// Basic Assimp structures for WebAssembly
struct aiVector3D {
    float x, y, z;
    aiVector3D() : x(0), y(0), z(0) {}
    aiVector3D(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct aiColor3D {
    float r, g, b;
    aiColor3D() : r(0), g(0), b(0) {}
    aiColor3D(float r_, float g_, float b_) : r(r_), g(g_), b(b_) {}
};

struct aiString {
    unsigned int length;
    char data[1024];
    aiString() : length(0) { data[0] = '\0'; }
    aiString(const std::string& str) {
        length = str.length();
        strncpy(data, str.c_str(), sizeof(data) - 1);
        data[sizeof(data) - 1] = '\0';
    }
};

struct aiFace {
    unsigned int mNumIndices;
    unsigned int* mIndices;
    
    aiFace() : mNumIndices(0), mIndices(nullptr) {}
    ~aiFace() { delete[] mIndices; }
};

struct aiColor4D {
    float r, g, b, a;
    aiColor4D() : r(0), g(0), b(0), a(1) {}
    aiColor4D(float r_, float g_, float b_, float a_) : r(r_), g(g_), b(b_), a(a_) {}
};

struct aiBone; // Forward declaration for minimal implementation

struct aiMesh {
    /** CRITICAL: Must match exact memory layout of real aiMesh! */
    unsigned int mPrimitiveTypes;
    unsigned int mNumVertices;
    unsigned int mNumFaces;
    aiVector3D* mVertices;
    aiVector3D* mNormals;
    aiVector3D* mTangents;
    aiVector3D* mBitangents;
    aiColor4D* mColors[8]; // AI_MAX_NUMBER_OF_COLOR_SETS
    aiVector3D* mTextureCoords[8]; // AI_MAX_NUMBER_OF_TEXTURECOORDS
    unsigned int mNumUVComponents[8]; // AI_MAX_NUMBER_OF_TEXTURECOORDS
    aiFace* mFaces;
    unsigned int mNumBones;
    aiBone** mBones;
    unsigned int mMaterialIndex;
    aiString mName;
    unsigned int mNumAnimMeshes;
    void** mAnimMeshes; // aiAnimMesh** but we don't need full implementation
    
    aiMesh() : mPrimitiveTypes(0), mNumVertices(0), mNumFaces(0), mVertices(nullptr), 
               mNormals(nullptr), mTangents(nullptr), mBitangents(nullptr),
               mFaces(nullptr), mNumBones(0), mBones(nullptr), mMaterialIndex(0),
               mNumAnimMeshes(0), mAnimMeshes(nullptr) {
        for (int i = 0; i < 8; i++) {
            mColors[i] = nullptr;
            mTextureCoords[i] = nullptr;
            mNumUVComponents[i] = 0;
        }
    }
    
    ~aiMesh() {
        delete[] mVertices;
        delete[] mNormals;
        delete[] mTangents;
        delete[] mBitangents;
        for (int i = 0; i < 8; i++) {
            delete[] mColors[i];
            delete[] mTextureCoords[i];
        }
        delete[] mFaces;
        // Note: mBones and mAnimMeshes are nullptr in our minimal implementation
    }
    
    // Methods that Model.h expects
    bool HasNormals() const { return mNormals != nullptr; }
};

// ASSIMP material property type info and constants
enum aiPropertyTypeInfo {
    aiPTI_Float   = 0x1,
    aiPTI_String  = 0x3,
    aiPTI_Integer = 0x4,
    aiPTI_Buffer  = 0x5
};

enum aiReturn {
    aiReturn_SUCCESS = 0x0,
    aiReturn_FAILURE = -0x1,
    aiReturn_OUTOFMEMORY = -0x3
};

// ASSIMP material property keys
#define AI_MATKEY_COLOR_DIFFUSE "$clr.diffuse",0,0
#define AI_MATKEY_COLOR_AMBIENT "$clr.ambient",0,0
#define AI_MATKEY_COLOR_SPECULAR "$clr.specular",0,0
#define AI_MATKEY_TEXTURE_BASE "$tex.file"
#define AI_MATKEY_TEXTURE(type, N) AI_MATKEY_TEXTURE_BASE,type,N

struct aiMaterialProperty {
    aiString mKey;
    unsigned int mSemantic;
    unsigned int mIndex;
    unsigned int mDataLength;
    aiPropertyTypeInfo mType;
    char* mData;
    
    aiMaterialProperty() : mSemantic(0), mIndex(0), mDataLength(0), mType(aiPTI_Float), mData(nullptr) {}
    ~aiMaterialProperty() { delete[] mData; }
};

struct aiMaterial {
    /** CRITICAL: Must match exact memory layout of real aiMaterial! */
    aiMaterialProperty** mProperties;
    unsigned int mNumProperties;
    unsigned int mNumAllocated;
    
    aiMaterial() : mProperties(nullptr), mNumProperties(0), mNumAllocated(0) {}
    
    ~aiMaterial() {
        for (unsigned int i = 0; i < mNumProperties; i++) {
            delete mProperties[i];
        }
        delete[] mProperties;
    }
    
    // Add a color property to the material
    aiReturn AddProperty(const aiColor3D* pInput, const char* pKey, unsigned int type = 0, unsigned int index = 0) {
        aiMaterialProperty* prop = new aiMaterialProperty();
        prop->mKey = aiString(pKey);
        prop->mSemantic = type;
        prop->mIndex = index;
        prop->mType = aiPTI_Float;
        prop->mDataLength = sizeof(aiColor3D);
        prop->mData = new char[prop->mDataLength];
        memcpy(prop->mData, pInput, prop->mDataLength);
        
        // Resize array if needed
        if (mNumProperties >= mNumAllocated) {
            mNumAllocated = (mNumAllocated == 0) ? 4 : mNumAllocated * 2;
            aiMaterialProperty** newProps = new aiMaterialProperty*[mNumAllocated];
            for (unsigned int i = 0; i < mNumProperties; i++) {
                newProps[i] = mProperties[i];
            }
            delete[] mProperties;
            mProperties = newProps;
        }
        
        mProperties[mNumProperties++] = prop;
        return aiReturn_SUCCESS;
    }
    
    // Add a string (texture path) property to the material
    aiReturn AddTextureProperty(const std::string& texturePath, const char* pKey, unsigned int type = 0, unsigned int index = 0) {
        aiMaterialProperty* prop = new aiMaterialProperty();
        prop->mKey = aiString(pKey);
        prop->mSemantic = type;
        prop->mIndex = index;
        prop->mType = aiPTI_String;
        
        // Store aiString in the data buffer
        aiString textureString(texturePath);
        prop->mDataLength = sizeof(aiString);
        prop->mData = new char[prop->mDataLength];
        memcpy(prop->mData, &textureString, prop->mDataLength);
        
        // Resize array if needed
        if (mNumProperties >= mNumAllocated) {
            mNumAllocated = (mNumAllocated == 0) ? 4 : mNumAllocated * 2;
            aiMaterialProperty** newProps = new aiMaterialProperty*[mNumAllocated];
            for (unsigned int i = 0; i < mNumProperties; i++) {
                newProps[i] = mProperties[i];
            }
            delete[] mProperties;
            mProperties = newProps;
        }
        
        mProperties[mNumProperties++] = prop;
        return aiReturn_SUCCESS;
    }
    
    // Get a color property from the material
    aiReturn Get(const char* pKey, unsigned int type, unsigned int index, aiColor3D& pOut) const {
        for (unsigned int i = 0; i < mNumProperties; i++) {
            aiMaterialProperty* prop = mProperties[i];
            if (strcmp(prop->mKey.data, pKey) == 0 && prop->mSemantic == type && prop->mIndex == index) {
                if (prop->mType == aiPTI_Float && prop->mDataLength >= sizeof(aiColor3D)) {
                    memcpy(&pOut, prop->mData, sizeof(aiColor3D));
                    return aiReturn_SUCCESS;
                }
            }
        }
        return aiReturn_FAILURE;
    }
    
    // Methods that Model.h expects
    unsigned int GetTextureCount(aiTextureType type) const {
        unsigned int count = 0;
        for (unsigned int i = 0; i < mNumProperties; i++) {
            aiMaterialProperty* prop = mProperties[i];
            if (strcmp(prop->mKey.data, AI_MATKEY_TEXTURE_BASE) == 0 && 
                prop->mSemantic == static_cast<unsigned int>(type) &&
                prop->mType == aiPTI_String) {
                count++;
            }
        }
        return count;
    }
    
    int GetTexture(aiTextureType type, unsigned int index, aiString* path) const {
        if (!path) return 1; // aiReturn_FAILURE
        
        for (unsigned int i = 0; i < mNumProperties; i++) {
            aiMaterialProperty* prop = mProperties[i];
            if (strcmp(prop->mKey.data, AI_MATKEY_TEXTURE_BASE) == 0 && 
                prop->mSemantic == static_cast<unsigned int>(type) &&
                prop->mIndex == index &&
                prop->mType == aiPTI_String &&
                prop->mDataLength >= sizeof(aiString)) {
                
                // Copy the aiString from the property data
                memcpy(path, prop->mData, sizeof(aiString));
                return 0; // aiReturn_SUCCESS
            }
        }
        return 1; // aiReturn_FAILURE - texture not found
    }
};

struct aiMatrix4x4 {
    float a1, a2, a3, a4;
    float b1, b2, b3, b4; 
    float c1, c2, c3, c4;
    float d1, d2, d3, d4;
    aiMatrix4x4() : a1(1), a2(0), a3(0), a4(0), b1(0), b2(1), b3(0), b4(0), 
                    c1(0), c2(0), c3(1), c4(0), d1(0), d2(0), d3(0), d4(1) {}
};

struct aiMetadata; // Forward declaration

struct aiNode {
    /** CRITICAL: Must match exact memory layout of real aiNode! */
    aiString mName;
    aiMatrix4x4 mTransformation;
    aiNode* mParent;
    unsigned int mNumChildren;
    aiNode** mChildren;
    unsigned int mNumMeshes;
    unsigned int* mMeshes;
    aiMetadata* mMetaData;
    
    aiNode() : mParent(nullptr), mNumChildren(0), mChildren(nullptr), 
               mNumMeshes(0), mMeshes(nullptr), mMetaData(nullptr) {}
    
    // Custom constructor for proper WebAssembly initialization
    aiNode(unsigned int numMeshes, unsigned int meshIndex) 
        : mParent(nullptr), mNumChildren(0), mChildren(nullptr),
          mNumMeshes(numMeshes), mMetaData(nullptr) {
        if (numMeshes > 0) {
            mMeshes = new unsigned int[numMeshes];
            mMeshes[0] = meshIndex;
        } else {
            mMeshes = nullptr;
        }
    }
    
    ~aiNode() {
        delete[] mMeshes;
        for (unsigned int i = 0; i < mNumChildren; i++) {
            delete mChildren[i];
        }
        delete[] mChildren;
        // Note: don't delete mMetaData as it's nullptr in our minimal implementation
    }
};

struct aiScene {
    /** CRITICAL: Must match exact memory layout of real aiScene! */
    unsigned int mFlags;
    aiNode* mRootNode;
    unsigned int mNumMeshes;
    aiMesh** mMeshes;
    unsigned int mNumMaterials;
    aiMaterial** mMaterials;
    unsigned int mNumAnimations;
    void** mAnimations;  // aiAnimation** but we don't need full implementation
    unsigned int mNumTextures;
    void** mTextures;    // aiTexture** but we don't need full implementation
    unsigned int mNumLights;
    void** mLights;      // aiLight** but we don't need full implementation
    unsigned int mNumCameras;
    void** mCameras;     // aiCamera** but we don't need full implementation
    void* mPrivate;      // Private data pointer
    
    aiScene() : mFlags(AI_SCENE_FLAGS_INCOMPLETE), mRootNode(nullptr),
                mNumMeshes(0), mMeshes(nullptr), mNumMaterials(0), mMaterials(nullptr),
                mNumAnimations(0), mAnimations(nullptr), mNumTextures(0), mTextures(nullptr),
                mNumLights(0), mLights(nullptr), mNumCameras(0), mCameras(nullptr),
                mPrivate(nullptr) {}
    
    ~aiScene() {
        for (unsigned int i = 0; i < mNumMeshes; i++) {
            delete mMeshes[i];
        }
        delete[] mMeshes;
        
        for (unsigned int i = 0; i < mNumMaterials; i++) {
            delete mMaterials[i];
        }
        delete[] mMaterials;
        
        delete mRootNode;
        // Note: We don't delete other pointers as they're null in our minimal implementation
    }
};

// Assimp::Importer method implementations
Assimp::Importer::Importer() : m_scene(nullptr), m_error("") {}

Assimp::Importer::~Importer() { 
    if (m_scene) {
        delete m_scene;
    }
}

const char* Assimp::Importer::GetErrorString() const { 
    return m_error.c_str(); 
}

// Helper function to get directory from file path
std::string Assimp::Importer::GetDirectory(const std::string& path) {
    size_t lastSlash = path.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        return path.substr(0, lastSlash + 1);
    }
    return "";
}

// MTL file loader
bool Assimp::Importer::LoadMTL(const std::string& mtlPath, std::vector<aiMaterial*>& materials, std::map<std::string, int>& materialMap) {
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        EM_ASM_({
            console.log('MTL DEBUG: Cannot open MTL file: ' + UTF8ToString($0));
        }, mtlPath.c_str());
        return false;
    }
    
    aiMaterial* currentMaterial = nullptr;
    std::string line;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "newmtl") {
            std::string materialName;
            iss >> materialName;
            
            // Create new material
            currentMaterial = new aiMaterial();
            materials.push_back(currentMaterial);
            materialMap[materialName] = materials.size() - 1;
            
            EM_ASM_({
                console.log('MTL DEBUG: Created material: ' + UTF8ToString($0) + ' at index ' + $1);
            }, materialName.c_str(), static_cast<int>(materials.size() - 1));
        }
        else if (type == "Kd" && currentMaterial) {
            // Diffuse color
            float r, g, b;
            iss >> r >> g >> b;
            aiColor3D diffuseColor(r, g, b);
            currentMaterial->AddProperty(&diffuseColor, AI_MATKEY_COLOR_DIFFUSE);
            
            EM_ASM_({
                console.log('MTL DEBUG: Set diffuse color (' + $0 + ', ' + $1 + ', ' + $2 + ')');
            }, r, g, b);
        }
        else if (type == "Ka" && currentMaterial) {
            // Ambient color
            float r, g, b;
            iss >> r >> g >> b;
            aiColor3D ambientColor(r, g, b);
            currentMaterial->AddProperty(&ambientColor, AI_MATKEY_COLOR_AMBIENT);
        }
        else if (type == "Ks" && currentMaterial) {
            // Specular color
            float r, g, b;
            iss >> r >> g >> b;
            aiColor3D specularColor(r, g, b);
            currentMaterial->AddProperty(&specularColor, AI_MATKEY_COLOR_SPECULAR);
        }
        else if (type == "map_Kd" && currentMaterial) {
            // Diffuse texture map
            std::string texturePath;
            iss >> texturePath;
            currentMaterial->AddTextureProperty(texturePath, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));
            
            EM_ASM_({
                console.log('MTL DEBUG: Added diffuse texture: ' + UTF8ToString($0));
            }, texturePath.c_str());
        }
        else if (type == "map_Ka" && currentMaterial) {
            // Ambient texture map
            std::string texturePath;
            iss >> texturePath;
            currentMaterial->AddTextureProperty(texturePath, AI_MATKEY_TEXTURE(aiTextureType_AMBIENT, 0));
            
            EM_ASM_({
                console.log('MTL DEBUG: Added ambient texture: ' + UTF8ToString($0));
            }, texturePath.c_str());
        }
        else if (type == "map_Ks" && currentMaterial) {
            // Specular texture map
            std::string texturePath;
            iss >> texturePath;
            currentMaterial->AddTextureProperty(texturePath, AI_MATKEY_TEXTURE(aiTextureType_SPECULAR, 0));
            
            EM_ASM_({
                console.log('MTL DEBUG: Added specular texture: ' + UTF8ToString($0));
            }, texturePath.c_str());
        }
    }
    
    EM_ASM_({
        console.log('MTL DEBUG: Loaded ' + $0 + ' materials from MTL file');
    }, static_cast<int>(materials.size()));
    
    return true;
}

// Simple OBJ loader implementation
bool Assimp::Importer::LoadOBJ(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        m_error = "Cannot open file: " + path;
        return false;
    }
    
    std::vector<aiVector3D> vertices;
    std::vector<aiVector3D> normals;
    std::vector<aiVector3D> texCoords;
    std::vector<std::array<int, 9>> faces; // v/vt/vn indices for triangles
    
    // Material system
    std::vector<aiMaterial*> materials;
    std::map<std::string, int> materialMap;
    int currentMaterial = -1;
    std::vector<int> faceMaterials; // Material index for each face
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vertices.push_back(aiVector3D(x, y, z));
        }
        else if (type == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            normals.push_back(aiVector3D(x, y, z));
        }
        else if (type == "vt") {
            float u, v;
            iss >> u >> v;
            texCoords.push_back(aiVector3D(u, v, 0));
        }
        else if (type == "mtllib") {
            std::string mtlFile;
            iss >> mtlFile;
            std::string mtlPath = GetDirectory(path) + mtlFile;
            
            EM_ASM_({
                console.log('MTL DEBUG: Loading MTL file: ' + UTF8ToString($0));
            }, mtlPath.c_str());
            
            LoadMTL(mtlPath, materials, materialMap);
        }
        else if (type == "usemtl") {
            std::string materialName;
            iss >> materialName;
            
            auto it = materialMap.find(materialName);
            if (it != materialMap.end()) {
                currentMaterial = it->second;
                EM_ASM_({
                    console.log('MTL DEBUG: Using material: ' + UTF8ToString($0) + ' (index ' + $1 + ')');
                }, materialName.c_str(), currentMaterial);
            }
        }
        else if (type == "f") {
            std::string vertex;
            std::vector<std::array<int, 3>> faceVerts;
            
            while (iss >> vertex) {
                std::array<int, 3> indices = {-1, -1, -1};
                size_t pos1 = vertex.find('/');
                if (pos1 != std::string::npos) {
                    indices[0] = std::stoi(vertex.substr(0, pos1)) - 1;
                    size_t pos2 = vertex.find('/', pos1 + 1);
                    if (pos2 != std::string::npos) {
                        if (pos2 > pos1 + 1) {
                            indices[1] = std::stoi(vertex.substr(pos1 + 1, pos2 - pos1 - 1)) - 1;
                        }
                        if (pos2 + 1 < vertex.length()) {
                            indices[2] = std::stoi(vertex.substr(pos2 + 1)) - 1;
                        }
                    }
                } else {
                    indices[0] = std::stoi(vertex) - 1;
                }
                faceVerts.push_back(indices);
            }
            
            // Convert to triangles if needed - use separate vector to avoid iterator corruption
            for (size_t i = 1; i < faceVerts.size() - 1; i++) {
                std::array<int, 9> triangle;
                // Triangle: 0, i, i+1
                for (int j = 0; j < 3; j++) {
                    triangle[j] = faceVerts[0][j];
                    triangle[j+3] = faceVerts[i][j];
                    triangle[j+6] = faceVerts[i+1][j];
                }
                faces.push_back(triangle);
                faceMaterials.push_back(currentMaterial); // Store material index for this face
            }
        }
    }
    
    if (vertices.empty()) {
        m_error = "No vertices found in OBJ file";
        return false;
    }
    
    // Debug OBJ parsing results
    EM_ASM_({
        console.log('OBJ PARSING DEBUG: vertices = ' + $0 + ', normals = ' + $1 + ', texCoords = ' + $2 + ', faces = ' + $3);
    }, static_cast<int>(vertices.size()), static_cast<int>(normals.size()), 
       static_cast<int>(texCoords.size()), static_cast<int>(faces.size()));
    
    // Create aiScene
    m_scene = new aiScene();
    
    // Create single mesh
    m_scene->mNumMeshes = 1;
    m_scene->mMeshes = new aiMesh*[1];
    m_scene->mMeshes[0] = new aiMesh();
    aiMesh* mesh = m_scene->mMeshes[0];
    
    // Set up vertices
    mesh->mNumVertices = faces.size() * 3;
    EM_ASM_({
        console.log('LOADOBJ DEBUG: After assignment, mesh->mNumVertices = ' + $0);
        console.log('LOADOBJ DEBUG: Expected: faces.size() * 3 = ' + $1 + ' * 3 = ' + $2);
    }, static_cast<int>(mesh->mNumVertices), static_cast<int>(faces.size()), static_cast<int>(faces.size() * 3));
    mesh->mVertices = new aiVector3D[mesh->mNumVertices];
    mesh->mNormals = new aiVector3D[mesh->mNumVertices];
    mesh->mTextureCoords[0] = new aiVector3D[mesh->mNumVertices];
    
    // Set up faces
    mesh->mNumFaces = faces.size();
    mesh->mFaces = new aiFace[mesh->mNumFaces];
    
    for (size_t i = 0; i < faces.size(); i++) {
        aiFace& face = mesh->mFaces[i];
        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3];
        
        for (int j = 0; j < 3; j++) {
            int vertIdx = i * 3 + j;
            face.mIndices[j] = vertIdx;
            
            // Vertex position
            int vIdx = faces[i][j*3];
            if (vIdx >= 0 && vIdx < vertices.size()) {
                mesh->mVertices[vertIdx] = vertices[vIdx];
            }
            
            // Texture coordinates
            int vtIdx = faces[i][j*3 + 1];
            if (vtIdx >= 0 && vtIdx < texCoords.size()) {
                mesh->mTextureCoords[0][vertIdx] = texCoords[vtIdx];
            }
            
            // Normal
            int vnIdx = faces[i][j*3 + 2];
            if (vnIdx >= 0 && vnIdx < normals.size()) {
                mesh->mNormals[vertIdx] = normals[vnIdx];
            } else {
                mesh->mNormals[vertIdx] = aiVector3D(0, 1, 0); // Default normal
            }
        }
    }
    
    // Set up materials - use loaded MTL materials if available
    if (!materials.empty()) {
        m_scene->mNumMaterials = materials.size();
        m_scene->mMaterials = new aiMaterial*[materials.size()];
        for (size_t i = 0; i < materials.size(); i++) {
            m_scene->mMaterials[i] = materials[i];
        }
        
        // Use first material as default, or material 0 if no face materials assigned
        mesh->mMaterialIndex = (!faceMaterials.empty() && faceMaterials[0] >= 0) ? faceMaterials[0] : 0;
        
        EM_ASM_({
            console.log('MATERIAL DEBUG: Using ' + $0 + ' materials from MTL file');
            console.log('MATERIAL DEBUG: Mesh material index = ' + $1);
        }, static_cast<int>(materials.size()), static_cast<int>(mesh->mMaterialIndex));
    } else {
        // Create default material if no MTL loaded
        m_scene->mNumMaterials = 1;
        m_scene->mMaterials = new aiMaterial*[1];
        m_scene->mMaterials[0] = new aiMaterial();
        mesh->mMaterialIndex = 0;
        
        // Add default white color
        aiColor3D defaultColor(0.8f, 0.8f, 0.8f);
        m_scene->mMaterials[0]->AddProperty(&defaultColor, AI_MATKEY_COLOR_DIFFUSE);
        
        EM_ASM_({
            console.log('MATERIAL DEBUG: No MTL file loaded, using default material');
        });
    }
    
    // Create root node using custom constructor for proper WebAssembly initialization
    m_scene->mRootNode = new aiNode(1, 0);  // 1 mesh, mesh index 0
    
    // Aggressive memory debugging for WebAssembly
    EM_ASM_({
        console.log('AINODE MEMORY DEBUG: aiNode size = ' + $0 + ' bytes');
        console.log('AINODE MEMORY DEBUG: aiNode pointer = 0x' + $1.toString(16));
        console.log('AINODE MEMORY DEBUG: After constructor - mNumMeshes = ' + $2 + ', mNumChildren = ' + $3);
        console.log('AINODE MEMORY DEBUG: mMeshes pointer = 0x' + $4.toString(16));
        console.log('AINODE MEMORY DEBUG: mChildren pointer = 0x' + $5.toString(16));
    }, static_cast<int>(sizeof(aiNode)), 
       reinterpret_cast<uintptr_t>(m_scene->mRootNode),
       static_cast<int>(m_scene->mRootNode->mNumMeshes), 
       static_cast<int>(m_scene->mRootNode->mNumChildren),
       reinterpret_cast<uintptr_t>(m_scene->mRootNode->mMeshes),
       reinterpret_cast<uintptr_t>(m_scene->mRootNode->mChildren));
    
    // Mark scene as complete (clear incomplete flag)
    m_scene->mFlags = 0;  // Clear all flags to indicate successful loading
    
    return true;
}

const aiScene* Assimp::Importer::ReadFile(const char* path, unsigned int flags) {
    if (m_scene) {
        delete m_scene;
        m_scene = nullptr;
    }
    
    m_error = "";
    
    // Enhanced debug logging with null checks
    if (path == nullptr) {
        EM_ASM_({
            console.log('ASSIMP DEBUG: NULL path received!');
        });
        m_error = "NULL path provided to ReadFile";
        return nullptr;
    }
    
    std::string path_str(path);
    if (path_str.empty()) {
        EM_ASM_({
            console.log('ASSIMP DEBUG: Empty path received!');
        });
        m_error = "Empty path provided to ReadFile";
        return nullptr;
    }
    
    EM_ASM_({
        console.log('ASSIMP DEBUG: Path received: ' + UTF8ToString($0));
        console.log('ASSIMP DEBUG: Path length: ' + $1);
    }, path, static_cast<int>(path_str.length()));
    
    if (LoadOBJ(path_str)) {
        EM_ASM_({
            console.log('ASSIMP DEBUG: Successfully loaded OBJ file');
        });
        return m_scene;
    } else {
        EM_ASM_({
            console.log('ASSIMP DEBUG: Failed to load OBJ file: ' + UTF8ToString($0));
        }, m_error.c_str());
        return nullptr;
    }
}

// C API functions
extern "C" {
    unsigned int aiGetMaterialTextureCount(const aiMaterial* mat, int type) {
        return 0; // Simplified - no textures for now
    }
    
    int aiGetMaterialTexture(const aiMaterial* mat, int type, unsigned int index, 
                           aiString* path, int* mapping, unsigned int* uvindex,
                           float* blend, int* op, int* mapmode, unsigned int* flags) {
        return 1; // Return error - no textures
    }
    
    int aiGetMaterialColor(const aiMaterial* mat, const char* pKey, unsigned int type, 
                          unsigned int index, aiColor3D* pOut) {
        if (!mat || !pOut) return 1; // aiReturn_FAILURE
        return mat->Get(pKey, type, index, *pOut);
    }
}

#endif // __EMSCRIPTEN__
