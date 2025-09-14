#include "CorePCH.hpp"
#include "GL_RenderingStage.hpp"

namespace Motion
{
    unsigned GL_RenderingStage::ToGL(DepthFunction f)
    {
        switch (f)
        {
            case DepthFunction::Never:          return GL_NEVER;
            case DepthFunction::Less:           return GL_LESS;
            case DepthFunction::Equal:          return GL_EQUAL;
            case DepthFunction::LessEqual:      return GL_LEQUAL;
            case DepthFunction::Greater:        return GL_GREATER;
            case DepthFunction::NotEqual:       return GL_NOTEQUAL;
            case DepthFunction::GreaterEqual:   return GL_GEQUAL;
            case DepthFunction::Always:         return GL_ALWAYS;
        }

        MOTION_ASSERT(false, "Unknown DepthFunction");
        return GL_LESS;
    }


    unsigned GL_RenderingStage::ToGL(BlendFactor f)
    {
        switch (f)
        {
            case BlendFactor::Zero:                         return GL_ZERO;
            case BlendFactor::One:                          return GL_ONE;
            case BlendFactor::SrcColor:                     return GL_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor:             return GL_ONE_MINUS_SRC_COLOR;
            case BlendFactor::DstColor:                     return GL_DST_COLOR;
            case BlendFactor::OneMinusDstColor:             return GL_ONE_MINUS_DST_COLOR;
            case BlendFactor::SrcAlpha:                     return GL_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha:             return GL_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::DstAlpha:                     return GL_DST_ALPHA;
            case BlendFactor::OneMinusDstAlpha:             return GL_ONE_MINUS_DST_ALPHA;
            case BlendFactor::ConstantColor:                return GL_CONSTANT_COLOR;
            case BlendFactor::OneMinusConstantColor:        return GL_ONE_MINUS_CONSTANT_COLOR;
            case BlendFactor::ConstantAlpha:                return GL_CONSTANT_ALPHA;
            case BlendFactor::OneMinusConstantAlpha:        return GL_ONE_MINUS_CONSTANT_ALPHA;
            case BlendFactor::SrcAlphaSaturate:             return GL_SRC_ALPHA_SATURATE;
        }

        MOTION_ASSERT(false, "Unknown BlendFactor");
        return GL_ONE;
    }


    unsigned GL_RenderingStage::ToGL(BlendEquation e)
    {
        switch (e)
        {
            case BlendEquation::Add:                return GL_FUNC_ADD;
            case BlendEquation::Subtract:           return GL_FUNC_SUBTRACT;
            case BlendEquation::ReverseSubtract:    return GL_FUNC_REVERSE_SUBTRACT;
            case BlendEquation::Min:                return GL_MIN;
            case BlendEquation::Max:                return GL_MAX;
        }

        MOTION_ASSERT(false, "Unknown BlendEquation");
        return GL_FUNC_ADD;
    }

    unsigned GL_RenderingStage::ToGL(CullMode m)
    {
        switch (m)
        {
            case CullMode::Back:            return GL_BACK;
            case CullMode::Front:           return GL_FRONT;
            case CullMode::FrontAndBack:    return GL_FRONT_AND_BACK;
            case CullMode::None:            break; 
        }

        MOTION_ASSERT(false, "Unknown CullMode");
        return GL_BACK;
    }


    unsigned GL_RenderingStage::ToGL(FrontFace ff)
    {
        switch (ff)
        {
            case FrontFace::CCW:    return GL_CCW;
            case FrontFace::CW:     return GL_CW;
        }

        MOTION_ASSERT(false, "Unknown FrontFace");
        return GL_CCW;
    }

    GL_RenderingStage::GL_RenderingStage()
    {
        m_Defaults = StageStatus{};
        m_Desired = m_Defaults;
        m_Current = m_Defaults; 
    }

    void GL_RenderingStage::Apply(const StageStatus& state)
    {
        if (!m_Initialized)
        {
            ApplyDelta(StageStatus{}, state);
            m_Current = state;
            m_Desired = state;
            m_Initialized = true;
            return;
        }

        ApplyDelta(m_Current, state);
        m_Current = state;
        m_Desired = state;
    }

