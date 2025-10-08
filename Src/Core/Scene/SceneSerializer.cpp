#include "CorePCH.hpp"
#include "SceneSerializer.hpp"

namespace YAML
{

    template<>
    struct convert<glm::vec2> 
    {
        static Node encode(const glm::vec2& rhs) 
        {
            Node node;
            node["x"] = rhs.x;
            node["y"] = rhs.y;
            return node;
        }

        static bool decode(const Node& node, glm::vec2& rhs) 
        {
            rhs.x = node["x"].as<float>(0.0f);
            rhs.y = node["y"].as<float>(0.0f);
            return true;
        }
    };

    template<>
    struct convert<glm::vec3> 
    {
        static Node encode(const glm::vec3& rhs) 
        {
            Node node;
            node["x"] = rhs.x;
            node["y"] = rhs.y;
            node["z"] = rhs.z;
            return node;
        }

        static bool decode(const Node& node, glm::vec3& rhs) 
        {
            rhs.x = node["x"].as<float>(0.0f);
            rhs.y = node["y"].as<float>(0.0f);
            rhs.z = node["z"].as<float>(0.0f);
            return true;
        }
    };

    template<>
    struct convert<glm::vec4> 
    {
        static Node encode(const glm::vec4& rhs) 
        {
            Node node;
            node["x"] = rhs.x;
            node["y"] = rhs.y;
            node["z"] = rhs.z;
            node["w"] = rhs.w;
            return node;
        }

        static bool decode(const Node& node, glm::vec4& rhs) 
        {
            rhs.x = node["x"].as<float>(0.0f);
            rhs.y = node["y"].as<float>(0.0f);
            rhs.z = node["z"].as<float>(0.0f);
            rhs.w = node["w"].as<float>(0.0f);
            return true;
        }
    };

    template<>
    struct convert<glm::quat> 
    {
        static Node encode(const glm::quat& rhs) 
        {
            Node node;
            node["w"] = rhs.w;
            node["x"] = rhs.x;
            node["y"] = rhs.y;
            node["z"] = rhs.z;
            return node;
        }

        static bool decode(const Node& node, glm::quat& rhs) 
        {
            rhs.w = node["w"].as<float>(1.0f);
            rhs.x = node["x"].as<float>(0.0f);
            rhs.y = node["y"].as<float>(0.0f);
            rhs.z = node["z"].as<float>(0.0f);
            return true;
        }

    };

    template<>
    struct convert<rp3d::Vector2> 
    {
        static Node encode(const rp3d::Vector2& rhs) 
        {
            Node node;
            node["x"] = rhs.x;
            node["y"] = rhs.y;
            return node;
        }

        static bool decode(const Node& node, rp3d::Vector2& rhs) 
        {
            rhs.x = node["x"].as<float>(0.0f);
            rhs.y = node["y"].as<float>(0.0f);
            return true;
        }
    };

    template<>
    struct convert<rp3d::Vector3> 
    {
        static Node encode(const rp3d::Vector3& rhs) 
        {
            Node node;
            node["x"] = rhs.x;
            node["y"] = rhs.y;
            node["z"] = rhs.z;
            return node;
        }

        static bool decode(const Node& node, rp3d::Vector3& rhs) 
        {
            rhs.x = node["x"].as<float>(0.0f);
            rhs.y = node["y"].as<float>(0.0f);
            rhs.z = node["z"].as<float>(0.0f);
            return true;
        }
    };

    template<>
    struct convert<rp3d::Quaternion> 
    {
        static Node encode(const rp3d::Quaternion& rhs) 
        {
            Node node;
            node["w"] = rhs.w;
            node["x"] = rhs.x;
            node["y"] = rhs.y;
            node["z"] = rhs.z;
            return node;
        }

        static bool decode(const Node& node, rp3d::Quaternion& rhs) 
        {
            rhs.w = node["w"].as<float>(1.0f);
            rhs.x = node["x"].as<float>(0.0f);
            rhs.y = node["y"].as<float>(0.0f);
            rhs.z = node["z"].as<float>(0.0f);
            return true;
        }

    };

    template<>
    struct convert<std::filesystem::path>
    {
        static Node encode(const std::filesystem::path& rhs)
        {
            Node node;
            node = std::filesystem::absolute(rhs).string();
            return node;
        }

        static bool decode(const Node& node, std::filesystem::path& rhs)
        {
            rhs = node.as<std::string>();
            return true;
        }
    };

    template<>
    struct convert<Motion::TagComponent>
    {
        static Node encode(const Motion::TagComponent& rhs)
        {
            Node node;
            node["ID"]          = rhs.ID;
            node["Tag"]         = rhs.Tag;
            node["IsActive"]    = rhs.IsActive;
            return node;
        }

        static bool decode(const Node& node, Motion::TagComponent& rhs)
        {
            rhs.ID          = node["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            rhs.Tag         = node["Tag"].as<std::string>("Undefined");
            rhs.IsActive    = node["IsActive"].as<bool>(true);
            return true;
        }
    };

    template<>
    struct convert<Motion::TransformComponent>
    {
        static Node encode(const Motion::TransformComponent& rhs)
        {
            Node node;
            node["ID"]          = rhs.ID;
            node["Translation"] = rhs.Translation;
            node["Rotation"]    = rhs.Rotation;
            node["Scale"]       = rhs.Scale;
            return node;
        }

