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

	class DLLWGUI StyledRect : public Shader {
	  public:
		static VertexBinding VERTEX_BINDING_VERTEX;
		static VertexAttribute VERTEX_ATTRIBUTE_POSITION;

		static VertexBinding VERTEX_BINDING_UV;
		static VertexAttribute VERTEX_ATTRIBUTE_UV;

		static prosper::DescriptorSetInfo DESCRIPTOR_SET_STYLE;

#pragma pack(push, 1)
		struct Style {
			enum class GradientType : int32_t {
				Linear = 0,
				Radial,

				Count,
			};

			enum class GradientColorMode : int32_t { Solid = 1, Gradient2 = 2, Gradient3 = 3, Gradient4 = 4 };

			std::array<Vector4, 4> gradientColors {Vector4 {1.f, 1.f, 1.f, 1.f}, Vector4 {1.f, 1.f, 1.f, 1.f}, Vector4 {1.f, 1.f, 1.f, 1.f}, Vector4 {1.f, 1.f, 1.f, 1.f}};
			Vector4 gradientColorStops {0.f, 0.f, 0.f, 0.f};
			Vector4 borderColor {0.f, 0.f, 0.f, 1.f};
			Vector4 cornerRadii {0.f, 0.f, 0.f, 0.f};
			Vector2 gradientStart {0.f, 0.f};
			Vector2 gradientEnd {1.f, 1.f};
			float borderThickness = 0.f;
			GradientColorMode gradientColorMode = GradientColorMode::Solid;
			GradientType gradientType = GradientType::Linear;

			bool operator==(const Style &) const = default;
		  private:
			float _padding = 0.f;
		};
#pragma pack(pop)

		StyledRect(prosper::IPrContext &context, const std::string &identifier);
		StyledRect(prosper::IPrContext &context, const std::string &identifier, const std::string &vsShader, const std::string &fsShader, const std::string &gsShader = "");

		bool RecordDraw(prosper::ShaderBindState &bindState, const ElementData &pushConstants, prosper::IDescriptorSet &style, uint32_t testStencilLevel) const;
	  protected:
		void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		void InitializeShaderResources() override;
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
export namespace std {
	template<>
	struct hash<pragma::gui::shaders::StyledRect::Style> {
		std::size_t operator()(const pragma::gui::shaders::StyledRect::Style &s) const
		{
			std::size_t seed = 0;

			for(const auto &color : s.gradientColors)
				pragma::util::hash_combine(seed, color);

			pragma::util::hash_combine(seed, s.gradientColorStops);
			pragma::util::hash_combine(seed, s.borderColor);
			pragma::util::hash_combine(seed, s.cornerRadii);
			pragma::util::hash_combine(seed, s.gradientStart);
			pragma::util::hash_combine(seed, s.gradientEnd);
			pragma::util::hash_combine(seed, s.borderThickness);

			pragma::util::hash_combine(seed, pragma::math::to_integral(s.gradientColorMode));
			pragma::util::hash_combine(seed, pragma::math::to_integral(s.gradientType));
			return seed;
		}
	};
}
