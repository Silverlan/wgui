/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/wiscrollbar.h"
#include "wgui/types/wirect.h"
#include "wgui/types/wibutton.h"

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
void WIScrollBar::ScrollCallback(Vector2 offset)
{
	WIBase::ScrollCallback(offset);
	AddScrollOffset(static_cast<int>(-offset.y *static_cast<double>(GetScrollAmount())));
}
void WIScrollBar::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	WIBase::MouseCallback(button,state,mods);
	if(state == GLFW::KeyState::Press && button == GLFW::MouseButton::Left)
	{
		if(m_slider.IsValid())
		{
			int mouseX,mouseY;
			GetMousePos(&mouseX,&mouseY);
			WIScrollBarSlider *slider = m_slider.get<WIScrollBarSlider>();
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
}
void WIScrollBar::AddScrollOffset(int scroll)
{
	int newOffset = int(m_offset) +scroll;
	if(newOffset < 0)
		newOffset = 0;
	SetScrollOffset(newOffset);
}

void WIScrollBar::SetUp(unsigned int numElementsListed,unsigned int numElementsTotal)
{
	if(numElementsListed == 0 || numElementsListed >= numElementsTotal)
	{
		SetVisible(false);
		return;
	}
	SetVisible(true);
	m_numElements = numElementsTotal;
	m_numListed = numElementsListed;
	if(m_scrollAmount > m_numListed)
		m_scrollAmount = m_numListed;
	UpdateSliderSize();
	SetScrollOffset(m_offset);
}

void WIScrollBar::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	if(m_buttonUp.IsValid())
	{
		WIButton *button = m_buttonUp.get<WIButton>();
		if(m_bHorizontal)
			button->SetSize(y,y);
		else
			button->SetSize(x,x);
	}
	if(m_buttonDown.IsValid())
	{
		WIButton *button = m_buttonDown.get<WIButton>();
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
		WIScrollBarSlider *slider = m_slider.get<WIScrollBarSlider>();
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
		WIScrollBarSlider *slider = m_slider.get<WIScrollBarSlider>();
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
		WIScrollBarSlider *slider = m_slider.get<WIScrollBarSlider>();
		int h = (m_numElements > 0u) ?
			static_cast<int>(static_cast<float>(GetSliderHeight()) *(static_cast<float>(m_numListed) /static_cast<float>(m_numElements))) :
			0;
		if(h == 0)
			h = 1;
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
		WIScrollBarSlider *slider = m_slider.get<WIScrollBarSlider>();
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
	WIButton *buttonUp = m_buttonUp.get<WIButton>();
	buttonUp->AddCallback("OnPressed",FunctionCallback<>::Create(std::bind([](WIHandle hScrollBar) {
		if(!hScrollBar.IsValid())
			return;
		WIScrollBar *sb = hScrollBar.get<WIScrollBar>();
		sb->AddScrollOffset(-1);
	},this->GetHandle())));

	m_buttonDown = CreateChild<WIButton>();
	WIButton *buttonDown = m_buttonDown.get<WIButton>();
	buttonDown->AddCallback("OnPressed",FunctionCallback<>::Create(std::bind([](WIHandle hScrollBar) {
		if(!hScrollBar.IsValid())
			return;
		WIScrollBar *sb = hScrollBar.get<WIScrollBar>();
		sb->AddScrollOffset(1);
	},this->GetHandle())));

	m_slider = CreateChild<WIScrollBarSlider>();
	WIScrollBarSlider *slider = m_slider.get<WIScrollBarSlider>();
	slider->SetColor(0.75f,0.75f,0.75f,1.f);
	slider->AddCallback("OnPressed",FunctionCallback<>::Create(std::bind([](WIHandle hScrollBar) {
		if(!hScrollBar.IsValid())
			return;

	},this->GetHandle())));
	slider->AddCallback("OnDrag",FunctionCallback<>::Create(std::bind([](WIHandle hScrollBar) {
		if(!hScrollBar.IsValid())
			return;
		WIScrollBar *sb = hScrollBar.get<WIScrollBar>();
		if(!sb->m_slider.IsValid())
			return;
		WIScrollBarSlider *slider = sb->m_slider.get<WIScrollBarSlider>();
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
void WIScrollBarSlider::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	WIRect::MouseCallback(button,state,mods);
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
		}
		else if(state == GLFW::KeyState::Release)
			StopDragging();
	}
}

void WIScrollBarSlider::StopDragging()
{
	UpdatePosition();
	m_moveOffset = 0;
	m_moveOrigin = 0;
	m_bMoving = false;
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

void WIScrollBarSlider::ScrollCallback(Vector2 offset)
{
	WIRect::ScrollCallback(offset);
	WIBase *parent = GetParent();
	if(parent == NULL)
		return;
	parent->ScrollCallback(offset);
}