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
	auto it = m_childMargins.find(&child);
	if(it != m_childMargins.end())
		m_childMargins.erase(it);
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

bool pragma::gui::types::BaseBox::GetFixedWidth() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::FixedWidth); }
bool pragma::gui::types::BaseBox::GetFixedHeight() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::FixedHeight); }

void pragma::gui::types::BaseBox::SetAutoSizeActivatedValue(bool set) { math::set_flag(m_boxStateFlags, BoxStateFlags::AutoSizeActivated, set); }
bool pragma::gui::types::BaseBox::GetAutoSizeActivated() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::AutoSizeActivated); }

void pragma::gui::types::BaseBox::SetAutoFillWidth(bool set) { math::set_flag(m_boxStateFlags, BoxStateFlags::AutoFillWidth, set); }
bool pragma::gui::types::BaseBox::GetAutoFillWidth() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::AutoFillWidth); }

void pragma::gui::types::BaseBox::SetAutoFillHeight(bool set) { math::set_flag(m_boxStateFlags, BoxStateFlags::AutoFillHeight, set); }
bool pragma::gui::types::BaseBox::GetAutoFillHeight() const { return math::is_flag_set(m_boxStateFlags, BoxStateFlags::AutoFillHeight); }

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

// Auto-fill will stretch the children to fill out the size of the box.
// Width auto-fill on a horizontal box will cause the last child to be stretched to the remaining width.
// Height auto-fill on a horizontal box will cause all children to be stretched to the full height.
// The behavior for vertical boxes is the same, but opposite.
void pragma::gui::types::BaseBox::SetAutoFillContentsToWidth(bool autoFill)
{
	SetAutoFillWidth(autoFill);
	if(autoFill)
		SetFixedWidth(true);
}

void pragma::gui::types::BaseBox::SetAutoFillContentsToHeight(bool autoFill)
{
	SetAutoFillHeight(autoFill);
	if(autoFill)
		SetFixedHeight(true);
}

void pragma::gui::types::BaseBox::SetAutoFillContents(bool autoFill)
{
	SetAutoFillContentsToWidth(autoFill);
	SetAutoFillContentsToHeight(autoFill);
}

void pragma::gui::types::BaseBox::SetAutoFillTarget(WIBase *el) { m_autoFillTarget = el ? el->GetHandle() : WIHandle{}; }

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
	m_childMargins[&el] = {left, top, right, bottom};
	SetSizeUpdateRequired(true);
	ScheduleUpdate();
}

pragma::gui::types::BoxOffsets pragma::gui::types::BaseBox::GetChildMargin(const WIBase &el) const
{
	auto it = m_childMargins.find(&el);
	if(it != m_childMargins.end())
		return it->second;
	return BoxOffsets {0, 0, 0, 0};
}

pragma::gui::types::FlexBox::FlexBox(FlexDirection direction) : m_direction(direction)
{
	RegisterCallback<void>("OnContentsUpdated");
}

bool pragma::gui::types::FlexBox::IsHorizontalBox() const { return m_direction == FlexDirection::Horizontal; }
bool pragma::gui::types::FlexBox::IsVerticalBox() const { return m_direction == FlexDirection::Vertical; }

bool pragma::gui::types::FlexBox::HasBoxAlignedAnchor(WIBase *el) const
{
	if(!el)
		return false;
	return IsHorizontalBox() ? el->HasHorizontalAnchor() : el->HasVerticalAnchor();
}

