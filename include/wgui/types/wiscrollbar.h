/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WISCROLLBAR_H__
#define __WISCROLLBAR_H__
#include "wgui/wibase.h"
#include "wgui/types/wirect.h"
#include "wgui/types/wibutton.h"
#include "wgui/wihandle.h"

class DLLWGUI WIScrollBar;
class DLLWGUI WIScrollBarSlider
	: public WIRect
{
public:
	friend WIScrollBar;
protected:
	bool m_bHorizontal;
	int m_moveOrigin;
	int m_moveOffset;
	int m_min;
	int m_max;
	bool m_bMoving;
	void UpdatePosition();
	void StopDragging();
	int GetSliderWidth();
	int GetSliderHeight();
	int GetSliderX();
	int GetSliderY();
	void SetSliderWidth(int w);
	void SetSliderHeight(int h);
	void SetSliderX(int x);
	void SetSliderY(int y);
	void SetSliderPos(int x,int y);
	void SetSliderSize(int w,int h);
public:
	WIScrollBarSlider();
	virtual ~WIScrollBarSlider() override = default;
	virtual void Initialize() override;
	virtual util::EventReply ScrollCallback(Vector2 offset) override;
	virtual util::EventReply MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual void Think() override;
	void SetHorizontal(bool b);
	bool IsHorizontal();
	bool IsVertical();
	void SetLimits(int min,int max);
};

class DLLWGUI WIScrollBar
	: public WIBase
{
protected:
	bool m_bHorizontal;
	unsigned int m_offset;
	unsigned int m_numElements;
	unsigned int m_numListed;
	unsigned int m_scrollAmount;
	WIHandle m_buttonUp;
	WIHandle m_buttonDown;
	WIHandle m_slider;
	int GetSliderWidth();
	int GetSliderHeight();
	int GetSliderX();
	int GetSliderY();
	void SetSliderWidth(int w);
	void SetSliderHeight(int h);
	void SetSliderX(int x);
	void SetSliderY(int y);
	void SetSliderPos(int x,int y);
	void SetSliderSize(int w,int h);
	void UpdateSliderSize();
	void UpdateSliderOffset();
public:
	WIScrollBar();
	virtual ~WIScrollBar() override;
	void SetUp(unsigned int numElementsListed,unsigned int numElementsTotal);
	unsigned int GetScrollAmount();
	void SetScrollAmount(unsigned int am);
	unsigned int GetScrollOffset();
	void SetScrollOffset(unsigned int offset);
	void AddScrollOffset(int scroll);
	uint32_t GetElementCount() const;
	uint32_t GetScrollElementCount() const;
	uint32_t GetBottomScrollOffset();
	virtual void Initialize() override;
	void SetHorizontal(bool b);
	bool IsHorizontal();
	bool IsVertical();
	virtual util::EventReply ScrollCallback(Vector2 offset) override;
	virtual util::EventReply MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods) override;
	virtual void SetSize(int x,int y) override;
};

#endif
