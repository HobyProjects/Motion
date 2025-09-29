#pragma once

#include "Base.hpp"
#include "Entity.hpp"

namespace Motion
{
    class Importer
    {
        private:
            Importer() = default;
            ~Importer() = default;

            Importer(const Importer&) = delete;
            Importer& operator=(const Importer&) = delete;
            Importer(Importer&&) = delete;
            Importer& operator=(Importer&&) = delete;

        public:
            static std::shared_ptr<Entity> ImportModelAsync(const std::filesystem::path& path, bool shouldExport, const std::string& exportPath);
    };
}