#pragma once

#include <string>
#include <functional>
#include <memory>
#include <filesystem>

#include <imgui/imgui.h>

#include "Scope.hpp"
#include "CardView.hpp"
#include "Responsive.hpp"
#include "Texture.hpp"

namespace Motion
{
    enum class TextureSlotAction
    {
        None,        
        Upload,       
        Reload,         
        Clear,         
        FlipVertical  
    };
    
    enum class TextureSlotStyle
    {
        Card,         
        Compact,      
        Detailed,       
        Gallery        
    };
    
    struct TextureSlotConfig
    {
        TextureSlotStyle Style{TextureSlotStyle::Card};
        
        int PreviewSize{150};          
        int TooltipPreviewSize{512};    
        
        bool ShowLabel{true};          
        bool ShowFilename{true};       
        bool ShowDimensions{true};      
        bool ShowHoverEffect{true};     
        
        bool AllowClear{true};        
        bool AllowReload{true};      
        bool AllowFlip{true};         
        bool EnableDragDrop{true};     
        bool EnableFileDialog{true};    
        
        CardStyle CardDesign{CardStyle::Neumorphic};
        bool CardHoverable{true};
        float CardRounding{12.0f};
        float CardPadding{12.0f};
        
        ImVec4 BackgroundColor{0.0f, 0.0f, 0.0f, 0.0f};
        ImVec4 BorderColor{0.0f, 0.0f, 0.0f, 0.0f};
        ImVec4 EmptySlotColor{0.0f, 0.0f, 0.0f, 0.0f};
    };
    
    using TextureSlotCallback = std::function<void(TextureSlotAction, std::shared_ptr<ITexture>&)>;
    
    class TextureSlotUtils
    {
    public:
        static ImTextureID AsImTextureID(const std::shared_ptr<ITexture>& texture)
        {
            if (!texture) 
                return (ImTextureID)(uintptr_t)0;
            return (ImTextureID)(uintptr_t)texture->GetID();
        }
        
        static std::string GetFilename(const std::shared_ptr<ITexture>& texture)
        {
            if (!texture) 
                return "(none)";
            
            const auto& spec = texture->GetSpecification();
            if (!spec.TextureFile.empty())
                return std::filesystem::path(spec.TextureFile).filename().string();
            
            if (spec.Width && spec.Height)
                return std::format("{}x{}", spec.Width, spec.Height);
            
            return "(runtime)";
        }

        static void DrawCheckerPattern(ImDrawList* drawList, const ImRect& rect, float cellSize = 10.0f)
        {
            const ImU32 lightColor = IM_COL32(110, 110, 110, 255);
            const ImU32 darkColor = IM_COL32(70, 70, 70, 255);

            for (float y = rect.Min.y; y < rect.Max.y; y += cellSize)
            {
                for (float x = rect.Min.x; x < rect.Max.x; x += cellSize)
                {
                    bool alternate = (static_cast<int>((x - rect.Min.x) / cellSize) + 
                                     static_cast<int>((y - rect.Min.y) / cellSize)) & 1;
                    
                    ImRect cellRect(
                        ImVec2(x, y),
                        ImVec2(std::min(x + cellSize, rect.Max.x), 
                               std::min(y + cellSize, rect.Max.y))
                    );
                    
                    drawList->AddRectFilled(cellRect.Min, cellRect.Max, 
                                           alternate ? lightColor : darkColor);
                }
            }
        }
        
        static bool LoadTextureFromDialog(std::shared_ptr<ITexture>& texture, TextureType type);
    };
    
    class TextureSlotRenderer
    {
    public:
        static void DrawCard(const char* label, std::shared_ptr<ITexture>& texture, 
                           TextureType type, const TextureSlotConfig& config,
                           TextureSlotAction& outAction, const TextureSlotCallback& callback)
        {
            ScopeID id(label);
            
            // Create card
            CardConfig cardConfig;
            cardConfig.Style = config.CardDesign;
            cardConfig.Hoverable = config.CardHoverable;
            cardConfig.Rounding = config.CardRounding;
            cardConfig.PaddingX = config.CardPadding;
            cardConfig.PaddingY = config.CardPadding;
            cardConfig.FixedSize = ImVec2(
                config.PreviewSize + config.CardPadding * 2.0f,
                config.PreviewSize + config.CardPadding * 2.0f + (config.ShowLabel ? 30.0f : 0.0f)
            );
            
            Card card(label, cardConfig);
            if(card.Begin())
            {
                ImVec2 cursorPos = ImGui::GetCursorPos();
                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                ImVec2 previewSize(config.PreviewSize, config.PreviewSize);
                
                DrawPreview(texture, screenPos, previewSize, config);
                ImGui::SetCursorScreenPos(screenPos);
                ImGui::InvisibleButton("##preview", previewSize);
                HandleInteractions(texture, type, config, outAction, callback);
                ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + config.PreviewSize));
                
