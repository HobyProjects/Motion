#include "CorePCH.hpp"
#include "SceneUtils.hpp"
#include "SceneSerializer.hpp"

namespace YAML
{
    template<> 
    struct convert<glm::vec2> 
    {
        static Node encode(const glm::vec2& v) 
        { 
            Node n; 
            n["x"] = v.x; 
            n["y"] = v.y; 
            return n; 
        }

        static bool decode(const Node& n, glm::vec2& v) 
        { 
            if(!n.IsMap()) return false; 

            v.x = n["x"].as<float>(0.f); 
            v.y = n["y"].as<float>(0.f); 

            return true; 
        }
    };

    template<> 
    struct convert<glm::vec3> 
    {
        static Node encode(const glm::vec3& v) 
        { 
            Node n; 
            n["x"] = v.x; 
            n["y"] = v.y; 
            n["z"] = v.z; 
            return n; 
        }

        static bool decode(const Node& n, glm::vec3& v) 
        { 
            if(!n.IsMap()) return false; 

            v.x = n["x"].as<float>(0.f); 
            v.y = n["y"].as<float>(0.f); 
            v.z = n["z"].as<float>(0.f); 

            return true; 
        }
    };

    template<> 
    struct convert<glm::vec4> 
    {
        static Node encode(const glm::vec4& v) 
        { 
            Node n; 

            n["x"] = v.x; 
            n["y"] = v.y; 
            n["z"] = v.z; 
            n["w"] = v.w; 

            return n; 
        }

        static bool decode(const Node& n, glm::vec4& v) 
        { 
            if(!n.IsMap()) return false; 

            v.x = n["x"].as<float>(0.f); 
            v.y = n["y"].as<float>(0.f); 
            v.z = n["z"].as<float>(0.f); 
            v.w = n["w"].as<float>(0.f); 

            return true; 
        }
    };

    template<> 
    struct convert<glm::quat> 
    {
        static Node encode(const glm::quat& q) 
        { 
            Node n; 

            n["w"] = q.w; 
            n["x"] = q.x; 
            n["y"] = q.y; 
            n["z"] = q.z; 

            return n; 
        }

        static bool decode(const Node& n, glm::quat& q) 
        { 
            if(!n.IsMap()) return false; 

            q.w = n["w"].as<float>(1.f); 
            q.x = n["x"].as<float>(0.f); 
            q.y = n["y"].as<float>(0.f); 
            q.z = n["z"].as<float>(0.f); 

            return true; 
        }
    };

    template<> 
    struct convert<rp3d::Vector2> 
    {
        static Node encode(const rp3d::Vector2& v) 
        { 
            Node n; 

            n["x"] = v.x; 
            n["y"] = v.y;

            return n; 
        }

        static bool decode(const Node& n, rp3d::Vector2& v) 
        { 
            if(!n.IsMap()) return false; 
            
            v.x = n["x"].as<float>(0.f); 
            v.y = n["y"].as<float>(0.f); 
            
            return true; 
        }
    };

    template<> struct convert<rp3d::Vector3> 
    {
        static Node encode(const rp3d::Vector3& v) 
        { 
            Node n; 
            n["x"] = v.x; 
            n["y"] = v.y; 
            n["z"] = v.z; 
            return n; 
        }

        static bool decode(const Node& n, rp3d::Vector3& v) 
        { 
            if(!n.IsMap()) return false;

            v.x = n["x"].as<float>(0.f); 
            v.y = n["y"].as<float>(0.f); 
            v.z = n["z"].as<float>(0.f);

            return true; 
        }
    };

    template<> struct convert<rp3d::Quaternion> 
    {
        static Node encode(const rp3d::Quaternion& q) 
        { 
            Node n;

            n["w"] = q.w; 
            n["x"] = q.x; 
            n["y"] = q.y; 
            n["z"] = q.z; 
            
            return n; 
        }

        static bool decode(const Node& n, rp3d::Quaternion& q) 
        { 
            if(!n.IsMap()) return false; 
            
            q.w = n["w"].as<float>(1.f); 
            q.x = n["x"].as<float>(0.f); 
            q.y = n["y"].as<float>(0.f); 
            q.z = n["z"].as<float>(0.f); 
            
            return true; 
        }
    };

    template<> 
    struct convert<std::filesystem::path> 
    {
        static Node encode(const std::filesystem::path& p) 
        { 
            Node n; 
            n = std::filesystem::absolute(p).string(); 
            return n; 
        }

        static bool decode(const Node& n, std::filesystem::path& p) 
        { 
            if(!n.IsScalar()) return false; 
            p = n.as<std::string>(); 
            return true; 
        }
    };