    void GL_RenderingStage::ApplyDelta(const StageStatus& from, const StageStatus& to)
    {
        // Depth test
        if (from.DepthTest != to.DepthTest)
        {
            if (to.DepthTest) glEnable(GL_DEPTH_TEST); 
            else glDisable(GL_DEPTH_TEST);
        }
        if (from.DepthWrite != to.DepthWrite)
        {
            glDepthMask(to.DepthWrite ? GL_TRUE : GL_FALSE);
        }
        if (from.DepthFunc != to.DepthFunc)
        {
            glDepthFunc(ToGL(to.DepthFunc));
        }

        // Culling & raster
        if (from.CullEnabled != to.CullEnabled)
        {
            if (to.CullEnabled) glEnable(GL_CULL_FACE); 
            else glDisable(GL_CULL_FACE);
        }
        if (to.CullEnabled && (from.CullingMode != to.CullingMode))
        {
            glCullFace(ToGL(to.CullingMode));
        }
        if (from.FrontFace != to.FrontFace)
        {
            glFrontFace(ToGL(to.FrontFace));
        }
        if (from.Wireframe != to.Wireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, to.Wireframe ? GL_LINE : GL_FILL);
        }

        // Polygon offset (usually used for decals/shadows)
        if (from.PolygonOffsetEnabled != to.PolygonOffsetEnabled)
        {
            if (to.PolygonOffsetEnabled) glEnable(GL_POLYGON_OFFSET_FILL); 
            else glDisable(GL_POLYGON_OFFSET_FILL);
        }
        if (to.PolygonOffsetEnabled && (from.PolygonOffsetFactor != to.PolygonOffsetFactor || from.PolygonOffsetUnits != to.PolygonOffsetUnits))
        {
            glPolygonOffset(to.PolygonOffsetFactor, to.PolygonOffsetUnits);
        }


        // Blending
        if (from.BlendEnabled != to.BlendEnabled)
        {
            if (to.BlendEnabled) glEnable(GL_BLEND); 
            else glDisable(GL_BLEND);
        }
        if (to.BlendEnabled && (from.SrcRGB != to.SrcRGB || from.DstRGB != to.DstRGB || from.SrcA != to.SrcA || from.DstA != to.DstA))
        {
            glBlendFuncSeparate(ToGL(to.SrcRGB), ToGL(to.DstRGB), ToGL(to.SrcA), ToGL(to.DstA));
        }

        // --- Blend equations
        if (to.BlendEnabled && (from.BlendEqRGB != to.BlendEqRGB || from.BlendEqA != to.BlendEqA)) 
        {
            glBlendEquationSeparate(GL_RenderingStage::ToGL(to.BlendEqRGB), GL_RenderingStage::ToGL(to.BlendEqA));
        }

        // --- Blend constant color
        if (to.BlendEnabled && (
            !FloatEq(from.BlendConst[0], to.BlendConst[0]) ||
            !FloatEq(from.BlendConst[1], to.BlendConst[1]) ||
            !FloatEq(from.BlendConst[2], to.BlendConst[2]) ||
            !FloatEq(from.BlendConst[3], to.BlendConst[3])
        )) 
        {
            glBlendColor(to.BlendConst[0], to.BlendConst[1], to.BlendConst[2], to.BlendConst[3]);
        }

        // --- Color write mask
        if (from.ColorMaskR != to.ColorMaskR ||
            from.ColorMaskG != to.ColorMaskG ||
            from.ColorMaskB != to.ColorMaskB ||
            from.ColorMaskA != to.ColorMaskA) 
        {
            glColorMask(to.ColorMaskR, to.ColorMaskG, to.ColorMaskB, to.ColorMaskA);
        }

