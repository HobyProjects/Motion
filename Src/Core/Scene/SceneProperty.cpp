#include "CorePCH.hpp"

namespace Motion
{
    namespace
    {
        // ----- Transform utilities (pure helpers) -----
        static float Wrap180(float a)
        {
            a = std::fmod(a + 180.0f, 360.0f);
            if (a < 0) a += 360.0f;
            return a - 180.0f;
        }

        static glm::vec3 WrapEuler(glm::vec3 deg)
        {
            return { Wrap180(deg.x), Wrap180(deg.y), Wrap180(deg.z) };
        }

        static bool ApplyRotationIfChanged(glm::quat& dst, const glm::vec3& eulerDeg)
        {
            const glm::vec3 rad = glm::radians(eulerDeg);
            glm::quat q = glm::normalize(glm::quat(rad));
            if (glm::any(glm::epsilonNotEqual(q, dst, 1e-6f)))
            {
                dst = q;
                return true;
            }
            return false;
        }
    } 

    // ============================================================================================
    // Construction & resources
    // ============================================================================================

    ScenePropertyPanel::ScenePropertyPanel(const std::string& name)
        : m_Title(name)
    {
        LoadDefaultBaseMaterials();
        std::memset(m_SearchBuf, 0, sizeof(m_SearchBuf));
    }

    void ScenePropertyPanel::LoadDefaultBaseMaterials()
    {
        static const std::array<const char*, 5> kPaths = {
            "Assets/Materials/Metal/Base.yaml",
            "Assets/Materials/Marble/Base.yaml",
            "Assets/Materials/Plastic/Base.yaml",
            "Assets/Materials/Rubber/Base.yaml",
            "Assets/Materials/Stone/Base.yaml",
        };

        m_BaseMaterial.reserve(kPaths.size());
        for (auto* p : kPaths)
            m_BaseMaterial.push_back(Material::CreateBase(p));
    }

    // ============================================================================================
    // Material UI
    // ============================================================================================

