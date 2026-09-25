// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.flexbox;

pragma::gui::types::BaseBox::BaseBox() : WIBase {} {}

void pragma::gui::types::BaseBox::Initialize()
{
	WIBase::Initialize();

	SetAutoSizeToContents(true);
}

void pragma::gui::types::BaseBox::OnChildAdded(WIBase *child)
{
	WIBase::OnChildAdded(child);

	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}
void pragma::gui::types::BaseBox::OnChildVisibilityChanged(WIBase &child, bool visible)
{
	if(IsBackgroundElement(child))
		return;
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}
void pragma::gui::types::BaseBox::OnChildSizeChanged(WIBase &child, const Vector2i &oldSize, ChangeSource changedSource)
{
	if(IsBackgroundElement(child))
		return;
	// Note: We mustn't update if the child is anchored, otherwise we end up in an infinite recursion!
	if(HasBoxAlignedAnchor(&child))
		return;
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}
void pragma::gui::types::BaseBox::OnChildDeleted(WIBase &child)
{
	auto it = m_childLayout.find(&child);
	if(it != m_childLayout.end())
		m_childLayout.erase(it);
	if(IsBackgroundElement(child))
		return;
	// We'll have to update whenever one of our children has been removed
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}
void pragma::gui::types::BaseBox::OnChildRemoved(WIBase *child)
{
	if(IsBackgroundElement(*child))
		return;
	WIBase::OnChildRemoved(child);

	ScheduleUpdate();
}

void pragma::gui::types::BaseBox::OnRemove() {}

bool pragma::gui::types::BaseBox::IsBackgroundElement(const WIBase &el) const { return el.IsBackgroundElement(); }

void pragma::gui::types::BaseBox::UpdateNonAnchoredSize(const Vector2i &curSize, const Vector2i &size)
{
	if(size == curSize)
		return;

	auto xAnchor = HasHorizontalAnchor();
	auto yAnchor = HasVerticalAnchor();

	if(!xAnchor && !yAnchor)
		UpdateSize(size);
	else if(!xAnchor && size.x != curSize.x)
		UpdateWidth(size.x);
	else if(!yAnchor && size.y != curSize.y)
		UpdateHeight(size.y);
}

void pragma::gui::types::BaseBox::SetSkipSizeUpdateSchedule(bool set) { math::set_flag(m_boxStateFlags, BoxStateFlags::SkipSizeUpdateSchedule, set); }
bool pragma::gui::types::BaseBox::GetSkipSizeUpdateSchedule() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::SkipSizeUpdateSchedule); }

void pragma::gui::types::BaseBox::SetSizeUpdateRequired(bool set) { math::set_flag(m_boxStateFlags, BoxStateFlags::SizeUpdateRequired, set); }
bool pragma::gui::types::BaseBox::GetSizeUpdateRequired() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::SizeUpdateRequired); }

bool pragma::gui::types::BaseBox::GetFixedWidth() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::FixedWidth) || GetHorizontalAlignment() == Alignment::Fill; }
bool pragma::gui::types::BaseBox::GetFixedHeight() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::FixedHeight) || GetVerticalAlignment() == Alignment::Fill; }

void pragma::gui::types::BaseBox::SetAutoSizeActivatedValue(bool set) { math::set_flag(m_boxStateFlags, BoxStateFlags::AutoSizeActivated, set); }
bool pragma::gui::types::BaseBox::GetAutoSizeActivated() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::AutoSizeActivated); }

void pragma::gui::types::BaseBox::UpdateSize(const Vector2i &size)
{
	SetSkipSizeUpdateSchedule(true);
	SetSize(size.x, size.y, ChangeSource::Content);
	SetSkipSizeUpdateSchedule(false);
}

void pragma::gui::types::BaseBox::UpdateWidth(int w)
{
	SetSkipSizeUpdateSchedule(true);
	SetWidth(w, false, ChangeSource::Content);
	SetSkipSizeUpdateSchedule(false);
}

void pragma::gui::types::BaseBox::UpdateHeight(int h)
{
	SetSkipSizeUpdateSchedule(true);
	SetHeight(h, false, ChangeSource::Content);
	SetSkipSizeUpdateSchedule(false);
}

void pragma::gui::types::BaseBox::OnSizeChanged(const Vector2i &oldSize, ChangeSource changedSource)
{
	WIBase::OnSizeChanged(oldSize, changedSource);
	if(GetSkipSizeUpdateSchedule())
		return;
	ScheduleUpdate();
}

void pragma::gui::types::BaseBox::SetFixedWidthValue(bool set) { math::set_flag(m_boxStateFlags, BoxStateFlags::FixedWidth, set); }

void pragma::gui::types::BaseBox::SetFixedHeightValue(bool set) { math::set_flag(m_boxStateFlags, BoxStateFlags::FixedHeight, set); }