        // --- Scissor test
        if (from.ScissorEnabled != to.ScissorEnabled) 
        {
            if (to.ScissorEnabled) glEnable(GL_SCISSOR_TEST);
            else                   glDisable(GL_SCISSOR_TEST);
        }
        if (to.ScissorEnabled && (
            from.Scissor.X      != to.Scissor.X ||
            from.Scissor.Y      != to.Scissor.Y ||
            from.Scissor.Width  != to.Scissor.Width ||
            from.Scissor.Height != to.Scissor.Height
        )) 
        {
            glScissor(to.Scissor.X, to.Scissor.Y, to.Scissor.Width, to.Scissor.Height);
        }
    }

    void GL_RenderingStage::SetDepthTest(bool enabled) 
    { 
        m_Desired.DepthTest = enabled; 
        Apply(m_Desired); 
    }
    
    void GL_RenderingStage::SetDepthWrite(bool enabled) 
    { 
        m_Desired.DepthWrite = enabled; 
        Apply(m_Desired); 
    }

    void GL_RenderingStage::SetDepthFunc(DepthFunction func) 
    { 
        m_Desired.DepthFunc = func; 
        Apply(m_Desired); 
    }


    void GL_RenderingStage::SetCullEnabled(bool enabled) 
    { 
        m_Desired.CullEnabled = enabled; 
        Apply(m_Desired); 
    }
    void GL_RenderingStage::SetCullMode(CullMode mode) 
    { 
        m_Desired.CullingMode = mode; 
        Apply(m_Desired); 
    }
    void GL_RenderingStage::SetFrontFace(FrontFace ff) 
    { 
        m_Desired.FrontFace = ff; 
        Apply(m_Desired); 
    }
    void GL_RenderingStage::SetWireframe(bool enabled) 
    { 
        m_Desired.Wireframe = enabled; 
        Apply(m_Desired); 
    }

    void GL_RenderingStage::SetPolygonOffsetEnabled(bool enabled) 
    { 
        m_Desired.PolygonOffsetEnabled = enabled; 
        Apply(m_Desired); 
    }

    void GL_RenderingStage::SetPolygonOffset(float factor, float units)
    { 
        m_Desired.PolygonOffsetFactor = factor; 
        m_Desired.PolygonOffsetUnits = units; 
        Apply(m_Desired); 
    }


    void GL_RenderingStage::SetBlendEnabled(bool enabled) 
    { 
        m_Desired.BlendEnabled = enabled; 
        Apply(m_Desired); 
    }

    void GL_RenderingStage::SetBlendFuncSeparate(BlendFactor srcRGB, BlendFactor dstRGB, BlendFactor srcA, BlendFactor dstA)
    { 
        m_Desired.SrcRGB = srcRGB; 
        m_Desired.DstRGB = dstRGB; 
        m_Desired.SrcA = srcA; 
        m_Desired.DstA = dstA; 
        Apply(m_Desired); 
    }

    void GL_RenderingStage::SetBlendEquationSeparate(BlendEquation eqRGB, BlendEquation eqA)
    { 
        m_Desired.BlendEqRGB = eqRGB; 
        m_Desired.BlendEqA = eqA; 
        Apply(m_Desired); 
    }

    void GL_RenderingStage::SetBlendConstant(float r, float g, float b, float a)
    { 
        m_Desired.BlendConst[0] = r; 
        m_Desired.BlendConst[1] = g; 
        m_Desired.BlendConst[2] = b; 
        m_Desired.BlendConst[3] = a; 
        Apply(m_Desired); 
    }


    void GL_RenderingStage::SetColorMask(bool r, bool g, bool b, bool a)
    { 
        m_Desired.ColorMaskR = r; 
        m_Desired.ColorMaskG = g; 
        m_Desired.ColorMaskB = b; 
        m_Desired.ColorMaskA = a; 
        Apply(m_Desired); 
    }

    void GL_RenderingStage::SetScissorEnabled(bool enabled)
    { 
        m_Desired.ScissorEnabled = enabled; 
        Apply(m_Desired); 
    }

    void GL_RenderingStage::SetScissorRect(ScissorRect rect)
    { 
        m_Desired.Scissor = rect; 
        Apply(m_Desired); 
    }
}
