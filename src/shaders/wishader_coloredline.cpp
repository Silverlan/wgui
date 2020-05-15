/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader_coloredline.hpp"
#include "wgui/wielementdata.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_command_buffer.hpp>

using namespace wgui;

decltype(ShaderColoredLine::VERTEX_BINDING_VERTEX) ShaderColoredLine::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderColoredLine::VERTEX_ATTRIBUTE_COLOR) ShaderColoredLine::VERTEX_ATTRIBUTE_COLOR = {VERTEX_BINDING_VERTEX,prosper::Format::R32G32B32A32_SFloat};
ShaderColoredLine::ShaderColoredLine(prosper::IPrContext &context,const std::string &identifier)
	: ShaderColored(context,identifier,"wgui/vs_wgui_colored_vertex","wgui/fs_wgui_colored_vertex")
{
	SetBaseShader<ShaderColored>();
}

void ShaderColoredLine::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderColored::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	pipelineInfo.SetPrimitiveTopology(prosper::PrimitiveTopology::LineList);
	pipelineInfo.ToggleDynamicStates(true,{prosper::DynamicState::LineWidth});
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_COLOR);
}

bool ShaderColoredLine::Draw(
	const std::shared_ptr<prosper::IBuffer> &vertBuffer,const std::shared_ptr<prosper::IBuffer> &colorBuffer,
	uint32_t vertCount,float lineWidth,const wgui::ElementData &pushConstants
)
{
	auto drawCmd = GetCurrentCommandBuffer();
	if(drawCmd == nullptr)
		return false;
	drawCmd->RecordSetLineWidth(lineWidth);
	if(
		RecordBindVertexBuffers({vertBuffer.get(),colorBuffer.get()}) == false ||
		RecordPushConstants(pushConstants) == false ||
		RecordDraw(vertCount) == false
	)
		return false;
	return true;
}
