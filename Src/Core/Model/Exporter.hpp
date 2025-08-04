#pragma once

#include <assimp/scene.h>

#include "Base.hpp"
#include "Model.hpp"

#define MESH_BEGIN "[MESH_BEGIN]"
#define MESH_END "[MESH_END]"

#define META_BEGIN "[META_BEGIN]"
#define META_END "[META_END]"
#define META_VERSION(V) std::format("[VER:{}]", V)
#define META_ENDIAN(E) std::format("[ENDIAN:{}]", E)
#define META_COUNT(C) std::format("[COUNT:{}]", C)
#define META_BOUNDS(MIN, MAX) std::format("[BOUNDS:{},{},{}|{},{},{}]", MIN.x, MIN.y, MIN.z, MAX.x, MAX.y, MAX.z)
#define META_ID(ID) std::format("[ID:{}]", ID)
#define META_ENGINE(NAME) std::format("[ENGINE:{}]", NAME)
#define META_BUILD(DATE) std::format("[BUILD:{}]", DATE)

#define MATERIALS_BEGIN "[MATERIALS_BEGIN]"
#define MATERIALS_END "[MATERIALS_END]"
#define MATERIAL(ID) std::format("[MAT:{}]", ID)
#define MATERIAL_END "[MAT_END]"
#define MAT_ATTR_BEGIN "[ATTR_BEGIN]"
#define MAT_ATTR_END "[ATTR_END]"
#define ATTR_VEC3(NAME, V) std::format("[ATTR:{}:{},{},{}]", NAME, V.x, V.y, V.z)
#define ATTR_FLOAT(NAME, V) std::format("[ATTR:{}:{}]", NAME, V)

#define TEXTURES_BEGIN "[TEXTURES_BEGIN]"
#define TEXTURES_END "[TEXTURES_END]"
#define TEXTURE_DATA_END "[TEX_DATA_END]"
#define TEXTURE_DATA_BEGIN "[TEX_DATA_BEGIN]"
#define TEXTURE(ID) std::format("[TEX:{}]", ID)
#define TEXTURE_END "[TEX_END]"
#define TEX_META(TYPE, W, H, CH, S) std::format("[TEX_META:{},{},{},{},{}]", TYPE, W, H, CH, S)
#define TEX_REF(SLOT, ID) std::format("[TEX_REF:{}={}]", SLOT, ID)

#define MESH_BLOCK(ID) std::format("[M{}_BEGIN]", ID)
#define MESH_BLOCK_END(ID) std::format("[M{}_END]", ID)
#define VTX_BEGIN(N) std::format("[VTX:{}]", N)
#define VTX_END "[VTX_END]"
#define IDX_BEGIN(N) std::format("[IDX:{}]", N)
#define IDX_END "[IDX_END]"
#define MAT_REF(ID) std::format("[MAT_REF:{}]", ID)


namespace Motion
{
    struct EmbeddedTexture
    {
        std::uint8_t* Data{ nullptr };
        std::size_t Size{ 0 };
        std::int32_t Width{ 0 };
        std::int32_t Height{ 0 };
        std::int32_t Channels{ 0 };
        std::string Type{ "Unknown" };

        EmbeddedTexture() = default;
        EmbeddedTexture(std::uint8_t* data, std::size_t size, std::int32_t width, std::int32_t height, std::int32_t channels, const std::string& type)
            : Data(data), Size(size), Width(width), Height(height), Channels(channels), Type(type) {
        }

        ~EmbeddedTexture() = default;
    };

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