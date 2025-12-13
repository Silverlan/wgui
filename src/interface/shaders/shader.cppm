// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.gui:shaders.shader;

import :draw_state;
import :enums;
export import pragma.prosper;

#undef DrawState

export namespace pragma::gui {
	class WGUI;
	namespace shaders {
		DLLWGUI prosper::IRenderPass &get_render_pass(prosper::IPrContext &context);
		DLLWGUI void get_render_pass(WGUI &gui, prosper::IPrContext &context, std::shared_ptr<prosper::IRenderPass> &outRenderPass, bool msaa);
		DLLWGUI void initialize_stencil_properties(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx);
		class DLLWGUI Shader : public prosper::ShaderGraphics {
		public:
			static VertexBinding VERTEX_BINDING_VERTEX;
			static VertexAttribute VERTEX_ATTRIBUTE_POSITION;
			static StencilPipeline ToStencilPipelineIndex(uint32_t pipelineIdx, bool *optOutMsaa = nullptr);
			static uint32_t ToAbsolutePipelineIndex(StencilPipeline pipelineIdx, bool msaa);
			static bool IsMsaaPipeline(uint32_t pipelineIdx);

			Shader(prosper::IPrContext &context, const std::string &identifier);
			Shader(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");
			bool RecordBeginDraw(prosper::ShaderBindState &bindState, DrawState &drawState, uint32_t width, uint32_t height, StencilPipeline pipelineIdx, bool msaa) const;
			virtual size_t GetBaseTypeHashCode() const override;
			using ShaderGraphics::RecordBeginDraw;
		protected:
			bool RecordSetStencilReference(prosper::ShaderBindState &bindState, uint32_t testStencilLevel) const;
			virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
			virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
			void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx, bool enableStencilTest);
		};
	}
}
