// bestow-assets/src/loaders/ModelLoader.cpp
// Model loading using assimp (FBX, glTF, OBJ, etc.)
//
// This file is part of the bestow.assets.impl module partition.
// It provides the implementation for loading 3D models with skeletal
// animation data.

module;

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <spdlog/spdlog.h>

module bestow.assets.impl;

import std;

namespace bestow {

//==========================================================================
// Assimp Conversion Helpers (internal to this translation unit)
//==========================================================================

namespace {

void assimpToMat4(const aiMatrix4x4& in, float out[16]) {
    // Assimp is row-major, we store column-major
    out[0]  = in.a1; out[4]  = in.a2; out[8]  = in.a3; out[12] = in.a4;
    out[1]  = in.b1; out[5]  = in.b2; out[9]  = in.b3; out[13] = in.b4;
    out[2]  = in.c1; out[6]  = in.c2; out[10] = in.c3; out[14] = in.c4;
    out[3]  = in.d1; out[7]  = in.d2; out[11] = in.d3; out[15] = in.d4;
}

MeshData processAssimpMesh(const aiMesh* mesh, const std::vector<ModelData::Bone>& skeletonBones) {
    MeshData meshData;
    meshData.name = mesh->mName.C_Str();

    meshData.vertices.reserve(mesh->mNumVertices);
    meshData.indices.reserve(mesh->mNumFaces * 3);

    // Track if we have tangents and bone data
    meshData.hasTangents = mesh->HasTangentsAndBitangents();
    meshData.hasBoneData = mesh->HasBones();

    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        Vertex3DData vertex{};

        // Position
        vertex.position[0] = mesh->mVertices[i].x;
        vertex.position[1] = mesh->mVertices[i].y;
        vertex.position[2] = mesh->mVertices[i].z;

        // Normal
        if (mesh->HasNormals()) {
            vertex.normal[0] = mesh->mNormals[i].x;
            vertex.normal[1] = mesh->mNormals[i].y;
            vertex.normal[2] = mesh->mNormals[i].z;
        }

        // Texture coordinates
        if (mesh->HasTextureCoords(0)) {
            vertex.texCoord[0] = mesh->mTextureCoords[0][i].x;
            vertex.texCoord[1] = mesh->mTextureCoords[0][i].y;
        }

        // Tangent (with handedness in w)
        if (mesh->HasTangentsAndBitangents()) {
            vertex.tangent[0] = mesh->mTangents[i].x;
            vertex.tangent[1] = mesh->mTangents[i].y;
            vertex.tangent[2] = mesh->mTangents[i].z;
            // Calculate handedness from bitangent
            const auto& n = mesh->mNormals[i];
            const auto& t = mesh->mTangents[i];
            const auto& b = mesh->mBitangents[i];
            float cross_x = n.y * t.z - n.z * t.y;
            float cross_y = n.z * t.x - n.x * t.z;
            float cross_z = n.x * t.y - n.y * t.x;
            float dot = cross_x * b.x + cross_y * b.y + cross_z * b.z;
            vertex.tangent[3] = (dot < 0.0f) ? -1.0f : 1.0f;
        }

        // Vertex color
        if (mesh->HasVertexColors(0)) {
            auto& c = mesh->mColors[0][i];
            vertex.color[0] = c.r;
            vertex.color[1] = c.g;
            vertex.color[2] = c.b;
            vertex.color[3] = c.a;
        } else {
            vertex.color[0] = 1.0f;
            vertex.color[1] = 1.0f;
            vertex.color[2] = 1.0f;
            vertex.color[3] = 1.0f;
        }

        // Bone weights/indices initialized to zero (will be filled below)
        for (int j = 0; j < 4; ++j) {
            vertex.boneIndices[j] = 0;
            vertex.boneWeights[j] = 0.0f;
        }

        meshData.vertices.push_back(vertex);
    }