    template<> 
    struct convert<Motion::TagComponent> 
    {
        static Node encode(const Motion::TagComponent& c)
        { 
            Node n; 

            n["ID"]         =c.ID; 
            n["Tag"]        =c.Tag; 
            n["IsActive"]   =c.IsActive; 

            return n; 
        }

        static bool decode(const Node& n, Motion::TagComponent& c)
        { 
            if(!n.IsMap()) return false; 

            c.ID        = n["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID()); 
            c.Tag       = n["Tag"].as<std::string>("Undefined"); 
            c.IsActive  = n["IsActive"].as<bool>(true); 

            return true; 
        }
    };

    template<> 
    struct convert<Motion::TransformComponent> 
    {
        static Node encode(const Motion::TransformComponent& c)
        { 
            Node n;

            n["ID"]             = c.ID; 
            n["Translation"]    = c.Translation; 
            n["Rotation"]       = c.Rotation; 
            n["Scale"]          = c.Scale; 

            return n;
        }

        static bool decode(const Node& n, Motion::TransformComponent& c)
        { 
            if(!n.IsMap()) return false; 

            c.ID            = n["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID()); 
            c.Translation   = n["Translation"].as<glm::vec3>(glm::vec3(0.f)); 
            c.Rotation      = n["Rotation"].as<glm::quat>(glm::quat(1,0,0,0)); 
            c.Scale         = n["Scale"].as<glm::vec3>(glm::vec3(1.f));

            return true; 
        }
    };

    template<> 
    struct convert<Motion::RigidBodyComponent> 
    {
        static Node encode(const Motion::RigidBodyComponent& rb)
        {
            auto toStr=[](Motion::BodyType t)
            { 
                switch(t)
                {
                    case Motion::BodyType::Static:  return "Static"; 
                    case Motion::BodyType::Dynamic: return "Dynamic"; 
                    default:                        return "Undefined";
                }
            };

            Node n; 

            n["ID"]             = rb.ID; 
            n["BodyType"]       = toStr(rb.Type); 
            n["LinearDamping"]  = rb.LinearDamping; 
            n["AngularDamping"] = rb.AngularDamping;

            n["LockX"]    = rb.LockX; 
            n["LockY"]    = rb.LockY; 
            n["LockZ"]    = rb.LockZ; 
            n["LockRotX"] = rb.LockRotX; 
            n["LockRotY"] = rb.LockRotY; 
            n["LockRotZ"] = rb.LockRotZ; 
            
            return n;
        }

        static bool decode(const Node& n, Motion::RigidBodyComponent& rb)
        {
            auto toType=[](const std::string& s)
            { 
                if(s=="Static") return Motion::BodyType::Static; 
                return Motion::BodyType::Dynamic; 
            };

            if(!n.IsMap()) return false;

            rb.ID               = n["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            rb.Type             = toType(n["BodyType"].as<std::string>("Dynamic"));
            rb.LinearDamping    = n["LinearDamping"].as<float>(0.2f);
            rb.AngularDamping   = n["AngularDamping"].as<float>(0.05f);

            rb.LockX            = n["LockX"].as<bool>(false); 
            rb.LockY            = n["LockY"].as<bool>(false); 
            rb.LockZ            = n["LockZ"].as<bool>(false);

            rb.LockRotX         = n["LockRotX"].as<bool>(false); 
            rb.LockRotY         = n["LockRotY"].as<bool>(false); 
            rb.LockRotZ         = n["LockRotZ"].as<bool>(false);
            return true;
        }
    };

    template<> 
    struct convert<Motion::ColliderComponent> 
    {
        static Node encode(const Motion::ColliderComponent& c)
        {
            auto toStr=[](Motion::ShapeType t)
            {
                switch(t)
                {
                    case Motion::ShapeType::Box:            return "Box"; 
                    case Motion::ShapeType::Sphere:         return "Sphere";
                    case Motion::ShapeType::Capsule:        return "Capsule"; 
                    case Motion::ShapeType::Convex:         return "Convex";
                    case Motion::ShapeType::Concave:        return "Concave"; 
                    default:                                return "Undefined";
                }
            };

            Node n; 
            
            n["ID"]                 = c.ID; 
            n["Shape"]              = toStr(c.Type); 
            n["BoxHalfExtents"]     = c.BoxHalfExtents; 
            n["SphereRadius"]       = c.SphereRadius;
            n["CapsuleRadius"]      = c.Capsule.Radius; 
            n["CapsuleHeight"]      = c.Capsule.Height; 
            n["CapsuleAxis"]        = c.Capsule.Axis;
            n["Friction"]           = c.Friction; 
            n["Restitution"]        = c.Restitution; 
            n["MassDensity"]        = c.MassDensity;
            n["LastAppliedScale"]   = c.LastAppliedScale; 
            n["LocalTransform"]     = c.LocalTransform; 
            n["LocalRotation"]      = c.LocalRotation; 
            
            return n;
        }

