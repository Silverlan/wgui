// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.root;

constexpr uint32_t TOOLTIP_HOVER_DELAY_MS = 750;
pragma::gui::types::WIRoot::WIRoot() : WIBase(), m_hTooltipTarget {}, m_tCursorOver() {}

pragma::gui::types::WIRoot::~WIRoot() {}

void pragma::gui::types::WIRoot::Initialize()
{
	WIBase::Initialize();
	SetBaseElement(true);
	SetMouseMovementCheckEnabled(true);
	EnableThinking();
}

Vector2 pragma::gui::types::WIRoot::GetCursorPos() const
{
	if(m_cursorPosOverride)
		return *m_cursorPosOverride;
	auto *window = GetWindow();
	return window ? (*window)->GetCursorPos() : Vector2 {};
}

void pragma::gui::types::WIRoot::SetCursorPosOverride(const Vector2 &pos) { m_cursorPosOverride = pos; }
void pragma::gui::types::WIRoot::ClearCursorPosOverride() { m_cursorPosOverride = {}; }
const std::weak_ptr<const prosper::Window> &pragma::gui::types::WIRoot::GetWindowPtr() const { return m_window; }
void pragma::gui::types::WIRoot::SetWindow(const std::shared_ptr<const prosper::Window> &window) { m_window = window; }
pragma::platform::Cursor::Shape pragma::gui::types::WIRoot::GetMainCursor() const { return m_mainCursor; }
const pragma::platform::CursorHandle &pragma::gui::types::WIRoot::GetMainCustomCursor() const { return m_mainCustomCursor; }
void pragma::gui::types::WIRoot::SetMainCursor(platform::Cursor::Shape cursor) { m_mainCursor = cursor; }
void pragma::gui::types::WIRoot::SetMainCustomCursor(const platform::CursorHandle &hCursor) { m_mainCustomCursor = hCursor; }
void pragma::gui::types::WIRoot::SetFocusEnabled(bool enabled) { m_focusEnabled = enabled; }
bool pragma::gui::types::WIRoot::IsFocusEnabled() const { return m_focusEnabled; }
pragma::gui::types::WIBase *pragma::gui::types::WIRoot::GetFocusedElement() { return m_elFocused.get(); }
void pragma::gui::types::WIRoot::SetFocusedElement(WIBase *el) { m_elFocused = el ? el->GetHandle() : WIHandle {}; }
uint32_t pragma::gui::types::WIRoot::GetFocusCount() const { return m_focusCount; }
void pragma::gui::types::WIRoot::SetFocusCount(uint32_t focusCount) { m_focusCount = focusCount; }
std::deque<pragma::gui::WIHandle> &pragma::gui::types::WIRoot::GetFocusTrapStack() { return m_focusTrapStack; }

bool is_valid(const pragma::gui::WIHandle &hEl);
void pragma::gui::types::WIRoot::RestoreTrappedFocus(WIBase *elRef)
{
	for(auto it = m_focusTrapStack.rbegin(); it != m_focusTrapStack.rend();) {
		auto &hEl = *it;
		++it;
		if(!is_valid(hEl))
			it = std::deque<WIHandle>::reverse_iterator(m_focusTrapStack.erase(it.base()));
		else if(hEl.get() != elRef && hEl->IsVisible()) {
			hEl->RequestFocus();
			break;
		}
	}
}

const prosper::Window *pragma::gui::types::WIRoot::GetWindow() const { return const_cast<WIRoot *>(this)->GetWindow(); }
prosper::Window *pragma::gui::types::WIRoot::GetWindow()
{
	if(m_window.expired())
		return nullptr;
	return const_cast<prosper::Window *>(m_window.lock().get());
}

pragma::gui::types::WIBase &pragma::gui::types::WIRoot::FindTooltipBaseElement(WIBase &el)
{
	auto *p = &el;
	while(p) {
		if(p->IsBaseElement())
			return *p;
		p = p->GetParent();
	}
	return *this;
}