        static bool decode(const Node& node, Motion::TransformComponent& rhs)
        {
            rhs.ID          = node["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            rhs.Translation = node["Translation"].as<glm::vec3>(glm::vec3(0.0f));
            rhs.Rotation    = node["Rotation"].as<glm::quat>(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            rhs.Scale       = node["Scale"].as<glm::vec3>(glm::vec3(1.0f));
            return true;
        }
    };

    template<>
    struct convert<Motion::RigidBodyComponent>
    {
        static Node encode(const Motion::RigidBodyComponent& rhs)
        {
            auto getBodyString = [](Motion::BodyType type) -> std::string
            {
                switch (type)
                {
                    case Motion::BodyType::Static: return "Static";
                    case Motion::BodyType::Dynamic: return "Dynamic";
                    default: return "Undefined";
                };
            };

            Node node;
            node["ID"]              = rhs.ID;
            node["BodyType"]        = getBodyString(rhs.Type);
            node["LinearDamping"]   = rhs.LinearDamping;
            node["AngularDamping"]  = rhs.AngularDamping;
            node["LockX"]           = rhs.LockX;
            node["LockY"]           = rhs.LockY;
            node["LockZ"]           = rhs.LockZ;
            node["LockRotX"]        = rhs.LockRotX;
            node["LockRotY"]        = rhs.LockRotY;
            node["LockRotZ"]        = rhs.LockRotZ;
            return node;
        }

        static bool decode(const Node& node, Motion::RigidBodyComponent& rhs)
        {
            auto getBodyType = [](const std::string& type) -> Motion::BodyType
            {
                if (type == "Static")   return Motion::BodyType::Static;
                if (type == "Dynamic")  return Motion::BodyType::Dynamic;
                return Motion::BodyType::Dynamic;
            };

            rhs.ID              = node["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            rhs.Type            = getBodyType(node["BodyType"].as<std::string>("Dynamic"));
            rhs.LinearDamping   = node["LinearDamping"].as<float>(0.2f);
            rhs.AngularDamping  = node["AngularDamping"].as<float>(0.05f);
            rhs.LockX           = node["LockX"].as<bool>(false);
            rhs.LockY           = node["LockY"].as<bool>(false);
            rhs.LockZ           = node["LockZ"].as<bool>(false);
            rhs.LockRotX        = node["LockRotX"].as<bool>(false);
            rhs.LockRotY        = node["LockRotY"].as<bool>(false);
            rhs.LockRotZ        = node["LockRotZ"].as<bool>(false);
            return true;
        }
    };

    template<>
    struct convert<Motion::ColliderComponent>
    {
        static Node encode(const Motion::ColliderComponent& rhs)
        {
            auto getShapeString = [](Motion::ShapeType type) -> std::string
            {
                switch (type)
                {
                    case Motion::ShapeType::Box:        return "Box";
                    case Motion::ShapeType::Sphere:     return "Sphere";
                    case Motion::ShapeType::Capsule:    return "Capsule";
                    case Motion::ShapeType::Convex:     return "Convex";
                    case Motion::ShapeType::Concave:    return "Concave";
                    case Motion::ShapeType::HightField: return "HightField";
                    default: return "Undefined";
                };
            };

            Node node;
            node["ID"]                  = rhs.ID;
            node["Shape"]               = getShapeString(rhs.Type);
            node["BoxHalfExtents"]      = rhs.BoxHalfExtents;
            node["SphereRadius"]        = rhs.SphereRadius;
            node["CapsuleRadius"]       = rhs.Capsule.Radius;
            node["CapsuleHeight"]       = rhs.Capsule.Height;
            node["CapsuleAxis"]         = rhs.Capsule.Axis;
            node["Friction"]            = rhs.Friction;
            node["Restitution"]         = rhs.Restitution;
            node["MassDensity"]         = rhs.MassDensity;
            node["LastAppliedScale"]    = rhs.LastAppliedScale;
            node["LocalTransform"]      = rhs.LocalTransform;
            node["LocalRotation"]       = rhs.LocalRotation;
            return node;
        }

        static bool decode(const Node& node, Motion::ColliderComponent& rhs)
        {
            auto getShapeType = [](const std::string& type) -> Motion::ShapeType
            {
                if (type == "Box")          return Motion::ShapeType::Box;
                if (type == "Sphere")       return Motion::ShapeType::Sphere;
                if (type == "Capsule")      return Motion::ShapeType::Capsule;
                if (type == "Convex")       return Motion::ShapeType::Convex;
                if (type == "Concave")      return Motion::ShapeType::Concave;
                if (type == "HightField")   return Motion::ShapeType::HightField;
                return Motion::ShapeType::Box;
            };

            rhs.ID                  = node["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            rhs.Type                = getShapeType(node["Shape"].as<std::string>("Box"));
            rhs.BoxHalfExtents      = node["BoxHalfExtents"].as<glm::vec3>(glm::vec3(0.5f, 0.5f, 0.5f));
            rhs.SphereRadius        = node["SphereRadius"].as<float>(0.5f);
            rhs.Capsule.Radius      = node["CapsuleRadius"].as<float>(0.5f);
            rhs.Capsule.Height      = node["CapsuleHeight"].as<float>(1.0f);
            rhs.Capsule.Axis        = node["CapsuleAxis"].as<std::int32_t>(1);
            rhs.Friction            = node["Friction"].as<float>(0.5f);
            rhs.Restitution         = node["Restitution"].as<float>(0.2f);
            rhs.MassDensity         = node["MassDensity"].as<float>(500.0f);
            rhs.LastAppliedScale    = node["LastAppliedScale"].as<glm::vec3>(glm::vec3(1.0f, 1.0f, 1.0f));
            rhs.LocalTransform      = node["LocalTransform"].as<glm::vec3>(glm::vec3(0.0f, 0.0f, 0.0f));
            rhs.LocalRotation       = node["LocalRotation"].as<glm::quat>(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            return true;
        }
    };

    template<>
    struct convert<Motion::MeshComponent>
    {
        static Node encode(const Motion::MeshComponent& rhs)
        {
            Node node;
            node["ID"]          = rhs.ID;
            node["Name"]        = rhs.Name;
            node["MinBounds"]   = rhs.MinBounds;
            node["MaxBounds"]   = rhs.MaxBounds;
            return node;
        }

        static bool decode(const Node& node, Motion::MeshComponent& rhs)
        {
            rhs.ID          = node["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            rhs.MinBounds   = node["MinBounds"].as<glm::vec3>(glm::vec3(0.0f, 0.0f, 0.0f));
            rhs.MaxBounds   = node["MaxBounds"].as<glm::vec3>(glm::vec3(0.0f, 0.0f, 0.0f));
            return true;
        }
    };

    template<>
    struct convert<Motion::ModelComponent>
    {
        static Node encode(const Motion::ModelComponent& rhs)
        {
            Node node;
            node["ID"]              = rhs.ID;
            node["FilePath"]        = rhs.FilePath;
            node["MeshCount"]       = rhs.MeshCount;
            node["MinBounds"]       = rhs.MinBounds;
            node["MaxBounds"]       = rhs.MaxBounds;
            return node;
        }

        static bool decode(const Node& node, Motion::ModelComponent& rhs)
        {
            rhs.ID              = node["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            rhs.FilePath        = node["FilePath"].as<std::filesystem::path>(std::filesystem::path(""));
            rhs.MeshCount       = node["MeshCount"].as<std::uint32_t>(0);
            rhs.MinBounds       = node["MinBounds"].as<glm::vec3>(glm::vec3(0.0f, 0.0f, 0.0f));
            rhs.MaxBounds       = node["MaxBounds"].as<glm::vec3>(glm::vec3(0.0f, 0.0f, 0.0f));
            return true;
        }
    };

    template<>
    struct convert<Motion::MaterialComponent>
    {
        static Node encode(const Motion::MaterialComponent& rhs)
        {
            Node node;
            node["ID"] = rhs.ID;

            if (!rhs.MaterialPointer) 
            {
                auto noneTex = [&](Node parent, const char* key) 
                {
                    parent[key]["Name"] = "None";
                    parent[key]["Type"] = "None";
                    parent[key]["File"] = "None";
                };

                noneTex(node["Core"],   "BaseTexture");
                noneTex(node["Core"],   "NormalTexture");
                noneTex(node["Core"],   "MetallicTexture");
                noneTex(node["Core"],   "RoughnessTexture");
                noneTex(node["Core"],   "AOTexture");
                noneTex(node["Core"],   "EmissiveTexture");
                noneTex(node["Packed"], "OpacityTexture");
                noneTex(node["Packed"], "ORMTexture");

                return node;
            }

            const Motion::ResolvedMaterials materials = Motion::GetResolvedMaterials(rhs.MaterialPointer.get());
            {
                Node prop = node["Core"]["Property"];
                prop["BaseColorFactor"]  = materials.BaseColorFactor;
                prop["RoughnessFactor"]  = materials.RoughnessFactor;
                prop["MetallicFactor"]   = materials.MetallicFactor;
                prop["EmissiveFactor"]   = materials.EmissiveFactor;
                prop["NormalScale"]      = materials.NormalScale;
                prop["OpacityFactor"]    = materials.OpacityFactor;
                prop["EmissiveStrength"] = materials.EmissiveStrength;
                prop["AOStrength"]       = materials.AOStrength;
            }

            auto writeTexture = [](Node parent, const char* key, const std::weak_ptr<Motion::ITexture>& wtex)
            {
                Node n = parent[key];
                if (auto tex = wtex.lock()) 
                {
                    const auto& spec = tex->GetSpecification();
                    n["Valid"] = "true";
                    n["Type"] = Motion::GetTextureTypeString(spec.Type);
                    n["File"] = spec.TextureFile;
                } 
                else 
                {
                    n["Valid"] = "false";
                    n["Type"] = "None";
                    n["File"] = "None";
                }
            };

            writeTexture(node["Core"], "BaseTexture",      materials.BaseColor);
            writeTexture(node["Core"], "NormalTexture",    materials.Normal);
            writeTexture(node["Core"], "MetallicTexture",  materials.Metallic);
            writeTexture(node["Core"], "RoughnessTexture", materials.Roughness);
            writeTexture(node["Core"], "AOTexture",        materials.AO);
            writeTexture(node["Core"], "EmissiveTexture",  materials.Emissive);
            writeTexture(node["Core"], "OpacityTexture",   materials.Opacity);

            writeTexture(node["Packed"], "ORMTexture",     materials.ORM);

            if (const auto& baseMaterial = rhs.MaterialPointer->GetBaseMaterial()) 
            {
                node["Base"]["YAML"]                        = baseMaterial->YamlFilePath;
                node["Base"]["Property"]["BaseColor"]       = baseMaterial->BaseColor;
                node["Base"]["Property"]["MetallicFactor"]  = baseMaterial->MetallicFactor;
                node["Base"]["Property"]["RoughnessFactor"] = baseMaterial->RoughnessFactor;
                node["Base"]["Property"]["OpacityFactor"]   = baseMaterial->OpacityFactor;
            }

            return node;
        }


        static bool decode(const Node& node, Motion::MaterialComponent& rhs)
        {
            rhs.ID = node["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            if (!rhs.MaterialPointer) rhs.MaterialPointer = Motion::Material::Create();

            auto N = [&](const std::initializer_list<const char*> path) -> Node 
            {
                const Node* cur = &node;
                for (auto key : path) {
                    if (!cur->IsDefined()) return Node();
                    Node next = (*cur)[key];
                    if (!next.IsDefined()) return Node();
                    cur = &next;
                }
                return *cur;
            };

            auto as_vec3 = [](const Node& n, const glm::vec3& d) 
            {
                return n.IsDefined() ? n.as<glm::vec3>(d) : d;
            };
            auto as_vec4 = [](const Node& n, const glm::vec4& d) 
            {
                return n.IsDefined() ? n.as<glm::vec4>(d) : d;
            };

            auto as_f32 = [](const Node& n, float d)
            {
                return n.IsDefined() ? n.as<float>(d) : d;
            };

            auto loadTex = [&](const char* parentKey, const char* texKey) -> std::shared_ptr<Motion::ITexture> 
            {
                Node texN = N({parentKey, texKey});
                if (!texN.IsDefined()) return nullptr;
                bool validFlag = false;
                if (Node v = texN["Valid"]; v.IsDefined()) 
                {
                    if (v.IsScalar()) 
                    {
                        validFlag = v.as<bool>(false);
                    }
                } 
                else 
                {
                    const std::string name = texN["Name"].as<std::string>("");
                    const std::string file = texN["File"].as<std::string>("");
                    validFlag = (!file.empty()) || (!name.empty() && name != "None");
                }

                if (!validFlag) return nullptr;

                std::string file = texN["File"].as<std::string>("");
                std::string typeName = texN["Type"].as<std::string>("");
                if (file.empty()) return nullptr;

                auto type = Motion::GetTextureTypeFromName(typeName);
                return Motion::ITexture::Create(file, type);
            };

            if (rhs.MaterialPointer->Has<Motion::CoreMaterialComponents>()) 
            {
                auto& core = rhs.MaterialPointer->Get<Motion::CoreMaterialComponents>();
                Node P = N({"Core", "Property"});

                core.BaseColorFactor   = as_vec4(P["BaseColorFactor"], glm::vec4(1.0f));
                core.EmissiveFactor    = as_vec3(P["EmissiveFactor"], glm::vec3(0.0f));
                core.MetallicFactor    = as_f32 (P["MetallicFactor"], 0.0f);
                core.RoughnessFactor   = as_f32 (P["RoughnessFactor"], 0.5f);
                core.OpacityFactor     = as_f32 (P["OpacityFactor"], 1.0f);
                core.EmissiveStrength  = as_f32 (P["EmissiveStrength"], 1.0f);
                core.OcclusionStrength = as_f32 (P["AOStrength"], 1.0f);
                core.NormalScale       = as_f32 (P["NormalScale"], 1.0f);

                core.BaseColorTexture = loadTex("Core", "BaseTexture");
                core.NormalTexture    = loadTex("Core", "NormalTexture");
                core.MetallicTexture  = loadTex("Core", "MetallicTexture");
                core.RoughnessTexture = loadTex("Core", "RoughnessTexture");
                core.OcclusionTexture = loadTex("Core", "AOTexture");
                core.EmissiveTexture  = loadTex("Core", "EmissiveTexture");
            }

            if (Node B = node["Base"]; B.IsDefined()) 
            {
                const std::string yamlPath = B["YAML"].as<std::string>("");
                if (!yamlPath.empty()) 
                {
                    std::shared_ptr<Motion::BaseMaterial> base =
                        Motion::Material::CreateBase(yamlPath);
                    if (base) 
                    {
                        Node BP = B["Property"];
                        base->BaseColor      = as_vec3(BP["BaseColor"],      base->BaseColor);
                        base->MetallicFactor = as_f32 (BP["MetallicFactor"], base->MetallicFactor);
                        base->RoughnessFactor= as_f32 (BP["RoughnessFactor"],base->RoughnessFactor);
                        base->OpacityFactor  = as_f32 (BP["OpacityFactor"],  base->OpacityFactor);
                        rhs.MaterialPointer->SetBaseMaterial(base);
                    }
                }
            }

            return true;
        }

    };

    template<>
    struct convert<Motion::SceneEnvironment>
    {
        static Node encode(const Motion::SceneEnvironment& rhs)
        {
            Node node;
            node["SunDirection"]    = rhs.Sun.Direction;
            node["SunIntensity"]    = rhs.Sun.Intensity;
            node["SunColor"]        = rhs.Sun.Color;
            node["SunCastShadow"]   = rhs.Sun.CastShadow;
            node["SunShowDir"]      = rhs.Sun.ShowDir;
            return node;
        }

        static bool decode(const Node& node, Motion::SceneEnvironment& rhs)
        {
            rhs.Sun.Direction   = node["SunDirection"].as<glm::vec3>(rhs.Sun.Direction);
            rhs.Sun.Intensity   = node["SunIntensity"].as<float>(rhs.Sun.Intensity);
            rhs.Sun.Color       = node["SunColor"].as<glm::vec3>(rhs.Sun.Color);
            rhs.Sun.CastShadow  = node["SunCastShadow"].as<bool>(rhs.Sun.CastShadow);
            rhs.Sun.ShowDir     = node["SunShowDir"].as<bool>(rhs.Sun.ShowDir);
            return true;
        }
    };

    template<>
    struct convert<rp3d::PhysicsWorld::WorldSettings>
    {
        static Node encode(const rp3d::PhysicsWorld::WorldSettings& rhs)
        {
            Node node;
            node["WorldName"]                           = rhs.worldName;
            node["WorldGravity"]                        = rhs.gravity;
            node["DefaultRestitution"]                  = rhs.defaultBounciness;
            node["DefaultFriction"]                     = rhs.defaultFrictionCoefficient;
            node["SleepingEnabled"]                     = rhs.isSleepingEnabled;
            node["DefaultVelocityIterations"]           = rhs.defaultVelocitySolverNbIterations;
            node["DefaultPositionIterations"]           = rhs.defaultPositionSolverNbIterations;
            node["PersistentContactDistanceThreshold"]  = rhs.persistentContactDistanceThreshold;
            node["RestitutionVelocityThreshold"]        = rhs.restitutionVelocityThreshold;
            node["DefaultTimeBeforeSleep"]              = rhs.defaultTimeBeforeSleep;
            node["DefaultSleepLinearVelocity"]          = rhs.defaultSleepLinearVelocity;
            node["DefaultSleepAngularVelocity"]         = rhs.defaultSleepAngularVelocity;
            node["CosAngleSimilarContactManifold"]      = rhs.cosAngleSimilarContactManifold;
            return node;
        }

        static bool decode(const Node& node, rp3d::PhysicsWorld::WorldSettings& rhs)
        {
            rhs.worldName                           = node["WorldName"].as<std::string>(rhs.worldName);
            rhs.gravity                             = node["WorldGravity"].as<rp3d::Vector3>(rhs.gravity);
            rhs.defaultBounciness                   = node["DefaultRestitution"].as<float>(rhs.defaultBounciness);
            rhs.defaultFrictionCoefficient          = node["DefaultFriction"].as<float>(rhs.defaultFrictionCoefficient);
            rhs.isSleepingEnabled                   = node["SleepingEnabled"].as<bool>(rhs.isSleepingEnabled);
            rhs.defaultVelocitySolverNbIterations   = node["DefaultVelocityIterations"].as<std::uint32_t>(rhs.defaultVelocitySolverNbIterations);
            rhs.defaultPositionSolverNbIterations   = node["DefaultPositionIterations"].as<std::uint32_t>(rhs.defaultPositionSolverNbIterations);
            rhs.persistentContactDistanceThreshold  = node["PersistentContactDistanceThreshold"].as<float>(rhs.persistentContactDistanceThreshold);
            rhs.restitutionVelocityThreshold        = node["RestitutionVelocityThreshold"].as<float>(rhs.restitutionVelocityThreshold);
            rhs.defaultTimeBeforeSleep              = node["DefaultTimeBeforeSleep"].as<float>(rhs.defaultTimeBeforeSleep);
            rhs.defaultSleepLinearVelocity          = node["DefaultSleepLinearVelocity"].as<float>(rhs.defaultSleepLinearVelocity);
            rhs.defaultSleepAngularVelocity         = node["DefaultSleepAngularVelocity"].as<float>(rhs.defaultSleepAngularVelocity);
            rhs.cosAngleSimilarContactManifold      = node["CosAngleSimilarContactManifold"].as<float>(rhs.cosAngleSimilarContactManifold);
            return true;
        }
    };
}

namespace Motion
{
    bool SceneSerializer::Serialize(Scene* scene, const std::filesystem::path& path)
    {
        if(!scene) return false;
        if(path.empty()) return false;

        YAML::Node root;

        YAML::Node sceneNode;
        sceneNode["ID"]             = scene->m_SceneID;
        sceneNode["Name"]           = scene->m_SceneName;
        sceneNode["Environment"]    = scene->m_Environment;
        sceneNode["PhysicsWorld"]   = scene->m_PhySettings;

        if(scene->GetEntityCount())
        {
            YAML::Node entities(YAML::NodeType::Sequence);
            scene->ForEachRootEntity([&](entt::registry& registry, entt::entity entity)
            {
                if(registry.valid(entity) && scene->IsRootEntity(entity))
                {
                    const auto* tag = registry.try_get<TagComponent>(entity);
                        entities["Entity"]["Tag"] = *tag;

                    if(const auto* model = registry.try_get<ModelComponent>(entity))
                        entities["Entity"]["Model"] = *model;

                    if(const auto* transform = registry.try_get<TransformComponent>(entity))
                        entities["Entity"]["Transform"] = *transform;

                    scene->ForEachNodeEntity(entity, [&](entt::registry& r, entt::entity e)
                    {
                        if(const auto* tag = r.try_get<TagComponent>(e))
                            entities["Entity"]["NodeEntity"]["Tag"] = *tag;

                        if(const auto* transform = r.try_get<TransformComponent>(e))
                            entities["Entity"]["NodeEntity"]["Transform"] = *transform;

                        if(const auto* model = r.try_get<ModelComponent>(e))
                            entities["Entity"]["NodeEntity"]["Model"] = *model;

                        if(const auto* mesh = r.try_get<MeshComponent>(e))
                            entities["Entity"]["NodeEntity"]["Mesh"] = *mesh;

                        if(const auto* material = r.try_get<MaterialComponent>(e))
                            entities["Entity"]["NodeEntity"]["Material"] = *material;

                        if(const auto* rigidBody = r.try_get<RigidBodyComponent>(e))
                            entities["Entity"]["NodeEntity"]["RigidBody"] = *rigidBody;

                        if(const auto* collider = r.try_get<ColliderComponent>(e))
                            entities["Entity"]["NodeEntity"]["Collider"] = *collider;
                    });
                }

                sceneNode["Entities"] = entities;
            });
            
        }

        root["Scene"] = sceneNode;
        
        try
        {
            std::filesystem::path dir = std::filesystem::absolute(path);
            std::ofstream file(dir, std::ios::out | std::ios::trunc);
            file << root << std::endl;
            file.close();  
        }
        catch(const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Failed to serialize scene: {}", e.what());
            return false;
        }

        return true;
    }


    std::shared_ptr<Scene> SceneSerializer::Deserialize(const std::filesystem::path& path)
    {
        try
        {
            if(!std::filesystem::exists(path)) return nullptr;

            YAML::Node root         = YAML::LoadFile(path.string());
            const auto& sceneNode   = root["Scene"];

            UUID sceneID                    = sceneNode["ID"].as<UUID>(UniqueIdentity::GetUniqueID());
            std::string sceneName           = sceneNode["Name"].as<std::string>("Untitled");
            std::shared_ptr<Scene> scene    = std::make_shared<Scene>(sceneID, sceneName);
            if(!scene) return nullptr;

            const SceneEnvironment environment                              = sceneNode["Environment"].as<SceneEnvironment>(SceneEnvironment{});
            const rp3d::PhysicsWorld::WorldSettings physicsWorldSettings    = sceneNode["PhysicsWorld"].as<rp3d::PhysicsWorld::WorldSettings>(rp3d::PhysicsWorld::WorldSettings{});

            auto& sceneEnvironment          = scene->GetEnvironment();
            auto& scenePhysicsWorldSettings = scene->GetPhysicsWorldSettings();

            sceneEnvironment.Sun        = environment.Sun;
            scenePhysicsWorldSettings   = physicsWorldSettings;

            if(sceneNode["Entities"].IsDefined())
            {
                const auto& entities = root["Entities"];
    
                struct NodeEntity
                {
                    TagComponent tag;
                    TransformComponent transform;
                    MeshComponent mesh;
                    MaterialComponent material;
                    RigidBodyComponent rigidBody;
                    ColliderComponent collider;
                };

                struct Entity
                {
                    TagComponent tag;
                    ModelComponent model;
                    TransformComponent transform;
                    std::unordered_map<std::string, NodeEntity> nodeEntities;
                };
    
                std::vector<Entity> entitiesData;

                for(const auto& entity : entities)
                {
                    const auto& currentEntity = entity["Entity"];

                    Entity e;
                    e.tag = currentEntity["Tag"].as<TagComponent>(TagComponent{});
                    e.model = currentEntity["Model"].as<ModelComponent>(ModelComponent{});
                    e.transform = currentEntity["Transform"].as<TransformComponent>(TransformComponent{});

                    entitiesData.push_back(e);

                    if(currentEntity["NodeEntity"].IsDefined())
                    {
                        for(const auto& nodeEntity : currentEntity["NodeEntity"])
                        {
                            NodeEntity ne;
                            ne.tag          = nodeEntity["Tag"].as<TagComponent>(TagComponent{});
                            ne.transform    = nodeEntity["Transform"].as<TransformComponent>(TransformComponent{});
                            ne.mesh         = nodeEntity["Mesh"].as<MeshComponent>(MeshComponent{});
                            ne.material     = nodeEntity["Material"].as<MaterialComponent>(MaterialComponent{});
                            ne.rigidBody    = nodeEntity["RigidBody"].as<RigidBodyComponent>(RigidBodyComponent{});
                            ne.collider     = nodeEntity["Collider"].as<ColliderComponent>(ColliderComponent{});
                            e.nodeEntities[ne.tag.Tag] = ne;
                        }
                    }   
                }

                const BufferLayout layout
                {
                    { "a_Position",   BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Position)    },
                    { "a_TexCoords",  BufferComponents::UV,   BufferStride::F2, false, offsetof(Vertex, TexCoord)    },
                    { "a_Normals",    BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Normal)      },
                    { "a_Tangents",   BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)     },
                    { "a_Bitangents", BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Bitangent)   },
                };

                for(auto& entity : entitiesData)
                {
                    if(entity.model.FilePath.empty()) continue;
                    std::shared_ptr<ImportedResults> results = Importer::ImportModelAsync(entity.model.FilePath, false, "default");
                    if(!results)
                    {
                        MOTION_CORE_ERROR("Failed to import model: {}", entity.model.FilePath.string());
                        continue;
                    }

                    if(entity.model.MeshCount != results->MeshCount)
                    {
                        MOTION_CORE_ERROR("Failed to import model: {} - mesh count mismatch", entity.model.FilePath.string());
                        continue;
                    }

                    entt::entity root = scene->m_SceneRegistry.create();

                    auto& tag   = scene->m_SceneRegistry.emplace<TagComponent>(root);
                    tag.Tag     = entity.tag.Tag;

                    auto& transform = scene->m_SceneRegistry.emplace<TransformComponent>(root);
                    transform       = entity.transform;

                    auto& model     = scene->m_SceneRegistry.emplace<ModelComponent>(root);
                    model.FilePath  = entity.model.FilePath;
                    model.MeshCount = entity.model.MeshCount;
                    model.MinBounds = entity.model.MinBounds;
                    model.MaxBounds = entity.model.MaxBounds;

                    std::vector<entt::entity> children;
                    children.reserve(entity.model.MeshCount);

                    for(const auto& [_, mesh] : results->Meshes)
                    {
                        if(entity.nodeEntities.contains(mesh.Name))
                        {
                            auto& nodeEntity = entity.nodeEntities.at(mesh.Name);

                            entt::entity e = scene->m_SceneRegistry.create();

                            auto& tag   = scene->m_SceneRegistry.emplace<TagComponent>(e);
                            tag.Tag     = nodeEntity.tag.Tag;

                            auto& transform = scene->m_SceneRegistry.emplace<TransformComponent>(e);
                            transform       = nodeEntity.transform;

                            auto& meshComponent         = scene->m_SceneRegistry.emplace<MeshComponent>(e);
                            meshComponent.ID            = nodeEntity.mesh.ID;
                            meshComponent.MeshPointer   = Mesh::Create(mesh.Vertices.data(), mesh.Vertices.size(),
                                                             mesh.Indices.data(), mesh.Indices.size(), layout);

                            auto& material  = scene->m_SceneRegistry.emplace<MaterialComponent>(e);
                            material        = nodeEntity.material;
                            if(material.MaterialPointer == nullptr)
                            {
                                material.MaterialPointer = Material::Create();
                            }

                            auto& rigidBody = scene->m_SceneRegistry.emplace<RigidBodyComponent>(e);
                            rigidBody       = nodeEntity.rigidBody;

                            auto& collider  = scene->m_SceneRegistry.emplace<ColliderComponent>(e);
                            collider        = nodeEntity.collider;

                            CreateRigidBody(scene->m_PhyWorld, &scene->m_SceneRegistry, e);

                            std::vector<glm::vec3> verts;
                            verts.reserve(mesh.Vertices.size());
                            std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(verts),
                                        [](const Vertex& v) { return v.Position; });

                            CreateConvexCollider(&scene->m_PhyCommon, &scene->m_SceneRegistry, e, verts);

                            children.push_back(e);
                        }
                    }

                    entt::entity prev = entt::null;
                    for (std::size_t i = 0; i < children.size(); ++i)
                    {
                        auto e = children[i];
                        scene->m_SceneRegistry.emplace<HierarchyComponent>(e, root, entt::null, entt::null);
                        if (i > 0) scene->m_SceneRegistry.get<HierarchyComponent>(prev).NextSibling = e;
                        prev = e;
                    }

                    scene->m_SceneRegistry.emplace<HierarchyComponent>(root, entt::null,
                        children.empty() ? entt::null : children.front(), entt::null);

                    scene->EmplaceEntity(root);
                }
            }
        }
        catch(const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Failed to deserialize scene: {}", e.what());
            return false;
        }
    }

    bool SceneSerializer::SerializeRuntime(Scene* scene, const std::filesystem::path& path)
    {
        if(!scene) return false;
        if(path.empty()) return false;

        YAML::Node root;

        YAML::Node sceneNode;
        sceneNode["ID"]             = scene->m_SceneID;
        sceneNode["Name"]           = scene->m_SceneName;
        sceneNode["Environment"]    = scene->m_Environment;
        sceneNode["PhysicsWorld"]   = scene->m_PhySettings;

        if(scene->GetEntityCount())
        {
            YAML::Node entities(YAML::NodeType::Sequence);
            scene->ForEachRootEntity([&](entt::registry& registry, entt::entity entity)
            {
                if(registry.valid(entity) && scene->IsRootEntity(entity))
                {
                    const auto* tag = registry.try_get<TagComponent>(entity);
                        entities["Entity"]["Tag"] = *tag;

                    if(const auto* transform = registry.try_get<TransformComponent>(entity))
                        entities["Entity"]["Transform"] = *transform;

                    scene->ForEachNodeEntity(entity, [&](entt::registry& r, entt::entity e)
                    {
                        if(const auto* tag = r.try_get<TagComponent>(e))
                            entities["Entity"]["NodeEntity"]["Tag"] = *tag;

                        if(const auto* transform = r.try_get<TransformComponent>(e))
                            entities["Entity"]["NodeEntity"]["Transform"] = *transform;

                        if(const auto* rigidBody = r.try_get<RigidBodyComponent>(e))
                            entities["Entity"]["NodeEntity"]["RigidBody"] = *rigidBody;

                        if(const auto* collider = r.try_get<ColliderComponent>(e))
                            entities["Entity"]["NodeEntity"]["Collider"] = *collider;
                    });
                }

                sceneNode["Entities"] = entities;
            });
            
        }

        root["Scene"] = sceneNode;
        
        try
        {
            std::filesystem::path dir = std::filesystem::absolute(path);
            std::ofstream file(dir, std::ios::out | std::ios::trunc);
            file << root << std::endl;
            file.close();  
        }
        catch(const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Failed to serialize scene: {}", e.what());
            return false;
        }

        return true;
    }

    bool SceneSerializer::DeserializeRuntime(Scene* scene, const std::filesystem::path& path)
    {
        try
        {
            if(!scene) return false;
            if(!std::filesystem::exists(path)) return false;

            YAML::Node root = YAML::LoadFile(path.string());
            const auto& sceneNode = root["Scene"];

            const SceneEnvironment environment = sceneNode["Environment"].as<SceneEnvironment>(SceneEnvironment{});
            const rp3d::PhysicsWorld::WorldSettings physicsWorldSettings = sceneNode["PhysicsWorld"].as<rp3d::PhysicsWorld::WorldSettings>(rp3d::PhysicsWorld::WorldSettings{});

            auto& sceneEnvironment          = scene->GetEnvironment();
            auto& scenePhysicsWorldSettings = scene->GetPhysicsWorldSettings();

            sceneEnvironment.Sun        = environment.Sun;
            scenePhysicsWorldSettings   = physicsWorldSettings;

            struct NodeEntity
            {
                TagComponent tag;
                TransformComponent transform;
                RigidBodyComponent rigidBody;
                ColliderComponent collider;
            };

            struct Entity
            {
                TagComponent tag;
                TransformComponent transform;
                std::unordered_map<std::string, NodeEntity> nodeEntities;
            };

            std::vector<Entity> entitiesData;

            if(sceneNode["Entities"].IsDefined())
            {
                const auto& entities = root["Entities"];

                for(const auto& entity : entities)
                {
                    const auto& currentEntity = entity["Entity"];

                    Entity e;
                    e.tag = currentEntity["Tag"].as<TagComponent>(TagComponent{});
                    e.transform = currentEntity["Transform"].as<TransformComponent>(TransformComponent{});

                    entitiesData.push_back(e);

                    if(currentEntity["NodeEntity"].IsDefined())
                    {
                        for(const auto& nodeEntity : currentEntity["NodeEntity"])
                        {
                            NodeEntity ne;
                            ne.tag          = nodeEntity["Tag"].as<TagComponent>(TagComponent{});
                            ne.transform    = nodeEntity["Transform"].as<TransformComponent>(TransformComponent{});
                            ne.rigidBody    = nodeEntity["RigidBody"].as<RigidBodyComponent>(RigidBodyComponent{});
                            ne.collider     = nodeEntity["Collider"].as<ColliderComponent>(ColliderComponent{});

                            e.nodeEntities[ne.tag.Tag] = ne;
                        }
                    }   
                }
            }
    
            scene->ForEachRootEntity([&](entt::registry& registry, entt::entity root)
            {
                for(const auto& entity : entitiesData)
                {
                    if(const auto* tag = registry.try_get<TagComponent>(root); tag && tag->Tag == entity.tag.Tag)
                    {
                        if(auto* transform = registry.try_get<TransformComponent>(root); transform)
                        {
                            transform->Translation  = entity.transform.Translation;
                            transform->Rotation     = entity.transform.Rotation;
                            transform->Scale        = entity.transform.Scale;
                        }

                        scene->ForEachNodeEntity(root, [&](entt::registry& r, entt::entity e)
                        {
                            if(const auto* tag = r.try_get<TagComponent>(e); tag && entity.nodeEntities.contains(tag->Tag))
                            {
                                const auto& ne = entity.nodeEntities.at(tag->Tag);

                                if(auto* transform = r.try_get<TransformComponent>(e); transform)
                                {
                                    transform->Translation  = ne.transform.Translation;
                                    transform->Rotation     = ne.transform.Rotation;
                                    transform->Scale        = ne.transform.Scale;
                                }

                                if(auto* rigidBody = r.try_get<RigidBodyComponent>(e))
                                {
                                    rigidBody->AngularDamping   = ne.rigidBody.AngularDamping;
                                    rigidBody->LinearDamping    = ne.rigidBody.LinearDamping;
                                    rigidBody->LockZ            = ne.rigidBody.LockZ;
                                    rigidBody->LockY            = ne.rigidBody.LockY;
                                    rigidBody->LockX            = ne.rigidBody.LockX;
                                    rigidBody->LockRotZ         = ne.rigidBody.LockRotZ;
                                    rigidBody->LockRotY         = ne.rigidBody.LockRotY;
                                    rigidBody->LockRotX         = ne.rigidBody.LockRotX;
                                }

                                if(auto* collider = r.try_get<ColliderComponent>(e))
                                {
                                    collider->BoxHalfExtents    = ne.collider.BoxHalfExtents;
                                    collider->Friction          = ne.collider.Friction;
                                    collider->Restitution       = ne.collider.Restitution;
                                    collider->MassDensity       = ne.collider.MassDensity;
                                    collider->LocalTransform    = ne.collider.LocalTransform;
                                    collider->LocalRotation     = ne.collider.LocalRotation;
                                    collider->LastAppliedScale  = ne.collider.LastAppliedScale;
                                }
                            }
                        });
                    }
                }
            });        
        }
        catch(const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Failed to deserialize scene: {}", e.what());
            return false;
        }
    }
}