        static bool decode(const Node& n, Motion::ColliderComponent& c)
        {
            auto toType=[](const std::string& s)
            { 
                if(s=="Sphere")      return Motion::ShapeType::Sphere; 
                if(s=="Capsule")     return Motion::ShapeType::Capsule;
                if(s=="Convex")      return Motion::ShapeType::Convex; 
                if(s=="Concave")     return Motion::ShapeType::Concave;

                return Motion::ShapeType::Box; 
            };

            if(!n.IsMap()) return false;

            c.ID                = n["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID());
            c.Type              = toType(n["Shape"].as<std::string>("Box"));
            c.BoxHalfExtents    = n["BoxHalfExtents"].as<glm::vec3>(glm::vec3(0.5f));
            c.SphereRadius      = n["SphereRadius"].as<float>(0.5f);
            c.Capsule.Radius    = n["CapsuleRadius"].as<float>(0.5f);
            c.Capsule.Height    = n["CapsuleHeight"].as<float>(1.0f);
            c.Capsule.Axis      = n["CapsuleAxis"].as<std::int32_t>(1);
            c.Friction          = n["Friction"].as<float>(0.5f);
            c.Restitution       = n["Restitution"].as<float>(0.2f);
            c.MassDensity       = n["MassDensity"].as<float>(500.0f);
            c.LastAppliedScale  = n["LastAppliedScale"].as<glm::vec3>(glm::vec3(1.0f));
            c.LocalTransform    = n["LocalTransform"].as<glm::vec3>(glm::vec3(0.0f));
            c.LocalRotation     = n["LocalRotation"].as<glm::quat>(glm::quat(1,0,0,0));

            return true;
        }
    };

    template<> 
    struct convert<Motion::MeshComponent> 
    {
        static Node encode(const Motion::MeshComponent& m)
        { 
            Node n; 
            n["ID"]         = m.ID; 
            n["Name"]       = m.Name; 
            n["MinBounds"]  = m.MinBounds; 
            n["MaxBounds"]  = m.MaxBounds; 
            n["MeshIndex"]  = m.MeshIndex; 
            return n; 
        }

