/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wi9slicerect.hpp"
#include "wgui/types/wirect.h"
#include "wgui/shaders/witexturedsubrect.hpp"
#include <prosper_descriptor_set_group.hpp>

namespace wgui {
	LINK_WGUI_TO_CLASS(WI9SliceRectSegment, WI9SliceRectSegment);
	LINK_WGUI_TO_CLASS(WI9SliceRect, WI9SliceRect);
};
wgui::WI9SliceRectSegment::WI9SliceRectSegment() : WITexturedRect {} {}

void wgui::WI9SliceRectSegment::BindShader(wgui::ShaderTextured &shader, prosper::ShaderBindState &bindState, wgui::DrawState &drawState) { static_cast<wgui::ShaderTexturedSubRect &>(shader).RecordSetImageOffset(bindState, GetRenderImageOffset(), GetRenderImageScale()); }

void wgui::WI9SliceRectSegment::Initialize()
{
	WITexturedRect::Initialize();

	SetShader(*WGUI::GetInstance().GetTexturedSubRectShader());
}

//////////////

wgui::WI9SliceRect::WI9SliceRect() : WIBase {} {}

void wgui::WI9SliceRect::Initialize()
{
	WIBase::Initialize();

	SetSize(512, 512);

	for(size_t i = 0; i < m_segmentElements.size(); ++i) {
		auto *el = WGUI::GetInstance().Create<WI9SliceRectSegment>(this);
		el->SetSize(32, 32);
		m_segmentElements[i] = el->GetHandle();
	}
	UpdateSegments();
}