                if(config.ShowLabel || config.ShowFilename)
                {
                    ImGui::Spacing();
                    
                    if(config.ShowLabel)
                    {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                                         GetTextureTypeString(type).c_str());
                    }
                    
                    if(config.ShowFilename && texture)
                    {
                        std::string filename = TextureSlotUtils::GetFilename(texture);
                        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + config.PreviewSize);
                        ImGui::TextDisabled("%s", filename.c_str());
                        ImGui::PopTextWrapPos();
                    }
                }
                
                card.End();
            }
        }
        
        static void DrawCompact(const char* label, std::shared_ptr<ITexture>& texture, 
                              TextureType type, const TextureSlotConfig& config,
                              TextureSlotAction& outAction, const TextureSlotCallback& callback)
        {
            ScopeID id(label);
            
            ImVec2 screenPos = ImGui::GetCursorScreenPos();
            ImVec2 previewSize(config.PreviewSize, config.PreviewSize);
            
            DrawPreview(texture, screenPos, previewSize, config);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImRect previewRect(screenPos, ImVec2(screenPos.x + previewSize.x, screenPos.y + previewSize.y));
            drawList->AddRect(previewRect.Min, previewRect.Max, 
                            ImGui::GetColorU32(ImGuiCol_Border), 4.0f, 0, 1.5f);
            
            ImGui::InvisibleButton("##preview", previewSize);
            HandleInteractions(texture, type, config, outAction, callback);
            if(config.ShowLabel)
            {
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
                    ImGui::Text("%s", GetTextureTypeString(type).c_str());
                    
                    if(texture && config.ShowDimensions)
                    {
                        const auto& spec = texture->GetSpecification();
                        ImGui::TextDisabled("%ux%u", spec.Width, spec.Height);
                    }
                }
                ImGui::EndGroup();
            }
        }
        
        static void DrawDetailed(const char* label, std::shared_ptr<ITexture>& texture, 
                               TextureType type, const TextureSlotConfig& config,
                               TextureSlotAction& outAction, const TextureSlotCallback& callback)
        {
            ScopeID id(label);
            
            ImGui::BeginGroup();
            {
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), 
                                 "%s", GetTextureTypeString(type).c_str());
                ImGui::Separator();
                ImGui::Spacing();
                
                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                ImVec2 previewSize(config.PreviewSize, config.PreviewSize);
                DrawPreview(texture, screenPos, previewSize, config);
                
                ImGui::InvisibleButton("##preview", previewSize);
                HandleInteractions(texture, type, config, outAction, callback);
                ImGui::Spacing();
                
                if(texture)
                {
                    const auto& spec = texture->GetSpecification();
                    
                    ImGui::BeginGroup();
                    {
                        ImGui::TextDisabled("File:");
                        ImGui::TextDisabled("Size:");
                        ImGui::TextDisabled("Format:");
                    }
                    ImGui::EndGroup();
                    
                    ImGui::SameLine(100);
                    
                    ImGui::BeginGroup();
                    {
                        std::string filename = TextureSlotUtils::GetFilename(texture);
                        ImGui::Text("%s", filename.c_str());
                        ImGui::Text("%ux%u", spec.Width, spec.Height);
                        ImGui::Text("%s", spec.Channels == 4 ? "RGBA" : "Other");
                    }
                    ImGui::EndGroup();
                }
                else
                {
                    ImGui::TextDisabled("No texture loaded");
                }
            }
            ImGui::EndGroup();
        }
        
        static void DrawGallery(const char* label, std::shared_ptr<ITexture>& texture, 
                              TextureType type, const TextureSlotConfig& config,
                              TextureSlotAction& outAction, const TextureSlotCallback& callback)
        {
            ScopeID id(label);
            
            // Large card for gallery view
            CardConfig cardConfig = CardPresets::Elevated();
            cardConfig.FixedSize = ImVec2(
                config.PreviewSize + 32.0f,
                config.PreviewSize + 80.0f
            );
            cardConfig.Hoverable = true;
            
            Card card(label, cardConfig);
            if(card.Begin())
            {
                ImGui::Spacing();
                
                // Centered preview
                float indent = 16.0f;
                ImGui::Indent(indent);
                
                ImVec2 screenPos = ImGui::GetCursorScreenPos();
                ImVec2 previewSize(config.PreviewSize, config.PreviewSize);
                
                // Draw preview
                DrawPreview(texture, screenPos, previewSize, config);
                
                // Invisible button for interaction
                ImGui::InvisibleButton("##preview", previewSize);
                
                // Handle interactions
                HandleInteractions(texture, type, config, outAction, callback);
                
                ImGui::Unindent(indent);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Info footer
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), 
                                 "%s", GetTextureTypeString(type).c_str());
                
                if(texture)
                {
                    const auto& spec = texture->GetSpecification();
                    ImGui::TextDisabled("%ux%u", spec.Width, spec.Height);
                }
                
                card.End();
            }
        }
        
    private:
        static void DrawPreview(const std::shared_ptr<ITexture>& texture, 
                              const ImVec2& screenPos, const ImVec2& size,
                              const TextureSlotConfig& config)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImRect previewRect(screenPos, ImVec2(screenPos.x + size.x, screenPos.y + size.y));
            
            if(texture)
            {
                // Draw texture
                ImTextureID texID = TextureSlotUtils::AsImTextureID(texture);
                drawList->AddImage(texID, previewRect.Min, previewRect.Max, 
                                 ImVec2(0, 1), ImVec2(1, 0));
            }
            else
            {
                // Draw empty slot with checker pattern
                TextureSlotUtils::DrawCheckerPattern(drawList, previewRect, 12.0f);
                
                // Draw "Click to load" text
                const char* text = "Click to load";
                ImVec2 textSize = ImGui::CalcTextSize(text);
                ImVec2 textPos(
                    previewRect.Min.x + (size.x - textSize.x) * 0.5f,
                    previewRect.Min.y + (size.y - textSize.y) * 0.5f
                );
                
                drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), text);
            }
        }
        
        static void HandleInteractions(std::shared_ptr<ITexture>& texture, 
                                      TextureType type, 
                                      const TextureSlotConfig& config,
                                      TextureSlotAction& outAction,
                                      const TextureSlotCallback& callback)
        {
            // Tooltip with larger preview
            if(ImGui::IsItemHovered())
            {
                ShowTooltip(texture, type, config.TooltipPreviewSize);
            }
            
            // Click to load
            if(config.EnableFileDialog && ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                if(TextureSlotUtils::LoadTextureFromDialog(texture, type))
                {
                    outAction = TextureSlotAction::Upload;
                    if(callback) callback(outAction, texture);
                }
            }
            
            // Context menu
            if(ImGui::BeginPopupContextItem("##ctx"))
            {
                ShowContextMenu(texture, type, config, outAction, callback);
                ImGui::EndPopup();
            }
            
            // Drag and drop
            if(config.EnableDragDrop && ImGui::BeginDragDropTarget())
            {
                HandleDragDrop(texture, type, outAction, callback);
                ImGui::EndDragDropTarget();
            }
        }
        
        static void ShowTooltip(const std::shared_ptr<ITexture>& texture, 
                              TextureType type, int tooltipSize)
        {
            ImGui::BeginTooltip();
            
            ImVec2 previewSize(tooltipSize, tooltipSize);
            
            if(texture)
            {
                ImTextureID texID = TextureSlotUtils::AsImTextureID(texture);
                ImGui::Image(texID, previewSize, ImVec2(0, 1), ImVec2(1, 0));
            }
            else
            {
                ImGui::Dummy(previewSize);
            }
            
            ImGui::Separator();
            ImGui::Text("Expected: %s", GetTextureTypeString(type).c_str());
            
            if(texture)
            {
                const auto& spec = texture->GetSpecification();
                if(!spec.TextureFile.empty())
                {
                    ImGui::TextWrapped("%s", spec.TextureFile.c_str());
                }
                ImGui::Text("Size: %ux%u", spec.Width, spec.Height);
            }
            else
            {
                ImGui::TextDisabled("No texture loaded");
            }
            
            ImGui::EndTooltip();
        }
        
        static void ShowContextMenu(std::shared_ptr<ITexture>& texture, 
                                   TextureType type, 
                                   const TextureSlotConfig& config,
                                   TextureSlotAction& outAction,
                                   const TextureSlotCallback& callback)
        {
            if(config.EnableFileDialog && ImGui::MenuItem("Load Texture..."))
            {
                if(TextureSlotUtils::LoadTextureFromDialog(texture, type))
                {
                    outAction = TextureSlotAction::Upload;
                    if(callback) callback(outAction, texture);
                }
            }
            
            bool canReload = texture && !texture->GetSpecification().TextureFile.empty();
            if(config.AllowReload && ImGui::MenuItem("Reload", nullptr, false, canReload))
            {
                const auto& file = texture->GetSpecification().TextureFile;
                texture->ReloadFromFile(file, type);
                outAction = TextureSlotAction::Reload;
                if(callback) callback(outAction, texture);
            }
            
            if(texture)
            {
                ImGui::Separator();
                
                if(config.AllowFlip && ImGui::MenuItem("Flip Vertically"))
                {
                    const auto& file = texture->GetSpecification().TextureFile;
                    bool flip = !texture->GetSpecification().FlipOnLoadDefault;
                    texture->ReloadFromFile(file, type, flip);
                    outAction = TextureSlotAction::FlipVertical;
                    if(callback) callback(outAction, texture);
                }
                
                ImGui::Separator();
            }
            
            if(config.AllowClear && ImGui::MenuItem("Clear", nullptr, false, texture != nullptr))
            {
                texture.reset();
                outAction = TextureSlotAction::Clear;
                if(callback) callback(outAction, texture);
            }
        }
        
        static void HandleDragDrop(std::shared_ptr<ITexture>& texture, 
                                  TextureType type,
                                  TextureSlotAction& outAction,
                                  const TextureSlotCallback& callback)
        {
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                const char* path = static_cast<const char*>(payload->Data);
                if(path && *path)
                {
                    if(!texture)
                        texture = ITexture::Create(path, type);
                    else
                        texture->ReloadFromFile(path, type);
                    
                    outAction = TextureSlotAction::Upload;
                    if(callback) callback(outAction, texture);
                }
            }
        }
    };
    
    inline TextureSlotAction TextureSlot(const char* label, 
                                        std::shared_ptr<ITexture>& texture, 
                                        TextureType type,
                                        const TextureSlotConfig& config = {},
                                        const TextureSlotCallback& callback = nullptr)
    {
        TextureSlotAction action = TextureSlotAction::None;
        
        switch(config.Style)
        {
            case TextureSlotStyle::Card:
                TextureSlotRenderer::DrawCard(label, texture, type, config, action, callback);
                break;
                
            case TextureSlotStyle::Compact:
                TextureSlotRenderer::DrawCompact(label, texture, type, config, action, callback);
                break;
                
            case TextureSlotStyle::Detailed:
                TextureSlotRenderer::DrawDetailed(label, texture, type, config, action, callback);
                break;
                
            case TextureSlotStyle::Gallery:
                TextureSlotRenderer::DrawGallery(label, texture, type, config, action, callback);
                break;
        }
        
        return action;
    }
    
    namespace TextureSlotPresets
    {
        inline TextureSlotConfig Card()
        {
            TextureSlotConfig config;
            config.Style = TextureSlotStyle::Card;
            config.CardDesign = CardStyle::Neumorphic;
            config.PreviewSize = 150;
            return config;
        }
        
        inline TextureSlotConfig Compact()
        {
            TextureSlotConfig config;
            config.Style = TextureSlotStyle::Compact;
            config.PreviewSize = 100;
            config.ShowFilename = false;
            return config;
        }
        
        inline TextureSlotConfig Detailed()
        {
            TextureSlotConfig config;
            config.Style = TextureSlotStyle::Detailed;
            config.PreviewSize = 200;
            config.ShowDimensions = true;
            return config;
        }
        
        inline TextureSlotConfig Gallery()
        {
            TextureSlotConfig config;
            config.Style = TextureSlotStyle::Gallery;
            config.PreviewSize = 256;
            config.CardDesign = CardStyle::Elevated;
            return config;
        }
        
        inline TextureSlotConfig Small()
        {
            TextureSlotConfig config;
            config.Style = TextureSlotStyle::Card;
            config.PreviewSize = 80;
            config.CardPadding = 8.0f;
            return config;
        }
        
        inline TextureSlotConfig Large()
        {
            TextureSlotConfig config;
            config.Style = TextureSlotStyle::Card;
            config.PreviewSize = 256;
            config.CardPadding = 16.0f;
            return config;
        }
    }
}