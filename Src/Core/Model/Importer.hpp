#pragma once

#include "Base.hpp"
#include "Model.hpp"

#include "TaskManager.hpp"

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
            static TaskManager::TaskId ImportModelAsync(const std::filesystem::path& path, bool shouldExport, const std::string& exportPath, std::function<void(std::shared_ptr<StaticMesh>)> onCompleted, std::function<void(std::int32_t)> onProgress);
    };
}