/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader_colored.hpp"
#include "wgui/wielementdata.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>

using namespace wgui;

ShaderColored::ShaderColored(prosper::IPrContext &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_colored","wgui/fs_wgui_colored")
{}

ShaderColored::ShaderColored(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderColored::RecordDraw(prosper::ShaderBindState &bindState,prosper::IBuffer &vertBuffer,uint32_t vertCount,const wgui::ElementData &pushConstants,uint32_t testStencilLevel) const
{
	if(
		RecordBindVertexBuffer(bindState,vertBuffer) == false ||
		RecordPushConstants(bindState,pushConstants) == false ||
		RecordSetStencilReference(bindState,testStencilLevel) == false ||
		ShaderGraphics::RecordDraw(bindState,vertCount) == false
	)
		return false;
	return true;
}

void ShaderColored::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddDescriptorSetGroup(pipelineInfo,pipelineIdx,DESCRIPTOR_SET);
	AttachPushConstantRange(pipelineInfo,pipelineIdx,0u,sizeof(wgui::ElementData),prosper::ShaderStageFlags::VertexBit | prosper::ShaderStageFlags::FragmentBit);
}

///////////////////////

ShaderColoredRect::ShaderColoredRect(prosper::IPrContext &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_colored_cheap","wgui/fs_wgui_colored_cheap")
{}

ShaderColoredRect::ShaderColoredRect(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: Shader(context,identifier,vsShader,fsShader,gsShader)
{}

bool ShaderColoredRect::RecordDraw(prosper::ShaderBindState &bindState,const wgui::ElementData &pushConstants,uint32_t testStencilLevel) const
{
	if(
		RecordPushConstants(bindState,pushConstants) == false ||
		RecordBindVertexBuffer(bindState,*WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexBuffer()) == false ||
		RecordSetStencilReference(bindState,testStencilLevel) == false ||
		ShaderGraphics::RecordDraw(bindState,prosper::CommonBufferCache::GetSquareVertexCount()) == false
	)
		return false;
	return true;
}

void ShaderColoredRect::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
	Shader::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddDescriptorSetGroup(pipelineInfo,pipelineIdx,DESCRIPTOR_SET);
	AttachPushConstantRange(pipelineInfo,pipelineIdx,0u,sizeof(wgui::ElementData),prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
}

///////////////////////

ShaderStencil::ShaderStencil(prosper::IPrContext &context,const std::string &identifier)
	: Shader(context,identifier,"wgui/vs_wgui_colored_cheap","wgui/fs_wgui_stencil")
{}

bool ShaderStencil::RecordDraw(prosper::ShaderBindState &bindState,const wgui::ElementData &pushConstants,uint32_t testStencilLevel) const
{
	if(
		RecordPushConstants(bindState,pushConstants) == false ||
		RecordBindVertexBuffer(bindState,*WGUI::GetInstance().GetContext().GetCommonBufferCache().GetSquareVertexBuffer()) == false ||
		RecordSetStencilReference(bindState,testStencilLevel) == false ||
		ShaderGraphics::RecordDraw(bindState,prosper::CommonBufferCache::GetSquareVertexCount()) == false
	)
		return false;
	return true;
}

void ShaderStencil::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	pipelineInfo.ToggleStencilTest(true);
	pipelineInfo.ToggleDynamicState(true,prosper::DynamicState::StencilReference);

	// Disable color write
	pipelineInfo.SetColorBlendAttachmentProperties(
		0,false,prosper::BlendOp::Add,prosper::BlendOp::Add,prosper::BlendFactor::Zero,prosper::BlendFactor::Zero,prosper::BlendFactor::Zero,prosper::BlendFactor::Zero,prosper::ColorComponentFlags::None
	);

	switch(Shader::ToStencilPipelineIndex(pipelineIdx))
	{
	case wgui::StencilPipeline::Test:
	case wgui::StencilPipeline::Increment:
		pipelineInfo.SetStencilTestProperties(
			true,
			prosper::StencilOp::Keep, /* fail */
			prosper::StencilOp::IncrementAndClamp, /* pass */
			prosper::StencilOp::Keep, /* depth fail */
			prosper::CompareOp::Equal,
			~0,~0,
			0
		);
		break;
	case wgui::StencilPipeline::Decrement:
		pipelineInfo.SetStencilTestProperties(
			true,
			prosper::StencilOp::Keep, /* fail */
			prosper::StencilOp::DecrementAndClamp, /* pass */
			prosper::StencilOp::Keep, /* depth fail */
			prosper::CompareOp::Equal,
			~0,~0,
			0
		);
		break;
	}

	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AddDescriptorSetGroup(pipelineInfo,pipelineIdx,DESCRIPTOR_SET);
	AttachPushConstantRange(pipelineInfo,pipelineIdx,0u,sizeof(wgui::ElementData),prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit);
}
