#include "CorePCH.hpp"

namespace Motion
{
    // ========================================
    // Factory Implementation
    // ========================================

    std::shared_ptr<IPostProcessEffect> IPostProcessEffect::Create(PostProcessEffectType type)
    {
        switch (type)
        {
            case PostProcessEffectType::ToneMapping:
                return std::make_shared<GL_ToneMappingEffect>();
            case PostProcessEffectType::Bloom:
                return std::make_shared<GL_BloomEffect>();
            case PostProcessEffectType::FXAA:
                return std::make_shared<GL_FXAAEffect>();
            case PostProcessEffectType::ColorGrading:
                return std::make_shared<GL_ColorGradingEffect>();
            case PostProcessEffectType::Vignette:
                return std::make_shared<GL_VignetteEffect>();
            case PostProcessEffectType::ChromaticAber:
                return std::make_shared<GL_ChromaticAberrationEffect>();
            default:
                return nullptr;
        }
    }

    // ========================================
    // Base GL_PostProcessEffect
    // ========================================

    GL_PostProcessEffect::GL_PostProcessEffect(PostProcessEffectType type)
        : m_Type(type), m_Enabled(true)
    {
    }

    // ========================================
    // ToneMapping Effect
    // ========================================

    GL_ToneMappingEffect::GL_ToneMappingEffect()
        : GL_PostProcessEffect(PostProcessEffectType::ToneMapping)
    {
    }

    void GL_ToneMappingEffect::Init()
    {
        const char* vertexSource = R"(
            #version 450 core
            layout (location = 0) in vec2 aPosition;
            layout (location = 1) in vec2 aTexCoord;
            
            out vec2 TexCoords;
            
            void main()
            {
                TexCoords = aTexCoord;
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        )";

        const char* fragmentSource = R"(
            #version 450 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D uSceneTexture;
            uniform int uToneMappingOperator;
            uniform float uExposure;
            uniform float uGamma;
            uniform float uWhitePoint;
            
            // Reinhard tone mapping
            vec3 Reinhard(vec3 color)
            {
                return color / (color + vec3(1.0));
            }
            
            // Reinhard luminance-based tone mapping
            vec3 ReinhardLuminance(vec3 color)
            {
                float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
                float toneMappedLuma = luma / (1.0 + luma);
                return color * (toneMappedLuma / luma);
            }
            
            // Uncharted 2 tone mapping (John Hable)
            vec3 Uncharted2Tonemap(vec3 x)
            {
                float A = 0.15;
                float B = 0.50;
                float C = 0.10;
                float D = 0.20;
                float E = 0.02;
                float F = 0.30;
                return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
            }
            
            vec3 Uncharted2(vec3 color)
            {
                color = Uncharted2Tonemap(color * 2.0);
                vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(uWhitePoint));
                return color * whiteScale;
            }
            
            // ACES filmic tone mapping
            vec3 ACESFilm(vec3 x)
            {
                float a = 2.51;
                float b = 0.03;
                float c = 2.43;
                float d = 0.59;
                float e = 0.14;
                return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
            }
            
            // Simple exposure tone mapping
            vec3 ExposureToneMapping(vec3 color)
            {
                return vec3(1.0) - exp(-color * uExposure);
            }
            