    // Process bone weights - store directly in vertex data
    // IMPORTANT: Must look up skeleton bone index by name, not use mesh bone array index
    if (mesh->HasBones()) {
        for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
            const aiBone* bone = mesh->mBones[b];
            std::string boneName = bone->mName.C_Str();

            // Find skeleton bone index by name
            int skeletonBoneIndex = -1;
            for (std::size_t s = 0; s < skeletonBones.size(); ++s) {
                if (skeletonBones[s].name == boneName) {
                    skeletonBoneIndex = static_cast<int>(s);
                    break;
                }
            }

            if (skeletonBoneIndex < 0) {
                continue;  // Bone not found in skeleton, skip
            }

            for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                unsigned int vertexId = bone->mWeights[w].mVertexId;
                float weight = bone->mWeights[w].mWeight;

                auto& vertex = meshData.vertices[vertexId];

                // Find an empty slot in the vertex's bone data
                for (int slot = 0; slot < 4; ++slot) {
                    if (vertex.boneWeights[slot] == 0.0f) {
                        vertex.boneWeights[slot] = weight;
                        vertex.boneIndices[slot] = static_cast<unsigned char>(skeletonBoneIndex);
                        break;
                    }
                }
            }
        }
    }

    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            meshData.indices.push_back(face.mIndices[j]);
        }
    }

    // Calculate bounds
    if (!meshData.vertices.empty()) {
        const auto& firstPos = meshData.vertices[0].position;
        float minBounds[3] = {firstPos[0], firstPos[1], firstPos[2]};
        float maxBounds[3] = {firstPos[0], firstPos[1], firstPos[2]};

        for (const auto& v : meshData.vertices) {
            minBounds[0] = std::min(minBounds[0], v.position[0]);
            minBounds[1] = std::min(minBounds[1], v.position[1]);
            minBounds[2] = std::min(minBounds[2], v.position[2]);
            maxBounds[0] = std::max(maxBounds[0], v.position[0]);
            maxBounds[1] = std::max(maxBounds[1], v.position[1]);
            maxBounds[2] = std::max(maxBounds[2], v.position[2]);
        }
        std::copy(minBounds, minBounds + 3, meshData.boundsMin);
        std::copy(maxBounds, maxBounds + 3, meshData.boundsMax);
    }

    // Create a submesh for the material
    SubMeshData subMesh;
    subMesh.indexOffset = 0;
    subMesh.indexCount = static_cast<std::uint32_t>(meshData.indices.size());
    subMesh.materialIndex = mesh->mMaterialIndex;
    std::copy(meshData.boundsMin, meshData.boundsMin + 3, subMesh.boundsMin);
    std::copy(meshData.boundsMax, meshData.boundsMax + 3, subMesh.boundsMax);
    meshData.subMeshes.push_back(subMesh);

    return meshData;
}

// Helper to extract embedded texture from Assimp scene
void extractEmbeddedTexture(const aiScene* scene, const std::string& texPath,
                            MaterialTextureRef& texRef) {
    texRef.path = texPath;

    // Use Assimp's GetEmbeddedTexture which handles both "*N" references
    // and file paths that might map to embedded textures
    const aiTexture* texture = scene->GetEmbeddedTexture(texPath.c_str());

    if (texture != nullptr) {
        if (texture->mHeight == 0) {
            // Compressed format (PNG, JPG, etc.) - mWidth is the size in bytes
            // The data needs to be decoded by stb_image or similar
            texRef.embeddedData.assign(
                reinterpret_cast<const unsigned char*>(texture->pcData),
                reinterpret_cast<const unsigned char*>(texture->pcData) + texture->mWidth
            );
            texRef.embeddedWidth = -1;  // Indicates compressed, needs decoding
            texRef.embeddedHeight = -1;
            texRef.embeddedChannels = 4;

            spdlog::info("[ModelLoader] Extracted embedded compressed texture '{}': {} bytes",
                         texPath, texture->mWidth);
        } else {
            // Raw RGBA data
            std::size_t dataSize = texture->mWidth * texture->mHeight * 4;
            texRef.embeddedData.resize(dataSize);

            // Convert from ARGB8888 to RGBA8888
            const unsigned char* src = reinterpret_cast<const unsigned char*>(texture->pcData);
            for (std::size_t i = 0; i < texture->mWidth * texture->mHeight; ++i) {
                texRef.embeddedData[i * 4 + 0] = src[i * 4 + 2];  // R
                texRef.embeddedData[i * 4 + 1] = src[i * 4 + 1];  // G
                texRef.embeddedData[i * 4 + 2] = src[i * 4 + 0];  // B
                texRef.embeddedData[i * 4 + 3] = src[i * 4 + 3];  // A
            }

            texRef.embeddedWidth = static_cast<int>(texture->mWidth);
            texRef.embeddedHeight = static_cast<int>(texture->mHeight);
            texRef.embeddedChannels = 4;

            spdlog::info("[ModelLoader] Extracted embedded raw texture '{}': {}x{}",
                         texPath, texture->mWidth, texture->mHeight);
        }
    } else {
        spdlog::info("[ModelLoader] No embedded texture found for path: '{}'", texPath);
    }
}

