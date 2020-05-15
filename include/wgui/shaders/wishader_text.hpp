/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHADER_TEXT_HPP__
#define __WISHADER_TEXT_HPP__

#include "wishader.hpp"
#include "wgui/wielementdata.hpp"

class FontInfo;
namespace wgui
{
	class DLLWGUI ShaderText
		: public Shader
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_GLYPH;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_GLYPH_INDEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_GLYPH_BOUNDS;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;
		static prosper::DescriptorSetInfo DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER;

#pragma pack(push,1)
		struct PushConstants
		{
			float widthScale;
			float heightScale;
			uint32_t glyphMapWidth;
			uint32_t glyphMapHeight;
			uint32_t maxGlyphBitmapWidth;
			uint32_t yOffset;
		};
#pragma pack(pop)

		ShaderText(prosper::IPrContext &context,const std::string &identifier);
		ShaderText(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");

		bool Draw(
			prosper::IBuffer &glyphBoundsIndexBuffer,
			prosper::IDescriptorSet &descTextureSet,const PushConstants &pushConstants,
			uint32_t instanceCount
		);
		using Shader::BeginDraw;
		using ShaderGraphics::BeginDraw;
	protected:
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};

	///////////////////////

	class DLLWGUI ShaderTextRect
		: public ShaderText
	{
	public:
#pragma pack(push,1)
		struct PushConstants
		{
			wgui::ElementData elementData;
			ShaderText::PushConstants fontInfo;
			int32_t alphaOnly;
		};
#pragma pack(pop)

		ShaderTextRect(prosper::IPrContext &context,const std::string &identifier);
		ShaderTextRect(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");

		bool Draw(
			prosper::IBuffer &glyphBoundsIndexBuffer,
			prosper::IDescriptorSet &descTextureSet,const PushConstants &pushConstants,
			uint32_t instanceCount
		);
	protected:
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};

	///////////////////////

	class DLLWGUI ShaderTextRectColor
		: public ShaderTextRect
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_COLOR;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_COLOR;
		ShaderTextRectColor(prosper::IPrContext &context,const std::string &identifier);
		ShaderTextRectColor(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");

		bool Draw(
			prosper::IBuffer &glyphBoundsIndexBuffer,prosper::IBuffer &colorBuffer,
			prosper::IDescriptorSet &descTextureSet,const PushConstants &pushConstants,
			uint32_t instanceCount
		);
	protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};
};

#endif
