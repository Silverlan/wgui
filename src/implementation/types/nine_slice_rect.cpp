// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :types.nine_slice_rect;

#undef DrawState

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

std::tuple<float, float, float, float> wgui::WI9SliceRect::GetSegmentAnchor(Segment segment) const
{
	switch(segment) {
	case Segment::TopLeftCorner:
		return {0.f, 0.f, 0.f, 0.f};
	case Segment::TopRightCorner:
		return {1.f, 0.f, 1.f, 0.f};
	case Segment::BottomLeftCorner:
		return {0.f, 1.f, 0.f, 1.f};
	case Segment::BottomRightCorner:
		return {1.f, 1.f, 1.f, 1.f};
	case Segment::TopEdge:
		return {0.f, 0.f, 1.f, 0.f};
	case Segment::BottomEdge:
		return {0.f, 1.f, 1.f, 1.f};
	case Segment::LeftEdge:
		return {0.f, 0.f, 0.f, 1.f};
	case Segment::RightEdge:
		return {1.f, 0.f, 1.f, 1.f};
	case Segment::Center:
		return {0.f, 0.f, 1.f, 1.f};
	}
	return {0.f, 0.f, 0.f, 0.f};
}

std::pair<int32_t, int32_t> wgui::WI9SliceRect::GetSegmentOffset(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const
{
	uint32_t x = 0;
	switch(segment) {
	case Segment::TopLeftCorner:
	case Segment::LeftEdge:
	case Segment::BottomLeftCorner:
		x = 0;
		break;
	case Segment::TopRightCorner:
	case Segment::RightEdge:
	case Segment::BottomRightCorner:
		x = imgWidth - m_nineSlice.right;
		break;
	case Segment::TopEdge:
	case Segment::BottomEdge:
	case Segment::Center:
		x = m_nineSlice.left;
		break;
	}

	uint32_t y = 0;
	switch(segment) {
	case Segment::TopLeftCorner:
	case Segment::TopEdge:
	case Segment::TopRightCorner:
		y = 0;
		break;
	case Segment::BottomLeftCorner:
	case Segment::BottomEdge:
	case Segment::BottomRightCorner:
		y = imgHeight - m_nineSlice.bottom;
		break;
	case Segment::LeftEdge:
	case Segment::RightEdge:
	case Segment::Center:
		y = m_nineSlice.top;
		break;
	}
	return {x, y};
}

std::pair<int32_t, int32_t> wgui::WI9SliceRect::GetSegmentSize(Segment segment, uint32_t imgWidth, uint32_t imgHeight) const
{
	uint32_t w = 0;
	switch(segment) {
	case Segment::TopLeftCorner:
	case Segment::BottomLeftCorner:
	case Segment::LeftEdge:
		w = m_nineSlice.left;
		break;
	case Segment::TopRightCorner:
	case Segment::BottomRightCorner:
	case Segment::RightEdge:
		w = m_nineSlice.right;
		break;
	case Segment::TopEdge:
	case Segment::BottomEdge:
	case Segment::Center:
		w = imgWidth - (m_nineSlice.left + m_nineSlice.right);
		break;
	}

	uint32_t h = 0;
	switch(segment) {
	case Segment::TopLeftCorner:
	case Segment::TopRightCorner:
	case Segment::TopEdge:
		h = m_nineSlice.top;
		break;
	case Segment::BottomLeftCorner:
	case Segment::BottomRightCorner:
	case Segment::BottomEdge:
		h = m_nineSlice.bottom;
		break;
	case Segment::LeftEdge:
	case Segment::RightEdge:
	case Segment::Center:
		h = imgHeight - (m_nineSlice.top + m_nineSlice.bottom);
		break;
	}
	return {w, h};
}

void wgui::WI9SliceRect::UpdateSegments()
{
	if(!m_material)
		return;
	auto *albedoMap = m_material->GetAlbedoMap();
	if(!albedoMap || !albedoMap->texture)
		return;
	auto &tex = *static_cast<Texture *>(albedoMap->texture.get());
	uint32_t imgWidth = tex.GetWidth();
	uint32_t imgHeight = tex.GetHeight();
	Vector2 offset {0.f, 0.f};
	Vector2 scale {0.f, 0.f};

	auto origWidth = GetWidth();
	auto origHeight = GetHeight();
	SetSize(imgWidth, imgHeight);

	for(auto &hEl : m_segmentElements) {
		if(!hEl.IsValid())
			continue;
		hEl->ClearAnchor();
		hEl->SetSize(0, 0);
	}

	auto updateOffsetScale = [&](Segment segment) {
		auto [x, y] = GetSegmentOffset(segment, imgWidth, imgHeight);
		auto [w, h] = GetSegmentSize(segment, imgWidth, imgHeight);

		// Half pixel offset to prevent texture bleeding
		offset.x = (x + 0.5f) / float(imgWidth);
		offset.y = (y + 0.5f) / float(imgHeight);
		scale.x = (w - 1.0f) / float(imgWidth);
		scale.y = (h - 1.0f) / float(imgHeight);
	};

	auto updateSegment = [this, imgWidth, imgHeight](Segment segment, const Vector2 &offset, const Vector2 &scale, int32_t x, int32_t y) -> WI9SliceRectSegment * {
		auto *el = static_cast<WI9SliceRectSegment *>(m_segmentElements[umath::to_integral(segment)].get());
		if(!el)
			return nullptr;
		auto [w, h] = GetSegmentSize(segment, imgWidth, imgHeight);
		el->SetRenderImageOffset(offset);
		el->SetRenderImageScale(scale);
		el->SetSize(w, h);
		el->SetPos(x, y);
		el->GetColorProperty()->Link(*GetColorProperty());
		return el;
	};

	// Top left corner
	updateOffsetScale(Segment::TopLeftCorner);
	auto *elTopLeft = updateSegment(Segment::TopLeftCorner, offset, scale, 0, 0);

	// Top right corner
	updateOffsetScale(Segment::TopRightCorner);
	auto *elTopRight = updateSegment(Segment::TopRightCorner, offset, scale, GetWidth() - m_nineSlice.right, 0);

	// Bottom left corner
	updateOffsetScale(Segment::BottomLeftCorner);
	auto *elBottomLeft = updateSegment(Segment::BottomLeftCorner, offset, scale, 0, GetHeight() - m_nineSlice.bottom);

	// Bottom right corner
	updateOffsetScale(Segment::BottomRightCorner);
	auto *elBottomRight = updateSegment(Segment::BottomRightCorner, offset, scale, GetWidth() - m_nineSlice.right, GetHeight() - m_nineSlice.bottom);

	// Top edge
	updateOffsetScale(Segment::TopEdge);
	auto *elTop = updateSegment(Segment::TopEdge, offset, scale, m_nineSlice.left, 0);
	if(elTop)
		elTop->SetWidth(GetWidth() - (m_nineSlice.left + m_nineSlice.right), false);

	// Bottom edge
	updateOffsetScale(Segment::BottomEdge);
	auto *elBottom = updateSegment(Segment::BottomEdge, offset, scale, m_nineSlice.left, GetHeight() - m_nineSlice.bottom);
	if(elBottom)
		elBottom->SetWidth(GetWidth() - (m_nineSlice.left + m_nineSlice.right), false);

	// Left edge
	updateOffsetScale(Segment::LeftEdge);
	auto *elLeft = updateSegment(Segment::LeftEdge, offset, scale, 0, m_nineSlice.top);
	if(elLeft)
		elLeft->SetHeight(GetHeight() - (m_nineSlice.top + m_nineSlice.bottom), false);

	// Right edge
	updateOffsetScale(Segment::RightEdge);
	auto *elRight = updateSegment(Segment::RightEdge, offset, scale, GetWidth() - m_nineSlice.right, m_nineSlice.top);
	if(elRight)
		elRight->SetHeight(GetHeight() - (m_nineSlice.top + m_nineSlice.bottom), false);

	// Center
	updateOffsetScale(Segment::Center);
	auto *elCenter = updateSegment(Segment::Center, offset, scale, m_nineSlice.left, m_nineSlice.top);
	if(elCenter) {
		elCenter->SetWidth(GetWidth() - (m_nineSlice.left + m_nineSlice.right), false);
		elCenter->SetHeight(GetHeight() - (m_nineSlice.top + m_nineSlice.bottom), false);
		elCenter->SetPos(m_nineSlice.left, m_nineSlice.top);
	}

	for(size_t i = 0; i < m_segmentElements.size(); ++i) {
		auto &hEl = m_segmentElements[i];
		if(!hEl.IsValid())
			continue;
		auto segment = static_cast<Segment>(i);
		auto [xAnchor, yAnchor, xScale, yScale] = GetSegmentAnchor(segment);
		hEl->SetAnchor(xAnchor, yAnchor, xScale, yScale);
	}
	SetSize(origWidth, origHeight);
}

Material *wgui::WI9SliceRect::GetMaterial() { return m_material.get(); }

void wgui::WI9SliceRect::SetMaterial(Material &mat)
{
	if(&mat == m_material.get())
		return;
	auto *albedoMap = mat.GetAlbedoMap();
	if(!albedoMap || !albedoMap->texture)
		return;
	auto &tex = *static_cast<Texture *>(albedoMap->texture.get());
	m_nineSlice = {};
	auto &slice = m_nineSlice;
	auto texWidth = tex.GetWidth();
	if(!mat.GetProperty<uint32_t>("9slice/leftInset", &slice.left))
		slice.left = texWidth / 4;
	if(!mat.GetProperty<uint32_t>("9slice/rightInset", &slice.right))
		slice.right = slice.left;

	auto texHeight = tex.GetHeight();
	if(!mat.GetProperty<uint32_t>("9slice/topInset", &slice.top))
		slice.top = texHeight / 4;
	if(!mat.GetProperty<uint32_t>("9slice/bottomInset", &slice.bottom))
		slice.bottom = slice.top;

	for(auto &hEl : m_segmentElements) {
		if(!hEl.IsValid())
			continue;
		static_cast<WI9SliceRectSegment *>(hEl.get())->SetMaterial(&mat);
	}
	m_material = mat.GetHandle();

	UpdateSegments();
}

void wgui::WI9SliceRect::SetMaterial(const std::string &matPath)
{
	auto &matLoadHandler = WGUI::GetInstance().GetMaterialLoadHandler();
	if(!matLoadHandler)
		return;
	auto *mat = matLoadHandler(matPath);
	if(!mat)
		return;
	SetMaterial(*mat);
}