void pragma::gui::types::WIRoot::Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
{
	WIBase::Think(drawCmd);
	if(!m_hTooltip.IsValid()) {
		auto *pTooltip = WGUI::GetInstance().Create<WITooltip>(this);
		pTooltip->SetVisible(false);
		pTooltip->SetZPos(100'000);
		m_hTooltip = pTooltip->GetHandle();
	}
	if(m_hTooltipTarget.IsValid() == true) {
		auto *pTooltip = static_cast<WITooltip *>(m_hTooltip.get());
		if(pTooltip->IsVisible() == false) {
			auto t = util::Clock::now();
			auto tDelta = std::chrono::duration_cast<std::chrono::milliseconds>(t - m_tCursorOver).count();
			if(tDelta >= TOOLTIP_HOVER_DELAY_MS) {
				auto *el = m_hTooltipTarget.get();
				int32_t x, y;
				WGUI::GetInstance().GetMousePos(x, y);
				//pTooltip->FadeIn(0.1f);
				pTooltip->SetParent(&FindTooltipBaseElement(*el));
				pTooltip->SetText(el->GetTooltip());
				pTooltip->SetZPos(std::numeric_limits<int>::max());
				pTooltip->SetName("tooltip");
				pTooltip->RefreshSkin();
				auto xMax = GetWidth() - pTooltip->GetWidth();
				auto yMax = GetHeight() - pTooltip->GetHeight();
				if(xMax >= 0 && xMax < x)
					x = xMax;
				auto yBottom = el->GetAbsolutePos().y + el->GetHeight();
				if(y < yBottom)
					y = yBottom;
				if(yMax >= 0 && yMax < y)
					y = yMax;
				pTooltip->SetPos(x, y);
				pTooltip->SetVisible(true);

				el->CallCallbacks<void, WITooltip *>("OnShowTooltip", pTooltip);
			}
		}
	}
}

void pragma::gui::types::WIRoot::OnCursorMoved(int x, int y)
{
	WIBase::OnCursorMoved(x, y);
	if(!m_hTooltip.IsValid())
		return;
	auto *pTooltip = static_cast<WITooltip *>(m_hTooltip.get());
	auto *el = WGUI::GetInstance().GetGUIElement(this, x, y, [](WIBase *elChild) -> bool { return elChild->HasTooltip(); });
	if(el == nullptr || el == this) {
		auto *elTgt = m_hTooltipTarget.get();
		m_hTooltipTarget = WIHandle {};
		pTooltip->SetVisible(false);
		if(elTgt)
			elTgt->CallCallbacks<void, WITooltip *>("OnHideTooltip", pTooltip);
		//pTooltip->SetAlpha(0.f);
		return;
	}
	if(el == m_hTooltipTarget.get())
		return;
	m_tCursorOver = util::Clock::now();
	if(pTooltip->IsVisible() == true) {
		m_tCursorOver -= std::chrono::milliseconds(TOOLTIP_HOVER_DELAY_MS);
		pTooltip->SetVisible(false);
	}
	auto *elTgtPrev = m_hTooltipTarget.get();
	m_hTooltipTarget = el->GetHandle();
	if(elTgtPrev)
		elTgtPrev->CallCallbacks<void, WITooltip *>("OnHideTooltip", pTooltip);
}

void pragma::gui::types::WIRoot::Setup() {}

void pragma::gui::types::WIRoot::SetIMETarget(WIBase *el)
{
	m_hImeTarget = el ? m_hImeTarget->GetHandle() : WIHandle {};
	if(m_hImeTarget.expired()) {
		auto *window = GetWindow();
		if(window)
			(*window)->ResetPreeditText();
		return;
	}
	UpdateIMETarget();
}

void pragma::gui::types::WIRoot::UpdateIMETarget()
{
	auto *window = GetWindow();
	if(!window || (*window)->IsIMEEnabled() == false || m_hImeTarget.expired())
		return;
	Vector2i pos, size;
	m_hImeTarget->GetAbsoluteVisibleBounds(pos, size);
	(*window)->SetPreeditCursorRectangle(pos.x, pos.y, size.x, size.y);
}

bool pragma::gui::types::WIRoot::IsMainFileHovering() const { return m_fileDragHover; }
void pragma::gui::types::WIRoot::SetMainFileHovering(bool hovering)
{
	m_fileDragHover = hovering;
	if(hovering == false) {
		auto hoverElements = std::move(m_fileHoverElements);
		m_fileHoverElements.clear();
		for(auto &hEl : hoverElements) {
			if(!hEl.IsValid())
				continue;
			hEl.get()->SetFileHovering(false);
		}
	}
	else {
		std::function<void(WIBase &)> updateHoverStates = nullptr;
		updateHoverStates = [&updateHoverStates](WIBase &el) {
			if(el.IsVisible() == false)
				return;
			if(*el.GetMouseInBoundsProperty() && el.GetFileDropInputEnabled())
				el.SetFileHovering(true);
			for(auto &hEl : *el.GetChildren()) {
				if(!hEl.IsValid())
					continue;
				updateHoverStates(*hEl.get());
			}
		};
		updateHoverStates(*this);
	}
}
void pragma::gui::types::WIRoot::SetFileHoverElement(WIBase &el, bool hovering)
{
	auto it = std::find_if(m_fileHoverElements.begin(), m_fileHoverElements.end(), [&el](const WIHandle &hEl) { return hEl.get() == &el; });
	if(hovering) {
		if(it != m_fileHoverElements.end())
			return;
		m_fileHoverElements.push_back(el.GetHandle());
		return;
	}
	if(it == m_fileHoverElements.end())
		return;
	m_fileHoverElements.erase(it);
}

void pragma::gui::types::WIRoot::DropFiles(const std::vector<std::string> &files)
{
	for(auto &hEl : m_fileHoverElements) {
		if(!hEl.IsValid() || !hEl->IsFileHovering())
			continue;
		if(hEl->OnFilesDropped(files) == util::EventReply::Handled)
			break;
	}
	SetMainFileHovering(false);
}
