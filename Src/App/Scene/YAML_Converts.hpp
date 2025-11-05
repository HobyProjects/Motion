#pragma once

#include <yaml-cpp/yaml.h>
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
    struct convert<rp3d::Vector3> 
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

    template<> 
    struct convert<rp3d::Quaternion> 
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
            n["ID"]         = c.ID; 
            n["Tag"]        = c.Tag; 
            n["IsActive"]   = c.IsActive; 
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
            auto toStr = [](Motion::BodyType t) -> const char*
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
            n["LockX"]          = rb.LockX; 
            n["LockY"]          = rb.LockY; 
            n["LockZ"]          = rb.LockZ; 
            n["LockRotX"]       = rb.LockRotX; 
            n["LockRotY"]       = rb.LockRotY; 
            n["LockRotZ"]       = rb.LockRotZ; 
            return n;
        }

        static bool decode(const Node& n, Motion::RigidBodyComponent& rb)
        {
            auto toType = [](const std::string& s)
            { 
                if(s == "Static") return Motion::BodyType::Static; 
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
            auto toStr = [](Motion::ShapeType t) -> const char*
            {
                switch(t)
                {
                    case Motion::ShapeType::Box:        return "Box"; 
                    case Motion::ShapeType::Sphere:     return "Sphere";
                    case Motion::ShapeType::Capsule:    return "Capsule"; 
                    case Motion::ShapeType::Convex:     return "Convex";
                    case Motion::ShapeType::Concave:    return "Concave"; 
                    default:                            return "Undefined";
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
            auto toType = [](const std::string& s)
            { 
                if(s == "Sphere")   return Motion::ShapeType::Sphere; 
                if(s == "Capsule")  return Motion::ShapeType::Capsule;
                if(s == "Convex")   return Motion::ShapeType::Convex; 
                if(s == "Concave")  return Motion::ShapeType::Concave;
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
            c.MassDensity       = n["MassDensity"].as<float>(7.5f);
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

        static bool decode(const Node& n, Motion::MeshComponent& m)
        { 
            if(!n.IsMap()) return false; 
            m.ID        = n["ID"].as<Motion::UUID>(Motion::UniqueIdentity::GetUniqueID()); 
            m.Name      = n["Name"].as<std::string>("Undefined"); 
            m.MinBounds = n["MinBounds"].as<glm::vec3>(glm::vec3(0)); 
            m.MaxBounds = n["MaxBounds"].as<glm::vec3>(glm::vec3(0)); 
            m.MeshIndex = n["MeshIndex"].as<std::uint32_t>(0); 
            return true; 
        }
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

}