void pragma::gui::types::BaseBox::SetFixedWidth(bool fixed)
{
	auto size = GetSize();
	SetFixedWidthValue(fixed);

	if(GetAutoSizeActivated())
		m_autoSizeRestore = {!GetFixedWidth(), !GetFixedHeight()};
	else
		SetAutoSizeToContents(!GetFixedWidth(), !GetFixedHeight());
	ApplySize(size); // Keep old size for now
}

void pragma::gui::types::BaseBox::SetFixedHeight(bool fixed)
{
	auto size = GetSize();
	SetFixedHeightValue(fixed);

	if(GetAutoSizeActivated())
		m_autoSizeRestore = {!GetFixedWidth(), !GetFixedHeight()};
	else
		SetAutoSizeToContents(!GetFixedWidth(), !GetFixedHeight());
	ApplySize(size); // Keep old size for now
}

void pragma::gui::types::BaseBox::SetFixedSize(bool fixed)
{
	SetFixedWidth(fixed);
	SetFixedHeight(fixed);
}

void pragma::gui::types::BaseBox::SetAutoSizeActivated(bool activated, bool updateImmediately)
{
	if(GetAutoSizeActivated() == activated)
		return;

	SetAutoSizeActivatedValue(activated);

	if(!activated) {
		m_autoSizeRestore = {ShouldAutoSizeToContentsX(), ShouldAutoSizeToContentsY()};
		SetAutoSizeToContents(false, false);
		return;
	}

	if(m_autoSizeRestore.has_value())
		SetAutoSizeToContents(m_autoSizeRestore->first, m_autoSizeRestore->second, updateImmediately);
}

void pragma::gui::types::BaseBox::SetSpacing(int32_t spacing)
{
	m_spacing = spacing;
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}

void pragma::gui::types::BaseBox::SetPadding(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	m_padding = {left, top, right, bottom};
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}

void pragma::gui::types::BaseBox::SetChildMargin(const WIBase &el, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	m_childLayout[&el].margin = {left, top, right, bottom};
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}

pragma::gui::types::BoxOffsets pragma::gui::types::BaseBox::GetChildMargin(const WIBase &el) const
{
	auto it = m_childLayout.find(&el);
	if(it != m_childLayout.end())
		return it->second.margin;
	return BoxOffsets {0, 0, 0, 0};
}

void pragma::gui::types::BaseBox::SetChildFlex(const WIBase &el, float flex)
{
	m_childLayout[&el].flex = flex;
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}

float pragma::gui::types::BaseBox::GetChildFlex(const WIBase &el) const
{
	auto it = m_childLayout.find(&el);
	if(it != m_childLayout.end())
		return it->second.flex;
	return 0.0f;
}

pragma::gui::types::FlexBox::FlexBox(FlexDirection direction) : m_direction(direction) { RegisterCallback<void>("OnContentsUpdated"); }

bool pragma::gui::types::FlexBox::IsHorizontalBox() const { return m_direction == FlexDirection::Horizontal; }
bool pragma::gui::types::FlexBox::IsVerticalBox() const { return m_direction == FlexDirection::Vertical; }

bool pragma::gui::types::FlexBox::HasBoxAlignedAnchor(WIBase *el) const
{
	if(!el)
		return false;
	return IsHorizontalBox() ? el->HasHorizontalAnchor() : el->HasVerticalAnchor();
}

void pragma::gui::types::FlexBox::SetAlignItems(FlexAlign align)
{
	m_alignItems = align;
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}

void pragma::gui::types::FlexBox::SetJustifyContent(FlexJustify justify)
{
	m_justifyContent = justify;
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}

