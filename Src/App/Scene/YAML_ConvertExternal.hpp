#pragma once

#include "Texture.hpp"
#include "YAML_Converts.hpp"

namespace YAML
{
    template<> 
    struct convert<Motion::MaterialComponent> 
    {
        static Node encode(const Motion::MaterialComponent& rhs)
        {
            Node node;
            node["ID"] = rhs.ID;

            if (!rhs.MaterialPointer)
            {
                node["HasMaterial"] = false;
                return node;
            }

            node["HasMaterial"] = true;

            const Motion::ResolvedMaterials M = Motion::GetResolvedMaterials(rhs.MaterialPointer.get());

            Node prop = node["Core"]["Property"];
            prop["BaseColorFactor"]  = M.BaseColorFactor;
            prop["RoughnessFactor"]  = M.RoughnessFactor;
            prop["MetallicFactor"]   = M.MetallicFactor;
            prop["EmissiveFactor"]   = M.EmissiveFactor;
            prop["NormalScale"]      = M.NormalScale;
            prop["OpacityFactor"]    = M.OpacityFactor;
            prop["EmissiveStrength"] = M.EmissiveStrength;
            prop["AOStrength"]       = M.AOStrength;

            auto writeTex = [](const Node& parent, const char* key, const std::weak_ptr<Motion::ITexture>& w)
            {
                Node n = parent[key];
                if (auto t = w.lock())
                {
                    const auto& s = t->GetSpecification();
                    n["Valid"] = true;
                    n["Type"]  = Motion::GetTextureTypeString(s.Type);
                    n["File"]  = s.TextureFile;
                } 
                else 
                {
                    n["Valid"] = false; 
                    n["Type"]  = "None"; 
                    n["File"]  = "None";
                }
            };

            writeTex(node["Core"], "BaseTexture",      M.BaseColor);
            writeTex(node["Core"], "NormalTexture",    M.Normal);
            writeTex(node["Core"], "MetallicTexture",  M.Metallic);
            writeTex(node["Core"], "RoughnessTexture", M.Roughness);
            writeTex(node["Core"], "AOTexture",        M.AO);
            writeTex(node["Core"], "EmissiveTexture",  M.Emissive);
            writeTex(node["Core"], "OpacityTexture",   M.Opacity);
            writeTex(node["Packed"], "ORMTexture",     M.ORM);

            if (const auto& base = rhs.MaterialPointer->GetBaseMaterial())
            {
                node["Base"]["YAML"]                        = base->YamlFilePath;
                node["Base"]["Property"]["BaseColor"]       = base->BaseColor;
                node["Base"]["Property"]["MetallicFactor"]  = base->MetallicFactor;
                node["Base"]["Property"]["RoughnessFactor"] = base->RoughnessFactor;
                node["Base"]["Property"]["OpacityFactor"]   = base->OpacityFactor;
            }

            return node;
        }