        static bool decode(const Node& n, Motion::MeshComponent& m){ if(!n.IsMap()) return false; m.ID=n["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID()); m.Name=n["Name"].as<std::string>("Undefined"); m.MinBounds=n["MinBounds"].as<glm::vec3>(glm::vec3(0)); m.MaxBounds=n["MaxBounds"].as<glm::vec3>(glm::vec3(0)); m.MeshIndex=n["MeshIndex"].as<std::uint32_t>(0); return true; }
    };

    template<> 
    struct convert<Motion::ModelComponent> 
    {
        static Node encode(const Motion::ModelComponent& m)
        { 
            Node n;

            n["ID"]         = m.ID; 
            n["FilePath"]   = m.FilePath; 
            n["MeshCount"]  = m.MeshCount; 
            n["MinBounds"]  = m.MinBounds; 
            n["MaxBounds"]  = m.MaxBounds; 

            return n; 
        }

        static bool decode(const Node& n, Motion::ModelComponent& m)
        { 
            if(!n.IsMap()) return false; 
            
            m.ID        = n["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID()); 
            m.FilePath  = n["FilePath"].as<std::filesystem::path>(std::filesystem::path{}); 
            m.MeshCount = n["MeshCount"].as<std::uint32_t>(0); 
            m.MinBounds = n["MinBounds"].as<glm::vec3>(glm::vec3(0)); 
            m.MaxBounds = n["MaxBounds"].as<glm::vec3>(glm::vec3(0));

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

            auto noneTex = [&](Node parent, const char* key){
                parent[key]["Valid"] = false;
                parent[key]["Type"]  = "None";
                parent[key]["File"]  = "None";
            };

            if (!rhs.MaterialPointer)
            {
                noneTex(node["Core"],   "BaseTexture");
                noneTex(node["Core"],   "NormalTexture");
                noneTex(node["Core"],   "MetallicTexture");
                noneTex(node["Core"],   "RoughnessTexture");
                noneTex(node["Core"],   "AOTexture");
                noneTex(node["Core"],   "EmissiveTexture");
                noneTex(node["Core"],   "OpacityTexture");
                noneTex(node["Packed"], "ORMTexture");
                return node;
            }

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

            auto writeTex = [](Node parent, const char* key, const std::weak_ptr<Motion::ITexture>& w)
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
                    n["Valid"]  = false; 
                    n["Type"]   ="None"; 
                    n["File"]   ="None";
                }
            };

            writeTex(node["Core"],   "BaseTexture",      M.BaseColor);
            writeTex(node["Core"],   "NormalTexture",    M.Normal);
            writeTex(node["Core"],   "MetallicTexture",  M.Metallic);
            writeTex(node["Core"],   "RoughnessTexture", M.Roughness);
            writeTex(node["Core"],   "AOTexture",        M.AO);
            writeTex(node["Core"],   "EmissiveTexture",  M.Emissive);
            writeTex(node["Core"],   "OpacityTexture",   M.Opacity);
            writeTex(node["Packed"], "ORMTexture",       M.ORM);

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
            if (!rhs.MaterialPointer) rhs.MaterialPointer = Motion::Material::Create();

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

            auto as_vec3 = [](const Node& n, const glm::vec3& d){ return n.IsDefined()? n.as<glm::vec3>(d):d; };
            auto as_vec4 = [](const Node& n, const glm::vec4& d){ return n.IsDefined()? n.as<glm::vec4>(d):d; };
            auto as_f32  = [](const Node& n, float d){ return n.IsDefined()? n.as<float>(d):d; };

            auto loadTex = [&](const char* parentKey, const char* texKey) -> std::shared_ptr<Motion::ITexture> 
            {
                Node tex = N({parentKey, texKey});
                if(!tex.IsDefined()) return nullptr;

                bool valid = false;
                if(Node v = tex["Valid"]; v.IsDefined() && v.IsScalar()) valid = v.as<bool>(false);
                else 
                {
                    const std::string name = tex["Name"].as<std::string>("");
                    const std::string file = tex["File"].as<std::string>("");
                    valid = (!file.empty()) || (!name.empty() && name != "None");
                }

                if(!valid) return nullptr;
                const std::string file = tex["File"].as<std::string>("");
                if(file.empty()) return nullptr;

                const std::string typeName = tex["Type"].as<std::string>("");
                auto type = Motion::GetTextureTypeFromName(typeName);
                return Motion::ITexture::Create(file, type);
            };

            if (rhs.MaterialPointer->Has<Motion::CoreMaterialComponents>())
            {
                auto& core = rhs.MaterialPointer->Get<Motion::CoreMaterialComponents>();

                Node P = N({"Core","Property"});
                core.BaseColorFactor   = as_vec4(P["BaseColorFactor"], glm::vec4(1.f));
                core.EmissiveFactor    = as_vec3(P["EmissiveFactor"], glm::vec3(0.f));
                core.MetallicFactor    = as_f32 (P["MetallicFactor"], 0.f);
                core.RoughnessFactor   = as_f32 (P["RoughnessFactor"], 0.5f);
                core.OpacityFactor     = as_f32 (P["OpacityFactor"], 1.f);
                core.EmissiveStrength  = as_f32 (P["EmissiveStrength"], 1.f);
                core.OcclusionStrength = as_f32 (P["AOStrength"], 1.f);
                core.NormalScale       = as_f32 (P["NormalScale"], 1.f);

                core.BaseColorTexture = loadTex("Core","BaseTexture");
                core.NormalTexture    = loadTex("Core","NormalTexture");
                core.MetallicTexture  = loadTex("Core","MetallicTexture");
                core.RoughnessTexture = loadTex("Core","RoughnessTexture");
                core.OcclusionTexture = loadTex("Core","AOTexture");
                core.EmissiveTexture  = loadTex("Core","EmissiveTexture");
            }

            if (Node B = node["Base"]; B.IsDefined())
            {
                const std::string yamlPath = B["YAML"].as<std::string>("");
                if(!yamlPath.empty())
                {
                    if(auto base = Motion::Material::CreateBase(yamlPath))
                    {
                        Node BP = B["Property"];
                        base->BaseColor       = as_vec3(BP["BaseColor"],      base->BaseColor);
                        base->MetallicFactor  = as_f32 (BP["MetallicFactor"], base->MetallicFactor);
                        base->RoughnessFactor = as_f32 (BP["RoughnessFactor"],base->RoughnessFactor);
                        base->OpacityFactor   = as_f32 (BP["OpacityFactor"],  base->OpacityFactor);

                        rhs.MaterialPointer->SetBaseMaterial(base);
                    }
                }
            }

            return true;
        }
    };

    template<> struct convert<Motion::ScenePhysicsWorld::WorldLighting> 
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
            
