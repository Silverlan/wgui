// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"


export module pragma.gui:shaders.textured;

export import :element_data;
export import :shaders.shader;

export namespace wgui {
	class DLLWGUI ShaderTextured : public Shader {
	  public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;

		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_UV;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

		enum class Channel : uint8_t { Red = 0, Green, Blue, Alpha };

#pragma pack(push, 1)
		struct PushConstants {
			wgui::ElementData elementData;
			int32_t alphaOnly;
			float lod;
			Channel red;
			Channel green;
			Channel blue;
			Channel alpha;
		};
#pragma pack(pop)

		ShaderTextured(prosper::IPrContext &context, const std::string &identifier);
		ShaderTextured(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, const std::shared_ptr<prosper::IBuffer> &vertBuffer, const std::shared_ptr<prosper::IBuffer> &uvBuffer, uint32_t vertCount, prosper::IDescriptorSet &descSetTexture, const PushConstants &pushConstants,
		  uint32_t testStencilLevel) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
		virtual void InitializePushConstantRanges();
	};

	///////////////////////

	class DLLWGUI ShaderTexturedRectExpensive : public Shader {
	  public:
#pragma pack(push, 1)
		struct PushConstants {
			wgui::ElementData elementData;
			int32_t alphaOnly;
			float lod;
			ShaderTextured::Channel red;
			ShaderTextured::Channel green;
			ShaderTextured::Channel blue;
			ShaderTextured::Channel alpha;
			AlphaMode alphaMode;
			float alphaCutoff;
		};
		static_assert(sizeof(AlphaMode) == sizeof(uint32_t));
#pragma pack(pop)

		ShaderTexturedRectExpensive(prosper::IPrContext &context, const std::string &identifier);
		ShaderTexturedRectExpensive(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, const PushConstants &pushConstants, prosper::IDescriptorSet &descSet, uint32_t testStencilLevel) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};

	///////////////////////

	class DLLWGUI ShaderTexturedRect : public Shader {
	  public:
#pragma pack(push, 1)
		struct PushConstants {
			wgui::ElementData elementData;
			int32_t alphaOnly;
			float lod;
			ShaderTextured::Channel red;
			ShaderTextured::Channel green;
			ShaderTextured::Channel blue;
			ShaderTextured::Channel alpha;
		};
#pragma pack(pop)

		ShaderTexturedRect(prosper::IPrContext &context, const std::string &identifier);
		ShaderTexturedRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, const PushConstants &pushConstants, prosper::IDescriptorSet &descSet, uint32_t testStencilLevel) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};
