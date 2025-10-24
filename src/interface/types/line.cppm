// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "wgui/wguidefinitions.h"

#undef DrawState

export module pragma.gui:types.line;

import :buffer_base;
import :draw_info;
import :draw_state;
export import pragma.util;

export {
	class DLLWGUI WILineBase {
	private:
		unsigned int m_lineWidth;
	protected:
		WILineBase();
	public:
		unsigned int GetLineWidth();
		void SetLineWidth(unsigned int width);
	};

	class DLLWGUI WILine : public WIBufferBase, WILineBase {
	private:
		util::PVector2iProperty m_posStart = nullptr;
		util::PVector2iProperty m_posEnd = nullptr;
		Color m_colStart;
		Color m_colEnd;
		float m_dot;
		std::shared_ptr<prosper::IBuffer> m_bufColor = nullptr;
		void UpdateColorBuffer();
	public:
		WILine();
		virtual ~WILine() override;
		virtual void SetColor(float r, float g, float b, float a = 1.f) override;
		void SetLineWidth(unsigned int width);
		unsigned int GetLineWidth();
		std::pair<Vector2i, Vector2i> GetNormalizedLineBounds() const;

		const util::PVector2iProperty &GetStartPosProperty() const;
		const util::PVector2iProperty &GetEndPosProperty() const;

		Vector2i &GetStartPos() const;
		void SetStartPos(Vector2i pos);
		void SetStartPos(int x, int y);
		Vector2i &GetEndPos() const;
		void SetEndPos(Vector2i pos);
		void SetEndPos(int x, int y);
		void SetStartColor(const Color &col);
		void SetEndColor(const Color &col);
		const Color &GetStartColor() const;
		const Color &GetEndColor() const;
		virtual void SizeToContents(bool x = true, bool y = true) override;
		virtual unsigned int GetVertexCount() override;
		virtual void Render(const wgui::DrawInfo &drawInfo, wgui::DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale = {1.f, 1.f}, uint32_t testStencilLevel = 0u, wgui::StencilPipeline stencilPipeline = wgui::StencilPipeline::Test) override;
		virtual Mat4 GetTransformPose(const Vector2i &origin, int w, int h, const Mat4 &poseParent, const Vector2 &scale = {1.f, 1.f}) const override;
	};
};