            L.Direction     = n["SunDirection"].as<glm::vec3>(L.Direction); 
            L.Intensity     = n["SunIntensity"].as<float>(L.Intensity); 
            L.Color         = n["SunColor"].as<glm::vec3>(L.Color); 
            L.ShowGuizmo    = n["SunShowDir"].as<bool>(L.ShowGuizmo);

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

} 

namespace Motion
{
    bool SceneSerializer::Serialize(Scene* scene, const std::filesystem::path& path)
    {
        if (!scene || path.empty()) return false;

        YAML::Node root;
        YAML::Node sceneNode;
        sceneNode["ID"]            = scene->m_Specification.ID;
        sceneNode["Name"]          = scene->m_Specification.Name;
        sceneNode["SavedPath"]     = scene->m_Specification.SavedPath;
        sceneNode["Lighting"]      = scene->m_Physics.SunLight;
        sceneNode["WorldSettings"] = scene->m_Physics.Settings;

        YAML::Node entities(YAML::NodeType::Sequence);

        if (!scene->m_Entities.EntryPoints.empty())
        {
            const auto& r = scene->m_Entities.Registry;

            scene->ForEachRootEntity([&](entt::entity rootEnt)
            {
                if (!r.valid(rootEnt) || !scene->IsRootEntity(rootEnt)) return;

                YAML::Node entity;
                if (const auto* tag = r.try_get<TagComponent>(rootEnt))        entity["Tag"]       = *tag;
                if (const auto* model = r.try_get<ModelComponent>(rootEnt))    entity["Model"]     = *model;
                if (const auto* tr = r.try_get<TransformComponent>(rootEnt))   entity["Transform"] = *tr;

                YAML::Node nodes(YAML::NodeType::Sequence);
                scene->ForEachNodeEntity(rootEnt, [&](entt::entity nodeEnt)
                {
                    YAML::Node n;
                    if (const auto* t = r.try_get<TagComponent>(nodeEnt))           n["Tag"]        = *t;
                    if (const auto* tr = r.try_get<TransformComponent>(nodeEnt))    n["Transform"]  = *tr;
                    if (const auto* m = r.try_get<ModelComponent>(nodeEnt))         n["Model"]      = *m;
                    if (const auto* mesh = r.try_get<MeshComponent>(nodeEnt))       n["Mesh"]       = *mesh;
                    if (const auto* mat = r.try_get<MaterialComponent>(nodeEnt))    n["Material"]   = *mat;
                    if (const auto* rb  = r.try_get<RigidBodyComponent>(nodeEnt))   n["RigidBody"]  = *rb;
                    if (const auto* col = r.try_get<ColliderComponent>(nodeEnt))    n["Collider"]   = *col;
                    nodes.push_back(n);
                });

                if (nodes.IsSequence() && nodes.size() > 0)
                    entity["Nodes"] = nodes;

                entities.push_back(entity);
            });
        }

        sceneNode["Entities"] = entities;
        root["Scene"]         = sceneNode;

        try
        {
            std::filesystem::path abs = std::filesystem::absolute(path);
            std::ofstream file(abs, std::ios::out | std::ios::trunc);
            file << root << std::endl;
            file.close();  // FIXED: Explicitly close file

        } catch (const std::exception& e) 
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
            if (!std::filesystem::exists(path)) return nullptr;

            YAML::Node root      = YAML::LoadFile(path.string());
            YAML::Node sceneNode = root["Scene"];

            UUID sceneID                    = sceneNode["ID"].as<UUID>(UniqueIdentity::GetUniqueID());
            std::string sceneName           = sceneNode["Name"].as<std::string>("Untitled");
            std::filesystem::path savedPath = sceneNode["SavedPath"].as<std::filesystem::path>(std::filesystem::path{});

            SceneSpecification spec{ };
            spec.ID        = sceneID;
            spec.Name      = sceneName;
            spec.SavedPath = savedPath;

            auto scene = std::make_shared<Scene>(spec);
            if (!scene) return nullptr;

            scene->m_Physics.SunLight = sceneNode["Lighting"].as<ScenePhysicsWorld::WorldLighting>(ScenePhysicsWorld::WorldLighting{});
            scene->m_Physics.Settings = sceneNode["WorldSettings"].as<rp3d::PhysicsWorld::WorldSettings>(rp3d::PhysicsWorld::WorldSettings{});

            if (YAML::Node entities = sceneNode["Entities"]; entities.IsSequence())
            {
                struct NodeEntity 
                {
                    TagComponent         tag{};
                    TransformComponent   transform{};
                    MeshComponent        mesh{};
                    MaterialComponent    material{};
                    RigidBodyComponent   rigidBody{};
                    ColliderComponent    collider{};
                    bool                 hasMesh{false};
                };

                struct Entity 
                {
                    TagComponent         tag{};
                    ModelComponent       model{};
                    TransformComponent   transform{};
                    std::vector<NodeEntity> nodes{};
                };

                std::vector<Entity> parsed;

                for (const auto& entityNode : entities)
                {
                    const YAML::Node eN = entityNode;
                    Entity E;
                    if (auto n = eN["Tag"];       n.IsDefined()) E.tag       = n.as<TagComponent>(TagComponent{});
                    if (auto n = eN["Model"];     n.IsDefined()) E.model     = n.as<ModelComponent>(ModelComponent{});
                    if (auto n = eN["Transform"]; n.IsDefined()) E.transform = n.as<TransformComponent>(TransformComponent{});

                    if (YAML::Node nodes = eN["Nodes"]; nodes.IsSequence())
                    {
                        for (const auto& nn : nodes)
                        {
                            NodeEntity NE;
                            if (auto n = nn["Tag"];        n.IsDefined()) NE.tag        = n.as<TagComponent>(TagComponent{});
                            if (auto n = nn["Transform"];  n.IsDefined()) NE.transform  = n.as<TransformComponent>(TransformComponent{});
                            if (auto n = nn["Mesh"];       n.IsDefined()) { NE.mesh = n.as<MeshComponent>(MeshComponent{}); NE.hasMesh = true; }
                            if (auto n = nn["Material"];   n.IsDefined()) NE.material   = n.as<MaterialComponent>(MaterialComponent{});
                            if (auto n = nn["RigidBody"];  n.IsDefined()) NE.rigidBody  = n.as<RigidBodyComponent>(RigidBodyComponent{});
                            if (auto n = nn["Collider"];   n.IsDefined()) NE.collider   = n.as<ColliderComponent>(ColliderComponent{});
                            E.nodes.push_back(NE);
                        }
                    }

                    parsed.push_back(std::move(E));
                }

                const BufferLayout layout 
                {
                    { "a_Position",   BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Position)    },
                    { "a_TexCoords",  BufferComponents::UV,   BufferStride::F2, false, offsetof(Vertex, TexCoord)    },
                    { "a_Normals",    BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Normal)      },
                    { "a_Tangents",   BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)     },
                    { "a_Bitangents", BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Bitangent)   },
                };