void pragma::gui::types::FlexBox::DoUpdate()
{
	auto size = GetSize();
	const auto &padding = GetPadding();
	auto spacing = GetSpacing();

	auto isHoriz = IsHorizontalBox();

	auto mainPos = isHoriz ? padding.left : padding.top;
	auto crossMax = isHoriz ? padding.top : padding.left;
	auto isFirstChild = true;

	int32_t lastChildIdx = -1;
	int32_t autoFillChildIdx = -1;

	auto &children = *GetChildren();
	for(size_t i = 0; i < children.size(); ++i) {
		auto &hChild = children[i];
		if(!hChild.IsValid() || !hChild->IsSelfVisible() || IsBackgroundElement(*hChild.get()))
			continue;
		auto *child = hChild.get();
		auto margin = GetChildMargin(*child);

		if(!isFirstChild)
			mainPos += spacing;

		auto mainMarginStart = isHoriz ? margin.left : margin.top;
		auto mainMarginEnd = isHoriz ? margin.right : margin.bottom;
		auto crossMarginStart = isHoriz ? margin.top : margin.left;
		auto crossMarginEnd = isHoriz ? margin.bottom : margin.right;

		mainPos += mainMarginStart;

		if (isHoriz) {
			child->ApplyX(mainPos);
			child->ApplyY(padding.top + margin.top);
		} else {
			child->ApplyY(mainPos);
			child->ApplyX(padding.left + margin.left);
		}

		auto autoFillCross = isHoriz ? GetAutoFillHeight() : GetAutoFillWidth();
		if(autoFillCross && !HasBoxAlignedAnchor(child)) {
			auto targetCross = (isHoriz ? size.y : size.x) - (isHoriz ? (padding.top + padding.bottom) : (padding.left + padding.right)) - crossMarginStart - crossMarginEnd;
			if(isHoriz)
				child->ApplyHeight(std::max(targetCross, 0));
			else
				child->ApplyWidth(std::max(targetCross, 0));
		}

		mainPos += (isHoriz ? child->GetWidth() : child->GetHeight()) + mainMarginEnd;

		auto childCrossEnd = isHoriz ? child->GetBottom() : child->GetRight();
		crossMax = std::max(crossMax, childCrossEnd + crossMarginEnd);

		lastChildIdx = i;
		isFirstChild = false;
		if(child == m_autoFillTarget.get())
			autoFillChildIdx = i;
	}

	if(autoFillChildIdx == -1)
		autoFillChildIdx = lastChildIdx;

	auto curSize = size;
	auto isFixedMain = isHoriz ? GetFixedWidth() : GetFixedHeight();
	auto isAutoFillMain = isHoriz ? GetAutoFillWidth() : GetAutoFillHeight();

	auto mainPaddingStart = isHoriz ? padding.left : padding.top;
	auto mainPaddingEnd = isHoriz ? padding.right : padding.bottom;

	if(!isFixedMain) {
		auto finalMainSize = isFirstChild ? (mainPaddingStart + mainPaddingEnd) : (mainPos + mainPaddingEnd);
		if(isHoriz)
			size.x = finalMainSize;
		else
			size.y = finalMainSize;
	}
	else if(isAutoFillMain && autoFillChildIdx >= 0 && !HasBoxAlignedAnchor(children[autoFillChildIdx].get())) {
		auto &afChild = children[autoFillChildIdx];
		auto afMargin = GetChildMargin(*afChild);
		auto afMarginEnd = isHoriz ? afMargin.right : afMargin.bottom;

		int32_t widthOrHeight;
		int32_t sizeAdd = 0;

		if(afChild.get() == children[lastChildIdx].get()) {
			auto afStart = isHoriz ? afChild->GetLeft() : afChild->GetTop();
			widthOrHeight = (isHoriz ? size.x : size.y) - afStart - mainPaddingEnd - afMarginEnd;
		}
		else {
			auto *lChild = children[lastChildIdx].get();
			auto lMargin = GetChildMargin(*lChild);
			auto lMarginEnd = isHoriz ? lMargin.right : lMargin.bottom;
			auto lEnd = isHoriz ? lChild->GetRight() : lChild->GetBottom();

			sizeAdd = (isHoriz ? size.x : size.y) - lEnd - mainPaddingEnd - lMarginEnd;
			widthOrHeight = (isHoriz ? afChild->GetWidth() : afChild->GetHeight()) + sizeAdd;
		}

		if(isHoriz)
			afChild->ApplyWidth(std::max(widthOrHeight, 0));
		else
			afChild->ApplyHeight(std::max(widthOrHeight, 0));
		afChild->Update();

		if(sizeAdd != 0) {
			for(size_t i = autoFillChildIdx + 1; i < children.size(); ++i) {
				auto *child = children[i].get();
				if(child->IsSelfVisible() && !IsBackgroundElement(*child)) {
					if(isHoriz)
						child->ApplyX(child->GetX() + sizeAdd);
					else
						child->ApplyY(child->GetY() + sizeAdd);
				}
			}
		}
	}

	if(GetSizeUpdateRequired()) {
		auto isFixedCross = isHoriz ? GetFixedHeight() : GetFixedWidth();
		auto crossPaddingEnd = isHoriz ? padding.bottom : padding.right;

		if(!isFixedCross) {
			if(isHoriz)
				size.y = crossMax + crossPaddingEnd;
			else
				size.x = crossMax + crossPaddingEnd;
		}

		size.x = std::max(size.x, 0);
		size.y = std::max(size.y, 0);

		UpdateNonAnchoredSize(curSize, size);
		CallCallbacks("OnContentsUpdated");
		SetSizeUpdateRequired(false);
	}
}