    void ScenePropertyPanel::DrawMaterialUI(ScenePanelContext&, std::shared_ptr<Material>& mat)
    {
        if (!mat) return;

        if (ImGui::TreeNodeEx("Material Properties", ImGuiTreeNodeFlags_Framed))
        {
            DrawAttributes(mat);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_Framed))
        {
            DrawTexturesSlots(mat);
            ImGui::TreePop();
        }
    }

    void ScenePropertyPanel::DrawAttributes(std::shared_ptr<Material>& mat)
    {
        if (BeginPropertyGrid("##base-material"))
        {
            auto base = mat->GetBaseMaterial();
            std::vector<std::string> names;
            names.reserve(m_BaseMaterial.size() + 1);
            names.push_back("None");
            std::ranges::transform(m_BaseMaterial, std::back_inserter(names),
                [](const auto& m) { return m->Name; });

            std::int32_t index = 0;
            if (base)
            {
                if (auto it = std::ranges::find(names, base->Name); it != names.end())
                    index = static_cast<std::int32_t>(std::distance(names.begin(), it));
            }

            ComboBox("Base Material", names, index,
                [&](std::int32_t, const std::string& selectedName)
                {
                    if (selectedName == "None") { mat->SetBaseMaterial(nullptr); return; }
                    if (auto it = std::ranges::find_if(m_BaseMaterial, [&](const auto& m){ return m->Name == selectedName; });
                        it != m_BaseMaterial.end())
                    {
                        mat->SetBaseMaterial(*it);
                    }
                });

            EndPropertyGrid();
        }

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

        if (mat->Has<CoreMaterialComponents>())
        {
            auto& C = mat->Get<CoreMaterialComponents>();
            if (BeginPropertyGrid("##core-pbr"))
            {
                ColorEdit4("Base Color",            C.BaseColorFactor);
                SliderFloat("Metallic Factor",      &C.MetallicFactor, 0.0f, 1.0f, "%.3f");
                SliderFloat("Roughness Factor",     &C.RoughnessFactor, 0.0f, 1.0f, "%.3f");
                SliderFloat("Normal Scaling",       &C.NormalScale,     0.0f, 1.0f, "%.3f");
                SliderFloat("Occlusion Strength",   &C.OcclusionStrength, 0.0f, 1.0f, "%.3f");
                ColorEdit3("Emissive Factor",       C.EmissiveFactor);
                SliderFloat("Emissive Strength",    &C.EmissiveStrength, 0.0f, 1.0f, "%.3f");
                SliderFloat("Opacity Factor",       &C.OpacityFactor, 0.0f, 1.0f, "%.3f");
                EndPropertyGrid();
            }
        }
        else
        {
            ImGui::TextDisabled(ICON_MD_INFO " No textures assigned");
        }
    }

    void ScenePropertyPanel::DrawTexturesSlots(std::shared_ptr<Material>& mat)
    {
        if (mat->Has<CoreMaterialComponents>())
        {
            auto& C = mat->Get<CoreMaterialComponents>();

            struct Row { const char* Label; std::shared_ptr<ITexture>& Tex; TextureType Type; };
            std::vector<Row> textures =
            {
                {"Base Color",  C.BaseColorTexture, TextureType::BaseColorTexture},
                {"Metallic",    C.MetallicTexture,  TextureType::MetallicTexture},
                {"Roughness",   C.RoughnessTexture, TextureType::RoughnessTexture},
                {"Normal",      C.NormalTexture,    TextureType::NormalTexture},
                {"Occlusion",   C.OcclusionTexture, TextureType::AmbientOcclusionTexture},
                {"Emissive",    C.EmissiveTexture,  TextureType::EmissiveTexture},
            };

            const int columns = 4;
            ImGui::BeginTable("##core-pbr", columns, ImGuiTableFlags_NoBordersInBody);
            for (size_t i = 0; i < textures.size(); ++i)
            {
                if (i % columns == 0) ImGui::TableNextRow();
                ImGui::TableNextColumn();
                TextureSlot(textures[i].Label, textures[i].Tex, textures[i].Type);
            }
            ImGui::EndTable();
        }
        else
        {
            ImGui::TextDisabled(ICON_FA_INFO " No textures assigned");
        }
    }

    // ============================================================================================
    // Entity / components
    // ============================================================================================

    void ScenePropertyPanel::RenderNodeEntities(ScenePanelContext& c, entt::registry& r, entt::entity root)
    {
        c.ScenePointer->ForEachNodeEntity(root, [&](entt::registry& r2, entt::entity e)
        {
            if (e == root) return;
            const TagComponent* tagOpt = r2.try_get<TagComponent>(e);
            const char* label = tagOpt ? tagOpt->Tag.c_str() : "Unnamed";

            ImGui::PushID(static_cast<std::int32_t>(entt::to_integral(e)));

            const ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth |
                ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

            const bool open = ImGui::TreeNodeEx("##node", flags, "%s %s", label, (e == c.ScenePointer->GetSelectedEntity()) ? ICON_MD_STAR : "");
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                c.ScenePointer->SelectedEntity(e);

            if (open)
            {
                if (tagOpt && tagOpt->IsActive)
                {
                    RenderTagAndModel(r2, e);
                    RenderTransform(r2, e);
                    RenderPhysics(c, r2, e);
                    RenderMaterialEditor(c, r2, e);
                }
                else
                {
                    ImGui::TextDisabled(ICON_FA_INFO " Entity is inactive");
                }
                ImGui::TreePop();
            }

            ImGui::PopID();
        });
    }

    void ScenePropertyPanel::RenderTagAndModel(entt::registry& r, entt::entity e)
    {
        if (!r.any_of<TagComponent>(e)) return;

        auto& tag = r.get<TagComponent>(e);

        BeginPropertyGrid("##tag-grid");
        TextBox("Name Tag", tag.Tag, false);
        ToggleSwitch("Is Active", tag.IsActive);

        if (r.any_of<ModelComponent>(e))
        {
            const auto& model = r.get<ModelComponent>(e);

            std::string file  = model.FilePath.filename().string();
            std::string path  = model.FilePath.parent_path().string();
            std::string count = std::to_string(model.MeshCount);

            TextBox("Model File", file, true);
            TextBox("Model Path", path, true);
            TextBox("Mesh Count", count, true);
        }
        EndPropertyGrid();
    }

    void ScenePropertyPanel::RenderTransform(entt::registry& r, entt::entity e)
    {
        if (auto* tr = r.try_get<TransformComponent>(e))
        {
            BeginPropertyGrid("##transform-grid");

            glm::vec3 t = tr->Translation;
            if (DragFloat3("Translation (m)", t, 0.1f))
                tr->Translation = t;

            glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(tr->Rotation));
            eulerDeg = WrapEuler(eulerDeg);

            glm::vec3 edited = eulerDeg;
            if (DragFloat3("Rotation (deg)", edited, 0.1f))
                ApplyRotationIfChanged(tr->Rotation, edited);

            glm::vec3 s = tr->Scale;
            if (DragFloat3("Scale (m)", s, 0.1f))
                tr->Scale = s;

            EndPropertyGrid();
        }
    }

    void ScenePropertyPanel::DrawRigidBodyUI(RigidBodyComponent& rb)
    {
        // Interaction type
        std::int32_t interaction = (rb.PhysicsBody->getType() == rp3d::BodyType::DYNAMIC) ? 1 : 0;
        ComboBox("Interaction", { "Static", "Dynamic" }, interaction,
            [&](std::int32_t idx, const std::string&)
            {
                if (idx == 0) { rb.Type = BodyType::Static;  rb.PhysicsBody->setType(rp3d::BodyType::STATIC); }
                if (idx == 1) { rb.Type = BodyType::Dynamic; rb.PhysicsBody->setType(rp3d::BodyType::DYNAMIC); }
            });

        auto* body = rb.PhysicsBody;

        float mass = static_cast<float>(body->getMass());
        if (DragFloat("Compute Mass", &mass, 0.001f, 0.0f, 1e10f)) body->setMass(mass);

        float linDamp = static_cast<float>(body->getLinearDamping());
        if (DragFloat("Linear Damping", &linDamp, 0.01f, 0.0f, 1.0f)) body->setLinearDamping(linDamp);

        float angDamp = static_cast<float>(body->getAngularDamping());
        if (DragFloat("Angular Damping", &angDamp, 0.01f, 0.0f, 1.0f)) body->setAngularDamping(angDamp);
    }

    void ScenePropertyPanel::DrawColliderUI(ColliderComponent& cc)
    {
        float bounce = cc.Restitution;
        if (DragFloat("Bounce", &bounce, 0.001f, 0.0f, 1.0f))
        {
            cc.Collider->getMaterial().setBounciness(bounce);
            cc.Restitution = bounce;
        }

        float friction = cc.Friction;
        if (DragFloat("Friction", &friction, 0.001f, 0.0f, 1.0f))
        {
            cc.Collider->getMaterial().setFrictionCoefficient(friction);
            cc.Friction = friction;
        }

        float density = cc.MassDensity;
        if (DragFloat("Density", &density, 0.01f, 0.0f, FLT_MAX))
        {
            cc.Collider->getMaterial().setMassDensity(density);
            cc.MassDensity = density;
        }
    }

    void ScenePropertyPanel::RenderPhysics(ScenePanelContext&, entt::registry& r, entt::entity e)
    {
        auto* cc = r.try_get<ColliderComponent>(e);
        auto* rb = r.try_get<RigidBodyComponent>(e);
        if (!cc || !rb) return;

        BeginPropertyGrid("##physics-grid");
        DrawRigidBodyUI(*rb);
        DrawColliderUI(*cc);
        EndPropertyGrid();
    }

    void ScenePropertyPanel::RenderMaterialEditor(ScenePanelContext& c, entt::registry& r, entt::entity e)
    {
        if (auto* material = r.try_get<MaterialComponent>(e))
        {
            static bool isEditorOpen = false; // OK to keep local-static: it's a single toggle UI
            if (ImGui::Button(ICON_MD_IMAGE " Material Editor"))
                isEditorOpen = !isEditorOpen;

            if (isEditorOpen)
            {
                std::string w = std::string(ICON_MD_IMAGE " Material Editor##") + std::to_string((uintptr_t)material->ID);
                if (ImGui::Begin(w.c_str(), &isEditorOpen, ImGuiWindowFlags_NoDocking))
                {
                    if (ImGui::BeginChild("##inspector-area", ImVec2(0.0f, 0.0f)))
                    {
                        if (material->MaterialPointer)
                            DrawMaterialUI(c, material->MaterialPointer);
                        else
                            ImGui::TextDisabled(ICON_MD_INFO " No material assigned");
                        ImGui::EndChild();
                    }
                    ImGui::End();
                }
            }
        }
    }

    // ============================================================================================
    // RenderUI decomposition
    // ============================================================================================

    void ScenePropertyPanel::RenderToolbarAndSearch(ScenePanelContext& ctx)
    {
        // search box
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 38.0f);
        ImGui::InputTextWithHint("##SearchScenes", ICON_MD_SEARCH " Search scenes...", m_SearchBuf, sizeof(m_SearchBuf));
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button(ICON_MD_ADD "##AddEntity"))
        {
            DialogBoxes::InitializeCOM();

            OpenDialogOptions options{};
            options.Title = L"Import Model";
            options.DefaultExtension = L"obj";
            options.AllowMultiSelect = false;
            options.InitialDirectory = std::filesystem::current_path();
            options.Filters =
            {
                {L"Mesh Files", L"*.fbx;*.obj;*.gltf;*.glb"},
                {L"All Files",  L"*.*"}
            };

            if (auto path = DialogBoxes::OpenFileDialog(options); !path.empty())
            {
                m_ImportFuture = std::async(std::launch::async, [path]
                {
                    return Importer::ImportModelAsync(path, false, "default");
                });

                ImGui::OpenPopup("ImportEntity##Popup");
            }

            DialogBoxes::UninitializeCOM();
        }

        ImGui::Separator();
    }

    void ScenePropertyPanel::RenderImportPopup(ScenePanelContext& context)
    {
        using namespace std::chrono_literals;

        if (ImGui::BeginPopupModal("ImportEntity##Popup", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
        {
            float t = float(fmod(ImGui::GetTime(), 1.0));
            float p = 0.25f + 0.5f * (0.5f - std::abs(t - 0.5f)) * 2.0f;
            ImGui::TextUnformatted("Crunching triangles and untangling meshes…");
            ImGui::Dummy(ImVec2(0, 8));
            ImGui::ProgressBar(p, ImVec2(320, 0), "Working");

            if (m_ImportFuture.valid() && m_ImportFuture.wait_for(0ms) == std::future_status::ready)
            {
                const auto& results = m_ImportFuture.get();
                if (results)
                {
                    auto* scene    = context.ScenePointer;
                    auto* registry = context.SceneRegistry;

                    const BufferLayout layout
                    {
                        { "a_Position",   BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Position)    },
                        { "a_TexCoords",  BufferComponents::UV,   BufferStride::F2, false, offsetof(Vertex, TexCoord)    },
                        { "a_Normals",    BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Normal)      },
                        { "a_Tangents",   BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)     },
                        { "a_Bitangents", BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Bitangent)   },
                    };

                    // Root entity
                    entt::entity root = registry->create();
                    registry->emplace<TagComponent>(root, results->Name);
                    registry->emplace<TransformComponent>(root);

                    auto& modelCompo     = registry->emplace<ModelComponent>(root);
                    modelCompo.FilePath  = results->FilePath;
                    modelCompo.MeshCount = results->MeshCount;
                    modelCompo.MaxBounds = results->MAX;
                    modelCompo.MinBounds = results->MIN;

                    std::vector<entt::entity> children;
                    children.reserve(results->MeshCount);

                    for (const auto& [index, mesh] : results->Meshes)
                    {
                        (void)index;

                        entt::entity e = registry->create();
                        registry->emplace<TagComponent>(e, mesh.Name);
                        registry->emplace<TransformComponent>(e);
                        registry->emplace<RigidBodyComponent>(e);
                        registry->emplace<ColliderComponent>(e);

                        auto& meshCompo = registry->emplace<MeshComponent>(e);
                        meshCompo.MeshPointer = Mesh::Create(mesh.Vertices.data(), mesh.Vertices.size(),
                                                             mesh.Indices.data(), mesh.Indices.size(), layout);
                        meshCompo.MaxBounds = mesh.MAX;
                        meshCompo.MinBounds = mesh.MIN;
                        meshCompo.Name      = mesh.Name;

                        auto& materialCompo = registry->emplace<MaterialComponent>(e);
                        materialCompo.MaterialPointer = Material::Create();

                        CreateRigidBody(context.PhysicsWorld, registry, e);

                        std::vector<glm::vec3> verts;
                        verts.reserve(mesh.Vertices.size());
                        std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(verts),
                                       [](const Vertex& v) { return v.Position; });

                        CreateConvexCollider(context.PhysicsCommon, registry, e, verts);

                        children.push_back(e);
                    }

                    // Link hierarchy
                    entt::entity prev = entt::null;
                    for (std::size_t i = 0; i < children.size(); ++i)
                    {
                        auto e = children[i];
                        registry->emplace<HierarchyComponent>(e, root, entt::null, entt::null);
                        if (i > 0) registry->get<HierarchyComponent>(prev).NextSibling = e;
                        prev = e;
                    }

                    registry->emplace<HierarchyComponent>(root, entt::null,
                        children.empty() ? entt::null : children.front(), entt::null);

                    scene->EmplaceEntity(root);
                }
                else
                {
                    MOTION_CORE_ERROR("Failed to import model");
                }

                ImGui::CloseCurrentPopup();
                m_ImportFuture = std::future<std::shared_ptr<Motion::ImportedResults>>{};
            }

            ImGui::EndPopup();
        }
    }

    void ScenePropertyPanel::RenderEntityHierarchy(ScenePanelContext& context)
    {
        const ImGuiTreeNodeFlags flags =
              ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_FramePadding;

        if (ImGui::TreeNodeEx("##entities", flags, ICON_FA_CUBE " Entities"))
        {
            context.ScenePointer->ForEachRootEntity([&](entt::registry& r, entt::entity e)
            {
                ImGui::PushID(static_cast<std::int32_t>(entt::to_integral(e)));
                const TagComponent* tagOpt = r.try_get<TagComponent>(e);
                const char* label = tagOpt ? tagOpt->Tag.c_str() : "Unnamed";

                if (ImGui::TreeNodeEx("##root-node", flags, ICON_FA_CUBE " %s", label))
                {       
                    if (tagOpt && tagOpt->IsActive)
                        RenderNodeEntities(context, r, e);
                    else
                        ImGui::TextDisabled(ICON_FA_INFO " Entity is inactive");

                    ImGui::TreePop();
                }

                ImGui::PopID();
            });

            ImGui::TreePop();
        }
    }

    void ScenePropertyPanel::RenderEnvironmentSettings(ScenePanelContext& ctx)
    {
        const ImGuiTreeNodeFlags flags =
              ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_FramePadding;

        if (ImGui::TreeNodeEx("##environment", flags, ICON_FA_SUN " Environment"))
        {
            ImGui::Indent();

            if (ImGui::CollapsingHeader(ICON_MD_LIGHTBULB " Light", flags))
            {
                auto& env = ctx.ScenePointer->GetEnvironment();
                BeginPropertyGrid("##sun-properties");

                DragFloat3("Direction", env.Sun.Direction);
                ColorEdit3("Color",     env.Sun.Color);
                DragFloat("Intensity",  &env.Sun.Intensity, 0.01f, 0.0f, 50.0f);
                ToggleSwitch("Show Direction", env.Sun.ShowDir);

                EndPropertyGrid();
            }

            if (ImGui::CollapsingHeader(ICON_FA_EARTH_ASIA " World", flags))
            {
                auto* world = ctx.WorldSettings;
                BeginPropertyGrid("##world-properties");

                std::string worldName = world->worldName.empty() ? "New World" : world->worldName;
                TextBox("World Name", worldName);

                glm::vec3 gravity = ToVec3(world->gravity);
                if (DragFloat3("Gravity", gravity, 0.01f, -50.0f, 50.0f))
                    world->gravity = ToVec3(gravity);

                float defaultRestitution = world->defaultBounciness;
                if (DragFloat("Default Restitution", &defaultRestitution, 0.01f, 0.0f, 1.0f))
                    world->defaultBounciness = defaultRestitution;

                float defaultFriction = world->defaultFrictionCoefficient;
                if (DragFloat("Default Friction", &defaultFriction, 0.01f, 0.0f, 1.0f))
                    world->defaultFrictionCoefficient = defaultFriction;

                ToggleSwitch("Allow Sleeping", world->isSleepingEnabled);

                EndPropertyGrid();

                if (ImGui::CollapsingHeader(ICON_FA_CALCULATOR " Advanced Settings", flags))
                {
                    GridSpec spec;
                    spec.twoColumns = true;
                    spec.labelWidth = 300.0f;

                    BeginPropertyGrid("##world-advanced-properties", spec);

                    float velIters = static_cast<float>(world->defaultVelocitySolverNbIterations);
                    if (DragFloat("Velocity Solver Iterations", &velIters, 1.0f, 1.0f, 100.0f))
                        world->defaultVelocitySolverNbIterations = static_cast<uint32_t>(velIters);

                    float posIters = static_cast<float>(world->defaultPositionSolverNbIterations);
                    if (DragFloat("Position Solver Iterations", &posIters, 1.0f, 1.0f, 100.0f))
                        world->defaultPositionSolverNbIterations = static_cast<uint32_t>(posIters);

                    float rvThresh = world->restitutionVelocityThreshold;
                    if (DragFloat("Restitution Velocity Threshold", &rvThresh, 0.01f, 0.0f, 10.0f))
                        world->restitutionVelocityThreshold = rvThresh;

                    float sleepLin = world->defaultSleepLinearVelocity;
                    if (DragFloat("Sleep Linear Velocity", &sleepLin, 0.01f, 0.0f, 10.0f))
                        world->defaultSleepLinearVelocity = sleepLin;

                    float sleepAng = world->defaultSleepAngularVelocity;
                    if (DragFloat("Sleep Angular Velocity", &sleepAng, 0.01f, 0.0f, 10.0f))
                        world->defaultSleepAngularVelocity = sleepAng;

                    float cosAngle = world->cosAngleSimilarContactManifold;
                    if (DragFloat("Angle Similar Contact Manifold", &cosAngle, 0.01f, 0.0f, 1.0f))
                        world->cosAngleSimilarContactManifold = std::clamp(cosAngle, 0.0f, 1.0f);

                    float tBeforeSleep = world->defaultTimeBeforeSleep;
                    if (DragFloat("Time Before Sleep", &tBeforeSleep, 0.01f, 0.0f, 10.0f))
                        world->defaultTimeBeforeSleep = tBeforeSleep;

                    EndPropertyGrid();
                }
            }

            ImGui::Unindent();
            ImGui::TreePop();
        }
    }

    void ScenePropertyPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::Begin(m_Title.c_str());
        {
            // Toolbar: search + import
            RenderToolbarAndSearch(context);

            // Import modal
            RenderImportPopup(context);

            // Entities
            RenderEntityHierarchy(context);

            // Environment
            RenderEnvironmentSettings(context);
        }
        ImGui::End();
    }
}
