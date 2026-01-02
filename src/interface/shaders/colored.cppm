// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:shaders.colored;

export import :element_data;
export import :shaders.shader;

export namespace pragma::gui::shaders {
	class DLLWGUI ShaderColored : public Shader {
	  public:
		ShaderColored(prosper::IPrContext &context, const std::string &identifier);
		ShaderColored(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, prosper::IBuffer &vertBuffer, uint32_t vertCount, const ElementData &pushConstants, uint32_t testStencilLevel = 0u) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};

	///////////////////////

	class DLLWGUI ShaderColoredRect : public Shader {
	  public:
		ShaderColoredRect(prosper::IPrContext &context, const std::string &identifier);
		ShaderColoredRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, const ElementData &pushConstants, uint32_t testStencilLevel) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};

	///////////////////////

	class DLLWGUI ShaderStencil : public Shader {
	  public:
		ShaderStencil(prosper::IPrContext &context, const std::string &identifier);

		bool RecordDraw(prosper::ShaderBindState &bindState, const ElementData &pushConstants, uint32_t testStencilLevel) const;
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};
