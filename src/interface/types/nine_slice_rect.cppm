// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.gui:types.nine_slice_rect;

export import :types.segmented_rect;

export namespace pragma::gui::types {
	class DLLWGUI WI9SliceRect : public WIBaseSegmentedRect<WITexturedRect> {
	  public:
		WI9SliceRect();
		void Initialize() override;
		void BindShader(shaders::ShaderTextured &shader, prosper::ShaderBindState &bindState, DrawState &drawState) override;
	  protected:
		void OnMaterialChanged() override;
		material::Material *GetSegmentMaterial() override;
		Vector2 m_textureSize;
		Vector4 m_borderSizes;
	};
};
