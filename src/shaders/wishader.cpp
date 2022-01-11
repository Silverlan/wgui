/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/shaders/wishader.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <prosper_window.hpp>
#include <prosper_render_pass.hpp>
#include <prosper_command_buffer.hpp>

using namespace wgui;

decltype(Shader::VERTEX_BINDING_VERTEX) Shader::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(Shader::VERTEX_ATTRIBUTE_POSITION) Shader::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX,prosper::CommonBufferCache::GetSquareVertexFormat()};

decltype(Shader::DESCRIPTOR_SET) Shader::DESCRIPTOR_SET = {
	{
		prosper::DescriptorSetInfo::Binding {
			prosper::DescriptorType::UniformBufferDynamic,
			prosper::ShaderStageFlags::FragmentBit | prosper::ShaderStageFlags::VertexBit
		}
	}
};
Shader::Shader(prosper::IPrContext &context,const std::string &identifier)
	: ShaderGraphics(context,identifier,"wgui/vs_wgui_colored","wgui/fs_wgui_colored")
{
	SetPipelineCount(umath::to_integral(wgui::StencilPipeline::Count) *2);
}

Shader::Shader(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader,const std::string &gsShader)
	: ShaderGraphics(context,identifier,vsShader,fsShader,gsShader)
{
	SetPipelineCount(umath::to_integral(wgui::StencilPipeline::Count) *2);
}

void Shader::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx,bool enableStencilTest)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	ToggleDynamicScissorState(pipelineInfo,true);
	if(enableStencilTest)
		wgui::initialize_stencil_properties(pipelineInfo,pipelineIdx);
	if(IsMsaaPipeline(pipelineIdx))
		pipelineInfo.SetMultisamplingProperties(WGUI::MSAA_SAMPLE_COUNT,0.f,~0u);
}

void Shader::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	InitializeGfxPipeline(pipelineInfo,pipelineIdx,true);
}

size_t Shader::GetBaseTypeHashCode() const {return typeid(Shader).hash_code();}

void Shader::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	wgui::get_render_pass(WGUI::GetInstance(),GetContext(),outRenderPass,IsMsaaPipeline(pipelineIdx));
}

wgui::StencilPipeline Shader::ToStencilPipelineIndex(uint32_t pipelineIdx,bool *optOutMsaa)
{
	if(pipelineIdx >= umath::to_integral(wgui::StencilPipeline::Count))
	{
		if(optOutMsaa)
			*optOutMsaa = true;
		return static_cast<wgui::StencilPipeline>(pipelineIdx -umath::to_integral(wgui::StencilPipeline::Count));
	}
	if(optOutMsaa)
		*optOutMsaa = false;
	return static_cast<StencilPipeline>(pipelineIdx);
}
uint32_t Shader::ToAbsolutePipelineIndex(wgui::StencilPipeline pipelineIdx,bool msaa)
{
	if(msaa)
		return umath::to_integral(pipelineIdx) +umath::to_integral(wgui::StencilPipeline::Count);
	return umath::to_integral(pipelineIdx);
}

static bool is_msaa_pipeline(uint32_t pipelineIdx) {return pipelineIdx >= umath::to_integral(wgui::StencilPipeline::Count);}
bool Shader::IsMsaaPipeline(uint32_t pipelineIdx) {return is_msaa_pipeline(pipelineIdx);}

bool Shader::RecordBeginDraw(prosper::ShaderBindState &bindState,wgui::DrawState &drawState,uint32_t width,uint32_t height,StencilPipeline pipelineIdx,bool msaa) const
{
	auto idx = ToAbsolutePipelineIndex(pipelineIdx,msaa);
	if(ShaderGraphics::RecordBeginDraw(bindState,idx,RecordFlags::None) == false || bindState.commandBuffer.RecordSetViewport(width,height) == false)
		return false;
	uint32_t x,y,w,h;
	drawState.GetScissor(x,y,w,h);
	return bindState.commandBuffer.RecordSetScissor(w,h,x,y);
}

bool Shader::RecordSetStencilReference(prosper::ShaderBindState &bindState,uint32_t testStencilLevel) const
{
	return bindState.commandBuffer.RecordSetStencilReference(prosper::StencilFaceFlags::FrontBit,testStencilLevel);
}

//////////////////////

prosper::IRenderPass &wgui::get_render_pass(prosper::IPrContext &context)
{
	return context.GetWindow().GetStagingRenderPass();
}

void wgui::get_render_pass(WGUI &gui,prosper::IPrContext &context,std::shared_ptr<prosper::IRenderPass> &outRenderPass,bool msaa)
{
	if(msaa)
		outRenderPass = gui.GetMsaaRenderPass().shared_from_this();
	else
		outRenderPass = get_render_pass(context).shared_from_this();
}

void wgui::initialize_stencil_properties(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	pipelineInfo.ToggleStencilTest(true);
	pipelineInfo.ToggleDynamicState(true,prosper::DynamicState::StencilReference);
	switch(Shader::ToStencilPipelineIndex(pipelineIdx))
	{
	case wgui::StencilPipeline::Test:
		pipelineInfo.SetStencilTestProperties(
			true,
			prosper::StencilOp::Keep, /* fail */
			prosper::StencilOp::Keep, /* pass */
			prosper::StencilOp::Keep, /* depth fail */
			prosper::CompareOp::Equal,
			~0,0,
			0
		);
		break;
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

		bool blendingEnabled;
		prosper::BlendOp blendOpColor;
		prosper::BlendOp blendOpAlpha;
		prosper::BlendFactor srcColorBlendFactor;
		prosper::BlendFactor dstColorBlendFactor;
		prosper::BlendFactor srcAlphaBlendFactor;
		prosper::BlendFactor dstAlphaBlendFactor;
		pipelineInfo.GetColorBlendAttachmentProperties(0,&blendingEnabled,&blendOpColor,&blendOpAlpha,&srcColorBlendFactor,&dstColorBlendFactor,&srcAlphaBlendFactor,&dstAlphaBlendFactor,nullptr);
		// Disable color write
		pipelineInfo.SetColorBlendAttachmentProperties(
			0,&blendingEnabled,blendOpColor,blendOpAlpha,srcColorBlendFactor,dstColorBlendFactor,srcAlphaBlendFactor,dstAlphaBlendFactor,prosper::ColorComponentFlags::None
		);
		break;
	}
}