MaterialData processAssimpMaterial(const aiMaterial* material, const aiScene* scene) {
    MaterialData matData;

    aiString name;
    if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        matData.name = name.C_Str();
    }

    // Base color (PBR) or diffuse color (legacy)
    aiColor4D baseColor;
    if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
        matData.baseColorFactor[0] = baseColor.r;
        matData.baseColorFactor[1] = baseColor.g;
        matData.baseColorFactor[2] = baseColor.b;
        matData.baseColorFactor[3] = baseColor.a;
    } else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS) {
        matData.baseColorFactor[0] = baseColor.r;
        matData.baseColorFactor[1] = baseColor.g;
        matData.baseColorFactor[2] = baseColor.b;
        matData.baseColorFactor[3] = baseColor.a;
    }

    // Metallic factor
    float metallic = 0.0f;
    if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
        matData.metallicFactor = metallic;
    }

    // Roughness factor
    float roughness = 1.0f;
    if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
        matData.roughnessFactor = roughness;
    }

    // Emissive color
    aiColor3D emissive;
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
        matData.emissiveFactor[0] = emissive.r;
        matData.emissiveFactor[1] = emissive.g;
        matData.emissiveFactor[2] = emissive.b;
    }

    // Texture paths and embedded textures
    aiString texPath;
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
        spdlog::info("[ModelLoader] Material '{}' has diffuse texture: '{}'",
                     matData.name, texPath.C_Str());
        extractEmbeddedTexture(scene, texPath.C_Str(), matData.baseColorTexture);
    } else {
        spdlog::info("[ModelLoader] Material '{}' has no diffuse texture", matData.name);
    }
    if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
        extractEmbeddedTexture(scene, texPath.C_Str(), matData.normalTexture);
    }
    if (material->GetTexture(aiTextureType_METALNESS, 0, &texPath) == AI_SUCCESS) {
        extractEmbeddedTexture(scene, texPath.C_Str(), matData.metallicRoughnessTexture);
    }
    if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texPath) == AI_SUCCESS) {
        extractEmbeddedTexture(scene, texPath.C_Str(), matData.occlusionTexture);
    }
    if (material->GetTexture(aiTextureType_EMISSIVE, 0, &texPath) == AI_SUCCESS) {
        extractEmbeddedTexture(scene, texPath.C_Str(), matData.emissiveTexture);
    }

    // Double-sided rendering
    int twoSided = 0;
    if (material->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS) {
        matData.doubleSided = (twoSided != 0);
    }

    return matData;
}