                for (auto& E : parsed)
                {
                    if (E.model.FilePath.empty()) continue;

                    ImportSettings settings;
                    settings.ShouldExport = false;
                    settings.ExportPath   = std::filesystem::path{};
                    settings.FilePath     = E.model.FilePath;

                    auto results = Importer::ImportEntity(settings);
                    if (!results) {
                        MOTION_CORE_ERROR("Failed to import model: {}", E.model.FilePath.string());
                        continue;
                    }
                    if (E.model.MeshCount != results->MeshCount) {
                        MOTION_CORE_ERROR("Failed to import model: {} - mesh count mismatch", E.model.FilePath.string());
                        continue;
                    }

                    auto& r = scene->m_Entities.Registry;
                    entt::entity rootE = r.create();

                    auto& tag   = r.emplace<TagComponent>(rootE);
                    tag.Tag     = E.tag.Tag;

                    auto& tr    = r.emplace<TransformComponent>(rootE);
                    tr          = E.transform;

                    auto& model = r.emplace<ModelComponent>(rootE);
                    model.FilePath  = E.model.FilePath;
                    model.MeshCount = E.model.MeshCount;
                    model.MinBounds = E.model.MinBounds;
                    model.MaxBounds = E.model.MaxBounds;

                    std::vector<entt::entity> children;
                    children.reserve(E.model.MeshCount);

                    std::unordered_map<uint32_t, const NodeEntity*> byIndex;
                    for (const auto& NE : E.nodes) if (NE.hasMesh) byIndex[NE.mesh.MeshIndex] = &NE;

                    for (const auto& [index, meshData] : results->Meshes)
                    {
                        const NodeEntity* NE = nullptr;
                        if (auto it = byIndex.find(index); it != byIndex.end()) NE = it->second;
                        if (!NE) continue;

                        entt::entity e = r.create();

                        auto& ttag = r.emplace<TagComponent>(e);
                        ttag.Tag    = NE->tag.Tag;

                        auto& ttr  = r.emplace<TransformComponent>(e);
                        ttr        = NE->transform;

                        auto& mcmp = r.emplace<MeshComponent>(e);
                        mcmp.ID            = NE->mesh.ID;
                        mcmp.MeshIndex     = index;
                        mcmp.MeshPointer   = Mesh::Create(meshData.Vertices.data(), meshData.Vertices.size(),
                                                          meshData.Indices.data(),  meshData.Indices.size(),  layout);
                        mcmp.MinBounds     = meshData.MIN;
                        mcmp.MaxBounds     = meshData.MAX;

                        auto& mat = r.emplace<MaterialComponent>(e);
                        mat       = NE->material;
                        if (!mat.MaterialPointer) mat.MaterialPointer = Material::Create();

                        auto& rb  = r.emplace<RigidBodyComponent>(e);
                        rb        = NE->rigidBody;

                        auto& col = r.emplace<ColliderComponent>(e);
                        col       = NE->collider;

                        CreateRigidBody(scene->m_Physics.World, &r, e);

                        std::vector<glm::vec3> verts;
                        verts.reserve(meshData.Vertices.size());
                        std::transform(meshData.Vertices.begin(), meshData.Vertices.end(), std::back_inserter(verts),
                                       [](const Vertex& v){ return v.Position; });

                        CreateConvexCollider(&scene->m_Physics.Properties, &r, e, verts);

                        children.push_back(e);
                    }

                    entt::entity prev = entt::null;
                    for (auto e : children)
                    {
                        r.emplace<HierarchyComponent>(e, rootE, entt::null, entt::null);
                        if (prev == entt::null) {
                            auto& hc = r.emplace_or_replace<HierarchyComponent>(rootE);
                            hc.Parent = entt::null;
                            hc.FirstChild = e;
                            hc.NextSibling = entt::null;
                        } else {
                            auto& hprev = r.get<HierarchyComponent>(prev);
                            hprev.NextSibling = e;
                        }
                        prev = e;
                    }

                    scene->EmplaceEntity(rootE);
                }
            }