        static bool decode(const Node& node, Motion::MaterialComponent& rhs)
        {
            rhs.ID = node["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            
            if (!node["HasMaterial"].as<bool>(true))
            {
                rhs.MaterialPointer = nullptr;
                return true;
            }

            if (!rhs.MaterialPointer) 
                rhs.MaterialPointer = Motion::Material::Create();

            auto N = [&](std::initializer_list<const char*> path) -> Node 
            {
                const Node* cur = &node;
                for (auto key : path)
                {
                    if(!cur->IsDefined()) return Node();
                    Node next = (*cur)[key];
                    if(!next.IsDefined()) return Node();
                    cur = &next;
                }
                return *cur;
            };

            auto as_vec3 = [](const Node& n, const glm::vec3& d) { return n.IsDefined() ? n.as<glm::vec3>(d) : d; };
            auto as_vec4 = [](const Node& n, const glm::vec4& d) { return n.IsDefined() ? n.as<glm::vec4>(d) : d; };
            auto as_f32  = [](const Node& n, float d) { return n.IsDefined() ? n.as<float>(d) : d; };

            auto loadTex = [&](const char* parentKey, const char* texKey) -> std::shared_ptr<Motion::ITexture> 
            {
                Node tex = N({parentKey, texKey});
                if(!tex.IsDefined()) return nullptr;

                bool valid = tex["Valid"].as<bool>(false);
                if(!valid) return nullptr;
                
                const std::string file = tex["File"].as<std::string>("");
                if(file.empty() || file == "None") return nullptr;

                if(!std::filesystem::exists(file))
                {
                    MOTION_CORE_WARN("Texture file not found: {}", file);
                    return nullptr;
                }

                const std::string typeName = tex["Type"].as<std::string>("");
                auto type = Motion::GetTextureTypeFromName(typeName);
                
                return Motion::ITexture::Create(file, type);
            };

            if (rhs.MaterialPointer->Has<Motion::CoreMaterialComponents>())
            {
                auto& core = rhs.MaterialPointer->Get<Motion::CoreMaterialComponents>();

                Node P = N({"Core","Property"});
                core.BaseColorFactor   = as_vec4(P["BaseColorFactor"],   glm::vec4(1.f));
                core.EmissiveFactor    = as_vec3(P["EmissiveFactor"],    glm::vec3(0.f));
                core.MetallicFactor    = as_f32 (P["MetallicFactor"],    0.f);
                core.RoughnessFactor   = as_f32 (P["RoughnessFactor"],   0.5f);
                core.OpacityFactor     = as_f32 (P["OpacityFactor"],     1.f);
                core.EmissiveStrength  = as_f32 (P["EmissiveStrength"],  1.f);
                core.OcclusionStrength = as_f32 (P["AOStrength"],        1.f);
                core.NormalScale       = as_f32 (P["NormalScale"],       1.f);

                core.BaseColorTexture  = loadTex("Core", "BaseTexture");
                core.NormalTexture     = loadTex("Core", "NormalTexture");
                core.MetallicTexture   = loadTex("Core", "MetallicTexture");
                core.RoughnessTexture  = loadTex("Core", "RoughnessTexture");
                core.OcclusionTexture  = loadTex("Core", "AOTexture");
                core.EmissiveTexture   = loadTex("Core", "EmissiveTexture");
            }

            if (Node B = node["Base"]; B.IsDefined())
            {
                const std::string yamlPath = B["YAML"].as<std::string>("");
                if(!yamlPath.empty() && std::filesystem::exists(yamlPath))
                {
                    if(auto base = Motion::Material::CreateBase(yamlPath))
                    {
                        Node BP = B["Property"];
                        base->BaseColor       = as_vec3(BP["BaseColor"],       base->BaseColor);
                        base->MetallicFactor  = as_f32 (BP["MetallicFactor"],  base->MetallicFactor);
                        base->RoughnessFactor = as_f32 (BP["RoughnessFactor"], base->RoughnessFactor);
                        base->OpacityFactor   = as_f32 (BP["OpacityFactor"],   base->OpacityFactor);

                        rhs.MaterialPointer->SetBaseMaterial(base);
                    }
                }
                else if(!yamlPath.empty())
                {
                    MOTION_CORE_WARN("Base material YAML not found: {}", yamlPath);
                }
            }

            return true;
        }
    };

    template<> 
    struct convert<Motion::ScenePhysicsWorld::WorldLighting> 
    {
        static Node encode(const Motion::ScenePhysicsWorld::WorldLighting& L)
        { 
            Node n; 
            n["SunDirection"]   = L.Direction; 
            n["SunIntensity"]   = L.Intensity; 
            n["SunColor"]       = L.Color; 
            n["SunShowDir"]     = L.ShowGuizmo; 
            return n; 
        }

        static bool decode(const Node& n, Motion::ScenePhysicsWorld::WorldLighting& L)
        { 
            if(!n.IsMap()) return false; 
            L.Direction  = n["SunDirection"].as<glm::vec3>(L.Direction); 
            L.Intensity  = n["SunIntensity"].as<float>(L.Intensity); 
            L.Color      = n["SunColor"].as<glm::vec3>(L.Color); 
            L.ShowGuizmo = n["SunShowDir"].as<bool>(L.ShowGuizmo);
            return true; 
        }
    };

