#pragma once

#include <assimp/scene.h>

#include "Base.hpp"
#include "Model.hpp"

#define MOTION_SECTION_FILE_BEGIN "[MMESH_BEGIN]"
#define MOTION_SECTION_FILE_END "[MMESH_END]"

#define MOTION_SECTION_META_BEGIN "[M_META_BEGIN]"
#define MOTION_SUBSECTION_META_MESH_COUNT(COUNT) std::format("[M_MSH_COUNT:{}]", COUNT)
#define MOTION_SUBSECTION_META_VERSION(VERSION) std::format("[M_META_VERSION:{}]", VERSION)
#define MOTION_SECTION_META_END "[M_META_END]"

#define MOTION_SECTION_VTX_BEGIN(meshIndex, size) std::format("[M_VTX_{}_BEGIN:{}]", meshIndex, size)
#define MOTION_SECTION_VTX_END(meshIndex) std::format("[M_VTX_{}_END]", meshIndex)

#define MOTION_SECTION_IDX_BEGIN(meshIndex, size) std::format("[M_IDX_{}_BEGIN:{}]", meshIndex, size)
#define MOTION_SECTION_IDX_END(meshIndex) std::format("[M_IDX_{}_END]", meshIndex)

#define MOTION_SECTION_MAT_BEGIN(meshIndex) std::format("[M_MAT_{}_BEGIN]", meshIndex)
#define MOTION_SECTION_MAT_END(meshIndex) std::format("[M_MAT_{}_END]", meshIndex)

#define MOTION_SUBSECTION_MAT_ATTRIBUTES_BEGIN std::format("[MAT_ATTRIBUTES_BEGIN]")
#define MOTION_SUBSECTION_MAT_ATTRIBUTES_END std::format("[MAT_ATTRIBUTES_END]")

#define MOTION_SUBSECTION_MAT_ATTRIBUTES_VEC3(name, attributes) std::format("[MAT_ATTRIBUTES:[{}]:[{},{},{}]]", name, attributes.x, attributes.y, attributes.z)
#define MOTION_SUBSECTION_MAT_ATTRIBUTES_FLOAT(name, attributes) std::format("[MAT_ATTRIBUTES:[{}]:{}]", name, attributes)

#define MOTION_MAT_ATTRIBUTE_BASE_COLOR "BaseColor"
#define MOTION_MAT_ATTRIBUTE_METALLIC "Metallic"
#define MOTION_MAT_ATTRIBUTE_ROUGHNESS "Roughness"
#define MOTION_MAT_ATTRIBUTE_AMBIENT_OCCLUSION "AmbientOcclusion"
#define MOTION_MAT_ATTRIBUTE_OPACITY "Opacity"
#define MOTION_MAT_ATTRIBUTE_DISPLACEMENT_SCALE "DisplacementScale"

#define MOTION_SUBSECTION_MAT_TEXTURE_TYPE(textureType) std::format("[MAT_TEXTURE_TYPE:{}]", MOTION_TOSTR(textureType))
#define MOTION_SUBSECTION_MAT_TEXTURE_DATA_BEGIN(size) std::format("[MAT_TEXTURE_DATA_BEGIN:{}]", size)
#define MOTION_SUBSECTION_MAT_TEXTURE_DATA_END std::format("[MAT_TEXTURE_DATA_END]")

#define MOTION_SUBSECTION_MAT_TEXTURE(OUTFILE, TEXTURE, DATA_PTR, DATA_SIZE) \
    CreateSection(OUTFILE, MOTION_SUBSECTION_MAT_TEXTURE_TYPE(TEXTURE));\
    if(DATA_PTR == nullptr || DATA_SIZE == 0)\
    {\
        CreateSection(OUTFILE, MOTION_SUBSECTION_MAT_TEXTURE_DATA_BEGIN(DATA_SIZE)); \
        BinaryWriter<std::uint8_t*>::Write(OUTFILE, nullptr, 0); \
        CreateSection(OUTFILE, MOTION_SUBSECTION_MAT_TEXTURE_DATA_END); \
    }\
    else \
    { \
        CreateSection(OUTFILE, MOTION_SUBSECTION_MAT_TEXTURE_DATA_BEGIN(DATA_SIZE)); \
        BinaryWriter<std::uint8_t*>::Write(OUTFILE, DATA_PTR, DATA_SIZE); \
        CreateSection(OUTFILE, MOTION_SUBSECTION_MAT_TEXTURE_DATA_END); \
    } 

namespace Motion
{
    class Exporter
    {
    private:
        Exporter() = default;
        ~Exporter() = default;

        Exporter(const Exporter&) = delete;
        Exporter& operator=(const Exporter&) = delete;
        Exporter(Exporter&&) = delete;
        Exporter& operator=(Exporter&&) = delete;

    public:
        static bool ExportModel(const aiScene* scene, const std::filesystem::path& output);
    };
}