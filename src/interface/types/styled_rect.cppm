// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.styled_rect;

export import :handle;
export import :shaders.colored;
export import :types.base;
export import :types.shape;

export namespace pragma::gui::types {
	class DLLWGUI StyledRect : public WIBase {
	public:
		static void clear_style_cache_buffer(prosper::IPrContext &context);
		using GradientType = shaders::StyledRect::Style::GradientType;

		struct SharedStyleData {
			std::shared_ptr<prosper::IBuffer> buffer;
			std::shared_ptr<prosper::IDescriptorSetGroup> dsg;
			bool invalidated = false;
		};

		struct ColorStop {
			Color color = colors::White;
			float position = 0.f;
		};

		StyledRect();
		~StyledRect() override = default;
		void DoUpdate() override;
		void Initialize() override;
		void OnRemove() override;
		void Render(const DrawInfo &drawInfo, DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale, uint32_t testStencilLevel = 0u, StencilPipeline stencilPipeline = StencilPipeline::Test) override;

		void ClearGradient();
		void SetLinearGradient(const Color &startColor, const Color &endColor, const Vector2 &startPos, const Vector2 &endPos);
		void SetGradient(std::span<const ColorStop> stops, const Vector2 &startPos, const Vector2 &endPos, GradientType type=GradientType::Linear);
		void SetGradientStops(std::span<const ColorStop> stops);
		void SetGradientType(GradientType type);

		void SetBorderColor(const Color &color);
		void SetCornerRadii(const Vector4 &radii);
		void SetGradientStart(const Vector2 &start);
		void SetGradientEnd(const Vector2 &end);
		void SetBorderThickness(float thickness);

		GradientType GetGradientType() const;
		const std::array<ColorStop,4> &GetGradientStops() const;
		const Color &GetBorderColor() const;
		const Vector4 &GetCornerRadii() const;
		const Vector2 &GetGradientStart() const;
		const Vector2 &GetGradientEnd() const;
		float GetBorderThickness() const;
	private:
		static shaders::StyledRect *GetShader();
		void UpdateDescriptorSet();
		void SetStyleDataDirty();
		[[nodiscard]] shaders::StyledRect::Style ToStyleData() const;

		GradientType m_gradientType;
		std::array<ColorStop, 4> m_gradientStops;
		uint32_t m_numGradientStops = 0;
		Color m_borderColor = colors::White;
		Vector4 m_cornerRadii = {0.f,0.f,0.f,0.f};
		Vector2 m_gradientStart = {0.f,0.f};
		Vector2 m_gradientEnd = {0.f,0.f};
		float m_borderThickness = 0.f;

		bool m_styleDirty = true;

		std::shared_ptr<SharedStyleData> m_sharedData = nullptr;
	};
}