            return scene;

        } catch (const std::exception& e) 
        {
            MOTION_CORE_CRITICAL("Failed to deserialize scene: {}", e.what());
            return nullptr;
        }
    }

    bool SceneSerializer::SerializeRuntime(Scene* scene, const std::filesystem::path& path)
    {
        if(!scene || path.empty()) return false;

        YAML::Node root, sceneNode;
        sceneNode["ID"]            = scene->m_Specification.ID;
        sceneNode["Name"]          = scene->m_Specification.Name;
        sceneNode["SavedPath"]     = scene->m_Specification.SavedPath;
        sceneNode["Lighting"]      = scene->m_Physics.SunLight;
        sceneNode["WorldSettings"] = scene->m_Physics.Settings;

        if(!scene->m_Entities.EntryPoints.empty())
        {
            auto& r = scene->m_Entities.Registry;
            YAML::Node entities(YAML::NodeType::Sequence);

            scene->ForEachRootEntity([&](entt::entity root)
            {
                if(!r.valid(root) || !scene->IsRootEntity(root)) return;

                YAML::Node entityNode;
                if (const auto* tag = r.try_get<TagComponent>(root))
                    entityNode["Entity"]["Tag"] = *tag;
                if (const auto* tr  = r.try_get<TransformComponent>(root))
                    entityNode["Entity"]["Transform"] = *tr;

                YAML::Node nodes(YAML::NodeType::Sequence);
                scene->ForEachNodeEntity(root, [&](entt::entity node)
                {
                    YAML::Node n;
                    if (const auto* tag = r.try_get<TagComponent>(node))          n["Tag"] = *tag;
                    if (const auto* tr  = r.try_get<TransformComponent>(node))    n["Transform"] = *tr;
                    if (const auto* rb  = r.try_get<RigidBodyComponent>(node))    n["RigidBody"] = *rb;
                    if (const auto* col = r.try_get<ColliderComponent>(node))     n["Collider"] = *col;
                    if (n.size() > 0) nodes.push_back(n);
                });

                if (nodes.size() > 0) entityNode["Entity"]["NodeEntity"] = nodes;
                entities.push_back(entityNode);
            });

            sceneNode["Entities"] = entities;
        }

        root["Scene"] = sceneNode;

        try {
            std::filesystem::path abs = std::filesystem::absolute(path);
            std::ofstream f(abs, std::ios::out | std::ios::trunc);
            f << root << std::endl; 
            f.close();  // FIXED: Explicitly close file
        } catch(const std::exception& e) {
            MOTION_CORE_CRITICAL("Failed to serialize runtime scene: {}", e.what());
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

            YAML::Node root  = YAML::LoadFile(path.string());
            const auto& sn   = root["Scene"];

            scene->m_Physics.SunLight = sn["Lighting"].as<ScenePhysicsWorld::WorldLighting>(ScenePhysicsWorld::WorldLighting{});
            scene->m_Physics.Settings = sn["WorldSettings"].as<rp3d::PhysicsWorld::WorldSettings>(rp3d::PhysicsWorld::WorldSettings{});

            struct NodeEntity {
                TagComponent         tag;
                TransformComponent   transform;
                RigidBodyComponent   rigidBody;
                ColliderComponent    collider;
            };
            struct Entity {
                TagComponent         tag;
                TransformComponent   transform;
                std::unordered_map<UUID, NodeEntity> byId;
                std::unordered_map<std::string, UUID> nameToId;
            };

            std::vector<Entity> data;

            if (sn["Entities"].IsDefined())
            {
                const auto& entities = sn["Entities"];
                for (const auto& elem : entities)
                {
                    const auto& eNode = elem["Entity"];
                    if (!eNode.IsDefined()) continue;

                    Entity e;
                    e.tag       = eNode["Tag"].as<TagComponent>(TagComponent{});
                    e.transform = eNode["Transform"].as<TransformComponent>(TransformComponent{});

                    if (const auto& nseq = eNode["NodeEntity"]; nseq.IsDefined())
                    {
                        for (const auto& n : nseq)
                        {
                            NodeEntity ne;
                            ne.tag        = n["Tag"].as<TagComponent>(TagComponent{});
                            ne.transform  = n["Transform"].as<TransformComponent>(TransformComponent{});
                            ne.rigidBody  = n["RigidBody"].as<RigidBodyComponent>(RigidBodyComponent{});
                            ne.collider   = n["Collider"].as<ColliderComponent>(ColliderComponent{});

                            e.nameToId[ne.tag.Tag] = ne.tag.ID;
                            e.byId.emplace(ne.tag.ID, std::move(ne));
                        }
                    }

                    data.push_back(std::move(e));
                }
            }

            auto& r = scene->m_Entities.Registry;

            scene->ForEachRootEntity([&](entt::entity root)
            {
                auto* rootTag = r.try_get<TagComponent>(root);
                if (!rootTag) return;

                Entity* match = nullptr;
                for (auto& e : data)
                {
                    if (e.tag.ID == rootTag->ID || e.tag.Tag == rootTag->Tag) { match = &e; break; }
                }
                if (!match) return;

                if (auto* tr = r.try_get<TransformComponent>(root))
                {
                    tr->Translation = match->transform.Translation;
                    tr->Rotation    = match->transform.Rotation;
                    tr->Scale       = match->transform.Scale;
                    tr->RebuildLocal();
                }

                scene->ForEachNodeEntity(root, [&](entt::entity node)
                {
                    auto* nTag = r.try_get<TagComponent>(node);
                    if (!nTag) return;

                    const NodeEntity* src = nullptr;

                    if (auto it = match->byId.find(nTag->ID); it != match->byId.end())
                        src = &it->second;
                    else {
                        if (auto itN = match->nameToId.find(nTag->Tag); itN != match->nameToId.end()) {
                            if (auto it2 = match->byId.find(itN->second); it2 != match->byId.end())
                                src = &it2->second;
                        }
                    }

                    if (!src) return;

                    if (auto* tr = r.try_get<TransformComponent>(node))
                    {
                        tr->Translation = src->transform.Translation;
                        tr->Rotation    = src->transform.Rotation;
                        tr->Scale       = src->transform.Scale;
                        tr->RebuildLocal();
                    }

                    if (auto* rb = r.try_get<RigidBodyComponent>(node))
                    {
                        rb->Type           = src->rigidBody.Type;
                        rb->LinearDamping  = src->rigidBody.LinearDamping;
                        rb->AngularDamping = src->rigidBody.AngularDamping;
                        rb->LockX          = src->rigidBody.LockX;
                        rb->LockY          = src->rigidBody.LockY;
                        rb->LockZ          = src->rigidBody.LockZ;
                        rb->LockRotX       = src->rigidBody.LockRotX;
                        rb->LockRotY       = src->rigidBody.LockRotY;
                        rb->LockRotZ       = src->rigidBody.LockRotZ;

                        auto* pb = rb->PhysicsBody;
                        pb->resetForce();
                        pb->resetTorque();
                    }

                    if (auto* col = r.try_get<ColliderComponent>(node))
                    {
                        col->Type             = src->collider.Type;
                        col->BoxHalfExtents   = src->collider.BoxHalfExtents;
                        col->SphereRadius     = src->collider.SphereRadius;
                        col->Capsule          = src->collider.Capsule;
                        col->Friction         = src->collider.Friction;
                        col->Restitution      = src->collider.Restitution;
                        col->MassDensity      = src->collider.MassDensity;
                        col->LastAppliedScale = src->collider.LastAppliedScale;
                        col->LocalTransform   = src->collider.LocalTransform;
                        col->LocalRotation    = src->collider.LocalRotation;
                    }
                });
            });

            scene->RefreshPhysicBodies();
            return true;
        }
        catch(const std::exception& e)
        {
            MOTION_CORE_CRITICAL("Failed to deserialize runtime scene: {}", e.what());
            return false;
        }
    }
}