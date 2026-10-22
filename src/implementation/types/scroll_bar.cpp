// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.button;
import :types.scroll_bar;

pragma::gui::types::WIScrollBar::WIScrollBar() : WIBase(), m_bHorizontal(false), m_offset(0), m_numElements(0), m_numListed(0), m_scrollAmount(1)
{
	SetMouseInputEnabled(true);
	SetKeyboardInputEnabled(true);

	RegisterCallback<void, unsigned int>("OnScrollOffsetChanged");
}

pragma::gui::types::WIScrollBar::~WIScrollBar() {}
unsigned int pragma::gui::types::WIScrollBar::GetScrollAmount() const { return math::min(m_scrollAmount, m_numListed); }
void pragma::gui::types::WIScrollBar::SetScrollAmount(unsigned int am) { m_scrollAmount = am; }
unsigned int pragma::gui::types::WIScrollBar::GetScrollOffset() const { return m_offset; }
void pragma::gui::types::WIScrollBar::SetScrollOffset(unsigned int offset)
{
	int diff = int(m_numElements) - int(m_numListed);
	if(diff < 0)
		diff = 0;
	auto udiff = static_cast<unsigned int>(diff);
	if(offset > udiff)
		offset = udiff;
	if(m_offset == offset)
		return;
	m_offset = offset;
	UpdateSliderOffset();
	CallCallbacks<void, unsigned int>("OnScrollOffsetChanged", offset);
}
pragma::util::EventReply pragma::gui::types::WIScrollBar::ScrollCallback(Vector2 offset, bool offsetAsPixels)
{
	if(WIBase::ScrollCallback(offset, offsetAsPixels) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(offsetAsPixels)
		AddScrollOffset(static_cast<int>(-offset.y));
	else {
		auto scrollAmount = GetScrollAmount();
		auto &window = WGUI::GetInstance().GetContext().GetWindow();
		auto isAltDown = (window->GetKeyState(platform::Key::LeftAlt) != platform::KeyState::Release || window->GetKeyState(platform::Key::RightAlt) != platform::KeyState::Release) ? true : false;
		if(isAltDown)
			scrollAmount = m_numListed;
		AddScrollOffset(static_cast<int>(-offset.y * static_cast<double>(scrollAmount)));
	}
	return util::EventReply::Handled;
}
pragma::util::EventReply pragma::gui::types::WIScrollBar::MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods)
{
	if(WIBase::MouseCallback(button, state, mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(state == platform::KeyState::Press && button == platform::MouseButton::Left) {
		if(m_slider.IsValid()) {
			int mouseX, mouseY;
			GetMousePos(&mouseX, &mouseY);
			WIScrollBarSlider *slider = static_cast<WIScrollBarSlider *>(m_slider.get());
			if(IsHorizontal()) {
				int x = slider->GetX();
				if(mouseX <= x)
					AddScrollOffset(-static_cast<int>(m_numListed));
				else
					AddScrollOffset(static_cast<int>(m_numListed));
			}
			else {
				int y = slider->GetY();
				if(mouseY <= y)
					AddScrollOffset(-static_cast<int>(m_numListed));
				else
					AddScrollOffset(static_cast<int>(m_numListed));
			}
		}
	}
	return util::EventReply::Handled;
}
void pragma::gui::types::WIScrollBar::AddScrollOffset(int scroll)
{
	int newOffset = int(m_offset) + scroll;
	if(newOffset < 0)
		newOffset = 0;
	SetScrollOffset(newOffset);
}

uint32_t pragma::gui::types::WIScrollBar::GetElementCount() const { return m_numElements; }
uint32_t pragma::gui::types::WIScrollBar::GetScrollElementCount() const { return m_numListed; }
uint32_t pragma::gui::types::WIScrollBar::GetBottomScrollOffset() const { return math::min(GetScrollOffset() + GetScrollElementCount(), GetElementCount()); }

void pragma::gui::types::WIScrollBar::SetUp(unsigned int numElementsListed, unsigned int numElementsTotal)
{
	m_numElements = numElementsTotal;
	m_numListed = numElementsListed;
	if(m_scrollAmount > m_numListed)
		m_scrollAmount = m_numListed;
	UpdateSliderSize();
	SetScrollOffset(m_offset);
	if(numElementsListed == 0 || numElementsListed >= numElementsTotal)
		SetVisible(false);
	else
		SetVisible(true);
}

void pragma::gui::types::WIScrollBar::UpdateSliderOffset()
{
	if(m_slider.IsValid()) {
		int h = GetSliderHeight();
		WIScrollBarSlider *slider = static_cast<WIScrollBarSlider *>(m_slider.get());
		int hSlide = h - slider->GetSliderHeight();
		slider->SetSliderY(static_cast<int>(static_cast<float>(m_offset) / static_cast<float>(static_cast<int>(m_numElements) - static_cast<int>(m_numListed)) * static_cast<float>(hSlide)));
	}
}
void pragma::gui::types::WIScrollBar::UpdateSliderSize()
{
	if(m_slider.IsValid()) {
		WIScrollBarSlider *slider = static_cast<WIScrollBarSlider *>(m_slider.get());
		int h = (m_numElements > 0u) ? static_cast<int>(static_cast<float>(GetSliderHeight()) * (static_cast<float>(m_numListed) / static_cast<float>(m_numElements))) : 0;
		if(h == 0)
			h = 1;
		slider->SetSliderSize(GetSliderWidth(), h);
		slider->SetSliderPos(0, GetSliderWidth());
	}
}
int pragma::gui::types::WIScrollBar::GetSliderWidth() const
{
	if(!m_sliderContainer.IsValid())
		return 0;
	return (m_bHorizontal == true) ? m_sliderContainer->GetHeight() : m_sliderContainer->GetWidth();
}
int pragma::gui::types::WIScrollBar::GetSliderHeight() const
{
	if(!m_sliderContainer.IsValid())
		return 0;
	return (m_bHorizontal == true) ? m_sliderContainer->GetWidth() : m_sliderContainer->GetHeight();
}
int pragma::gui::types::WIScrollBar::GetSliderX() const
{
	if(!m_sliderContainer.IsValid())
		return 0;
	return (m_bHorizontal == true) ? m_sliderContainer->GetY() : m_sliderContainer->GetX();
}
int pragma::gui::types::WIScrollBar::GetSliderY() const
{
	if(!m_sliderContainer.IsValid())
		return 0;
	return (m_bHorizontal == true) ? m_sliderContainer->GetX() : m_sliderContainer->GetY();
}
void pragma::gui::types::WIScrollBar::SetSliderWidth(int w)
{
	if(!m_sliderContainer.IsValid())
		return;
	return (m_bHorizontal == true) ? m_sliderContainer->SetHeight(w) : m_sliderContainer->SetWidth(w);
}
void pragma::gui::types::WIScrollBar::SetSliderHeight(int h)
{
	if(!m_sliderContainer.IsValid())
		return;
	return (m_bHorizontal == true) ? m_sliderContainer->SetWidth(h) : m_sliderContainer->SetHeight(h);
}
void pragma::gui::types::WIScrollBar::SetSliderX(int x)
{
	if(!m_sliderContainer.IsValid())
		return;
	return (m_bHorizontal == true) ? m_sliderContainer->SetY(x) : m_sliderContainer->SetX(x);
}
void pragma::gui::types::WIScrollBar::SetSliderY(int y)
{
	if(!m_sliderContainer.IsValid())
		return;
	return (m_bHorizontal == true) ? m_sliderContainer->SetX(y) : m_sliderContainer->SetY(y);
}
void pragma::gui::types::WIScrollBar::SetSliderPos(int x, int y)
{
	if(!m_sliderContainer.IsValid())
		return;
	(m_bHorizontal == true) ? m_sliderContainer->SetPos(y, x) : m_sliderContainer->SetPos(x, y);
}
void pragma::gui::types::WIScrollBar::SetSliderSize(int w, int h)
{
	if(!m_sliderContainer.IsValid())
		return;
	(m_bHorizontal == true) ? m_sliderContainer->SetSize(h, w) : m_sliderContainer->SetSize(w, h);
}
void pragma::gui::types::WIScrollBar::SetHorizontal(bool b)
{
	m_bHorizontal = b;
	if(auto *flexBox = static_cast<FlexBox*>(m_flexBox.get()); flexBox)
		flexBox->SetFlexDirection(b ? FlexDirection::Horizontal : FlexDirection::Vertical);
	if(auto *slider = static_cast<WIScrollBarSlider *>(m_slider.get()); slider)
		slider->SetHorizontal(b);
}
bool pragma::gui::types::WIScrollBar::IsHorizontal() const { return m_bHorizontal; }
bool pragma::gui::types::WIScrollBar::IsVertical() const { return !m_bHorizontal; }

void pragma::gui::types::WIScrollBar::Initialize()
{
	WIBase::Initialize();
	SetVisible(false);
	AddStyleClass("scrollbar");

	auto container = WGUI::GetInstance().CreateVBox(this);
	container->AddStyleClass("scrollbar__content-area");
	m_flexBox = container->GetHandle();

	auto hButtonUp = container->CreateChild<WIButton>();
	WIButton *buttonUp = static_cast<WIButton *>(hButtonUp.get());
	buttonUp->AddStyleClass("scrollbar__button");
	buttonUp->AddStyleClass("scrollbar__button--start");
	buttonUp->AddCallback("OnPressed",
	  FunctionCallback<>::Create(std::bind(
	    [](WIHandle hScrollBar) {
		    if(!hScrollBar.IsValid())
			    return;
		    WIScrollBar *sb = static_cast<WIScrollBar *>(hScrollBar.get());
		    sb->AddScrollOffset(-1);
	    },
	    this->GetHandle())));

	m_sliderContainer = container->CreateChild<WIBase>();
	m_sliderContainer->AddStyleClass("scrollbar__slider");
	auto hThis = GetHandle();
	m_sliderContainer->AddCallback("OnSizeChanged", FunctionCallback<void>::Create([this, hThis]() mutable {
		if(!hThis.IsValid())
			return;
		UpdateSliderSize();
		SetScrollOffset(m_offset);
		UpdateSliderOffset();
	}));

	m_slider = m_sliderContainer->CreateChild<WIScrollBarSlider>();
	WIScrollBarSlider *slider = static_cast<WIScrollBarSlider *>(m_slider.get());
	slider->AddStyleClass("scrollbar__slider-bar");
	slider->AddCallback("OnDrag",
	  FunctionCallback<>::Create(std::bind(
		[](WIHandle hScrollBar) {
			if(!hScrollBar.IsValid())
				return;
			WIScrollBar *sb = static_cast<WIScrollBar *>(hScrollBar.get());
			if(!sb->m_slider.IsValid())
				return;
			WIScrollBarSlider *slider = static_cast<WIScrollBarSlider *>(sb->m_slider.get());
			int y = slider->GetSliderY();
			int hSlide = sb->GetSliderHeight() - slider->GetSliderHeight();
			float scale = (hSlide != 0) ? (y / float(hSlide)) : 0.f;
			sb->SetScrollOffset(static_cast<unsigned int>(std::roundf(scale * static_cast<float>(sb->m_numElements - sb->m_numListed))));
			slider->SetSliderY(y);
		},
		this->GetHandle())));

	auto hButtonDown = container->CreateChild<WIButton>();
	WIButton *buttonDown = static_cast<WIButton *>(hButtonDown.get());
	buttonDown->AddStyleClass("scrollbar__button");
	buttonDown->AddStyleClass("scrollbar__button--end");
	buttonDown->AddCallback("OnPressed",
	  FunctionCallback<>::Create(std::bind(
	    [](WIHandle hScrollBar) {
		    if(!hScrollBar.IsValid())
			    return;
		    auto *sb = static_cast<WIScrollBar *>(hScrollBar.get());
		    sb->AddScrollOffset(1);
	    },
	    this->GetHandle())));
}

///////////////////////////////////////////

pragma::gui::types::WIScrollBarSlider::WIScrollBarSlider() : WIBase(), m_moveOffset(0), m_moveOrigin(0), m_bMoving(false), m_bHorizontal(false) { RegisterCallback<void>("OnDrag"); }

void pragma::gui::types::WIScrollBarSlider::Initialize()
{
	WIBase::Initialize();
	SetMouseInputEnabled(true);
}

int pragma::gui::types::WIScrollBarSlider::GetSliderWidth() const { return (m_bHorizontal == true) ? GetHeight() : GetWidth(); }
int pragma::gui::types::WIScrollBarSlider::GetSliderHeight() const { return (m_bHorizontal == true) ? GetWidth() : GetHeight(); }
int pragma::gui::types::WIScrollBarSlider::GetSliderX() const { return (m_bHorizontal == true) ? GetY() : GetX(); }
int pragma::gui::types::WIScrollBarSlider::GetSliderY() const { return (m_bHorizontal == true) ? GetX() : GetY(); }
void pragma::gui::types::WIScrollBarSlider::SetSliderWidth(int w) { return (m_bHorizontal == true) ? SetHeight(w) : SetWidth(w); }
void pragma::gui::types::WIScrollBarSlider::SetSliderHeight(int h) { return (m_bHorizontal == true) ? SetWidth(h) : SetHeight(h); }
void pragma::gui::types::WIScrollBarSlider::SetSliderX(int x) { return (m_bHorizontal == true) ? SetY(x) : SetX(x); }
void pragma::gui::types::WIScrollBarSlider::SetSliderY(int y) { return (m_bHorizontal == true) ? SetX(y) : SetY(y); }
void pragma::gui::types::WIScrollBarSlider::SetSliderPos(int x, int y) { (m_bHorizontal == true) ? SetPos(y, x) : SetPos(x, y); }
void pragma::gui::types::WIScrollBarSlider::SetSliderSize(int w, int h) { (m_bHorizontal == true) ? SetSize(h, w) : SetSize(w, h); }
pragma::util::EventReply pragma::gui::types::WIScrollBarSlider::MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods)
{
	if(WIBase::MouseCallback(button, state, mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(button == platform::MouseButton::Left) {
		if(state == platform::KeyState::Press) {
			int x, y;
			WGUI::GetInstance().GetMousePos(x, y);
			if(IsVertical()) {
				m_moveOffset = y;
				m_moveOrigin = GetPos().y;
			}
			else {
				m_moveOffset = x;
				m_moveOrigin = GetPos().x;
			}
			m_bMoving = true;
			EnableThinking();
		}
		else if(state == platform::KeyState::Release)
			StopDragging();
	}
	return util::EventReply::Handled;
}

void pragma::gui::types::WIScrollBarSlider::StopDragging()
{
	UpdatePosition();
	m_moveOffset = 0;
	m_moveOrigin = 0;
	m_bMoving = false;
	DisableThinking();
}

void pragma::gui::types::WIScrollBarSlider::UpdatePosition()
{
	if(m_bMoving == false)
		return;
	int x, y;
	WGUI::GetInstance().GetMousePos(x, y);
	auto *parent = GetParent();
	if(IsVertical()) {
		int offset = y - m_moveOffset;
		int yNew = m_moveOrigin + offset;
		int h = GetHeight();
		int32_t min = 0;
		auto max = (parent ? GetParent()->GetHeight() : 0) -h;
		yNew = math::clamp(yNew, min, max);
		SetY(yNew);
	}
	else {
		int offset = x - m_moveOffset;
		int xNew = m_moveOrigin + offset;
		int w = GetWidth();
		int32_t min = 0;
		auto max = (parent ? GetParent()->GetWidth() : 0) -w;
		xNew = math::clamp(xNew, min, max);
		SetX(xNew);
	}
	CallCallbacks<void>("OnDrag");
}

void pragma::gui::types::WIScrollBarSlider::Think(const std::shared_ptr<prosper::IPrimaryCommandBuffer> &drawCmd)
{
	WIBase::Think(drawCmd);
	UpdatePosition();
}

void pragma::gui::types::WIScrollBarSlider::SetHorizontal(bool b)
{
	StopDragging();
	m_bHorizontal = b;
}
bool pragma::gui::types::WIScrollBarSlider::IsHorizontal() const { return m_bHorizontal; }
bool pragma::gui::types::WIScrollBarSlider::IsVertical() const { return !m_bHorizontal; }

pragma::util::EventReply pragma::gui::types::WIScrollBarSlider::ScrollCallback(Vector2 offset, bool offsetAsPixels)
{
	if(WIBase::ScrollCallback(offset, offsetAsPixels) == util::EventReply::Handled)
		return util::EventReply::Handled;
	WIBase *parent = GetParent();
	if(parent == nullptr)
		return util::EventReply::Handled;
	return parent->ScrollCallback(offset, offsetAsPixels);
}