void wgui::WI9SliceRect::UpdateSegments()
{
	if(!m_material)
		return;
	auto *albedoMap = m_material->GetAlbedoMap();
	if(!albedoMap)
		return;
	uint32_t imgWidth = albedoMap->width;
	uint32_t imgHeight = albedoMap->height;
	uint32_t topLeftCornerWidth = 32;
	uint32_t topLeftCornerHeight = 32;
	Vector2 offset {0.f, 0.f};
	Vector2 scale {};
	scale.x = static_cast<float>(topLeftCornerWidth) / static_cast<float>(imgWidth);
	scale.y = static_cast<float>(topLeftCornerHeight) / static_cast<float>(imgHeight);

	auto updateSegment = [this](Segment segment, const Vector2 &offset, const Vector2 &scale, int32_t x, int32_t y) -> WI9SliceRectSegment * {
		auto *el = static_cast<WI9SliceRectSegment *>(m_segmentElements[umath::to_integral(segment)].get());
		if(!el)
			return nullptr;
		el->SetRenderImageOffset(offset);
		el->SetRenderImageScale(scale);
		el->SetPos(x, y);
		el->GetColorProperty()->Link(*GetColorProperty());
		return el;
	};

	// Top left corner
	auto *elTopLeft = updateSegment(Segment::TopLeftCorner, offset, scale, 0, 0);

	// Top right corner
	offset.x = static_cast<float>(imgWidth - topLeftCornerWidth) / static_cast<float>(imgWidth);
	auto *elTopRight = updateSegment(Segment::TopRightCorner, offset, scale, GetWidth() - 32, 0);

	// Bottom left corner
	offset.x = 0.f;
	offset.y = static_cast<float>(imgHeight - topLeftCornerHeight) / static_cast<float>(imgHeight);
	auto *elBottomLeft = updateSegment(Segment::BottomLeftCorner, offset, scale, 0, GetHeight() - 32);

	// Bottom right corner
	offset.x = static_cast<float>(imgWidth - topLeftCornerWidth) / static_cast<float>(imgWidth);
	offset.y = static_cast<float>(imgHeight - topLeftCornerHeight) / static_cast<float>(imgHeight);
	auto *elBottomRight = updateSegment(Segment::BottomRightCorner, offset, scale, GetWidth() - 32, GetHeight() - 32);

	// Top edge
	offset.x = static_cast<float>(topLeftCornerWidth) / static_cast<float>(imgWidth);
	offset.y = 0.f;
	auto horizontalEdgeScale = scale;
	horizontalEdgeScale.x = 1.f - horizontalEdgeScale.x * 2.f;
	auto *elTop = updateSegment(Segment::TopEdge, offset, horizontalEdgeScale, 32, 0);
	if(elTop)
		elTop->SetWidth(GetWidth() - 64, false);

	// Bottom edge
	offset.y = 1.f - scale.y;
	auto *elBottom = updateSegment(Segment::BottomEdge, offset, horizontalEdgeScale, 32, GetHeight() - 32);
	if(elBottom)
		elBottom->SetWidth(GetWidth() - 64, false);

	// Left edge
	offset.x = 0.f;
	offset.y = static_cast<float>(topLeftCornerHeight) / static_cast<float>(imgHeight);
	auto verticalEdgeScale = scale;
	verticalEdgeScale.y = 1.f - verticalEdgeScale.y * 2.f;
	auto *elLeft = updateSegment(Segment::LeftEdge, offset, verticalEdgeScale, 0, 32);
	if(elLeft)
		elLeft->SetHeight(GetHeight() - 64, false);

	// Right edge
	offset.x = 1.f - scale.x;
	auto *elRight = updateSegment(Segment::RightEdge, offset, verticalEdgeScale, GetWidth() - 32, 32);
	if(elRight)
		elRight->SetHeight(GetHeight() - 64, false);

	// Center
	offset.x = static_cast<float>(topLeftCornerWidth) / static_cast<float>(imgWidth);
	offset.y = static_cast<float>(topLeftCornerHeight) / static_cast<float>(imgHeight);
	auto centerScale = scale;
	centerScale.x = 1.f - centerScale.x * 2.f;
	centerScale.y = 1.f - centerScale.y * 2.f;
	auto *elCenter = updateSegment(Segment::Center, offset, centerScale, 32, 32);
	if(elCenter) {
		elCenter->SetWidth(GetWidth() - 64, false);
		elCenter->SetHeight(GetHeight() - 64, false);
		elCenter->SetPos(32, 32);
	}

	if(elTopLeft)
		elTopLeft->SetAnchor(0.f, 0.f, 0.f, 0.f);
	if(elTopRight)
		elTopRight->SetAnchor(1.f, 0.f, 1.f, 0.f);
	if(elBottomLeft)
		elBottomLeft->SetAnchor(0.f, 1.f, 0.f, 1.f);
	if(elBottomRight)
		elBottomRight->SetAnchor(1.f, 1.f, 1.f, 1.f);
	if(elTop)
		elTop->SetAnchor(0.f, 0.f, 1.f, 0.f);
	if(elBottom)
		elBottom->SetAnchor(0.f, 1.f, 1.f, 1.f);
	if(elLeft)
		elLeft->SetAnchor(0.f, 0.f, 0.f, 1.f);
	if(elRight)
		elRight->SetAnchor(1.f, 0.f, 1.f, 1.f);
	if(elCenter)
		elCenter->SetAnchor(0.f, 0.f, 1.f, 1.f);
}

void wgui::WI9SliceRect::SetMaterial(const std::string &matPath)
{
	auto &matLoadHandler = WGUI::GetInstance().GetMaterialLoadHandler();
	if(!matLoadHandler)
		return;
	auto *mat = matLoadHandler(matPath);
	if(!mat)
		return;
	auto *albedoMap = mat->GetAlbedoMap();
	if(!albedoMap)
		return;
	m_nineSlice = {};
	auto &slice = m_nineSlice;
	auto texWidth = albedoMap->width;
	if(!mat->GetProperty<uint32_t>("leftInset", &slice.left))
		slice.left = texWidth / 4;
	if(!mat->GetProperty<uint32_t>("rightInset", &slice.right))
		slice.right = slice.left;

	auto texHeight = albedoMap->height;
	if(!mat->GetProperty<uint32_t>("topInset", &slice.top))
		slice.top = texHeight / 4;
	if(!mat->GetProperty<uint32_t>("bottomInset", &slice.bottom))
		slice.bottom = slice.top;

	for(auto &hEl : m_segmentElements) {
		if(!hEl.IsValid())
			continue;
		static_cast<WI9SliceRectSegment *>(hEl.get())->SetMaterial(matPath);
	}
	m_materialPath = matPath;
	m_material = mat->GetHandle();

	UpdateSegments();
}