            void main()
            {
                vec3 hdrColor = texture(uSceneTexture, TexCoords).rgb;
                
                // Apply exposure
                hdrColor *= uExposure;
                
                vec3 toneMapped;
                
                // Apply tone mapping operator
                if (uToneMappingOperator == 0)
                    toneMapped = Reinhard(hdrColor);
                else if (uToneMappingOperator == 1)
                    toneMapped = ReinhardLuminance(hdrColor);
                else if (uToneMappingOperator == 2)
                    toneMapped = Uncharted2(hdrColor);
                else if (uToneMappingOperator == 3)
                    toneMapped = ACESFilm(hdrColor);
                else
                    toneMapped = ExposureToneMapping(hdrColor);
                
                // Gamma correction
                toneMapped = pow(toneMapped, vec3(1.0 / uGamma));
                
                FragColor = vec4(toneMapped, 1.0);
            }
        )";

        std::unordered_map<ShaderType, std::string> sources = {
            {ShaderType::Vertex, vertexSource},
            {ShaderType::Fragment, fragmentSource}
        };

        m_Shader = IShader::CreateShader(sources);
    }

    void GL_ToneMappingEffect::Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer)
    {
        outputFrameBuffer->Bind();
        
        m_Shader->Bind();
        m_Shader->SetUniform("uSceneTexture", 0);
        m_Shader->SetUniform("uToneMappingOperator", static_cast<int>(m_Config.toneMappingOp));
        m_Shader->SetUniform("uExposure", m_Config.exposure);
        m_Shader->SetUniform("uGamma", m_Config.gamma);
        m_Shader->SetUniform("uWhitePoint", m_Config.whitePoint);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        ScreenQuad::GetInstance().Render();

        m_Shader->Unbind();
        outputFrameBuffer->Unbind();
    }

    void GL_ToneMappingEffect::Resize(std::int32_t width, std::int32_t height)
    {
        // Nothing to resize for tone mapping
    }

    // ========================================
    // Bloom Effect
    // ========================================

    GL_BloomEffect::GL_BloomEffect()
        : GL_PostProcessEffect(PostProcessEffectType::Bloom)
    {
    }

    void GL_BloomEffect::Init()
    {
        // Brightness extraction shader
        const char* brightnessVert = R"(
            #version 450 core
            layout (location = 0) in vec2 aPosition;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoords;
            void main()
            {
                TexCoords = aTexCoord;
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        )";

        const char* brightnessFrag = R"(
            #version 450 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D uSceneTexture;
            uniform float uThreshold;
            
            void main()
            {
                vec3 color = texture(uSceneTexture, TexCoords).rgb;
                float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
                
                if(brightness > uThreshold)
                    FragColor = vec4(color, 1.0);
                else
                    FragColor = vec4(0.0, 0.0, 0.0, 1.0);
            }
        )";

        std::unordered_map<ShaderType, std::string> brightnessSources = {
            {ShaderType::Vertex, brightnessVert},
            {ShaderType::Fragment, brightnessFrag}
        };
        m_BrightnessShader = IShader::CreateShader(brightnessSources);

        // Gaussian blur shader
        const char* blurFrag = R"(
            #version 450 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D uTexture;
            uniform bool uHorizontal;
            uniform float uRadius;
            
            uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
            
            void main()
            {
                vec2 tex_offset = 1.0 / textureSize(uTexture, 0) * uRadius;
                vec3 result = texture(uTexture, TexCoords).rgb * weight[0];
                
                if(uHorizontal)
                {
                    for(int i = 1; i < 5; ++i)
                    {
                        result += texture(uTexture, TexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                        result += texture(uTexture, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
                    }
                }
                else
                {
                    for(int i = 1; i < 5; ++i)
                    {
                        result += texture(uTexture, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                        result += texture(uTexture, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
                    }
                }
                FragColor = vec4(result, 1.0);
            }
        )";

        std::unordered_map<ShaderType, std::string> blurSources = {
            {ShaderType::Vertex, brightnessVert},
            {ShaderType::Fragment, blurFrag}
        };
        m_BlurShader = IShader::CreateShader(blurSources);

        // Composite shader
        const char* compositeFrag = R"(
            #version 450 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D uSceneTexture;
            uniform sampler2D uBloomTexture;
            uniform float uIntensity;
            
            void main()
            {
                vec3 sceneColor = texture(uSceneTexture, TexCoords).rgb;
                vec3 bloomColor = texture(uBloomTexture, TexCoords).rgb;
                vec3 result = sceneColor + bloomColor * uIntensity;
                FragColor = vec4(result, 1.0);
            }
        )";

        std::unordered_map<ShaderType, std::string> compositeSources = {
            {ShaderType::Vertex, brightnessVert},
            {ShaderType::Fragment, compositeFrag}
        };
        m_CompositeShader = IShader::CreateShader(compositeSources);

        // Create framebuffers (will be properly sized on first use)
        Resize(1280, 720);
    }

    void GL_BloomEffect::Resize(std::int32_t width, std::int32_t height)
    {
        m_Width = width;
        m_Height = height;

        // Create brightness extraction buffer
        FrameBufferSpecification brightSpec;
        brightSpec.Name = "Bloom_Brightness";
        brightSpec.Width = width;
        brightSpec.Height = height;
        brightSpec.Samples = 1;
        brightSpec.Colors = {{ 0, 0, FrameBufferColorAttachmentStandards::HighDynamicRange }};
        brightSpec.Depth = { 0, 0, FrameBufferDepthAttachmentStandards::None };
        m_BrightnessBuffer = IFrameBuffer::Create(brightSpec);

        // Create ping-pong blur buffers
        m_BlurBuffers.clear();
        for (int i = 0; i < 2; ++i)
        {
            FrameBufferSpecification spec;
            spec.Name = "Bloom_Blur" + std::to_string(i);
            spec.Width = width;
            spec.Height = height;
            spec.Samples = 1;
            spec.Colors = {{ 0, 0, FrameBufferColorAttachmentStandards::HighDynamicRange }};
            spec.Depth = { 0, 0, FrameBufferDepthAttachmentStandards::None };
            m_BlurBuffers.push_back(IFrameBuffer::Create(spec));
        }
    }

    void GL_BloomEffect::ExtractBrightness(FrameTextureID inputTexture)
    {
        m_BrightnessBuffer->Bind();
        
        m_BrightnessShader->Bind();
        m_BrightnessShader->SetUniform("uSceneTexture", 0);
        m_BrightnessShader->SetUniform("uThreshold", m_Config.threshold);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        ScreenQuad::GetInstance().Render();

        m_BrightnessShader->Unbind();
        m_BrightnessBuffer->Unbind();
    }

    void GL_BloomEffect::BlurBrightness()
    {
        m_BlurShader->Bind();
        m_BlurShader->SetUniform("uTexture", 0);
        m_BlurShader->SetUniform("uRadius", m_Config.radius);

        FrameTextureID currentTexture = m_BrightnessBuffer->GetAttachment(
            FrameBufferColorAttachmentStandards::HighDynamicRange
        ).ID;

        // Perform blur iterations
        for (int i = 0; i < m_Config.iterations; ++i)
        {
            // Horizontal pass
            m_BlurBuffers[0]->Bind();
            m_BlurShader->SetUniform("uHorizontal", true);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, currentTexture);
            ScreenQuad::GetInstance().Render();
            m_BlurBuffers[0]->Unbind();

            // Vertical pass
            m_BlurBuffers[1]->Bind();
            m_BlurShader->SetUniform("uHorizontal", false);
            currentTexture = m_BlurBuffers[0]->GetAttachment(
                FrameBufferColorAttachmentStandards::HighDynamicRange
            ).ID;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, currentTexture);
            ScreenQuad::GetInstance().Render();
            m_BlurBuffers[1]->Unbind();

            currentTexture = m_BlurBuffers[1]->GetAttachment(
                FrameBufferColorAttachmentStandards::HighDynamicRange
            ).ID;
        }

        m_BlurShader->Unbind();
    }

    void GL_BloomEffect::Composite(FrameTextureID sceneTexture, IFrameBuffer* outputFrameBuffer)
    {
        outputFrameBuffer->Bind();

        m_CompositeShader->Bind();
        m_CompositeShader->SetUniform("uSceneTexture", 0);
        m_CompositeShader->SetUniform("uBloomTexture", 1);
        m_CompositeShader->SetUniform("uIntensity", m_Config.intensity);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneTexture);

        glActiveTexture(GL_TEXTURE1);
        FrameTextureID blurredTexture = m_BlurBuffers[1]->GetAttachment(
            FrameBufferColorAttachmentStandards::HighDynamicRange
        ).ID;
        glBindTexture(GL_TEXTURE_2D, blurredTexture);

        ScreenQuad::GetInstance().Render();

        m_CompositeShader->Unbind();
        outputFrameBuffer->Unbind();
    }

    void GL_BloomEffect::Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer)
    {
        if (!m_Config.enabled)
            return;

        ExtractBrightness(inputTexture);
        BlurBrightness();
        Composite(inputTexture, outputFrameBuffer);
    }

    // ========================================
    // FXAA Effect
    // ========================================

    GL_FXAAEffect::GL_FXAAEffect()
        : GL_PostProcessEffect(PostProcessEffectType::FXAA)
    {
    }

    void GL_FXAAEffect::Init()
    {
        const char* vertexSource = R"(
            #version 450 core
            layout (location = 0) in vec2 aPosition;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoords;
            void main()
            {
                TexCoords = aTexCoord;
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        )";

        const char* fragmentSource = R"(
            #version 450 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D uSceneTexture;
            uniform vec2 uInvScreenSize;
            uniform float uEdgeThreshold;
            uniform float uEdgeThresholdMin;
            uniform float uSubpixelQuality;
            
            float rgb2luma(vec3 rgb) {
                return dot(rgb, vec3(0.299, 0.587, 0.114));
            }
            
            void main()
            {
                vec3 colorCenter = texture(uSceneTexture, TexCoords).rgb;
                
                // Luma at current pixel
                float lumaCenter = rgb2luma(colorCenter);
                
                // Luma at four direct neighbors
                float lumaDown = rgb2luma(textureOffset(uSceneTexture, TexCoords, ivec2(0, -1)).rgb);
                float lumaUp = rgb2luma(textureOffset(uSceneTexture, TexCoords, ivec2(0, 1)).rgb);
                float lumaLeft = rgb2luma(textureOffset(uSceneTexture, TexCoords, ivec2(-1, 0)).rgb);
                float lumaRight = rgb2luma(textureOffset(uSceneTexture, TexCoords, ivec2(1, 0)).rgb);
                
                // Find minimum and maximum luma
                float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
                float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));
                
                // Compute the delta
                float lumaRange = lumaMax - lumaMin;
                
                // If variation is lower than threshold, we are not on an edge, skip FXAA
                if(lumaRange < max(uEdgeThresholdMin, lumaMax * uEdgeThreshold)) {
                    FragColor = vec4(colorCenter, 1.0);
                    return;
                }
                
                // Query the 4 remaining corners lumas
                float lumaDownLeft = rgb2luma(textureOffset(uSceneTexture, TexCoords, ivec2(-1, -1)).rgb);
                float lumaUpRight = rgb2luma(textureOffset(uSceneTexture, TexCoords, ivec2(1, 1)).rgb);
                float lumaUpLeft = rgb2luma(textureOffset(uSceneTexture, TexCoords, ivec2(-1, 1)).rgb);
                float lumaDownRight = rgb2luma(textureOffset(uSceneTexture, TexCoords, ivec2(1, -1)).rgb);
                
                // Combine the four edges lumas
                float lumaDownUp = lumaDown + lumaUp;
                float lumaLeftRight = lumaLeft + lumaRight;
                
                // Same for corners
                float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
                float lumaDownCorners = lumaDownLeft + lumaDownRight;
                float lumaRightCorners = lumaDownRight + lumaUpRight;
                float lumaUpCorners = lumaUpRight + lumaUpLeft;
                
                // Compute horizontal and vertical gradient
                float edgeHorizontal = abs(-2.0 * lumaLeft + lumaLeftCorners) + abs(-2.0 * lumaCenter + lumaDownUp) * 2.0 + abs(-2.0 * lumaRight + lumaRightCorners);
                float edgeVertical = abs(-2.0 * lumaUp + lumaUpCorners) + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0 + abs(-2.0 * lumaDown + lumaDownCorners);
                
                // Is the local edge horizontal or vertical?
                bool isHorizontal = (edgeHorizontal >= edgeVertical);
                
                // Select the two neighboring texels lumas in the opposite direction to the local edge
                float luma1 = isHorizontal ? lumaDown : lumaLeft;
                float luma2 = isHorizontal ? lumaUp : lumaRight;
                
                // Compute gradients in this direction
                float gradient1 = luma1 - lumaCenter;
                float gradient2 = luma2 - lumaCenter;
                
                // Which direction is the steepest?
                bool is1Steepest = abs(gradient1) >= abs(gradient2);
                
                // Gradient in the corresponding direction, normalized
                float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));
                
                // Choose the step size (one pixel) according to the edge direction
                float stepLength = isHorizontal ? uInvScreenSize.y : uInvScreenSize.x;
                
                // Average luma in the correct direction
                float lumaLocalAverage = 0.0;
                
                if(is1Steepest) {
                    stepLength = -stepLength;
                    lumaLocalAverage = 0.5 * (luma1 + lumaCenter);
                } else {
                    lumaLocalAverage = 0.5 * (luma2 + lumaCenter);
                }
                
                // Shift UV in the correct direction by half a pixel
                vec2 currentUv = TexCoords;
                if(isHorizontal) {
                    currentUv.y += stepLength * 0.5;
                } else {
                    currentUv.x += stepLength * 0.5;
                }
                
                // Compute offset and UVs
                vec2 offset = isHorizontal ? vec2(uInvScreenSize.x, 0.0) : vec2(0.0, uInvScreenSize.y);
                vec2 uv1 = currentUv - offset;
                vec2 uv2 = currentUv + offset;
                
                // Read the lumas at both ends of the exploration segment
                float lumaEnd1 = rgb2luma(texture(uSceneTexture, uv1).rgb);
                float lumaEnd2 = rgb2luma(texture(uSceneTexture, uv2).rgb);
                lumaEnd1 -= lumaLocalAverage;
                lumaEnd2 -= lumaLocalAverage;
                
                // If the luma deltas at the current extremities are larger than the local gradient, we have reached the side of the edge
                bool reached1 = abs(lumaEnd1) >= gradientScaled;
                bool reached2 = abs(lumaEnd2) >= gradientScaled;
                bool reachedBoth = reached1 && reached2;
                
                // If the side is not reached, we continue to explore in this direction
                if(!reached1) {
                    uv1 -= offset;
                }
                if(!reached2) {
                    uv2 += offset;
                }
                
                // If both sides have been reached, stop exploration
                if(!reachedBoth) {
                    for(int i = 2; i < 12; i++) {
                        if(!reached1) {
                            lumaEnd1 = rgb2luma(texture(uSceneTexture, uv1).rgb);
                            lumaEnd1 = lumaEnd1 - lumaLocalAverage;
                        }
                        if(!reached2) {
                            lumaEnd2 = rgb2luma(texture(uSceneTexture, uv2).rgb);
                            lumaEnd2 = lumaEnd2 - lumaLocalAverage;
                        }
                        reached1 = abs(lumaEnd1) >= gradientScaled;
                        reached2 = abs(lumaEnd2) >= gradientScaled;
                        reachedBoth = reached1 && reached2;
                        
                        if(!reached1) {
                            uv1 -= offset;
                        }
                        if(!reached2) {
                            uv2 += offset;
                        }
                        
                        if(reachedBoth) break;
                    }
                }
                
                // Compute the distances to each end of the edge
                float distance1 = isHorizontal ? (TexCoords.x - uv1.x) : (TexCoords.y - uv1.y);
                float distance2 = isHorizontal ? (uv2.x - TexCoords.x) : (uv2.y - TexCoords.y);
                
                // In which direction is the end of the edge closer?
                bool isDirection1 = distance1 < distance2;
                float distanceFinal = min(distance1, distance2);
                
                // Length of the edge
                float edgeThickness = (distance1 + distance2);
                
                // UV offset: read in the direction of the closest end of the edge
                float pixelOffset = -distanceFinal / edgeThickness + 0.5;
                
                // Is the luma at center smaller than the local average?
                bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;
                
                // If the luma at center is smaller than at its neighbor, the delta luma at each end should be positive (same variation)
                bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;
                
                // If the luma variation is incorrect, do not offset
                float finalOffset = correctVariation ? pixelOffset : 0.0;
                
                // Sub-pixel shifting
                float lumaAverage = (1.0/12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);
                float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter) / lumaRange, 0.0, 1.0);
                float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
                float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * uSubpixelQuality;
                
                // Pick the biggest of the two offsets
                finalOffset = max(finalOffset, subPixelOffsetFinal);
                
                // Compute the final UV coordinates
                vec2 finalUv = TexCoords;
                if(isHorizontal) {
                    finalUv.y += finalOffset * stepLength;
                } else {
                    finalUv.x += finalOffset * stepLength;
                }
                
                // Read the color at the new UV coordinates, and use it
                vec3 finalColor = texture(uSceneTexture, finalUv).rgb;
                FragColor = vec4(finalColor, 1.0);
            }
        )";

        std::unordered_map<ShaderType, std::string> sources = {
            {ShaderType::Vertex, vertexSource},
            {ShaderType::Fragment, fragmentSource}
        };

        m_Shader = IShader::CreateShader(sources);
    }

    void GL_FXAAEffect::Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer)
    {
        outputFrameBuffer->Bind();

        m_Shader->Bind();
        m_Shader->SetUniform("uSceneTexture", 0);
        m_Shader->SetUniform("uInvScreenSize", m_InvScreenSize);
        m_Shader->SetUniform("uEdgeThreshold", m_Config.edgeThreshold);
        m_Shader->SetUniform("uEdgeThresholdMin", m_Config.edgeThresholdMin);
        m_Shader->SetUniform("uSubpixelQuality", m_Config.subpixelQuality);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        ScreenQuad::GetInstance().Render();

        m_Shader->Unbind();
        outputFrameBuffer->Unbind();
    }

    void GL_FXAAEffect::Resize(std::int32_t width, std::int32_t height)
    {
        m_InvScreenSize = glm::vec2(1.0f / width, 1.0f / height);
    }

    // ========================================
    // ColorGrading Effect
    // ========================================

    GL_ColorGradingEffect::GL_ColorGradingEffect()
        : GL_PostProcessEffect(PostProcessEffectType::ColorGrading)
    {
    }

    void GL_ColorGradingEffect::Init()
    {
        const char* vertexSource = R"(
            #version 450 core
            layout (location = 0) in vec2 aPosition;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoords;
            void main()
            {
                TexCoords = aTexCoord;
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        )";

        const char* fragmentSource = R"(
            #version 450 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D uSceneTexture;
            uniform vec3 uShadows;
            uniform vec3 uMidtones;
            uniform vec3 uHighlights;
            uniform float uSaturation;
            uniform float uContrast;
            uniform float uBrightness;
            
            vec3 applyColorGrading(vec3 color)
            {
                // Luminance
                float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
                
                // Brightness
                color += uBrightness;
                
                // Contrast
                color = (color - 0.5) * uContrast + 0.5;
                
                // Saturation
                color = mix(vec3(luma), color, uSaturation);
                
                // Color wheels
                float shadowWeight = 1.0 - smoothstep(0.0, 0.5, luma);
                float highlightWeight = smoothstep(0.5, 1.0, luma);
                float midtoneWeight = 1.0 - shadowWeight - highlightWeight;
                
                color *= uShadows * shadowWeight + uMidtones * midtoneWeight + uHighlights * highlightWeight;
                
                return color;
            }
            
            void main()
            {
                vec3 color = texture(uSceneTexture, TexCoords).rgb;
                color = applyColorGrading(color);
                FragColor = vec4(color, 1.0);
            }
        )";

        std::unordered_map<ShaderType, std::string> sources = {
            {ShaderType::Vertex, vertexSource},
            {ShaderType::Fragment, fragmentSource}
        };

        m_Shader = IShader::CreateShader(sources);
    }

    void GL_ColorGradingEffect::Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer)
    {
        outputFrameBuffer->Bind();

        m_Shader->Bind();
        m_Shader->SetUniform("uSceneTexture", 0);
        m_Shader->SetUniform("uShadows", m_Config.shadows);
        m_Shader->SetUniform("uMidtones", m_Config.midtones);
        m_Shader->SetUniform("uHighlights", m_Config.highlights);
        m_Shader->SetUniform("uSaturation", m_Config.saturation);
        m_Shader->SetUniform("uContrast", m_Config.contrast);
        m_Shader->SetUniform("uBrightness", m_Config.brightness);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        ScreenQuad::GetInstance().Render();

        m_Shader->Unbind();
        outputFrameBuffer->Unbind();
    }

    void GL_ColorGradingEffect::Resize(std::int32_t width, std::int32_t height)
    {
        // Nothing to resize
    }

    // ========================================
    // Vignette Effect
    // ========================================

    GL_VignetteEffect::GL_VignetteEffect()
        : GL_PostProcessEffect(PostProcessEffectType::Vignette)
    {
    }

    void GL_VignetteEffect::Init()
    {
        const char* vertexSource = R"(
            #version 450 core
            layout (location = 0) in vec2 aPosition;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoords;
            void main()
            {
                TexCoords = aTexCoord;
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        )";

        const char* fragmentSource = R"(
            #version 450 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D uSceneTexture;
            uniform float uIntensity;
            uniform float uSmoothness;
            uniform vec3 uColor;
            
            void main()
            {
                vec3 color = texture(uSceneTexture, TexCoords).rgb;
                
                vec2 uv = TexCoords * 2.0 - 1.0;
                float dist = length(uv);
                float vignette = smoothstep(1.0 - uSmoothness, 1.0, dist);
                
                color = mix(color, uColor, vignette * uIntensity);
                
                FragColor = vec4(color, 1.0);
            }
        )";

        std::unordered_map<ShaderType, std::string> sources = {
            {ShaderType::Vertex, vertexSource},
            {ShaderType::Fragment, fragmentSource}
        };

        m_Shader = IShader::CreateShader(sources);
    }

    void GL_VignetteEffect::Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer)
    {
        outputFrameBuffer->Bind();

        m_Shader->Bind();
        m_Shader->SetUniform("uSceneTexture", 0);
        m_Shader->SetUniform("uIntensity", m_Config.intensity);
        m_Shader->SetUniform("uSmoothness", m_Config.smoothness);
        m_Shader->SetUniform("uColor", m_Config.color);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        ScreenQuad::GetInstance().Render();

        m_Shader->Unbind();
        outputFrameBuffer->Unbind();
    }

    void GL_VignetteEffect::Resize(std::int32_t width, std::int32_t height)
    {
        // Nothing to resize
    }

    // ========================================
    // ChromaticAberration Effect
    // ========================================

    GL_ChromaticAberrationEffect::GL_ChromaticAberrationEffect()
        : GL_PostProcessEffect(PostProcessEffectType::ChromaticAber)
    {
    }

    void GL_ChromaticAberrationEffect::Init()
    {
        const char* vertexSource = R"(
            #version 450 core
            layout (location = 0) in vec2 aPosition;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 TexCoords;
            void main()
            {
                TexCoords = aTexCoord;
                gl_Position = vec4(aPosition, 0.0, 1.0);
            }
        )";

        const char* fragmentSource = R"(
            #version 450 core
            out vec4 FragColor;
            in vec2 TexCoords;
            
            uniform sampler2D uSceneTexture;
            uniform float uIntensity;
            uniform vec2 uDirection;
            
            void main()
            {
                vec2 offset = uDirection * uIntensity;
                
                float r = texture(uSceneTexture, TexCoords + offset).r;
                float g = texture(uSceneTexture, TexCoords).g;
                float b = texture(uSceneTexture, TexCoords - offset).b;
                
                FragColor = vec4(r, g, b, 1.0);
            }
        )";

        std::unordered_map<ShaderType, std::string> sources = {
            {ShaderType::Vertex, vertexSource},
            {ShaderType::Fragment, fragmentSource}
        };

        m_Shader = IShader::CreateShader(sources);
    }

    void GL_ChromaticAberrationEffect::Process(FrameTextureID inputTexture, IFrameBuffer* outputFrameBuffer)
    {
        outputFrameBuffer->Bind();

        m_Shader->Bind();
        m_Shader->SetUniform("uSceneTexture", 0);
        m_Shader->SetUniform("uIntensity", m_Config.intensity);
        m_Shader->SetUniform("uDirection", m_Config.direction);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        ScreenQuad::GetInstance().Render();

        m_Shader->Unbind();
        outputFrameBuffer->Unbind();
    }

    void GL_ChromaticAberrationEffect::Resize(std::int32_t width, std::int32_t height)
    {
        // Nothing to resize
    }
}