void processAssimpNode(const aiNode* node, ModelData& modelData, int parentIndex) {
    ModelData::Node nodeData;
    nodeData.name = node->mName.C_Str();
    nodeData.parentIndex = parentIndex;
    assimpToMat4(node->mTransformation, nodeData.localTransform);

    int currentIndex = static_cast<int>(modelData.nodes.size());

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        nodeData.meshIndex = static_cast<int>(node->mMeshes[i]);
    }

    modelData.nodes.push_back(nodeData);

    if (parentIndex >= 0) {
        modelData.nodes[parentIndex].children.push_back(currentIndex);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processAssimpNode(node->mChildren[i], modelData, currentIndex);
    }
}

void processAssimpBones(const aiScene* scene, ModelData& modelData) {
    std::unordered_map<std::string, int> boneNameToIndex;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        if (!mesh->HasBones()) continue;

        for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
            const aiBone* bone = mesh->mBones[b];
            std::string boneName = bone->mName.C_Str();

            if (boneNameToIndex.find(boneName) == boneNameToIndex.end()) {
                int boneIndex = static_cast<int>(modelData.bones.size());
                boneNameToIndex[boneName] = boneIndex;

                ModelData::Bone boneData;
                boneData.name = boneName;
                boneData.parentIndex = -1;
                assimpToMat4(bone->mOffsetMatrix, boneData.offsetMatrix);

                modelData.bones.push_back(boneData);
            }
        }
    }

    // Establish parent relationships
    std::function<void(const aiNode*, int)> findBoneParent = [&](const aiNode* node, int parentBoneIndex) {
        std::string nodeName = node->mName.C_Str();
        auto it = boneNameToIndex.find(nodeName);
        int currentBoneIndex = (it != boneNameToIndex.end()) ? it->second : parentBoneIndex;

        if (it != boneNameToIndex.end() && parentBoneIndex >= 0) {
            modelData.bones[it->second].parentIndex = parentBoneIndex;
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            findBoneParent(node->mChildren[i], currentBoneIndex);
        }
    };

    findBoneParent(scene->mRootNode, -1);
}

void processAssimpAnimations(const aiScene* scene, ModelData& modelData) {
    for (unsigned int a = 0; a < scene->mNumAnimations; ++a) {
        const aiAnimation* anim = scene->mAnimations[a];

        ModelData::Animation animation;
        animation.name = anim->mName.C_Str();
        animation.duration = static_cast<float>(anim->mDuration);
        animation.ticksPerSecond = anim->mTicksPerSecond > 0
            ? static_cast<float>(anim->mTicksPerSecond)
            : 30.0f;

        spdlog::info("[ModelLoader] Animation '{}': duration={}, ticksPerSecond={}, calculated={}s",
                     animation.name, anim->mDuration, anim->mTicksPerSecond,
                     animation.duration / animation.ticksPerSecond);

        for (unsigned int c = 0; c < anim->mNumChannels; ++c) {
            const aiNodeAnim* channel = anim->mChannels[c];

            ModelData::AnimationChannel channelData;
            channelData.boneIndex = -1;
            channelData.boneName = channel->mNodeName.C_Str();

            // Try to find bone index in this file's skeleton (may not exist for animation-only files)
            for (std::size_t b = 0; b < modelData.bones.size(); ++b) {
                if (modelData.bones[b].name == channelData.boneName) {
                    channelData.boneIndex = static_cast<int>(b);
                    break;
                }
            }

            // Keep the channel even if boneIndex is -1 - we'll look up by name later
            // Only skip if boneName is empty
            if (channelData.boneName.empty()) continue;

            // Collect unique keyframe times
            std::set<float> keyframeTimes;
            for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k) {
                keyframeTimes.insert(static_cast<float>(channel->mPositionKeys[k].mTime));
            }
            for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k) {
                keyframeTimes.insert(static_cast<float>(channel->mRotationKeys[k].mTime));
            }
            for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k) {
                keyframeTimes.insert(static_cast<float>(channel->mScalingKeys[k].mTime));
            }

            for (float time : keyframeTimes) {
                ModelData::AnimationKeyframe keyframe;
                keyframe.time = time;

                // Position
                aiVector3D pos(0, 0, 0);
                for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k) {
                    if (static_cast<float>(channel->mPositionKeys[k].mTime) >= time) {
                        pos = channel->mPositionKeys[k].mValue;
                        break;
                    }
                    pos = channel->mPositionKeys[k].mValue;
                }
                keyframe.translation[0] = pos.x;
                keyframe.translation[1] = pos.y;
                keyframe.translation[2] = pos.z;

                // Rotation
                aiQuaternion rot(1, 0, 0, 0);
                for (unsigned int k = 0; k < channel->mNumRotationKeys; ++k) {
                    if (static_cast<float>(channel->mRotationKeys[k].mTime) >= time) {
                        rot = channel->mRotationKeys[k].mValue;
                        break;
                    }
                    rot = channel->mRotationKeys[k].mValue;
                }
                keyframe.rotation[0] = rot.x;
                keyframe.rotation[1] = rot.y;
                keyframe.rotation[2] = rot.z;
                keyframe.rotation[3] = rot.w;

                // Scale
                aiVector3D scale(1, 1, 1);
                for (unsigned int k = 0; k < channel->mNumScalingKeys; ++k) {
                    if (static_cast<float>(channel->mScalingKeys[k].mTime) >= time) {
                        scale = channel->mScalingKeys[k].mValue;
                        break;
                    }
                    scale = channel->mScalingKeys[k].mValue;
                }
                keyframe.scale[0] = scale.x;
                keyframe.scale[1] = scale.y;
                keyframe.scale[2] = scale.z;

                channelData.keyframes.push_back(keyframe);
            }

            animation.channels.push_back(channelData);
        }

        modelData.animations.push_back(animation);
    }
}

}  // anonymous namespace

