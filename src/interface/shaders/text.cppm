// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include <string>
#include <cinttypes>

#include <memory>

export module pragma.gui:shaders.text;

export import :element_data;
export import :shaders.shader;

export namespace wgui {
	class DLLWGUI ShaderText : public Shader {
	  public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_GLYPH;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_GLYPH_INDEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_GLYPH_BOUNDS;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;
		static prosper::DescriptorSetInfo DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER;

#pragma pack(push, 1)
		struct PushConstants {
			float widthScale;
			float heightScale;
			uint32_t glyphMapWidth;
			uint32_t glyphMapHeight;
			uint32_t maxGlyphBitmapWidth;
			uint32_t maxGlyphBitmapHeight;
			uint32_t yOffset;
			uint32_t numGlyphsPerRow;
		};
#pragma pack(pop)

		ShaderText(prosper::IPrContext &context, const std::string &identifier);
		ShaderText(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const;
		using Shader::RecordBeginDraw;
		using ShaderGraphics::RecordBeginDraw;
	  protected:
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};

	///////////////////////

	class DLLWGUI ShaderTextRect : public ShaderText {
	  public:
#pragma pack(push, 1)
		struct PushConstants {
			wgui::ElementData elementData;
			ShaderText::PushConstants fontInfo;
		};
#pragma pack(pop)

		ShaderTextRect(prosper::IPrContext &context, const std::string &identifier);
		ShaderTextRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const;
	  protected:
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};

	///////////////////////

	class DLLWGUI ShaderTextRectColor : public ShaderTextRect {
	  public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_COLOR;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_COLOR;
		ShaderTextRectColor(prosper::IPrContext &context, const std::string &identifier);
		ShaderTextRectColor(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &glyphBoundsIndexBuffer, prosper::IBuffer &colorBuffer, prosper::IDescriptorSet &descTextureSet, const PushConstants &pushConstants, uint32_t instanceCount, uint32_t testStencilLevel) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};
