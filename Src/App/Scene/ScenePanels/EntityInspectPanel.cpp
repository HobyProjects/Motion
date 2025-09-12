#include "CorePCH.hpp"
#include "EntityInspectPanel.hpp"

namespace Motion
{
    template<typename T>
    static bool Has(const std::shared_ptr<Entity>& e) { return e->HasComponent<T>(); }

    template<typename T>
    static T& Get(const std::shared_ptr<Entity>& e) { return e->GetComponent<T>(); }

    template<typename T>
    static void AddCopy(const std::shared_ptr<Entity>& dst, const T& src) 
    {
        dst->AddComponent<T>(src);
    }

    template<typename T>
    static void CopyComponentIfPresent(const std::shared_ptr<Entity>& src,const std::shared_ptr<Entity>& dst) 
    {
        if (Has<T>(src)) 
        {
            AddCopy<T>(dst, Get<T>(src));
        }
    }

    static std::shared_ptr<Entity> DuplicateEntity(Scene* scene, const std::shared_ptr<Entity>& src)
    {
        auto& fac = EntityFactory::GetInstance();

        std::string baseName = Has<TagComponent>(src) ? Get<TagComponent>(src).Tag : std::string("Entity");
        std::string name = baseName + " (Copy)";

        auto dst = fac.CreateEntity(name);
        CopyComponentIfPresent<TransformComponent>(src, dst);

        if (Has<MeshComponent>(src)) 
        {
            const auto& sm = Get<MeshComponent>(src);
            dst->AddComponent<MeshComponent>(sm.Model->GetName(), sm.Model);
        }

        // TODO: Add more components here 
        CopyComponentIfPresent<RigidBodyComponent>(src, dst);
        CopyComponentIfPresent<ColliderComponent>(src, dst);
        CopyComponentIfPresent<DampingComponent>(src, dst);
        
        scene->EmplaceEntity(dst);
        return dst;
    }

    static std::string EntityLabel(const std::shared_ptr<Entity>& e) 
    {
        if (e->HasComponent<TagComponent>()) return e->GetComponent<TagComponent>().Tag;
        return "Entity";
    }

    struct DragDupState 
    {
        bool                  active = false;
        std::shared_ptr<Entity> newEntity;
        std::weak_ptr<Entity>  source;
    };

    static DragDupState& GetDragDupState() 
    {
        static DragDupState s;
        return s;
    }

    void SceneEntityInspectPanel::RenderUI(ScenePanelContext& ctx)
    {
        if (!ctx.ActiveScene) return;

        ImGui::Begin(ICON_MD_LIST " Entities");

        if (ImGui::BeginPopupContextWindow("##import-mesh-context", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::MenuItem(ICON_MD_FILE_UPLOAD "  Import Model"))
            {
                if (auto path = DialogBoxes::OpenFileDialog(); !path.empty())
                {
                    auto ID = Importer::ImportModelAsync(path, false, "default",
                        [ctx, path](std::shared_ptr<Model> mesh)
                        {
                            if (!mesh)
                            {
                                MOTION_ERROR("Fail to import model in {}", path);
                                return;
                            }

                            auto scene = ctx.ActiveScene;
                            if (!scene)
                            {
                                MOTION_ERROR("No active scene when importing '{}'", path);
                                return;
                            }

                            auto& fac   = EntityFactory::GetInstance();
                            auto entity = fac.CreateEntity(mesh->GetName());
                            if (!entity)
                            {
                                MOTION_ERROR("EntityFactory::CreateEntity('{}') returned null", mesh->GetName());
                                return;
                            }

                            const auto& m = entity->AddComponent<MeshComponent>(mesh->GetName(), mesh);
                            const auto& t = entity->AddComponent<TransformComponent>();
                            auto& r = entity->AddComponent<RigidBodyComponent>();
                            entity->AddComponent<ColliderComponent>(m);
                            entity->AddComponent<DampingComponent>();
                            scene->EmplaceEntity(entity);
                        },
                        [&](std::int32_t /*progress*/) {}
                    );
                }
            }
            
            ImGui::EndPopup();
        }

        std::vector<std::shared_ptr<Entity>> toRemove;
        auto selected = ctx.ActiveScene->GetSelectedEntity();

        const bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
        bool shortcutDuplicate = false;
        bool shortcutDelete    = false;

        if (windowFocused && selected) 
        {
            const ImGuiIO& io = ImGui::GetIO();
            if ((io.KeyCtrl || io.KeySuper) && ImGui::IsKeyPressed(ImGuiKey_D, false))
                shortcutDuplicate = true;

            if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
                shortcutDelete = true;
        }

        if (shortcutDuplicate && selected) 
        {
            auto duplicated = DuplicateEntity(ctx.ActiveScene.get(), selected);
            ctx.ActiveScene->SelectedEntity(duplicated);
        }

        if (shortcutDelete && selected) 
        {
            toRemove.push_back(selected);
            ctx.ActiveScene->SelectedEntity(EntityFactory::EMPTYENTITY);
        }

        for (auto& ent : *ctx.ActiveScene) 
        {
            ImGui::PushID(ent.get());

            static bool isSelected = (selected == ent);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
                                    | ImGuiTreeNodeFlags_NoTreePushOnOpen
                                    | ImGuiTreeNodeFlags_SpanFullWidth
                                    | ImGuiTreeNodeFlags_Framed;
                                    
            if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

            const std::string label = EntityLabel(ent);
            ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) 
            {
                ctx.ActiveScene->SelectedEntity(ent);
                isSelected = true;
            }

            bool requestDuplicate = false;
            bool requestDelete    = false;

            if (ImGui::BeginPopupContextItem()) 
            {
                if (ImGui::MenuItem(ICON_MD_CONTENT_COPY "  Duplicate"))
                    requestDuplicate = true;

                if (ImGui::MenuItem(ICON_MD_DELETE "  Delete"))
                    requestDelete = true;

                ImGui::EndPopup();
            }

            if (isSelected) 
            {
                auto& state         = GetDragDupState();
                const ImGuiIO& io   = ImGui::GetIO();
                const bool altHeld  = io.KeyAlt;

                if (!state.active && altHeld && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) 
                {
                    state.active   = true;
                    state.source   = ent;
                    state.newEntity = DuplicateEntity(ctx.ActiveScene.get(), ent);

                    ctx.ActiveScene->SelectedEntity(state.newEntity);
                    if (Has<TransformComponent>(state.newEntity)) 
                    {
                        auto& t = Get<TransformComponent>(state.newEntity);
                        t.Translation += glm::vec3(0.05f, 0.05f, 0.05f); 
                    }
                }

                if (state.active && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) 
                {
                    state.active = false;
                    state.newEntity.reset();
                    state.source.reset();
                }
            }

            if (requestDuplicate) 
            {
                auto duplicated = DuplicateEntity(ctx.ActiveScene.get(), ent);
                ctx.ActiveScene->SelectedEntity(duplicated);
            }

            if (requestDelete) 
            {
                if (selected == ent)
                    ctx.ActiveScene->SelectedEntity(EntityFactory::EMPTYENTITY);

                toRemove.push_back(ent);
            }

            ImGui::PopID();
        }

        if (!toRemove.empty()) 
        {
            for (auto& e : toRemove)
                ctx.ActiveScene->RemoveEntity(e); 

            toRemove.clear();
        }

        ImGui::End();
    }
}