    template<> 
    struct convert<rp3d::PhysicsWorld::WorldSettings> 
    {
        static Node encode(const rp3d::PhysicsWorld::WorldSettings& s)
        { 
            Node n; 
            n["WorldName"]                              = s.worldName; 
            n["WorldGravity"]                           = s.gravity; 
            n["DefaultRestitution"]                     = s.defaultBounciness; 
            n["DefaultFriction"]                        = s.defaultFrictionCoefficient; 
            n["SleepingEnabled"]                        = s.isSleepingEnabled; 
            n["DefaultVelocityIterations"]              = s.defaultVelocitySolverNbIterations; 
            n["DefaultPositionIterations"]              = s.defaultPositionSolverNbIterations; 
            n["PersistentContactDistanceThreshold"]     = s.persistentContactDistanceThreshold; 
            n["RestitutionVelocityThreshold"]           = s.restitutionVelocityThreshold; 
            n["DefaultTimeBeforeSleep"]                 = s.defaultTimeBeforeSleep; 
            n["DefaultSleepLinearVelocity"]             = s.defaultSleepLinearVelocity; 
            n["DefaultSleepAngularVelocity"]            = s.defaultSleepAngularVelocity; 
            n["CosAngleSimilarContactManifold"]         = s.cosAngleSimilarContactManifold; 
            return n; 
        }

        static bool decode(const Node& n, rp3d::PhysicsWorld::WorldSettings& s)
        { 
            if(!n.IsMap()) return false; 
            s.worldName                             = n["WorldName"].as<std::string>(s.worldName); 
            s.gravity                               = n["WorldGravity"].as<rp3d::Vector3>(s.gravity); 
            s.defaultBounciness                     = n["DefaultRestitution"].as<float>(s.defaultBounciness); 
            s.defaultFrictionCoefficient            = n["DefaultFriction"].as<float>(s.defaultFrictionCoefficient); 
            s.isSleepingEnabled                     = n["SleepingEnabled"].as<bool>(s.isSleepingEnabled); 
            s.defaultVelocitySolverNbIterations     = n["DefaultVelocityIterations"].as<std::uint32_t>(s.defaultVelocitySolverNbIterations); 
            s.defaultPositionSolverNbIterations     = n["DefaultPositionIterations"].as<std::uint32_t>(s.defaultPositionSolverNbIterations); 
            s.persistentContactDistanceThreshold    = n["PersistentContactDistanceThreshold"].as<float>(s.persistentContactDistanceThreshold); 
            s.restitutionVelocityThreshold          = n["RestitutionVelocityThreshold"].as<float>(s.restitutionVelocityThreshold); 
            s.defaultTimeBeforeSleep                = n["DefaultTimeBeforeSleep"].as<float>(s.defaultTimeBeforeSleep); 
            s.defaultSleepLinearVelocity            = n["DefaultSleepLinearVelocity"].as<float>(s.defaultSleepLinearVelocity); 
            s.defaultSleepAngularVelocity           = n["DefaultSleepAngularVelocity"].as<float>(s.defaultSleepAngularVelocity); 
            s.cosAngleSimilarContactManifold        = n["CosAngleSimilarContactManifold"].as<float>(s.cosAngleSimilarContactManifold); 
            return true; 
        }
    };

    template<>
    struct convert<Motion::SerializedNode>
    {
        static Node encode(const Motion::SerializedNode& node)
        {
            Node n;
            n["Tag"] = node.Tag;
            n["Transform"] = node.Transform;
            
            if (node.HasMesh)       n["Mesh"] = node.Mesh;
            if (node.HasMaterial)   n["Material"] = node.Material;
            if (node.HasRigidBody)  n["RigidBody"] = node.RigidBody;
            if (node.HasCollider)   n["Collider"] = node.Collider;
            
            return n;
        }

