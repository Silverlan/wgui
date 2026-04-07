// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.segmented_rect;

pragma::gui::types::WISegmentedRectSegment::WISegmentedRectSegment() : WITexturedRect {} {}

void pragma::gui::types::WISegmentedRectSegment::BindShader(shaders::ShaderTextured &shader, prosper::ShaderBindState &bindState, DrawState &drawState) { static_cast<shaders::ShaderTexturedSubRect &>(shader).RecordSetImageOffset(bindState, GetRenderImageOffset(), GetRenderImageScale()); }

void pragma::gui::types::WISegmentedRectSegment::Initialize()
{
	WITexturedRect::Initialize();

	SetShader(*WGUI::GetInstance().GetShader<shaders::ShaderType::TexturedSubRect>());
}

//////////////

pragma::gui::types::WISegmentedRect::WISegmentedRect() : WIBaseSegmentedRect<WIBase> {} {}

void pragma::gui::types::WISegmentedRect::Initialize()
{
	WIBaseSegmentedRect::Initialize();

	for(size_t i = 0; i < m_segmentElements.size(); ++i) {
		auto *el = WGUI::GetInstance().Create<WISegmentedRectSegment>(this);
		el->SetSize(32, 32);
		m_segmentElements[i] = el->GetHandle();
	}
	UpdateSegments();
}

void pragma::gui::types::WISegmentedRect::UpdateSegments()
{
	uint32_t imgWidth;
	uint32_t imgHeight;
	if(!GetImageSize(imgWidth, imgHeight))
		return;
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

	auto updateSegment = [this, imgWidth, imgHeight](Segment segment, const Vector2 &offset, const Vector2 &scale, int32_t x, int32_t y) -> WISegmentedRectSegment * {
		auto *el = static_cast<WISegmentedRectSegment *>(m_segmentElements[math::to_integral(segment)].get());
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

pragma::material::Material *pragma::gui::types::WISegmentedRect::GetSegmentMaterial() { return GetMaterial(); }

void pragma::gui::types::WISegmentedRect::UpdateMaterial(material::Material &mat)
{
	WIBaseSegmentedRect<WIBase>::UpdateMaterial(mat);
	for(auto &hEl : m_segmentElements) {
		if(!hEl.IsValid())
			continue;
		static_cast<WISegmentedRectSegment *>(hEl.get())->SetMaterial(&mat);
	}
	UpdateSegments();
}

void pragma::gui::types::WISegmentedRect::SetMaterial(material::Material &mat)
{
	if(&mat == m_material.get())
		return;
	auto *albedoMap = mat.GetAlbedoMap();
	if(!albedoMap || !albedoMap->texture)
		return;
	auto &tex = *static_cast<material::Texture *>(albedoMap->texture.get());
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

	m_material = mat.GetHandle();
}

pragma::material::Material *pragma::gui::types::WISegmentedRect::GetMaterial() { return m_material.get(); }

void pragma::gui::types::WISegmentedRect::SetMaterial(const std::string &matPath)
{
	auto &matLoadHandler = WGUI::GetInstance().GetMaterialLoadHandler();
	if(!matLoadHandler)
		return;
	auto *mat = matLoadHandler(matPath);
	if(!mat)
		return;
	SetMaterial(*mat);
}
