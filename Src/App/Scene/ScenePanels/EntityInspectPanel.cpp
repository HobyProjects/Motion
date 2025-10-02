#include "CorePCH.hpp"
#include "EntityInspectPanel.hpp"

namespace Motion
{
    void SceneEntityInspectPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin(ICON_MD_LIST " Entities");

        struct ImportUIState 
        {
            bool showModal = false;
            double startTime = 0.0;
            std::future<std::shared_ptr<ImportedResults>> future; 
        };

        static ImportUIState s;

        if (ImGui::BeginPopupContextWindow("##import-mesh-context", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem(ICON_MD_FILE_UPLOAD "  Import Model")) 
            {
                if (auto path = DialogBoxes::OpenFileDialog(); !path.empty()) 
                {
                    s.future = std::async(std::launch::async, [path]
                    {
                        return Motion::Importer::ImportModelAsync(path, false, "default");
                    });

                    s.showModal = true;
                    s.startTime = ImGui::GetTime();
                    ImGui::OpenPopup("Importing model…");
                }
            }

            ImGui::EndPopup();
        }

        if (s.showModal) 
        {
            ImGui::OpenPopup("Importing model…");
            if (ImGui::BeginPopupModal("Importing model…", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
            {
                float t = float(fmod(ImGui::GetTime() - s.startTime, 1.0));
                float p = 0.25f + 0.5f * (0.5f - std::abs(t - 0.5f)) * 2.0f;

                ImGui::TextUnformatted("Crunching triangles and untangling meshes…");
                ImGui::Dummy(ImVec2(0, 8));
                ImGui::ProgressBar(p, ImVec2(320, 0), "Working");

                using namespace std::chrono_literals;
                if (s.future.valid() && s.future.wait_for(0ms) == std::future_status::ready)
                {
                    auto result = s.future.get(); 
                    if (result) 
                    {
                        auto& KX = KinetiX::GetInstance();
                        std::shared_ptr<Entity> root = Entity::Create(result->Name);

                        const BufferLayout layout
                        {
                            { "a_Position",   BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Position)    },
                            { "a_TexCoords",  BufferComponents::UV,   BufferStride::F2, false, offsetof(Vertex, TexCoord)    },
                            { "a_Normals",    BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Normal)      },
                            { "a_Tangents",   BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)     },
                            { "a_Bitangents", BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Bitangent)   },
                        };

                        auto& node  = root->Get<NodeComponent>();
                        node.IsRoot = true;
                        
                        bool nextNodeSet{false};
                        std::shared_ptr<Entity> lastEntt{nullptr};

                        for (const auto& [meshID, mesh] : result->Meshes)
                        {
                            auto entt = Entity::Create(std::format("{}[{}]", mesh.Name, meshID));
                            if (!nextNodeSet)
                            {
                                node.EnTTNext = entt;
                                nextNodeSet = true;
                            }

                            if(lastEntt) 
                            {
                                lastEntt->Get<NodeComponent>().EnTTNext     = entt;
                                lastEntt->Get<NodeComponent>().IsRoot       = false;
                            }
                            
                            auto& meshComponent = entt->Emplace<MeshComponent>();

                            auto meshPtr = Mesh::Create(
                                mesh.Vertices.data(), static_cast<std::uint32_t>(mesh.Vertices.size()), 
                                mesh.Indices.data(),  static_cast<std::uint32_t>(mesh.Indices.size()), 
                                layout
                            );

                            auto bounds = ModelBounds(mesh.Vertices);
                            meshPtr->MIN = bounds.first;
                            meshPtr->MAX = bounds.second;

                            std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(meshPtr->Positions), [](const Vertex& v) { return v.Position; });
                            meshPtr->Faces.insert(meshPtr->Faces.end(), mesh.Indices.begin(), mesh.Indices.end());
                            meshComponent.MeshPointer = std::move(meshPtr);

                            entt->Emplace<TransformComponent>();
                            entt->Emplace<RigidBodyComponent>();
                            entt->Emplace<ColliderComponent>();
                            
                            auto& material              = entt->Emplace<MaterialComponent>();
                            material.MaterialPointer    = Material::Create();

                            KX.CreateRigidBody(entt);
                            KX.CreateConvexCollider(entt, meshComponent.MeshPointer->Positions, meshComponent.MeshPointer->Faces);
                            lastEntt = entt;
                        }

                        ctx.ActiveScene->EmplaceEntity(root);
                        ctx.ActiveScene->SelectedEntity(root);
                    } 
                    else
                    {
                        MOTION_ERROR("Failed to import model.");
                    }

                    ImGui::CloseCurrentPopup();
                    s = ImportUIState{}; 
                }

                ImGui::EndPopup();
            }
        }
        
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed;
        static std::shared_ptr<Entity> selectedEntity = nullptr;
        for (auto& ent : *ctx.ActiveScene)
        {
            std::string name = ent->Get<TagComponent>().Tag;
            ImGui::PushID(ent.get());
            if (ImGui::CollapsingHeader(name.c_str(), flags))
            {
                if (ImGui::IsItemClicked())
                {
                    selectedEntity = ent;
                    ctx.ActiveScene->SelectedEntity(selectedEntity);
                }

                std::shared_ptr<Entity> current = ent;
                std::shared_ptr<Entity> next = nullptr;
                while (current)
                {
                    if (current->Has<NodeComponent>())
                    {
                        next = current->Get<NodeComponent>().EnTTNext;
                        if (current->Get<NodeComponent>().IsRoot)
                        {
                            current = next;
                            continue;
                        }
                    }

                    std::string currentEntityName = current->Get<TagComponent>().Tag;
                    ImGui::PushID(current.get());
                    ImGui::Indent();
                    if (ImGui::CollapsingHeader(currentEntityName.c_str()))
                    {
                        if (ImGui::IsItemClicked())
                        {
                            selectedEntity = current;
                            ctx.ActiveScene->SelectedEntity(selectedEntity);
                        }

                        ImGui::Indent();
                        ImGui::BulletText(ICON_FA_CUBE " Transform Component");
                        ImGui::BulletText(ICON_FA_CUBES " Mesh Component");
                        ImGui::BulletText(ICON_MD_IMAGE " Material Component");
                        ImGui::BulletText(ICON_MD_3D_ROTATION " RigidBody Component");
                        ImGui::BulletText(ICON_FA_BOX " Collider Component");
                        ImGui::Unindent();
                    }
                    ImGui::Unindent();
                    ImGui::PopID();

                    current = next;
                }
            }
            ImGui::PopID();
        }

        ImGui::End();
    }

}