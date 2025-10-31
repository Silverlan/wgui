// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


module pragma.gui;

import :shaders.colored_line;

using namespace wgui;

decltype(ShaderColoredLine::VERTEX_BINDING_VERTEX) ShaderColoredLine::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(ShaderColoredLine::VERTEX_ATTRIBUTE_COLOR) ShaderColoredLine::VERTEX_ATTRIBUTE_COLOR = {VERTEX_BINDING_VERTEX, prosper::Format::R32G32B32A32_SFloat};
ShaderColoredLine::ShaderColoredLine(prosper::IPrContext &context, const std::string &identifier) : ShaderColored(context, identifier, "programs/gui/colored_vertex", "programs/gui/colored_vertex") { SetBaseShader<ShaderColored>(); }

void ShaderColoredLine::InitializeShaderResources()
{
	ShaderColored::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_COLOR);
}
void ShaderColoredLine::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	ShaderColored::InitializeGfxPipeline(pipelineInfo, pipelineIdx);

	pipelineInfo.SetPrimitiveTopology(prosper::PrimitiveTopology::LineList);
	pipelineInfo.ToggleDynamicStates(true, {prosper::DynamicState::LineWidth});
}

bool ShaderColoredLine::RecordDraw(prosper::ShaderBindState &bindState, const std::shared_ptr<prosper::IBuffer> &vertBuffer, const std::shared_ptr<prosper::IBuffer> &colorBuffer, uint32_t vertCount, float lineWidth, const wgui::ElementData &pushConstants, uint32_t testStencilLevel) const
{
	bindState.commandBuffer.RecordSetLineWidth(lineWidth);
	if(RecordBindVertexBuffers(bindState, {vertBuffer.get(), colorBuffer.get()}) == false || RecordPushConstants(bindState, pushConstants) == false || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, vertCount) == false)
		return false;
	return true;
}
