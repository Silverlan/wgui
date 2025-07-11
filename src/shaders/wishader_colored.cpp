// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader_colored.hpp"
#include "wgui/wielementdata.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <shader/prosper_shader_t.hpp>
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>

using namespace wgui;

ShaderColored::ShaderColored(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/colored", "programs/gui/colored") {}

ShaderColored::ShaderColored(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

bool ShaderColored::RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &vertBuffer, uint32_t vertCount, const wgui::ElementData &pushConstants, uint32_t testStencilLevel) const
{
	if(RecordBindVertexBuffer(bindState, vertBuffer) == false || RecordPushConstants(bindState, pushConstants) == false || RecordSetStencilReference(bindState, testStencilLevel) == false || ShaderGraphics::RecordDraw(bindState, vertCount) == false)
		return false;
	return true;
}

void ShaderColored::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(0u, sizeof(wgui::ElementData), prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

void ShaderColored::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

///////////////////////

ShaderColoredRect::ShaderColoredRect(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/colored_cheap", "programs/gui/colored_cheap") {}

ShaderColoredRect::ShaderColoredRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : Shader(context, identifier, vsShader, fsShader, gsShader) {}

bool ShaderColoredRect::RecordDraw(prosper::ShaderBindState &bindState, const wgui::ElementData &pushConstants, uint32_t testStencilLevel) const
{
	if(RecordPushConstants(bindState, pushConstants) == false || RecordBindVertexBuffer(bindState, *WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexBuffer()) == false || RecordSetStencilReference(bindState, testStencilLevel) == false
	  || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount()) == false)
		return false;
	return true;
}

void ShaderColoredRect::InitializeShaderResources()
{
	Shader::InitializeShaderResources();

	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(0u, sizeof(wgui::ElementData), prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
}
void ShaderColoredRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

///////////////////////

ShaderStencil::ShaderStencil(prosper::IPrContext &context, const std::string &identifier) : Shader(context, identifier, "programs/gui/colored_cheap", "programs/gui/stencil") {}

bool ShaderStencil::RecordDraw(prosper::ShaderBindState &bindState, const wgui::ElementData &pushConstants, uint32_t testStencilLevel) const
{
	if(RecordPushConstants(bindState, pushConstants) == false || RecordBindVertexBuffer(bindState, *WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexBuffer()) == false || RecordSetStencilReference(bindState, testStencilLevel) == false
	  || ShaderGraphics::RecordDraw(bindState, prosper::CommonBufferCache::GetSquareVertexCount()) == false)
		return false;
	return true;
}

void ShaderStencil::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	pipelineInfo.ToggleStencilTest(true);
	pipelineInfo.ToggleDynamicState(true, prosper::DynamicState::StencilReference);

	// Disable color write
	pipelineInfo.SetColorBlendAttachmentProperties(0, false, prosper::BlendOp::Add, prosper::BlendOp::Add, prosper::BlendFactor::Zero, prosper::BlendFactor::Zero, prosper::BlendFactor::Zero, prosper::BlendFactor::Zero, prosper::ColorComponentFlags::None);

	switch(Shader::ToStencilPipelineIndex(pipelineIdx)) {
	case wgui::StencilPipeline::Test:
	case wgui::StencilPipeline::Increment:
		pipelineInfo.SetStencilTestProperties(true, prosper::StencilOp::Keep, /* fail */
		  prosper::StencilOp::IncrementAndClamp,                              /* pass */
		  prosper::StencilOp::Keep,                                           /* depth fail */
		  prosper::CompareOp::Equal, ~0, ~0, 0);
		break;
	case wgui::StencilPipeline::Decrement:
		pipelineInfo.SetStencilTestProperties(true, prosper::StencilOp::Keep, /* fail */
		  prosper::StencilOp::DecrementAndClamp,                              /* pass */
		  prosper::StencilOp::Keep,                                           /* depth fail */
		  prosper::CompareOp::Equal, ~0, ~0, 0);
		break;
	}
}

void ShaderStencil::InitializeShaderResources()
{
	AddVertexAttribute(VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(0u, sizeof(wgui::ElementData), prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
}
