#pragma once

#include <cstdint>
#include "Base.hpp"

namespace Motion
{
    enum class DepthFunction : std::uint32_t
    {
        Never           = 0,
        Less            = 1,
        Equal           = 2,
        LessEqual       = 3,
        Greater         = 4,
        NotEqual        = 5,
        GreaterEqual    = 6,
        Always          = 7
    };

    enum class BlendFactor : std::uint32_t
    {
        Zero                   = 0,
        One                    = 1,
        SrcColor               = 2,
        OneMinusSrcColor       = 3,
        DstColor               = 4,
        OneMinusDstColor       = 5,
        SrcAlpha               = 6,
        OneMinusSrcAlpha       = 7,
        DstAlpha               = 8,
        OneMinusDstAlpha       = 9,
        ConstantColor          = 10,
        OneMinusConstantColor  = 11,
        ConstantAlpha          = 12,
        OneMinusConstantAlpha  = 13,
        SrcAlphaSaturate       = 14
    };

    enum class BlendEquation : std::uint32_t
    {
        Add                 = 0,
        Subtract            = 1,
        ReverseSubtract     = 2,
        Min                 = 3,
        Max                 = 4
    };


    enum class CullMode : std::uint32_t
    {
        None                = 0,
        Back                = 1,
        Front               = 2,
        FrontAndBack        = 3
    };

    enum class FrontFace : std::uint32_t
    {
        CCW                 = 0,
        CW                  = 1
    };

    struct ScissorRect
    {
        std::int32_t X{0}, Y{0}, Width{0}, Height{0};
    };

    inline std::uint32_t operator|(DepthFunction a, DepthFunction b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(DepthFunction a, DepthFunction b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator|(BlendFactor a, BlendFactor b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(BlendFactor a, BlendFactor b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator|(BlendEquation a, BlendEquation b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(BlendEquation a, BlendEquation b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator|(CullMode a, CullMode b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(CullMode a, CullMode b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator|(FrontFace a, FrontFace b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(FrontFace a, FrontFace b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }

    struct StageStatus
    {
        bool DepthTest { true };
        bool DepthWrite { true };
        DepthFunction DepthFunc { DepthFunction::Less };

        bool CullEnabled { false };
        CullMode CullingMode { CullMode::Back };
        FrontFace FrontFace { FrontFace::CCW };
        bool Wireframe { false };

        bool PolygonOffsetEnabled { false };
        float PolygonOffsetFactor { 0.0f };
        float PolygonOffsetUnits { 0.0f };

        bool BlendEnabled { false };
        BlendEquation BlendEqRGB { BlendEquation::Add };
        BlendEquation BlendEqA { BlendEquation::Add };
        BlendFactor SrcRGB { BlendFactor::SrcAlpha };
        BlendFactor DstRGB { BlendFactor::OneMinusSrcAlpha };
        BlendFactor SrcA { BlendFactor::One };
        BlendFactor DstA { BlendFactor::OneMinusSrcAlpha };
        float BlendConst[4]{ 0.f, 0.f, 0.f, 0.f };

        bool ColorMaskR { true }, ColorMaskG { true }, ColorMaskB { true }, ColorMaskA { true };
        bool ScissorEnabled { false };
        ScissorRect Scissor;
    };

    class IRenderingStage
    {
        public:
            IRenderingStage() = default;
            ~IRenderingStage() = default;

            virtual void Apply(const StageStatus& state) = 0;
            virtual StageStatus Snapshot() const = 0; 
            virtual void Restore(const StageStatus& state) = 0; 
            virtual const StageStatus& Defaults() const = 0; 

            virtual void SetDepthTest(bool enabled) = 0;
            virtual void SetDepthWrite(bool enabled) = 0;
            virtual void SetDepthFunc(DepthFunction func) = 0;

            virtual void SetCullEnabled(bool enabled) = 0;
            virtual void SetCullMode(CullMode mode) = 0;
            virtual void SetFrontFace(FrontFace ff) = 0;
            virtual void SetWireframe(bool enabled) = 0;

            virtual void SetPolygonOffsetEnabled(bool enabled) = 0;
            virtual void SetPolygonOffset(float factor, float units) = 0;

            virtual void SetBlendEnabled(bool enabled) = 0;
            virtual void SetBlendFuncSeparate(BlendFactor srcRGB, BlendFactor dstRGB, BlendFactor srcA, BlendFactor dstA) = 0;
            virtual void SetBlendEquationSeparate(BlendEquation eqRGB, BlendEquation eqA) = 0;
            virtual void SetBlendConstant(float r, float g, float b, float a) = 0;

            virtual void SetColorMask(bool r, bool g, bool b, bool a) = 0;
            virtual void SetScissorEnabled(bool enabled) = 0;
            virtual void SetScissorRect(ScissorRect rect) = 0;

        public: 
            static std::shared_ptr<IRenderingStage> Create();
    };
}