void pragma::gui::types::FlexBox::DoUpdate()
{
	auto size = GetSize();
	const auto &padding = GetPadding();
	auto spacing = GetSpacing();

	auto isHoriz = IsHorizontalBox();

	auto mainPaddingStart = isHoriz ? padding.left : padding.top;
	auto mainPaddingEnd = isHoriz ? padding.right : padding.bottom;
	auto crossPaddingStart = isHoriz ? padding.top : padding.left;
	auto crossPaddingEnd = isHoriz ? padding.bottom : padding.right;

	float totalFlex = 0.0f;
	int32_t totalRigidMainSize = mainPaddingStart + mainPaddingEnd;
	int32_t crossMax = 0;

	auto &children = *GetChildren();
	std::vector<WIBase *> layoutChildren;
	layoutChildren.reserve(children.size());

	for(size_t i = 0; i < children.size(); ++i) {
		auto &hChild = children[i];
		if(!hChild.IsValid() || !hChild->IsSelfVisible() || IsBackgroundElement(*hChild.get()))
			continue;

		auto *child = hChild.get();
		layoutChildren.push_back(child);

		auto margin = GetChildMargin(*child);
		float flex = GetChildFlex(*child);

		auto mainMarginStart = isHoriz ? margin.left : margin.top;
		auto mainMarginEnd = isHoriz ? margin.right : margin.bottom;
		auto crossMarginStart = isHoriz ? margin.top : margin.left;
		auto crossMarginEnd = isHoriz ? margin.bottom : margin.right;

		int32_t childMainSize = isHoriz ? child->GetWidth() : child->GetHeight();
		int32_t childCrossSize = isHoriz ? child->GetHeight() : child->GetWidth();

		if(flex > 0.0f) {
			totalFlex += flex;
			totalRigidMainSize += mainMarginStart + mainMarginEnd;
		}
		else
			totalRigidMainSize += mainMarginStart + childMainSize + mainMarginEnd;

		crossMax = std::max(crossMax, childCrossSize + crossMarginStart + crossMarginEnd);
	}

	if(layoutChildren.size() > 1)
		totalRigidMainSize += static_cast<int32_t>(layoutChildren.size() - 1) * spacing;

	auto isFixedMain = isHoriz ? GetFixedWidth() : GetFixedHeight();
	auto isFixedCross = isHoriz ? GetFixedHeight() : GetFixedWidth();

	int32_t targetMainSize = isFixedMain ? (isHoriz ? size.x : size.y) : totalRigidMainSize;
	int32_t targetCrossSize = isFixedCross ? (isHoriz ? size.y : size.x) : (crossMax + crossPaddingStart + crossPaddingEnd);

	int32_t remainingMainSpace = std::max(0, targetMainSize - totalRigidMainSize);
	int32_t currentMainPos = mainPaddingStart;
	int32_t crossSpace = targetCrossSize - crossPaddingStart - crossPaddingEnd;

	// Justify-content
	auto dynamicSpacing = spacing;

	if(remainingMainSpace > 0 && totalFlex == 0.0f) {
		switch(m_justifyContent) {
		case FlexJustify::Start:
			break; // Default behavior
		case FlexJustify::End:
			currentMainPos += remainingMainSpace;
			break;
		case FlexJustify::Center:
			currentMainPos += remainingMainSpace / 2;
			break;
		case FlexJustify::SpaceBetween:
			if(layoutChildren.size() > 1)
				dynamicSpacing = spacing + (remainingMainSpace / (layoutChildren.size() - 1));
			break;
		case FlexJustify::SpaceEvenly:
			if(!layoutChildren.empty()) {
				int32_t space = remainingMainSpace / (layoutChildren.size() + 1);
				currentMainPos += space;
				dynamicSpacing = spacing + space;
			}
			break;
		}
	}
	//

	for(auto *child : layoutChildren) {
		auto margin = GetChildMargin(*child);
		float flex = GetChildFlex(*child);

		auto mainMarginStart = isHoriz ? margin.left : margin.top;
		auto mainMarginEnd = isHoriz ? margin.right : margin.bottom;
		auto crossMarginStart = isHoriz ? margin.top : margin.left;
		auto crossMarginEnd = isHoriz ? margin.bottom : margin.right;

		currentMainPos += mainMarginStart;

		int32_t childMainSize = isHoriz ? child->GetWidth() : child->GetHeight();
		if(flex > 0.0f) {
			float flexShare = flex / totalFlex;
			childMainSize = static_cast<int32_t>(remainingMainSpace * flexShare);
			if(isHoriz)
				child->ApplyWidth(childMainSize);
			else
				child->ApplyHeight(childMainSize);
		}

		int32_t childCrossSize = isHoriz ? child->GetHeight() : child->GetWidth();
		int32_t crossPos = crossPaddingStart + crossMarginStart;

		if(!HasBoxAlignedAnchor(child)) {
			if(m_alignItems == FlexAlign::Stretch) {
				childCrossSize = std::max(0, crossSpace - crossMarginStart - crossMarginEnd);
				if(isHoriz)
					child->ApplyHeight(childCrossSize);
				else
					child->ApplyWidth(childCrossSize);
				child->Update();
			}
			else if(m_alignItems == FlexAlign::Center)
				crossPos = crossPaddingStart + (crossSpace / 2) - (childCrossSize / 2);
			else if(m_alignItems == FlexAlign::End)
				crossPos = crossPaddingStart + crossSpace - crossMarginEnd - childCrossSize;
		}

		if(isHoriz) {
			child->ApplyX(currentMainPos);
			child->ApplyY(crossPos);
		}
		else {
			child->ApplyY(currentMainPos);
			child->ApplyX(crossPos);
		}

		currentMainPos += childMainSize + mainMarginEnd + dynamicSpacing;
	}

	if(GetSizeUpdateRequired()) {
		auto finalSize = size;
		if(!isFixedMain) {
			if(isHoriz)
				finalSize.x = targetMainSize;
			else
				finalSize.y = targetMainSize;
		}
		if(!isFixedCross) {
			if(isHoriz)
				finalSize.y = targetCrossSize;
			else
				finalSize.x = targetCrossSize;
		}

		finalSize.x = std::max(finalSize.x, 0);
		finalSize.y = std::max(finalSize.y, 0);

		UpdateNonAnchoredSize(size, finalSize);
		CallCallbacks("OnContentsUpdated");
		SetSizeUpdateRequired(false);
	}
}
