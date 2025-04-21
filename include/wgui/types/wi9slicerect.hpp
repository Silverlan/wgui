/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WI9SLICERECT_HPP__
#define __WI9SLICERECT_HPP__

#include "wirect.h"

namespace wgui {
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
		const std::string &GetMaterial() const { return m_materialPath; }
	  private:
		std::pair<int32_t, int32_t> GetSegmentSize(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const;
		std::pair<int32_t, int32_t> GetSegmentOffset(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const;
		std::tuple<float, float, float, float> GetSegmentAnchor(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const;
		void UpdateSegments();
		std::array<WIHandle, umath::to_integral(Segment::Count)> m_segmentElements;
		std::string m_materialPath;
		msys::MaterialHandle m_material;
		NineSlice m_nineSlice;
	};
};

#endif
