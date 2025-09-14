#pragma once

#include "RenderingStage.hpp"

namespace Motion
{
    class GL_RenderingStage : public IRenderingStage
    {
        public:
            GL_RenderingStage();
            ~GL_RenderingStage() = default;

            void Apply(const StageStatus& state) override;
            StageStatus Snapshot() const override { return m_Current; }
            void Restore(const StageStatus& state) override { Apply(state); }
            const StageStatus& Defaults() const override { return m_Defaults; }

            void SetDepthTest(bool enabled) override;
            void SetDepthWrite(bool enabled) override;
            void SetDepthFunc(DepthFunction func) override;

            void SetCullEnabled(bool enabled) override;
            void SetCullMode(CullMode mode) override;
            void SetFrontFace(FrontFace ff) override;
            void SetWireframe(bool enabled) override;

            void SetPolygonOffsetEnabled(bool enabled) override;
            void SetPolygonOffset(float factor, float units) override;

            void SetBlendEnabled(bool enabled) override;
            void SetBlendFuncSeparate(BlendFactor srcRGB, BlendFactor dstRGB, BlendFactor srcA, BlendFactor dstA) override;
            void SetBlendEquationSeparate(BlendEquation eqRGB, BlendEquation eqA) override;
            void SetBlendConstant(float r, float g, float b, float a) override;

            void SetColorMask(bool r, bool g, bool b, bool a) override;
            void SetScissorEnabled(bool enabled) override;
            void SetScissorRect(ScissorRect rect) override;


        private:
            void ApplyDelta(const StageStatus& from, const StageStatus& to);
            static unsigned ToGL(DepthFunction f);
            static unsigned ToGL(BlendFactor f);
            static unsigned ToGL(BlendEquation e);
            static unsigned ToGL(CullMode m);
            static unsigned ToGL(FrontFace ff);

            static bool FloatEq(float a, float b, float eps = 1e-6f) { return (a > b ? a - b : b - a) <= eps; }

            private:
            StageStatus m_Defaults{}; 
            StageStatus m_Current{}; 
            StageStatus m_Desired{}; 
            bool m_Initialized{false};
    };
}