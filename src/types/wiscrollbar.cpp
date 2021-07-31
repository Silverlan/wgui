/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wiscrollbar.h"
#include "wgui/types/wirect.h"
#include "wgui/types/wibutton.h"
#include <prosper_window.hpp>

LINK_WGUI_TO_CLASS(WIScrollBar,WIScrollBar);

WIScrollBar::WIScrollBar()
	: WIBase(),m_bHorizontal(false),m_offset(0),
	m_numElements(0),m_numListed(0),
	m_scrollAmount(1)
{
	SetMouseInputEnabled(true);
	SetKeyboardInputEnabled(true);

	RegisterCallback<void,unsigned int>("OnScrollOffsetChanged");
}

WIScrollBar::~WIScrollBar()
{}
unsigned int WIScrollBar::GetScrollAmount() {return m_scrollAmount;}
void WIScrollBar::SetScrollAmount(unsigned int am)
{
	if(am > m_numListed)
		am = m_numListed;
	m_scrollAmount = am;
}
unsigned int WIScrollBar::GetScrollOffset() {return m_offset;}
void WIScrollBar::SetScrollOffset(unsigned int offset)
{
	int diff = int(m_numElements) -int(m_numListed);
	if(diff < 0)
		diff = 0;
	auto udiff = static_cast<unsigned int>(diff);
	if(offset > udiff)
		offset = udiff;
	if(m_offset == offset)
		return;
	m_offset = offset;
	UpdateSliderOffset();
	CallCallbacks<void,unsigned int>("OnScrollOffsetChanged",offset);
}
util::EventReply WIScrollBar::ScrollCallback(Vector2 offset)
{
	if(WIBase::ScrollCallback(offset) == util::EventReply::Handled)
		return util::EventReply::Handled;
	auto scrollAmount = GetScrollAmount();
	auto &window = WGUI::GetInstance().GetContext().GetWindow();
	auto isShiftDown = (window->GetKeyState(GLFW::Key::LeftShift) != GLFW::KeyState::Release || window->GetKeyState(GLFW::Key::RightShift) != GLFW::KeyState::Release) ? true : false;
	if(isShiftDown)
		scrollAmount = m_numListed;
	AddScrollOffset(static_cast<int>(-offset.y *static_cast<double>(scrollAmount)));
	return util::EventReply::Handled;
}
util::EventReply WIScrollBar::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	if(WIBase::MouseCallback(button,state,mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(state == GLFW::KeyState::Press && button == GLFW::MouseButton::Left)
	{
		if(m_slider.IsValid())
		{
			int mouseX,mouseY;
			GetMousePos(&mouseX,&mouseY);
			WIScrollBarSlider *slider = static_cast<WIScrollBarSlider*>(m_slider.get());
			if(IsHorizontal())
			{
				int x = slider->GetX();
				if(mouseX <= x)
					AddScrollOffset(-static_cast<int>(m_numListed));
				else
					AddScrollOffset(static_cast<int>(m_numListed));
			}
			else
			{
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
void WIScrollBar::AddScrollOffset(int scroll)
{
	int newOffset = int(m_offset) +scroll;
	if(newOffset < 0)
		newOffset = 0;
	SetScrollOffset(newOffset);
}

uint32_t WIScrollBar::GetElementCount() const {return m_numElements;}
uint32_t WIScrollBar::GetScrollElementCount() const {return m_numListed;}
uint32_t WIScrollBar::GetBottomScrollOffset()
{
	return umath::min(GetScrollOffset() +GetScrollElementCount(),GetElementCount());
}

void WIScrollBar::SetUp(unsigned int numElementsListed,unsigned int numElementsTotal)
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

void WIScrollBar::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	if(m_buttonUp.IsValid())
	{
		WIButton *button = static_cast<WIButton*>(m_buttonUp.get());
		if(m_bHorizontal)
			button->SetSize(y,y);
		else
			button->SetSize(x,x);
	}
	if(m_buttonDown.IsValid())
	{
		WIButton *button = static_cast<WIButton*>(m_buttonDown.get());
		if(m_bHorizontal)
			button->SetSize(y,y);
		else
			button->SetSize(x,x);
		if(m_bHorizontal)
			button->SetPos(x -y,0);
		else
			button->SetPos(0,y -x);
	}
	if(m_slider.IsValid())
	{
		WIScrollBarSlider *slider = static_cast<WIScrollBarSlider*>(m_slider.get());
		if(m_bHorizontal)
		{
			slider->SetHeight(y);
			slider->SetLimits(y,x -y);
		}
		else
		{
			slider->SetWidth(x);
			slider->SetLimits(x,y -x);
		}
	}
	UpdateSliderSize();
	UpdateSliderOffset();
}
void WIScrollBar::UpdateSliderOffset()
{
	if(m_slider.IsValid())
	{
		int h = GetSliderHeight();
		int w = GetSliderWidth();
		WIScrollBarSlider *slider = static_cast<WIScrollBarSlider*>(m_slider.get());
		int hSlide = (h -w *2) -slider->GetSliderHeight();
		slider->SetSliderY(
			w +static_cast<int>(static_cast<float>(m_offset) /static_cast<float>(static_cast<int>(m_numElements) -static_cast<int>(m_numListed)) *static_cast<float>(hSlide))
		);
	}
}
void WIScrollBar::UpdateSliderSize()
{
	if(m_slider.IsValid())
	{
		WIScrollBarSlider *slider = static_cast<WIScrollBarSlider*>(m_slider.get());
		int h = (m_numElements > 0u) ?
			static_cast<int>(static_cast<float>(GetSliderHeight()) *(static_cast<float>(m_numListed) /static_cast<float>(m_numElements))) :
			0;
		if(h == 0)
			h = 1;
		auto minH = umath::min(slider->GetParent()->GetHeight(),20);
		h = umath::max(h,minH);
		slider->SetSliderSize(GetSliderWidth(),h);
		slider->SetSliderPos(0,GetSliderWidth());
	}
}
int WIScrollBar::GetSliderWidth() {return (m_bHorizontal == true) ? GetHeight() : GetWidth();}
int WIScrollBar::GetSliderHeight() {return (m_bHorizontal == true) ? GetWidth() : GetHeight();}
int WIScrollBar::GetSliderX() {return (m_bHorizontal == true) ? GetY() : GetX();}
int WIScrollBar::GetSliderY() {return (m_bHorizontal == true) ? GetX() : GetY();}
void WIScrollBar::SetSliderWidth(int w) {return (m_bHorizontal == true) ? SetHeight(w) : SetWidth(w);}
void WIScrollBar::SetSliderHeight(int h) {return (m_bHorizontal == true) ? SetWidth(h) : SetHeight(h);}
void WIScrollBar::SetSliderX(int x) {return (m_bHorizontal == true) ? SetY(x) : SetX(x);}
void WIScrollBar::SetSliderY(int y) {return (m_bHorizontal == true) ? SetX(y) : SetY(y);}
void WIScrollBar::SetSliderPos(int x,int y) {(m_bHorizontal == true) ? SetPos(y,x) : SetPos(x,y);}
void WIScrollBar::SetSliderSize(int w,int h) {(m_bHorizontal == true) ? SetSize(h,w) : SetSize(w,h);}
void WIScrollBar::SetHorizontal(bool b)
{
	m_bHorizontal = b;
	if(m_slider.IsValid())
	{
		WIScrollBarSlider *slider = static_cast<WIScrollBarSlider*>(m_slider.get());
		slider->SetHorizontal(b);
	}
}
bool WIScrollBar::IsHorizontal() {return m_bHorizontal;}
bool WIScrollBar::IsVertical() {return !m_bHorizontal;}

void WIScrollBar::Initialize()
{
	WIBase::Initialize();
	SetVisible(false);
	m_buttonUp = CreateChild<WIButton>();
	WIButton *buttonUp = static_cast<WIButton*>(m_buttonUp.get());
	buttonUp->AddCallback("OnPressed",FunctionCallback<>::Create(std::bind([](WIHandle hScrollBar) {
		if(!hScrollBar.IsValid())
			return;
		WIScrollBar *sb = static_cast<WIScrollBar*>(hScrollBar.get());
		sb->AddScrollOffset(-1);
	},this->GetHandle())));

	m_buttonDown = CreateChild<WIButton>();
	WIButton *buttonDown = static_cast<WIButton*>(m_buttonDown.get());
	buttonDown->AddCallback("OnPressed",FunctionCallback<>::Create(std::bind([](WIHandle hScrollBar) {
		if(!hScrollBar.IsValid())
			return;
		WIScrollBar *sb = static_cast<WIScrollBar*>(hScrollBar.get());
		sb->AddScrollOffset(1);
	},this->GetHandle())));

	m_slider = CreateChild<WIScrollBarSlider>();
	WIScrollBarSlider *slider = static_cast<WIScrollBarSlider*>(m_slider.get());
	slider->SetColor(0.75f,0.75f,0.75f,1.f);
	slider->AddCallback("OnPressed",FunctionCallback<>::Create(std::bind([](WIHandle hScrollBar) {
		if(!hScrollBar.IsValid())
			return;

	},this->GetHandle())));
	slider->AddCallback("OnDrag",FunctionCallback<>::Create(std::bind([](WIHandle hScrollBar) {
		if(!hScrollBar.IsValid())
			return;
		WIScrollBar *sb = static_cast<WIScrollBar*>(hScrollBar.get());
		if(!sb->m_slider.IsValid())
			return;
		WIScrollBarSlider *slider = static_cast<WIScrollBarSlider*>(sb->m_slider.get());
		int y = slider->GetSliderY();
		int hSlide = sb->GetSliderHeight() -sb->GetSliderWidth() *2 -slider->GetSliderHeight();
		float scale = (y -sb->GetSliderWidth()) /float(hSlide);
		sb->SetScrollOffset(static_cast<unsigned int>(std::roundf(scale *static_cast<float>(sb->m_numElements -sb->m_numListed))));
		slider->SetSliderY(y);
	},this->GetHandle())));
}

///////////////////////////////////////////

WIScrollBarSlider::WIScrollBarSlider()
	: WIRect(),m_moveOffset(0),m_moveOrigin(0),m_bMoving(false),
	m_min(0),m_max(0),m_bHorizontal(false)
{
	RegisterCallback<void>("OnDrag");
}

void WIScrollBarSlider::Initialize()
{
	WIRect::Initialize();
	SetMouseInputEnabled(true);
}

void WIScrollBarSlider::SetLimits(int min,int max)
{
	m_min = min;
	m_max = max;
}

int WIScrollBarSlider::GetSliderWidth() {return (m_bHorizontal == true) ? GetHeight() : GetWidth();}
int WIScrollBarSlider::GetSliderHeight() {return (m_bHorizontal == true) ? GetWidth() : GetHeight();}
int WIScrollBarSlider::GetSliderX() {return (m_bHorizontal == true) ? GetY() : GetX();}
int WIScrollBarSlider::GetSliderY() {return (m_bHorizontal == true) ? GetX() : GetY();}
void WIScrollBarSlider::SetSliderWidth(int w) {return (m_bHorizontal == true) ? SetHeight(w) : SetWidth(w);}
void WIScrollBarSlider::SetSliderHeight(int h) {return (m_bHorizontal == true) ? SetWidth(h) : SetHeight(h);}
void WIScrollBarSlider::SetSliderX(int x) {return (m_bHorizontal == true) ? SetY(x) : SetX(x);}
void WIScrollBarSlider::SetSliderY(int y) {return (m_bHorizontal == true) ? SetX(y) : SetY(y);}
void WIScrollBarSlider::SetSliderPos(int x,int y) {(m_bHorizontal == true) ? SetPos(y,x) : SetPos(x,y);}
void WIScrollBarSlider::SetSliderSize(int w,int h) {(m_bHorizontal == true) ? SetSize(h,w) : SetSize(w,h);}
util::EventReply WIScrollBarSlider::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	if(WIRect::MouseCallback(button,state,mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(button == GLFW::MouseButton::Left)
	{
		if(state == GLFW::KeyState::Press)
		{
			int x,y;
			WGUI::GetInstance().GetMousePos(x,y);
			if(IsVertical())
			{
				m_moveOffset = y;
				m_moveOrigin = GetPos().y;
			}
			else
			{
				m_moveOffset = x;
				m_moveOrigin = GetPos().x;
			}
			m_bMoving = true;
			EnableThinking();
		}
		else if(state == GLFW::KeyState::Release)
			StopDragging();
	}
	return util::EventReply::Handled;
}

void WIScrollBarSlider::StopDragging()
{
	UpdatePosition();
	m_moveOffset = 0;
	m_moveOrigin = 0;
	m_bMoving = false;
	DisableThinking();
}

void WIScrollBarSlider::UpdatePosition()
{
	if(m_bMoving == false)
		return;
	int x,y;
	WGUI::GetInstance().GetMousePos(x,y);
	if(IsVertical())
	{
		int offset = y -m_moveOffset;
		int yNew = m_moveOrigin +offset;
		int h = GetHeight();
		if(yNew < m_min)
			yNew = m_min;
		else if((yNew +h) > m_max)
			yNew = m_max -h;
		SetY(yNew);
	}
	else
	{
		int offset = x -m_moveOffset;
		int xNew = m_moveOrigin +offset;
		int w = GetWidth();
		if(xNew < m_min)
			xNew = m_min;
		else if((xNew +w) > m_max)
			xNew = m_max -w;
		SetX(xNew);
	}
	CallCallbacks<void>("OnDrag");
}

void WIScrollBarSlider::Think()
{
	WIRect::Think();
	UpdatePosition();
}

void WIScrollBarSlider::SetHorizontal(bool b)
{
	StopDragging();
	m_bHorizontal = b;
}
bool WIScrollBarSlider::IsHorizontal() {return m_bHorizontal;}
bool WIScrollBarSlider::IsVertical() {return !m_bHorizontal;}

util::EventReply WIScrollBarSlider::ScrollCallback(Vector2 offset)
{
	if(WIRect::ScrollCallback(offset) == util::EventReply::Handled)
		return util::EventReply::Handled;
	WIBase *parent = GetParent();
	if(parent == NULL)
		return util::EventReply::Handled;
	return parent->ScrollCallback(offset);
}
