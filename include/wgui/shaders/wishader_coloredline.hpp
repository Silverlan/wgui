/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISHADER_COLOREDLINE_HPP__
#define __WISHADER_COLOREDLINE_HPP__

#include "wishader_colored.hpp"

namespace wgui
{
	struct ElementData;
	class DLLWGUI ShaderColoredLine
		: public ShaderColored
	{
	public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_COLOR;

		ShaderColoredLine(prosper::IPrContext &context,const std::string &identifier);

		bool Draw(const std::shared_ptr<prosper::IBuffer> &vertBuffer,const std::shared_ptr<prosper::IBuffer> &colorBuffer,uint32_t vertCount,float lineWidth,const wgui::ElementData &pushConstants);
	protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
	};
};

#endif
