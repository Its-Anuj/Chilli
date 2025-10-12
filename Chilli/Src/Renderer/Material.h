#pragma once

#include "Shader.h"
#include "Buffer.h"
#include "Texture.h"

namespace Chilli
{
	struct Material
	{
	public:
		Material(const std::shared_ptr<GraphicsPipeline>& Shader);
		Material();
		~Material() {}

		bool IsDirty() const { return _Dirty; }

		void SetTexture(const char* Name, const std::shared_ptr<Texture>& Texture);
		void SetUniformBuffer(const char* Name, const std::shared_ptr<UniformBuffer>& BufferHandle);
		void SetShader(const std::shared_ptr<GraphicsPipeline>& Shader);

		void Update();
		
		const std::shared_ptr<struct MaterialBackend>& GetBackend() const { return _Backend; }
		const std::shared_ptr<GraphicsPipeline>& GetShader() const { return _Shader;; }
	private:
		bool _Dirty = false;
		std::shared_ptr<GraphicsPipeline> _Shader;
		std::unordered_map<std::string, std::shared_ptr<UniformBuffer>> _Uniforms;
		std::unordered_map<std::string, std::shared_ptr<Texture>> _Textures;

		std::shared_ptr<struct MaterialBackend> _Backend;
	};

	struct MaterialBackend
	{
	public:
		virtual void Update(const std::shared_ptr<GraphicsPipeline>& Shader,
			const std::unordered_map<std::string, std::shared_ptr<UniformBuffer>>& UBOs,
			const std::unordered_map<std::string, std::shared_ptr<Texture>>& Textures) = 0;
	private:
	};

}
