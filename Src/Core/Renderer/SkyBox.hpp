#pragma once

#include "Shaders.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"

namespace Motion::Core
{
	class ISkyBox
	{
		public:
			ISkyBox() = default;
			~ISkyBox() = default;

			virtual void LoadHDR(const std::filesystem::path &pathHDR) = 0;
			virtual void Render(const glm::mat4& viewMatrix, const glm::mat4 projectionMatrix) = 0;
			virtual std::shared_ptr<ITexture> GetEnvironmentCube() const = 0;

		protected:
		    virtual void CreateSkyBoxCube() = 0;
			virtual void CreateCubeMapFromHDR() = 0;
	};
}