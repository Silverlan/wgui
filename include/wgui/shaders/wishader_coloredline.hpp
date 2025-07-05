// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __WISHADER_COLOREDLINE_HPP__
#define __WISHADER_COLOREDLINE_HPP__

#include "wishader_colored.hpp"

namespace wgui {
	struct ElementData;
	class DLLWGUI ShaderColoredLine : public ShaderColored {
	  public:
		static prosper::ShaderGraphics::VertexBinding VERTEX_BINDING_VERTEX;
		static prosper::ShaderGraphics::VertexAttribute VERTEX_ATTRIBUTE_COLOR;

		ShaderColoredLine(prosper::IPrContext &context, const std::string &identifier);

		bool RecordDraw(prosper::ShaderBindState &bindState, const std::shared_ptr<prosper::IBuffer> &vertBuffer, const std::shared_ptr<prosper::IBuffer> &colorBuffer, uint32_t vertCount, float lineWidth, const wgui::ElementData &pushConstants, uint32_t testStencilLevel) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};

#endif