        static bool decode(const Node& n, Motion::SerializedNode& node)
        {
            if (!n.IsMap()) return false;

            node.Tag = n["Tag"].as<Motion::TagComponent>(Motion::TagComponent{});
            node.Transform = n["Transform"].as<Motion::TransformComponent>(Motion::TransformComponent{});
            
            if (n["Mesh"].IsDefined())
            {
                node.Mesh = n["Mesh"].as<Motion::MeshComponent>(Motion::MeshComponent{});
                node.HasMesh = true;
            }
            
            if (n["Material"].IsDefined())
            {
                node.Material = n["Material"].as<Motion::MaterialComponent>(Motion::MaterialComponent{});
                node.HasMaterial = true;
            }
            
            if (n["RigidBody"].IsDefined())
            {
                node.RigidBody = n["RigidBody"].as<Motion::RigidBodyComponent>(Motion::RigidBodyComponent{});
                node.HasRigidBody = true;
            }
            
            if (n["Collider"].IsDefined())
            {
                node.Collider = n["Collider"].as<Motion::ColliderComponent>(Motion::ColliderComponent{});
                node.HasCollider = true;
            }
            
            return true;
        }
    };

    template<>
    struct convert<Motion::SerializedEntity>
    {
        static Node encode(const Motion::SerializedEntity& entity)
        {
            Node n;
            n["Tag"] = entity.Tag;
            n["Transform"] = entity.Transform;
            
            if (entity.HasModel)
                n["Model"] = entity.Model;
            
            if (!entity.Nodes.empty())
            {
                Node nodes(NodeType::Sequence);
                for (const auto& node : entity.Nodes)
                    nodes.push_back(node);
                n["Nodes"] = nodes;
            }
            
            return n;
        }

        static bool decode(const Node& n, Motion::SerializedEntity& entity)
        {
            if (!n.IsMap()) return false;

            entity.Tag = n["Tag"].as<Motion::TagComponent>(Motion::TagComponent{});
            entity.Transform = n["Transform"].as<Motion::TransformComponent>(Motion::TransformComponent{});
            
            if (n["Model"].IsDefined())
            {
                entity.Model = n["Model"].as<Motion::ModelComponent>(Motion::ModelComponent{});
                entity.HasModel = true;
            }
            
            if (n["Nodes"].IsDefined() && n["Nodes"].IsSequence())
            {
                for (const auto& nodeYAML : n["Nodes"])
                {
                    auto node = nodeYAML.as<Motion::SerializedNode>(Motion::SerializedNode{});
                    entity.Nodes.push_back(node);
                }
            }
            
            return true;
        }
    };

    template<>
    struct convert<Motion::SerializedScene>
    {
        static Node encode(const Motion::SerializedScene& scene)
        {
            Node root;
            Node sceneNode;
            
            sceneNode["ID"] = scene.SceneID;
            sceneNode["Name"] = scene.Name;
            sceneNode["SavedPath"] = scene.SavedPath;
            sceneNode["Lighting"] = scene.Lighting;
            sceneNode["WorldSettings"] = scene.PhysicsSettings;
            
            Node entities(NodeType::Sequence);
            for (const auto& entity : scene.Entities)
                entities.push_back(entity);
            
            sceneNode["Entities"] = entities;
            root["Scene"] = sceneNode;
            
            return root;
        }

        static bool decode(const Node& root, Motion::SerializedScene& scene)
        {
            if (!root["Scene"].IsDefined())
                return false;

            const Node& sceneNode = root["Scene"];
            
            scene.SceneID = sceneNode["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            scene.Name = sceneNode["Name"].as<std::string>("Untitled");
            scene.SavedPath = sceneNode["SavedPath"].as<std::filesystem::path>(std::filesystem::path{});
            scene.Lighting = sceneNode["Lighting"].as<Motion::ScenePhysicsWorld::WorldLighting>(Motion::ScenePhysicsWorld::WorldLighting{});
            scene.PhysicsSettings = sceneNode["WorldSettings"].as<rp3d::PhysicsWorld::WorldSettings>(rp3d::PhysicsWorld::WorldSettings{});
            
            if (sceneNode["Entities"].IsDefined() && sceneNode["Entities"].IsSequence())
            {
                for (const auto& entityYAML : sceneNode["Entities"])
                {
                    auto entity = entityYAML.as<Motion::SerializedEntity>(Motion::SerializedEntity{});
                    scene.Entities.push_back(entity);
                }
            }
            
            return true;
        }
    };

}