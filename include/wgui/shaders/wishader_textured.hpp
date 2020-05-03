/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHADER_TEXTURED_HPP__
#define __WISHADER_TEXTURED_HPP__

#include "wishader.hpp"
#include "wgui/wielementdata.hpp"

namespace prosper {class IDescriptorSet;};

namespace wgui
{
	class DLLWGUI ShaderTextured
		: public Shader
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_POSITION;

		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_UV;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

		enum class Channel : uint8_t
		{
			Red = 0,
			Green,
			Blue,
			Alpha
		};

#pragma pack(push,1)
		struct PushConstants
		{
			wgui::ElementData elementData;
			int32_t alphaOnly;
			float lod;
			Channel red;
			Channel green;
			Channel blue;
			Channel alpha;
		};
#pragma pack(pop)

		ShaderTextured(prosper::IPrContext &context,const std::string &identifier);
		ShaderTextured(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");

		bool Draw(
			const std::shared_ptr<prosper::IBuffer> &vertBuffer,const std::shared_ptr<prosper::IBuffer> &uvBuffer,uint32_t vertCount,
			prosper::IDescriptorSet &descSetTexture,const PushConstants &pushConstants
		);
	protected:
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};

	///////////////////////

	class DLLWGUI ShaderTexturedRect
		: public Shader
	{
	public:
#pragma pack(push,1)
		struct PushConstants
		{
			wgui::ElementData elementData;
			int32_t alphaOnly;
			float lod;
			ShaderTextured::Channel red;
			ShaderTextured::Channel green;
			ShaderTextured::Channel blue;
			ShaderTextured::Channel alpha;
		};
#pragma pack(pop)

		ShaderTexturedRect(prosper::IPrContext &context,const std::string &identifier);
		ShaderTexturedRect(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");

		bool Draw(const PushConstants &pushConstants,prosper::IDescriptorSet &descSet);
	protected:
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};
};

#endif
