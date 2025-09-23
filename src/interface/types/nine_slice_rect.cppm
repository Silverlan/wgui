// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"
#include "mathutil/uvec.h"
#include "material.h"

#undef DrawState

export module pragma.gui:types.nine_slice_rect;

import :shaders.textured;
import :types.rect;

export namespace wgui {
	class DLLWGUI WI9SliceRectSegment : public WITexturedRect {
	  public:
		WI9SliceRectSegment();
		virtual void Initialize() override;
		void SetRenderImageOffset(const Vector2 &offset) { m_renderOffset = offset; }
		void SetRenderImageScale(const Vector2 &scale) { m_renderScale = scale; }
		const Vector2 &GetRenderImageOffset() const { return m_renderOffset; }
		const Vector2 &GetRenderImageScale() const { return m_renderScale; }

		virtual void BindShader(wgui::ShaderTextured &shader, prosper::ShaderBindState &bindState, wgui::DrawState &drawState) override;
	  private:
		Vector2 m_renderOffset {};
		Vector2 m_renderScale {1.f, 1.f};
	};

	class DLLWGUI WI9SliceRect : public WIBase {
	  public:
		enum class Segment : uint8_t {
			TopLeftCorner = 0,
			TopRightCorner,
			BottomLeftCorner,
			BottomRightCorner,
			TopEdge,
			BottomEdge,
			LeftEdge,
			RightEdge,
			Center,
			Count,
		};

		struct NineSlice {
			uint32_t left = 0;
			uint32_t right = 0;
			uint32_t top = 0;
			uint32_t bottom = 0;
		};

		WI9SliceRect();
		virtual void Initialize() override;
		void SetMaterial(const std::string &matPath);
		void SetMaterial(Material &mat);
		Material *GetMaterial();
	  private:
		std::pair<int32_t, int32_t> GetSegmentSize(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const;
		std::pair<int32_t, int32_t> GetSegmentOffset(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const;
		std::tuple<float, float, float, float> GetSegmentAnchor(Segment segment) const;
		void UpdateSegments();
		std::array<WIHandle, umath::to_integral(Segment::Count)> m_segmentElements;
		msys::MaterialHandle m_material;
		NineSlice m_nineSlice;
	};
};