//==========================================================================
// AssetSystem Model Loading Implementation
//==========================================================================

ModelData AssetSystem::loadModelFromFile(const std::filesystem::path& path) {
    ModelData modelData;
    modelData.name = path.stem().string();

    unsigned int flags =
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_PopulateArmatureData;

    Assimp::Importer importer;

    // Disable FBX pivot preservation - this prevents Assimp from creating
    // split transform nodes ($AssimpFbx$_Translation, $AssimpFbx$_Rotation, etc.)
    // which complicate skeleton and animation handling.
    // See: https://github.com/assimp/assimp/issues/4620
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    const aiScene* scene = importer.ReadFile(path.string(), flags);

    if (!scene || !scene->mRootNode) {
        spdlog::error("[AssetSystem] Failed to load model: {} - {}",
                      path.string(), importer.GetErrorString());
        return modelData;
    }

    // Note: AI_SCENE_FLAGS_INCOMPLETE is set for animation-only files (no meshes)
    // This is valid for Mixamo animation files, so we don't treat it as an error

    // Process scene hierarchy first (needed for bone parent relationships)
    processAssimpNode(scene->mRootNode, modelData, -1);
    modelData.rootNodeIndex = 0;

    // Process bones/skeleton BEFORE meshes (meshes need bone info for weights)
    processAssimpBones(scene, modelData);

    // Process meshes (uses modelData.bones for bone weight mapping)
    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        modelData.meshes.push_back(processAssimpMesh(scene->mMeshes[i], modelData.bones));
    }

    // Process materials (including embedded textures)
    if (scene->HasMaterials()) {
        spdlog::info("[AssetSystem] Processing {} materials, {} embedded textures",
                     scene->mNumMaterials, scene->mNumTextures);
        for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
            modelData.materials.push_back(processAssimpMaterial(scene->mMaterials[i], scene));
        }
    }

    // Process animations
    if (scene->HasAnimations()) {
        processAssimpAnimations(scene, modelData);
    }

    spdlog::info("[AssetSystem] Loaded model: {} ({} meshes, {} bones, {} animations)",
                 path.string(), modelData.meshes.size(),
                 modelData.bones.size(), modelData.animations.size());

    return modelData;
}

}  // namespace bestow
