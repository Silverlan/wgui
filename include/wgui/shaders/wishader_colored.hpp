/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHADER_COLORED_HPP__
#define __WISHADER_COLORED_HPP__

#include "wishader.hpp"

namespace wgui
{
	struct ElementData;
	class DLLWGUI ShaderColored
		: public Shader
	{
	public:
		ShaderColored(prosper::IPrContext &context,const std::string &identifier);
		ShaderColored(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");

		bool RecordDraw(prosper::ShaderBindState &bindState,prosper::IBuffer &vertBuffer,uint32_t vertCount,const wgui::ElementData &pushConstants,uint32_t testStencilLevel=0u) const;
	protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};

	///////////////////////

	class DLLWGUI ShaderColoredRect
		: public Shader
	{
	public:
		ShaderColoredRect(prosper::IPrContext &context,const std::string &identifier);
		ShaderColoredRect(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader="");

		bool RecordDraw(prosper::ShaderBindState &bindState,const ElementData &pushConstants,uint32_t testStencilLevel) const;
	protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};

	///////////////////////

	class DLLWGUI ShaderStencil
		: public Shader
	{
	public:
		ShaderStencil(prosper::IPrContext &context,const std::string &identifier);

		bool RecordDraw(prosper::ShaderBindState &bindState,const ElementData &pushConstants,uint32_t testStencilLevel) const;
	protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};
};

#endif
