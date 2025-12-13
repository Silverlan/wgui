// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :shaders.shader;

#undef DrawState

decltype(pragma::gui::shaders::Shader::VERTEX_BINDING_VERTEX) pragma::gui::shaders::Shader::VERTEX_BINDING_VERTEX = {prosper::VertexInputRate::Vertex};
decltype(pragma::gui::shaders::Shader::VERTEX_ATTRIBUTE_POSITION) pragma::gui::shaders::Shader::VERTEX_ATTRIBUTE_POSITION = {VERTEX_BINDING_VERTEX, prosper::CommonBufferCache::GetSquareVertexFormat()};

pragma::gui::shaders::Shader::Shader(prosper::IPrContext &context, const std::string &identifier) : ShaderGraphics(context, identifier, "programs/gui/colored", "programs/gui/colored") { SetPipelineCount(math::to_integral(StencilPipeline::Count) * 2); }

pragma::gui::shaders::Shader::Shader(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader) : ShaderGraphics(context, identifier, vsShader, fsShader, gsShader)
{
	SetPipelineCount(math::to_integral(StencilPipeline::Count) * 2);
}

void pragma::gui::shaders::Shader::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx, bool enableStencilTest)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
	ToggleDynamicScissorState(pipelineInfo, true);
	if(enableStencilTest)
		initialize_stencil_properties(pipelineInfo, pipelineIdx);
	if(IsMsaaPipeline(pipelineIdx))
		pipelineInfo.SetMultisamplingProperties(wGUI::MSAA_SAMPLE_COUNT, 0.f, ~0u);
}

void pragma::gui::shaders::Shader::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { InitializeGfxPipeline(pipelineInfo, pipelineIdx, true); }

size_t pragma::gui::shaders::Shader::GetBaseTypeHashCode() const { return typeid(Shader).hash_code(); }

void pragma::gui::shaders::Shader::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) { get_render_pass(WGUI::GetInstance(), GetContext(), outRenderPass, IsMsaaPipeline(pipelineIdx)); }

pragma::gui::StencilPipeline pragma::gui::shaders::Shader::ToStencilPipelineIndex(uint32_t pipelineIdx, bool *optOutMsaa)
{
	if(pipelineIdx >= math::to_integral(StencilPipeline::Count)) {
		if(optOutMsaa)
			*optOutMsaa = true;
		return static_cast<StencilPipeline>(pipelineIdx - math::to_integral(StencilPipeline::Count));
	}
	if(optOutMsaa)
		*optOutMsaa = false;
	return static_cast<StencilPipeline>(pipelineIdx);
}
uint32_t pragma::gui::shaders::Shader::ToAbsolutePipelineIndex(StencilPipeline pipelineIdx, bool msaa)
{
	if(msaa)
		return math::to_integral(pipelineIdx) + math::to_integral(StencilPipeline::Count);
	return math::to_integral(pipelineIdx);
}

static bool is_msaa_pipeline(uint32_t pipelineIdx) { return pipelineIdx >= pragma::math::to_integral(pragma::gui::StencilPipeline::Count); }
bool pragma::gui::shaders::Shader::IsMsaaPipeline(uint32_t pipelineIdx) { return is_msaa_pipeline(pipelineIdx); }

bool pragma::gui::shaders::Shader::RecordBeginDraw(prosper::ShaderBindState &bindState, DrawState &drawState, uint32_t width, uint32_t height, StencilPipeline pipelineIdx, bool msaa) const
{
	auto idx = ToAbsolutePipelineIndex(pipelineIdx, msaa);
	uint32_t x, y, w, h;
	drawState.GetScissor(x, y, w, h);
	return ShaderGraphics::RecordBindPipeline(bindState, idx) && RecordViewportScissor(bindState, width, height, x, y, w, h, idx);
}

bool pragma::gui::shaders::Shader::RecordSetStencilReference(prosper::ShaderBindState &bindState, uint32_t testStencilLevel) const { return bindState.commandBuffer.RecordSetStencilReference(prosper::StencilFaceFlags::FrontBit, testStencilLevel); }

//////////////////////

prosper::IRenderPass &pragma::gui::shaders::get_render_pass(prosper::IPrContext &context) { return context.GetWindow().GetStagingRenderPass(); }

void pragma::gui::shaders::get_render_pass(WGUI &gui, prosper::IPrContext &context, std::shared_ptr<prosper::IRenderPass> &outRenderPass, bool msaa)
{
	if(msaa)
		outRenderPass = gui.GetMsaaRenderPass().shared_from_this();
	else
		outRenderPass = get_render_pass(context).shared_from_this();
}

void pragma::gui::shaders::initialize_stencil_properties(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	pipelineInfo.ToggleStencilTest(true);
	pipelineInfo.ToggleDynamicState(true, prosper::DynamicState::StencilReference);
	switch(Shader::ToStencilPipelineIndex(pipelineIdx)) {
	case StencilPipeline::Test:
		pipelineInfo.SetStencilTestProperties(true, prosper::StencilOp::Keep, /* fail */
		  prosper::StencilOp::Keep,                                           /* pass */
		  prosper::StencilOp::Keep,                                           /* depth fail */
		  prosper::CompareOp::Equal, ~0, 0, 0);
		break;
	case StencilPipeline::Increment:
		pipelineInfo.SetStencilTestProperties(true, prosper::StencilOp::Keep, /* fail */
		  prosper::StencilOp::IncrementAndClamp,                              /* pass */
		  prosper::StencilOp::Keep,                                           /* depth fail */
		  prosper::CompareOp::Equal, ~0, ~0, 0);
		break;
	case StencilPipeline::Decrement:
		pipelineInfo.SetStencilTestProperties(true, prosper::StencilOp::Keep, /* fail */
		  prosper::StencilOp::DecrementAndClamp,                              /* pass */
		  prosper::StencilOp::Keep,                                           /* depth fail */
		  prosper::CompareOp::Equal, ~0, ~0, 0);

		bool blendingEnabled;
		prosper::BlendOp blendOpColor;
		prosper::BlendOp blendOpAlpha;
		prosper::BlendFactor srcColorBlendFactor;
		prosper::BlendFactor dstColorBlendFactor;
		prosper::BlendFactor srcAlphaBlendFactor;
		prosper::BlendFactor dstAlphaBlendFactor;
		pipelineInfo.GetColorBlendAttachmentProperties(0, &blendingEnabled, &blendOpColor, &blendOpAlpha, &srcColorBlendFactor, &dstColorBlendFactor, &srcAlphaBlendFactor, &dstAlphaBlendFactor, nullptr);
		// Disable color write
		pipelineInfo.SetColorBlendAttachmentProperties(0, blendingEnabled, blendOpColor, blendOpAlpha, srcColorBlendFactor, dstColorBlendFactor, srcAlphaBlendFactor, dstAlphaBlendFactor, prosper::ColorComponentFlags::None);
		break;
	}
}
