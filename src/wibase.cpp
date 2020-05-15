/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include <unordered_map>
#include <algorithm>
#include "wgui/wibase.h"
#include "wgui/wiskin.h"
#include "wgui/wgui.h"
#include "wgui/wihandle.h"
#include "wgui/shaders/wishader.hpp"
#include "wgui/types/wicontextmenu.hpp"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <sharedutils/scope_guard.h>
#include <atomic>
#include <sharedutils/property/util_property_color.hpp>

LINK_WGUI_TO_CLASS(WIBase,WIBase);

#pragma optimize("",off)
Vector4 WIBase::DrawInfo::GetColor(WIBase &el) const
{
	Vector4 color;
	if(this->color.has_value())
		color = *this->color;
	else
		color = el.GetColor().ToVector4();
	color.a *= WIBase::RENDER_ALPHA;
	return color;
}

/////////////

std::deque<WIHandle> WIBase::m_focusTrapStack;
float WIBase::RENDER_ALPHA = 1.f;

WIBase::WIBase()
	: CallbackHandler(),m_cursor(GLFW::Cursor::Shape::Default),
	m_color(util::ColorProperty::Create(Color::White)),
	m_pos(util::Vector2iProperty::Create()),
	m_size(util::Vector2iProperty::Create()),
	m_bVisible(util::BoolProperty::Create(true)),
	m_bHasFocus(util::BoolProperty::Create(false)),
	m_bMouseInBounds(util::BoolProperty::Create(false))
{
	RegisterCallback<void>("OnFocusGained");
	RegisterCallback<void>("OnFocusKilled");
	RegisterCallback<void>("SetPos");
	RegisterCallback<void>("SetSize");
	RegisterCallback<void,int32_t,int32_t>("OnCursorMoved");
	RegisterCallback<void>("Think");
	RegisterCallback<void>("OnRemove");
	RegisterCallback<void,WIBase*>("OnChildAdded");
	RegisterCallback<void,WIBase*>("OnChildRemoved");
	RegisterCallback<void>("OnCursorEntered");
	RegisterCallback<void>("OnCursorExited");
	RegisterCallbackWithOptionalReturn<util::EventReply,GLFW::MouseButton,GLFW::KeyState,GLFW::Modifier>("OnMouseEvent");
	RegisterCallbackWithOptionalReturn<util::EventReply>("OnMousePressed");
	RegisterCallbackWithOptionalReturn<util::EventReply>("OnMouseReleased");
	RegisterCallbackWithOptionalReturn<util::EventReply>("OnDoubleClick");
	RegisterCallbackWithOptionalReturn<util::EventReply,std::reference_wrapper<const GLFW::Joystick>,uint32_t,GLFW::KeyState>("OnJoystickEvent");
	RegisterCallbackWithOptionalReturn<util::EventReply,GLFW::Key,int,GLFW::KeyState,GLFW::Modifier>("OnKeyEvent");
	RegisterCallbackWithOptionalReturn<util::EventReply,int,GLFW::Modifier>("OnCharEvent");
	RegisterCallbackWithOptionalReturn<util::EventReply,Vector2>("OnScroll");

	//auto hThis = GetHandle();
	m_pos->AddCallback([this](std::reference_wrapper<const Vector2i> oldPos,std::reference_wrapper<const Vector2i> pos) {
		for(auto &pair : m_attachments)
			pair.second->UpdateAbsolutePosition();
		if(umath::is_flag_set(m_stateFlags,StateFlags::UpdatingAnchorTransform) == false)
		{
			UpdateAnchorTopLeftPixelOffsets();
			UpdateAnchorBottomRightPixelOffsets();
		}
		CallCallbacks<void>("SetPos");

		UpdateParentAutoSizeToContents();
	});
	m_size->AddCallback([this](std::reference_wrapper<const Vector2i> oldSize,std::reference_wrapper<const Vector2i> size) {
		for(auto &pair : m_attachments)
			pair.second->UpdateAbsolutePosition();
		if(umath::is_flag_set(m_stateFlags,StateFlags::UpdatingAnchorTransform) == false)
			UpdateAnchorBottomRightPixelOffsets();
		CallCallbacks<void>("SetSize");
		for(auto &hChild : m_children)
		{
			if(hChild.IsValid() == false)
				continue;
			hChild->UpdateAnchorTransform();
		}

		UpdateParentAutoSizeToContents();
	});
	m_bVisible->AddCallback([this](std::reference_wrapper<const bool> oldVisible,std::reference_wrapper<const bool> visible) {
		UpdateVisibility();
		UpdateParentAutoSizeToContents();
	});
	umath::set_flag(m_stateFlags,StateFlags::ParentVisible);
}
WIBase::~WIBase()
{
	if(m_cbAutoAlign.IsValid())
		m_cbAutoAlign.Remove();
	if(m_cbAutoCenterX.IsValid())
		m_cbAutoCenterX.Remove();
	if(m_cbAutoCenterXOwn.IsValid())
		m_cbAutoCenterXOwn.Remove();
	if(m_cbAutoCenterY.IsValid())
		m_cbAutoCenterY.Remove();
	if(m_cbAutoCenterYOwn.IsValid())
		m_cbAutoCenterYOwn.Remove();
	m_handle->Invalidate();
	m_handle = nullptr;
	for(unsigned int i=0;i<m_children.size();i++)
	{
		if(m_children[i].IsValid())
			m_children[i]->Remove();
	}
	m_fade = nullptr;
}
void WIBase::UpdateParentAutoSizeToContents()
{
	if(m_parent == nullptr || umath::is_flag_set(m_parent->m_stateFlags,StateFlags::AutoSizeToContentsX | StateFlags::AutoSizeToContentsY) == false || IsBackgroundElement())
		return;
	auto updateX = true;
	auto updateY = true;
	if(HasAnchor())
	{
		// If we have an axis anchor enabled that would change our size,
		// we mustn't let our parent resize itself on that axis or we could end
		// up in an infinite recursion!
		float left,top,right,bottom;
		GetAnchor(left,top,right,bottom);
		if(left != right) // Horizontal axis anchor
			updateX = false;
		if(top != bottom) // Vertical axis anchor
			updateY = false;
	}
	m_parent->UpdateAutoSizeToContents(updateX,updateY);
}
void WIBase::UpdateAutoSizeToContents(bool updateX,bool updateY)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::AutoSizeToContentsX | StateFlags::AutoSizeToContentsY))
	{
		SizeToContents(
			updateX && umath::is_flag_set(m_stateFlags,StateFlags::AutoSizeToContentsX),
			updateY && umath::is_flag_set(m_stateFlags,StateFlags::AutoSizeToContentsY)
		);
	}
}
void WIBase::UpdateVisibility()
{
	auto *parent = GetParent();
	umath::set_flag(m_stateFlags,StateFlags::ParentVisible,parent ? parent->IsVisible() : true);

	for(auto &hChild : *GetChildren())
	{
		if(hChild.IsValid() == false)
			continue;
		hChild->UpdateVisibility();
	}

	// If we just became visible and we have an update schedule, we need to inform the GUI instance
	if(IsVisible() && umath::is_flag_set(m_stateFlags,StateFlags::UpdateScheduledBit))
		WGUI::GetInstance().m_bGUIUpdateRequired = true;

	UpdateThink();
}
void WIBase::SetShouldScissor(bool b) {umath::set_flag(m_stateFlags,StateFlags::ShouldScissorBit,b);}
bool WIBase::GetShouldScissor() const {return umath::is_flag_set(m_stateFlags,StateFlags::ShouldScissorBit);}
GLFW::Cursor::Shape WIBase::GetCursor() const {return m_cursor;}
void WIBase::SetCursor(GLFW::Cursor::Shape cursor) {m_cursor = cursor;}
void WIBase::RemoveSafely()
{
	SetVisible(false);
	SetParent(nullptr);
	WGUI::GetInstance().RemoveSafely(*this);
	umath::set_flag(m_stateFlags,StateFlags::RemoveScheduledBit,true);
}
void WIBase::RemoveOnRemoval(WIBase *other)
{
	if(other == nullptr)
		return;
	auto hOther = other->GetHandle();
	CallOnRemove(FunctionCallback<void>::Create([hOther]() {
		if(hOther.IsValid() == false)
			return;
		hOther.get()->Remove();
	}));
}
void WIBase::ScheduleUpdate()
{
	//if(umath::is_flag_set(m_stateFlags,StateFlags::UpdateScheduledBit))
	//	return;
	WGUI::GetInstance().ScheduleElementForUpdate(*this);
}
void WIBase::SetAutoAlignToParent(bool bX,bool bY,bool bReload)
{
	if(bReload == false && bX == umath::is_flag_set(m_stateFlags,StateFlags::AutoAlignToParentXBit) && bY == umath::is_flag_set(m_stateFlags,StateFlags::AutoAlignToParentYBit))
		return;
	umath::set_flag(m_stateFlags,StateFlags::AutoAlignToParentXBit,bX);
	umath::set_flag(m_stateFlags,StateFlags::AutoAlignToParentYBit,bY);
	if(m_cbAutoAlign.IsValid())
		m_cbAutoAlign.Remove();
	if(umath::is_flag_set(m_stateFlags,StateFlags::AutoAlignToParentXBit) == false && umath::is_flag_set(m_stateFlags,StateFlags::AutoAlignToParentYBit) == false)
		return;
	auto *parent = GetParent();
	if(parent == nullptr)
		return;
	auto hEl = GetHandle();
	m_cbAutoAlign = parent->AddCallback("SetSize",FunctionCallback<>::Create([hEl]() {
		if(!hEl.IsValid())
			return;
		auto *p = hEl.get();
		auto *parent = p->GetParent();
		if(parent == nullptr)
			return;
		auto &pos = p->GetPos();
		auto size = p->GetSize();
		if((umath::is_flag_set(p->m_stateFlags,StateFlags::AutoAlignToParentXBit) == true && pos.x != 0) || (umath::is_flag_set(p->m_stateFlags,StateFlags::AutoAlignToParentYBit) == true && pos.y != 0))
			p->SetPos(0,0);
		if(umath::is_flag_set(p->m_stateFlags,StateFlags::AutoAlignToParentXBit) == true)
			size.x = parent->GetWidth();
		if(umath::is_flag_set(p->m_stateFlags,StateFlags::AutoAlignToParentYBit) == true)
			size.y = parent->GetHeight();
		p->SetSize(size);
	}));
	m_cbAutoAlign(this);
}
void WIBase::SetAutoCenterToParentX(bool b,bool bReload)
{
	if(b == umath::is_flag_set(m_stateFlags,StateFlags::AutoCenterToParentXBit) && (bReload == false || b == false))
		return;
	if(m_cbAutoCenterX.IsValid())
		m_cbAutoCenterX.Remove();
	if(m_cbAutoCenterXOwn.IsValid())
		m_cbAutoCenterXOwn.Remove();
	umath::set_flag(m_stateFlags,StateFlags::AutoCenterToParentXBit,b);
	if(b == false)
		return;
	WIBase *parent = GetParent();
	if(parent == nullptr)
		return;
	auto hEl = GetHandle();
	auto cb = [hEl]() {
		if(!hEl.IsValid())
			return;
		auto *p = hEl.get();
		p->CenterToParentX();
	};
	m_cbAutoCenterX = parent->AddCallback("SetSize",FunctionCallback<>::Create(cb));
	m_cbAutoCenterXOwn = AddCallback("SetSize",FunctionCallback<>::Create(cb));
	m_cbAutoCenterX(this);
}
void WIBase::SetAutoCenterToParentY(bool b,bool bReload)
{
	if(b == umath::is_flag_set(m_stateFlags,StateFlags::AutoCenterToParentYBit) && (bReload == false || b == false))
		return;
	if(m_cbAutoCenterY.IsValid())
		m_cbAutoCenterY.Remove();
	if(m_cbAutoCenterYOwn.IsValid())
		m_cbAutoCenterYOwn.Remove();
	umath::set_flag(m_stateFlags,StateFlags::AutoCenterToParentYBit,b);
	if(b == false)
		return;
	WIBase *parent = GetParent();
	if(parent == nullptr)
		return;
	auto hEl = GetHandle();
	auto cb = [hEl]() {
		if(!hEl.IsValid())
			return;
		auto *p = hEl.get();
		p->CenterToParentY();
	};
	m_cbAutoCenterY = parent->AddCallback("SetSize",FunctionCallback<>::Create(cb));
	m_cbAutoCenterYOwn = AddCallback("SetSize",FunctionCallback<>::Create(cb));
	m_cbAutoCenterY(this);
}
void WIBase::SetAutoAlignToParent(bool bX,bool bY) {SetAutoAlignToParent(bX,bY,false);}
void WIBase::SetAutoAlignToParent(bool b) {SetAutoAlignToParent(b,b,false);}
void WIBase::SetAutoCenterToParentX(bool b) {SetAutoCenterToParentX(b,false);}
void WIBase::SetAutoCenterToParentY(bool b) {SetAutoCenterToParentY(b,false);}
void WIBase::SetAutoCenterToParent(bool b) {SetAutoCenterToParentX(b);SetAutoCenterToParentY(b);}
bool WIBase::GetAutoAlignToParentX() const {return umath::is_flag_set(m_stateFlags,StateFlags::AutoAlignToParentXBit);}
bool WIBase::GetAutoAlignToParentY() const {return umath::is_flag_set(m_stateFlags,StateFlags::AutoAlignToParentYBit);}
bool WIBase::GetAutoAlignToParent() const {return (GetAutoAlignToParentX() == true || GetAutoAlignToParentY() == true) ? true : false;}
bool WIBase::GetMouseMovementCheckEnabled() {return umath::is_flag_set(m_stateFlags,StateFlags::MouseCheckEnabledBit);}
void WIBase::SetMouseMovementCheckEnabled(bool b)
{
	umath::set_flag(m_stateFlags,StateFlags::MouseCheckEnabledBit,b);
	UpdateThink();
}
void WIBase::Initialize() {}
void WIBase::InitializeHandle()
{
	InitializeHandle<WIHandle>();
}
void WIBase::InitializeHandle(std::shared_ptr<WIHandle> handle)
{
	m_handle = handle;
}
const std::shared_ptr<void> &WIBase::GetUserData() {return m_userData.at(0);}
const std::shared_ptr<void> &WIBase::GetUserData2() {return m_userData.at(1);}
const std::shared_ptr<void> &WIBase::GetUserData3() {return m_userData.at(2);}
const std::shared_ptr<void> &WIBase::GetUserData4() {return m_userData.at(3);}
void WIBase::SetUserData(const std::shared_ptr<void> &userData) {m_userData.at(0) = userData;}
void WIBase::SetUserData2(const std::shared_ptr<void> &userData) {m_userData.at(1) = userData;}
void WIBase::SetUserData3(const std::shared_ptr<void> &userData) {m_userData.at(2) = userData;}
void WIBase::SetUserData4(const std::shared_ptr<void> &userData) {m_userData.at(3) = userData;}
WIHandle WIBase::GetHandle() const {return *m_handle;}
void WIBase::TrapFocus(bool b)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::TrapFocusBit) == b)
		return;
	umath::set_flag(m_stateFlags,StateFlags::TrapFocusBit,b);
	if(b == true)
		m_focusTrapStack.push_back(this->GetHandle());
	else
	{
		for(auto it=m_focusTrapStack.begin();it!=m_focusTrapStack.end();it++)
		{
			auto &hEl = *it;
			if(!hEl.IsValid())
				it = m_focusTrapStack.erase(it);
			else if(hEl.get() == this)
			{
				m_focusTrapStack.erase(it);
				break;
			}
		}
	}
}
bool WIBase::IsFocusTrapped() {return umath::is_flag_set(m_stateFlags,StateFlags::TrapFocusBit);}
std::string WIBase::GetClass() const {return m_class;}
void WIBase::CallOnRemove(CallbackHandle callback) {AddCallback("OnRemove",callback);}
void WIBase::SetZPos(int zpos)
{
	m_zpos = zpos;
	WIBase *parent = GetParent();
	if(parent == NULL)
		return;
	parent->UpdateChildOrder(this);
}
static void InsertGUIElement(std::vector<WIHandle> &elements,const WIHandle &hElement,std::optional<uint32_t> index)
{
	WIBase *pChild = hElement.get();
	if(elements.empty() || (index.has_value() && *index >= elements.size()))
		elements.push_back(hElement);
	else if(index.has_value())
		elements.insert(elements.begin() +*index,hElement);
	else
	{
		int zpos = pChild->GetZPos();
		int numElements = static_cast<int>(elements.size());
		for(int i=numElements -1;i>=0;i--)
		{
			if(elements[i].IsValid())
			{
				WIBase *p = elements[i].get();
				if(zpos >= p->GetZPos())
				{
					elements.insert(elements.begin() +i +1,hElement);
					return;
				}
			}
		}
		elements.insert(elements.begin(),hElement);
	}
}
void WIBase::UpdateChildOrder(WIBase *child)
{
	if(child != NULL)
	{
		for(unsigned int i=0;i<m_children.size();i++)
		{
			WIHandle &hElement = m_children[i];
			if(hElement.IsValid())
			{
				WIBase *pElement = hElement.get();
				if(pElement == child)
				{
					m_children.erase(m_children.begin() +i);
					InsertGUIElement(m_children,pElement->GetHandle(),{});
					break;
				}
			}
		}
		return;
	}
	std::vector<WIHandle> sorted;
	unsigned int numChildren = static_cast<unsigned int>(m_children.size());
	for(unsigned int i=0;i<numChildren;i++)
	{
		WIHandle &hChild = m_children[i];
		if(hChild.IsValid())
			InsertGUIElement(sorted,hChild,{});
	}
	m_children = sorted;
}
int WIBase::GetX() const {return (*m_pos)->x;}
int WIBase::GetY() const {return (*m_pos)->y;}
int WIBase::GetLeft() const {return GetX();}
int WIBase::GetTop() const {return GetY();}
int WIBase::GetRight() const {return GetX() +GetWidth();}
int WIBase::GetBottom() const {return GetY() +GetHeight();}
void WIBase::SetLeft(int32_t pos) {SetX(pos);}
void WIBase::SetRight(int32_t pos) {SetLeft(pos -GetWidth());}
void WIBase::SetTop(int32_t pos) {SetY(pos);}
void WIBase::SetBottom(int32_t pos) {SetTop(pos -GetHeight());}
Vector2i WIBase::GetEndPos() const {return Vector2i(GetRight(),GetBottom());}
void WIBase::SetX(int x) {SetPos(x,(*m_pos)->y);}
void WIBase::SetY(int y) {SetPos((*m_pos)->x,y);}
float WIBase::GetAspectRatio() const
{
	auto h = GetHeight();
	return (h != 0) ? (GetWidth() /static_cast<float>(h)) : 1.f;
}
void WIBase::SetWidth(int w,bool keepRatio)
{
	auto h = (*m_size)->y;
	if(keepRatio)
		h = w *(1.f /GetAspectRatio());
	SetSize(w,h);
}
void WIBase::SetHeight(int h,bool keepRatio)
{
	auto w = (*m_size)->x;
	if(keepRatio)
		w = h *GetAspectRatio();
	SetSize(w,h);
}
int WIBase::GetZPos() const {return m_zpos;}
float WIBase::GetAlpha() const {return m_color->GetValue().a /255.f;}
void WIBase::SetAlpha(float alpha)
{
	auto &col = GetColor();
	*m_color = Color(col.r,col.g,col.b,alpha *255.f);
}
void WIBase::GetMousePos(int *x,int *y) const
{
	auto &context = WGUI::GetInstance().GetContext();
	auto &window = context.GetWindow();
	auto cursorPos = window.GetCursorPos();
	Vector2i cur = GetAbsolutePos();
	cursorPos.x -= cur.x;
	cursorPos.y -= cur.y;
	if(x != nullptr)
		*x = static_cast<int>(cursorPos.x);
	if(y != nullptr)
		*y = static_cast<int>(cursorPos.y);
}
void WIBase::SizeToContents(bool x,bool y)
{
	int width = 0;
	int height = 0;
	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIHandle &child = m_children[i];
		if(child.IsValid() == false || child->IsBackgroundElement())
			continue;
		WIBase *gui = child.get();
		int xChild,yChild,wChild,hChild;
		gui->GetPos(&xChild,&yChild);
		gui->GetSize(&wChild,&hChild);
		wChild += xChild;
		hChild += yChild;
		if(wChild > width)
			width = wChild;
		if(hChild > height)
			height = hChild;
	}
	if(x && y)
		SetSize(width,height);
	else if(x)
		SetWidth(width);
	else if(y)
		SetHeight(height);
}
void WIBase::SetBackgroundElement(bool backgroundElement,bool autoAlignToParent)
{
	umath::set_flag(m_stateFlags,StateFlags::IsBackgroundElement,backgroundElement);
	if(backgroundElement && autoAlignToParent)
	{
		//SetPos(0,0);
		//SetSize(GetParent()->GetSize());
		//SetAnchor(0.f,0.f,1.f,1.f);
		SetAutoAlignToParent(true);
		SetZPos(-100);
	}
}
bool WIBase::IsBackgroundElement() const {return umath::is_flag_set(m_stateFlags,StateFlags::IsBackgroundElement);}
bool WIBase::HasFocus() {return *m_bHasFocus;}
void WIBase::RequestFocus()
{
	if(HasFocus())
		return;
	if(!WGUI::GetInstance().SetFocusedElement(this))
		return;
	*m_bHasFocus = true;
	OnFocusGained();
}
const util::PBoolProperty &WIBase::GetFocusProperty() const {return m_bHasFocus;}
void WIBase::KillFocus(bool bForceKill)
{
	if(!HasFocus() || (bForceKill == false && IsFocusTrapped() && IsVisible()))
		return;
	*m_bHasFocus = false;
	WGUI::GetInstance().SetFocusedElement(NULL);
	if(bForceKill == false)
	{
		for(auto it=m_focusTrapStack.rbegin();it!=m_focusTrapStack.rend();)
		{
			auto &hEl = *it;
			++it;
			if(!hEl.IsValid())
				it = std::deque<WIHandle>::reverse_iterator(m_focusTrapStack.erase(it.base()));
			else if(hEl.get() != this && hEl->IsVisible())
			{
				hEl->RequestFocus();
				break;
			}
		}
		/*
		WIBase *parent = GetParent();
		WIBase *root = WGUI::GetBaseElement();
		while(parent != NULL && parent != root)
		{
			if(parent->IsFocusTrapped() && parent->IsVisible())
			{
				parent->RequestFocus();
				break;
			}
			parent = parent->GetParent();
		}
		*/
	}
	OnFocusKilled();
}
bool WIBase::IsDescendant(WIBase *el)
{
	if(el == this)
		return false;
	std::vector<WIHandle> *children = GetChildren();
	for(unsigned int i=0;i<children->size();i++)
	{
		WIHandle &hChild = (*children)[i];
		if(hChild.IsValid())
		{
			WIBase *child = hChild.get();
			if(child == el || child->IsDescendant(el))
				return true;
		}
	}
	return false;
}
bool WIBase::IsDescendantOf(WIBase *el) {return el->IsDescendant(this);}
bool WIBase::IsAncestor(WIBase *el)
{
	WIBase *parent = GetParent();
	WIBase *root = WGUI::GetInstance().GetBaseElement();
	while(parent != NULL && parent != root)
	{
		if(parent == el)
			return true;
		parent = parent->GetParent();
	}
	return false;
}
bool WIBase::IsAncestorOf(WIBase *el) {return el->IsAncestor(this);}
void WIBase::OnFocusGained()
{
	CallCallbacks<void>("OnFocusGained");
	OnDescendantFocusGained(*this);
}
void WIBase::OnFocusKilled()
{
	CallCallbacks<void>("OnFocusKilled");
	OnDescendantFocusKilled(*this);
}
void WIBase::OnDescendantFocusGained(WIBase &el)
{
	auto *parent = GetParent();
	if(parent)
		parent->OnDescendantFocusGained(el);
}
void WIBase::OnDescendantFocusKilled(WIBase &el)
{
	auto *parent = GetParent();
	if(parent)
		parent->OnDescendantFocusKilled(el);
}
const util::PBoolProperty &WIBase::GetVisibilityProperty() const {return m_bVisible;}
bool WIBase::IsParentVisible() const {return umath::is_flag_set(m_stateFlags,StateFlags::ParentVisible);}
bool WIBase::IsSelfVisible() const
{
	return *m_bVisible && (m_color->GetValue().a > 0.f || umath::is_flag_set(m_stateFlags,StateFlags::RenderIfZeroAlpha));
}
bool WIBase::IsVisible() const
{
	return (
		IsSelfVisible() &&
		IsParentVisible()
	) ? true : false;
}
void WIBase::OnVisibilityChanged(bool) {}
void WIBase::SetVisible(bool b)
{
	if(*m_bVisible == b || umath::is_flag_set(m_stateFlags,StateFlags::RemoveScheduledBit))
		return;
	*m_bVisible = b;
	OnVisibilityChanged(b);
	if(b == false)
	{
		UpdateMouseInBounds();
		WIBase *el = WGUI::GetInstance().GetFocusedElement();
		if(el != NULL && (el == this || el->IsDescendantOf(this)))
		{
			bool bTrapped = el->IsFocusTrapped();
			el->TrapFocus(false); // Temporarily disable trapping so we can kill the focus without having to force it. This will make sure the right parent element will regain its focus.
			el->KillFocus();
			if(bTrapped == true)
				el->TrapFocus(true);
		}
		else
			KillFocus();
	}
	else if(IsFocusTrapped())
		RequestFocus();
}
void WIBase::Update()
{
	umath::set_flag(m_stateFlags,StateFlags::IsBeingUpdated);
	DoUpdate();
	umath::set_flag(m_stateFlags,StateFlags::IsBeingUpdated,false);
	// Flag must be cleared after DoUpdate, in case DoUpdate has set it again!
	umath::set_flag(m_stateFlags,StateFlags::UpdateScheduledBit,false);
}
void WIBase::DoUpdate() {}
const util::PVector2iProperty &WIBase::GetPosProperty() const {return m_pos;}
const Vector2i &WIBase::GetPos() const {return *m_pos;}
void WIBase::GetPos(int *x,int *y) const
{
	*x = (*m_pos)->x;
	*y = (*m_pos)->y;
}
Vector2 WIBase::GetCenter() const {return Vector2{GetCenterX(),GetCenterY()};}
float WIBase::GetCenterX() const {return GetX() +GetHalfWidth();}
float WIBase::GetCenterY() const {return GetY() +GetHalfHeight();}
void WIBase::SetCenterPos(const Vector2i &pos)
{
	Vector2 centerPos {pos.x,pos.y};
	centerPos -= GetHalfSize();
	SetPos(centerPos.x,centerPos.y);
}
void WIBase::CenterToParent(bool applyAnchor)
{
	auto *parent = GetParent();
	if(parent == nullptr)
		return;
	SetCenterPos(parent->GetHalfSize());
	if(applyAnchor)
		SetAnchor(0.5f,0.5f,0.5f,0.5f);
}
void WIBase::CenterToParentX()
{
	auto *parent = GetParent();
	if(parent == nullptr)
		return;
	SetX(parent->GetHalfWidth() -GetHalfWidth());
}
void WIBase::CenterToParentY()
{
	auto *parent = GetParent();
	if(parent == nullptr)
		return;
	SetY(parent->GetHalfHeight() -GetHalfHeight());
}
Vector2 WIBase::GetHalfSize() const {return Vector2{GetHalfWidth(),GetHalfHeight()};}
float WIBase::GetHalfWidth() const {return GetWidth() *0.5f;}
float WIBase::GetHalfHeight() const {return GetHeight() *0.5f;}
Vector2i WIBase::GetAbsolutePos() const
{
	Vector2i pos = GetPos();
	WIBase *parent = GetParent();
	if(parent != NULL)
		pos += parent->GetAbsolutePos();
	return pos;
}
void WIBase::SetAbsolutePos(Vector2i pos)
{
	WIBase *parent = GetParent();
	if(parent != NULL)
		pos -= parent->GetAbsolutePos();
	SetPos(pos);
}
std::vector<WIHandle> *WIBase::GetChildren() {return &m_children;}
void WIBase::GetChildren(const std::string &className,std::vector<WIHandle> &children)
{
	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIHandle &hChild = m_children[i];
		if(hChild.IsValid() && ustring::compare(hChild->GetClass(),className,false))
			children.push_back(hChild);
	}
}
WIBase *WIBase::GetFirstChild(const std::string &className)
{
	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIHandle &hChild = m_children[i];
		if(hChild.IsValid() && ustring::compare(hChild->GetClass(),className,false))
			return hChild.get();
	}
	return NULL;
}
WIBase *WIBase::GetChild(unsigned int idx)
{
	unsigned int j = 0;
	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIHandle &hChild = m_children[i];
		if(hChild.IsValid() && j++ == idx)
			return hChild.get();
	}
	return NULL;
}
WIBase *WIBase::GetChild(const std::string &className,unsigned int idx)
{
	unsigned int j = 0;
	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIHandle &hChild = m_children[i];
		if(hChild.IsValid() && ustring::compare(hChild->GetClass(),className,false) && j++ == idx)
			return hChild.get();
	}
	return NULL;
}
WIBase *WIBase::FindChildByName(const std::string &name)
{
	std::vector<WIHandle>::iterator it;
	for(it=m_children.begin();it!=m_children.end();it++)
	{
		WIHandle &hChild = *it;
		if(hChild.IsValid() && ustring::compare(hChild->GetName(),name,false))
			return hChild.get();
	}
	return nullptr;
}
void WIBase::FindChildrenByName(const std::string &name,std::vector<WIHandle> &children)
{
	std::vector<WIHandle>::iterator it;
	for(it=m_children.begin();it!=m_children.end();it++)
	{
		WIHandle &hChild = *it;
		if(hChild.IsValid() && ustring::compare(hChild->GetName(),name,false))
			children.push_back(hChild);
	}
}
WIBase *WIBase::FindDescendantByName(const std::string &name)
{
	if(ustring::compare(GetName(),name,false))
		return this;
	for(auto &hChild : m_children)
	{
		if(hChild.IsValid() == false)
			continue;
		auto *r = hChild->FindDescendantByName(name);
		if(r != nullptr)
			return r;
	}
	return nullptr;
}
void WIBase::FindDescendantsByName(const std::string &name,std::vector<WIHandle> &children)
{
	if(ustring::compare(GetName(),name,false))
		children.push_back(GetHandle());
	for(auto &hChild : m_children)
	{
		if(hChild.IsValid() == false)
			continue;
		hChild->FindDescendantsByName(name,children);
	}
}
const Color &WIBase::GetColor() const {return *m_color;}
const std::shared_ptr<util::ColorProperty> &WIBase::GetColorProperty() const {return m_color;}
void WIBase::SetColor(const Vector4 &col) {SetColor(col.r,col.g,col.b,col.a);}
void WIBase::SetColor(const Color &col) {SetColor(static_cast<float>(col.r) /255.f,static_cast<float>(col.g) /255.f,static_cast<float>(col.b) /255.f,static_cast<float>(col.a) /255.f);}
void WIBase::SetColor(float r,float g,float b,float a) {*m_color = Color(r *255.f,g *255.f,b *255.f,a *255.f);}
void WIBase::SetPos(const Vector2i &pos) {SetPos(pos.x,pos.y);}
void WIBase::SetPos(int x,int y) {*m_pos = Vector2i{x,y};}
int WIBase::GetWidth() const {return (*m_size)->x;}
int WIBase::GetHeight() const {return (*m_size)->y;}
const util::PVector2iProperty &WIBase::GetSizeProperty() const {return m_size;}
const Vector2i &WIBase::GetSize() const {return *m_size;}
void WIBase::GetSize(int *w,int *h)
{
	*w = (*m_size)->x;
	*h = (*m_size)->y;
}
void WIBase::SetSize(const Vector2i &size) {SetSize(size.x,size.y);}
void WIBase::SetSize(int x,int y)
{
#ifdef WGUI_ENABLE_SANITY_EXCEPTIONS
	if(x < 0 || y < 0)
		throw std::logic_error("Negative size not allowed!");
#endif
	*m_size = Vector2i{x,y};
}
Mat4 WIBase::GetTransformedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) const
{
	Mat4 r = glm::translate(mat,Vector3(
		-((w -(*m_size)->x) /float(w)) +(origin.x /float(w)) *2,
		-((h -(*m_size)->y) /float(h)) +(origin.y /float(h)) *2,//-1 -(origin.y /float(h)) *2.f,//((h -m_size.y) /float(h)) -((origin.y /float(h)) *2),
		0
	));
	return GetScaledMatrix(w,h,r);
}
Mat4 WIBase::GetTransformedMatrix(int w,int h,Mat4 mat) const {return GetTransformedMatrix(*m_pos,w,h,mat);}
Mat4 WIBase::GetTransformedMatrix(int w,int h) const
{
	Mat4 mat(1.0f);
	return GetTransformedMatrix(*m_pos,w,h,mat);
}
Mat4 WIBase::GetTranslatedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) const
{
	return glm::translate(mat,Vector3(
		(origin.x /float(w)) *2,
		(origin.y /float(h)) *2,//0.f,//-((origin.y /float(h)) *2),
		0
	));
}
Mat4 WIBase::GetTranslatedMatrix(int w,int h,Mat4 mat) const
{
	return GetTranslatedMatrix(*m_pos,w,h,mat);
}
Mat4 WIBase::GetTranslatedMatrix(int w,int h) const
{
	Mat4 mat(1.0f);
	return GetTranslatedMatrix(w,h,mat);
}
Mat4 WIBase::GetScaledMatrix(int w,int h,Mat4 mat) const
{
	Vector3 scale((*m_size)->x /float(w),(*m_size)->y /float(h),0);
	return glm::scale(mat,scale);
}
Mat4 WIBase::GetScaledMatrix(int w,int h) const
{
	Mat4 mat(1.0f);
	return GetScaledMatrix(w,h,mat);
}
void WIBase::Render(const DrawInfo &drawInfo,const Mat4 &matDraw)
{
}
const std::string &WIBase::GetName() const {return m_name;}
void WIBase::SetName(const std::string &name) {m_name = name;}
void WIBase::OnCursorMoved(int x,int y)
{
	CallCallbacks<void,int32_t,int32_t>("OnCursorMoved",x,y);
}
void WIBase::SetSkin(std::string skinName)
{
	WISkin *skin = WGUI::GetInstance().GetSkin(skinName);
	if(skin != nullptr && m_skin == skin)
		return;
	ResetSkin();
	m_skin = skin;
}
void WIBase::ResetSkin()
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::SkinAppliedBit) == true)
	{
		if(m_skin != nullptr)
			m_skin->Release(this);
		else
		{
			WISkin *skinCurrent = WGUI::GetInstance().GetSkin();
			if(skinCurrent != nullptr)
				skinCurrent->Release(this);
		}
		umath::set_flag(m_stateFlags,StateFlags::SkinAppliedBit,false);
		UpdateThink();
	}
	m_skin = nullptr;
	std::vector<WIHandle> *children = GetChildren();
	for(unsigned int i=0;i<children->size();i++)
	{
		WIHandle &hChild = (*children)[i];
		if(hChild.IsValid())
			hChild->ResetSkin();
	}
}
void WIBase::ApplySkin(WISkin *skin)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::SkinAppliedBit) == true)
		return;
	umath::set_flag(m_stateFlags,StateFlags::SkinAppliedBit,true);
	UpdateThink();
	if(m_skin == nullptr)
		m_skin = skin;
	if(m_skin == nullptr)
	{
		auto *parent = GetParent();
		if(parent)
			m_skin = parent->GetSkin();
	}
	if(m_skin == nullptr)
		m_skin = WGUI::GetInstance().GetSkin();
	if(m_skin != nullptr)
		m_skin->Initialize(this);
	std::vector<WIHandle> *children = GetChildren();
	for(unsigned int i=0;i<children->size();i++)
	{
		WIHandle &hChild = (*children)[i];
		if(hChild.IsValid())
			hChild->ApplySkin(m_skin);
	}
}
WISkin *WIBase::GetSkin()
{
	if(m_skin)
		return m_skin;
	auto *parent = GetParent();
	return parent ? parent->GetSkin() : WGUI::GetInstance().GetSkin();
}
void WIBase::SetThinkIfInvisible(bool bThinkIfInvisible) {umath::set_flag(m_stateFlags,StateFlags::ThinkIfInvisibleBit,bThinkIfInvisible);}
void WIBase::SetRenderIfZeroAlpha(bool renderIfZeroAlpha) {umath::set_flag(m_stateFlags,StateFlags::RenderIfZeroAlpha,renderIfZeroAlpha);}
void WIBase::Think()
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::RemoveScheduledBit) == true)
		return;
	//if(umath::is_flag_set(m_stateFlags,StateFlags::UpdateScheduledBit) == true)
	//	Update();
	ApplySkin();
	//if(*m_bHasFocus == true)
	{
		auto &context = WGUI::GetInstance().GetContext();
		auto &window = context.GetWindow();
		if(window.IsFocused())
			UpdateChildrenMouseInBounds();
	}
	
	if(umath::is_flag_set(m_stateFlags,StateFlags::MouseCheckEnabledBit) == true)
	{
		int x,y;
		GetMousePos(&x,&y);
		if(x != m_lastMouseX || y != m_lastMouseY)
		{
			OnCursorMoved(x,y);
			m_lastMouseX = x;
			m_lastMouseY = y;
		}
	}
	if(m_fade != NULL)
	{
		float a = GetAlpha();
		float aTarget = m_fade->alphaTarget;
		float tDelta = static_cast<float>(WGUI::GetInstance().GetDeltaTime());
		if(tDelta < m_fade->timeLeft)
		{
			a += (aTarget -a) *(tDelta /m_fade->timeLeft);
			m_fade->timeLeft -= tDelta;
		}
		else
			a = aTarget;
		SetAlpha(a);
		if(a == aTarget)
		{
			if(m_fade->removeOnFaded == true)
			{
				Remove();
				return;
			}
			m_fade = nullptr;
			UpdateThink();
		}
	}
	CallCallbacks<void>("Think");
}
void WIBase::CalcBounds(const Mat4 &mat,int32_t w,int32_t h,Vector2i &outPos,Vector2i &outSize)
{
	outSize = Vector2i(mat[0][0] *w,mat[1][1] *h);
	outPos = Vector2i(
		(w -((-mat[3][0] *w) +outSize.x)) /2.f,
		(h -((-mat[3][1] *h) +outSize.y)) /2.f
	);
}
void WIBase::Draw(const DrawInfo &drawInfo,const Vector2i &offsetParent,const Vector2i &scissorOffset,const Vector2i &scissorSize)
{
	const auto w = drawInfo.size.x;
	const auto h = drawInfo.size.y;
	const auto &origin = drawInfo.offset;

	auto mat = drawInfo.transform;
	const auto bUseScissor = drawInfo.useScissor;
	const auto &matPostTransform = drawInfo.postTransform;

	auto matDraw = GetTransformedMatrix(origin,w,h,mat);
	if(matPostTransform.has_value())
		matDraw = *matPostTransform *matDraw;

	if(bUseScissor == true)
	{
		// Check if the element is outside the scissor bounds.
		// If that's the case, we can skip rendering it (and
		// all its children) altogether.

		// Calc (absolute) element position and size from Matrix
		Vector2i pos,size;
		CalcBounds(matDraw,w,h,pos,size);
		const auto margin = Vector2i(2,2); // Add a small margin to the element bounds, to account for precision errors
		auto boundsStart = pos -margin;
		auto boundsEnd = pos +size +margin;
		auto scissorEnd = scissorOffset +scissorSize;
		if(boundsEnd.x < scissorOffset.x || boundsEnd.y < scissorOffset.y || boundsStart.x > scissorEnd.x || boundsStart.y > scissorEnd.y)
			return; // Outside of scissor rect; Skip rendering
	}

	Render(drawInfo,matDraw);
	mat = GetTranslatedMatrix(origin,w,h,mat);
	auto &context = WGUI::GetInstance().GetContext();
	auto drawCmd = context.GetDrawCommandBuffer();
	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIBase *child = m_children[i].get();
		if(child != NULL && child->IsVisible())
		{
			auto bShouldScissor = (child->GetShouldScissor()) ? true : false;
			Vector2i posScissor(scissorOffset.x,scissorOffset.y);
			Vector2i szScissor(scissorSize.x,scissorSize.y);
			Vector2i posScissorEnd(posScissor[0] +szScissor[0],posScissor[1] +szScissor[1]);

			Vector2i offsetParentNew = offsetParent +child->GetPos();
			Vector2i posChild = offsetParentNew;
			const Vector2i &szChild = child->GetSize();
			Vector2i posChildEnd = posChild +szChild;
			for(unsigned char j=0;j<2;j++)
			{
				if(posChild[j] < posScissor[j])
					posChild[j] = posScissor[j];
				if(posChildEnd[j] > posScissorEnd[j])
					posChildEnd[j] = posScissorEnd[j];
			}
			if(bUseScissor == false)
				WGUI::GetInstance().SetScissor(0u,0u,context.GetWindowWidth(),context.GetWindowHeight());
			else if(bShouldScissor == false)
				WGUI::GetInstance().SetScissor(umath::max(posScissor[0],0),umath::max(posScissor[1],0),umath::max(szScissor[0],0),umath::max(szScissor[1],0)); // Use parent scissor values

			posScissor = posChild;
			szScissor = posChildEnd -posChild;

			//auto yScissor = context.GetHeight() -(posScissor[1] +szScissor[1]); // Move origin to top left (OpenGL)
			//drawCmd->SetScissor(posScissor[0],yScissor,szScissor[0],szScissor[1]);
			if(bUseScissor == true && bShouldScissor == true)
				WGUI::GetInstance().SetScissor(umath::max(posScissor[0],0),umath::max(posScissor[1],0),umath::max(szScissor[0],0),umath::max(szScissor[1],0));

			float aOriginal = WIBase::RENDER_ALPHA;
			if(child->ShouldIgnoreParentAlpha())
				WIBase::RENDER_ALPHA = 1.f;
			else
			{
				float alpha = child->GetAlpha();
				WIBase::RENDER_ALPHA = aOriginal *alpha;
				if(WIBase::RENDER_ALPHA > 1.f)
					WIBase::RENDER_ALPHA = 1.f;
				else if(WIBase::RENDER_ALPHA < 0.f)
					WIBase::RENDER_ALPHA = 0.f;
			}
			auto childDrawInfo = drawInfo;
			childDrawInfo.offset = *child->m_pos;
			childDrawInfo.transform = mat;
			childDrawInfo.useScissor = bShouldScissor;
			child->Draw(childDrawInfo,offsetParentNew,posScissor,szScissor);
			WIBase::RENDER_ALPHA = aOriginal;
		}
	}
}
void WIBase::Draw(const DrawInfo &drawInfo)
{
	if(GetClass() == "wiasseticon")
		std::cout<<"";
	auto scissorPos = GetPos();
	auto scissorSize = drawInfo.size;
	if(drawInfo.useScissor)
	{
		scissorPos = {};
		scissorSize = GetSize();
	}
	Draw(drawInfo,GetPos(),scissorPos,scissorSize);
}
void WIBase::Draw(int w,int h)
{
	if(!IsVisible())
		return;
	DrawInfo drawInfo {};
	drawInfo.offset = GetPos();
	drawInfo.useScissor = GetShouldScissor();
	drawInfo.size = {w,h};
	Draw(drawInfo);
}
std::string WIBase::GetDebugInfo() const {return "";}
void WIBase::SetTooltip(const std::string &msg) {m_toolTip = msg;}
const std::string &WIBase::GetTooltip() const {return m_toolTip;}
bool WIBase::HasTooltip() const {return !m_toolTip.empty();}
WIAttachment *WIBase::AddAttachment(const std::string &name,const Vector2 &position)
{
	auto lname = name;
	ustring::to_lower(lname);
	auto it = m_attachments.find(lname);
	if(it == m_attachments.end())
		it = m_attachments.insert(std::make_pair(lname,std::make_shared<WIAttachment>(*this))).first;
	it->second->SetRelativePosition(position);
	return it->second.get();
}
const WIAttachment *WIBase::GetAttachment(const std::string &name) const {return const_cast<WIBase*>(this)->GetAttachment(name);}
WIAttachment *WIBase::GetAttachment(const std::string &name)
{
	auto lname = name;
	ustring::to_lower(lname);
	auto it = m_attachments.find(lname);
	if(it == m_attachments.end())
		return nullptr;
	return it->second.get();
}
void WIBase::SetAttachmentPos(const std::string &name,const Vector2 &position)
{
	auto *pAttachment = GetAttachment(name);
	if(pAttachment == nullptr)
		return;
	pAttachment->SetRelativePosition(position);
}
const Vector2 *WIBase::GetAttachmentPos(const std::string &name) const
{
	auto *pAttachment = GetAttachment(name);
	if(pAttachment == nullptr)
		return nullptr;
	return &pAttachment->GetRelativePosition();
}
const Vector2i *WIBase::GetAbsoluteAttachmentPos(const std::string &name) const
{
	auto *pAttachment = GetAttachment(name);
	if(pAttachment == nullptr)
		return nullptr;
	return &pAttachment->GetAbsPosProperty()->GetValue();
}
const util::PVector2iProperty *WIBase::GetAttachmentPosProperty(const std::string &name) const
{
	auto *pAttachment = GetAttachment(name);
	if(pAttachment == nullptr)
		return nullptr;
	return &pAttachment->GetAbsPosProperty();
}
void WIBase::SetAutoSizeToContents(bool x,bool y)
{
	umath::set_flag(m_stateFlags,StateFlags::AutoSizeToContentsX,x);
	umath::set_flag(m_stateFlags,StateFlags::AutoSizeToContentsY,y);
	if(x || y)
		UpdateAutoSizeToContents();
}
void WIBase::SetAutoSizeToContents(bool autoSize) {SetAutoSizeToContents(autoSize,autoSize);}
bool WIBase::ShouldAutoSizeToContentsX() const {return umath::is_flag_set(m_stateFlags,StateFlags::AutoSizeToContentsX);}
bool WIBase::ShouldAutoSizeToContentsY() const {return umath::is_flag_set(m_stateFlags,StateFlags::AutoSizeToContentsY);}
bool WIBase::Wrap(WIBase &wrapper)
{
	auto *parent = GetParent();
	if(parent == nullptr)
		return false;
	auto pos = GetPos();
	auto size = GetSize();
	float left,top,right,bottom;
	auto hasAnchor = GetAnchor(left,top,right,bottom);
	ClearAnchor();
	SetParent(&wrapper);
	SetPos(0,0);

	wrapper.SetParent(parent);
	wrapper.SetPos(pos);
	wrapper.SetSize(size);
	if(hasAnchor)
		wrapper.SetAnchor(left,top,right,bottom);

	SetAnchor(0.f,0.f,1.f,1.f);
	return true;
}
WIBase *WIBase::Wrap(const std::string &wrapperClass)
{
	auto *parent = GetParent();
	if(parent == nullptr)
		return nullptr; // Can't wrap root element
	auto *wrapper = WGUI::GetInstance().Create(wrapperClass);
	if(wrapper == nullptr)
		return nullptr;
	Wrap(*wrapper);
	return wrapper;
}
bool WIBase::HasAnchor() const {return m_anchor.has_value();}
std::pair<Vector2,Vector2> WIBase::GetAnchorBounds(uint32_t refWidth,uint32_t refHeight) const
{
	auto anchorMin = Vector2{m_anchor->left *refWidth,m_anchor->top *refHeight};
	auto anchorMax = Vector2{m_anchor->right *refWidth,m_anchor->bottom *refHeight};
	return {anchorMin,anchorMax};
}
std::pair<Vector2,Vector2> WIBase::GetAnchorBounds() const
{
	auto *pParent = GetParent();
	auto w = pParent ? pParent->GetWidth() : GetWidth();
	auto h = pParent ? pParent->GetHeight() : GetHeight();
	return GetAnchorBounds(w,h);
}
void WIBase::ClearAnchor() {m_anchor = {};}
void WIBase::SetAnchor(float left,float top,float right,float bottom,uint32_t refWidth,uint32_t refHeight)
{
	m_anchor = WIAnchor{};
	m_anchor->left = left;
	m_anchor->right = right;
	m_anchor->top = top;
	m_anchor->bottom = bottom;
	m_anchor->referenceWidth = refWidth;
	m_anchor->referenceHeight = refHeight;

	if(m_anchor.has_value() && m_anchor->initialized == false)
	{
		m_anchor->initialized = true;

		UpdateAnchorTopLeftPixelOffsets();
		UpdateAnchorBottomRightPixelOffsets();
		UpdateAnchorTransform();
	}
}
void WIBase::UpdateAnchorTopLeftPixelOffsets()
{
	if(m_anchor.has_value() == false || m_anchor->initialized == false)
		return;
	auto anchorBounds = GetAnchorBounds(m_anchor->referenceWidth,m_anchor->referenceHeight);
	m_anchor->pxOffsetLeft = GetLeft() -anchorBounds.first.x;
	m_anchor->pxOffsetTop = GetTop() -anchorBounds.first.y;
}
void WIBase::UpdateAnchorBottomRightPixelOffsets()
{
	if(m_anchor.has_value() == false || m_anchor->initialized == false)
		return;
	auto anchorBounds = GetAnchorBounds(m_anchor->referenceWidth,m_anchor->referenceHeight);
	m_anchor->pxOffsetRight = GetRight() -anchorBounds.second.x;
	m_anchor->pxOffsetBottom = GetBottom() -anchorBounds.second.y;
}
void WIBase::SetAnchor(float left,float top,float right,float bottom)
{
	auto *pParent = GetParent();
	auto w = pParent ? pParent->GetWidth() : GetWidth();
	auto h = pParent ? pParent->GetHeight() : GetHeight();
	SetAnchor(left,top,right,bottom,w,h);
}
void WIBase::SetAnchorLeft(float f)
{
	if(m_anchor.has_value() == false)
		m_anchor = WIAnchor{};
	m_anchor->initialized = false;
	m_anchor->left = f;
}
void WIBase::SetAnchorRight(float f)
{
	if(m_anchor.has_value() == false)
		m_anchor = WIAnchor{};
	m_anchor->initialized = false;
	m_anchor->right = f;
}
void WIBase::SetAnchorTop(float f)
{
	if(m_anchor.has_value() == false)
		m_anchor = WIAnchor{};
	m_anchor->initialized = false;
	m_anchor->top = f;
}
void WIBase::SetAnchorBottom(float f)
{
	if(m_anchor.has_value() == false)
		m_anchor = WIAnchor{};
	m_anchor->initialized = false;
	m_anchor->bottom = f;
}
bool WIBase::GetAnchor(float &outLeft,float &outTop,float &outRight,float &outBottom) const
{
	if(m_anchor.has_value() == false)
	{
		outLeft = 0.f;
		outRight = 0.f;
		outTop = 0.f;
		outBottom = 0.f;
		return false;
	}
	outLeft = m_anchor->left;
	outRight = m_anchor->right;
	outTop = m_anchor->top;
	outBottom = m_anchor->bottom;
	return true;
}
void WIBase::UpdateAnchorTransform()
{
	auto *pParent = GetParent();
	if(pParent == nullptr || m_anchor.has_value() == false)
		return;
	auto szParent = pParent->GetSize();
	auto anchorBounds = GetAnchorBounds();
  
	auto elMin = Vector2{anchorBounds.first.x +m_anchor->pxOffsetLeft,anchorBounds.first.y +m_anchor->pxOffsetTop};
	auto elMax = Vector2{anchorBounds.second.x +m_anchor->pxOffsetRight,anchorBounds.second.y +m_anchor->pxOffsetBottom};
	auto sz = elMax -elMin;
	umath::set_flag(m_stateFlags,StateFlags::UpdatingAnchorTransform);
	SetPos(elMin);
	SetSize(sz);
	umath::set_flag(m_stateFlags,StateFlags::UpdatingAnchorTransform,false);
}
void WIBase::Remove()
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::IsBeingRemoved))
		return;
	umath::set_flag(m_stateFlags,StateFlags::IsBeingRemoved);
	for(auto &hChild : *GetChildren())
	{
		if(hChild.IsValid() == false)
			continue;
		hChild->Remove();
	}
	auto hThis = GetHandle();
	OnRemove();
	if(!hThis.IsValid())
		return;
	CallCallbacks<void>("OnRemove");
	if(!hThis.IsValid())
		return;
	WGUI::GetInstance().Remove(*this);
}
void WIBase::OnRemove() {}
void WIBase::UpdateThink()
{
	auto &wgui = WGUI::GetInstance();
	auto it = std::find_if(wgui.m_thinkingElements.begin(),wgui.m_thinkingElements.end(),[this](const WIHandle &hEl) {
		return hEl.get() == this;
	});
	if(ShouldThink())
	{
		if(it != wgui.m_thinkingElements.end())
			return;
		wgui.m_thinkingElements.push_back(GetHandle());
		return;
	}
	if(it == wgui.m_thinkingElements.end())
		return;
	wgui.m_thinkingElements.erase(it);

}
bool WIBase::ShouldThink() const
{
	auto shouldThink = (umath::is_flag_set(m_stateFlags,StateFlags::ThinkingEnabled | StateFlags::MouseCheckEnabledBit) ||
		m_fade != nullptr ||
		umath::is_flag_set(m_stateFlags,StateFlags::SkinAppliedBit) == false);
	if(shouldThink == false)
		return false;
	if(IsSelfVisible() == false)
		return m_fade != nullptr || umath::is_flag_set(m_stateFlags,StateFlags::ThinkIfInvisibleBit | StateFlags::RenderIfZeroAlpha);
	return IsParentVisible();
}
void WIBase::EnableThinking() {SetThinkingEnabled(true);}
void WIBase::DisableThinking() {SetThinkingEnabled(false);}
void WIBase::SetThinkingEnabled(bool enabled)
{
	umath::set_flag(m_stateFlags,StateFlags::ThinkingEnabled,enabled);
	UpdateThink();
}
uint64_t WIBase::GetIndex() const {return m_index;}
void WIBase::SetIndex(uint64_t idx) {m_index = idx;}
void WIBase::SetParent(WIBase *base,std::optional<uint32_t> childIndex)
{
	if(base == this || (m_parent.get() == base))
	{
		if(childIndex.has_value() == false)
			return;
		auto curChildIndex = m_parent->FindChildIndex(*this);
		if(curChildIndex.has_value() && *curChildIndex == *childIndex)
			return;
	}
	if(base != nullptr && base->GetParent() == this)
		base->ClearParent();
	ClearParent();
	if(base == NULL)
	{
		m_parent = WIHandle();
		if(GetAutoAlignToParent() == true)
			SetAutoAlignToParent(true,true);
		if(umath::is_flag_set(m_stateFlags,StateFlags::AutoCenterToParentXBit) == true)
			SetAutoCenterToParentX(true,true);
		if(umath::is_flag_set(m_stateFlags,StateFlags::AutoCenterToParentYBit) == true)
			SetAutoCenterToParentY(true,true);
		UpdateVisibility();
		return;
	}
	m_parent = base->GetHandle();
	base->AddChild(this,childIndex);
	if(GetAutoAlignToParent() == true)
		SetAutoAlignToParent(true,true);
	if(umath::is_flag_set(m_stateFlags,StateFlags::AutoCenterToParentXBit) == true)
		SetAutoCenterToParentX(true,true);
	if(umath::is_flag_set(m_stateFlags,StateFlags::AutoCenterToParentYBit) == true)
		SetAutoCenterToParentY(true,true);

	UpdateVisibility();
	base->UpdateAutoSizeToContents();
}
WIBase *WIBase::GetParent() const
{
	if(!m_parent.IsValid())
		return nullptr;
	return m_parent.get();
}
void WIBase::ClearParent()
{
	if(!m_parent.IsValid())
		return;
	WIBase *p = m_parent.get();
	m_parent = WIHandle();
	if(p == NULL)
		return;
	p->RemoveChild(this);
	p->UpdateAutoSizeToContents();
}
void WIBase::RemoveChild(WIBase *child)
{
	for(int i=static_cast<int>(m_children.size()) -1;i>=0;i--)
	{
		WIHandle &hnd = m_children[i];
		if(hnd.get() == child)
		{
			child->ClearParent();
			m_children.erase(m_children.begin() +i);
			OnChildRemoved(child);
		}
	}
}
void WIBase::AddChild(WIBase *child,std::optional<uint32_t> childIndex)
{
	if(child == this)
		return;
	auto curIndex = FindChildIndex(*child);
	if(curIndex.has_value())
	{
		if(childIndex.has_value() == false || *curIndex == *childIndex)
			return;
		child->ClearParent();
	}
	if(child->HasChild(this))
		child->RemoveChild(this);
	InsertGUIElement(m_children,child->GetHandle(),childIndex);
	child->SetParent(this);
	OnChildAdded(child);
}
bool WIBase::HasChild(WIBase *child)
{
	for(unsigned int i=0;i<m_children.size();i++)
	{
		if(m_children[i].get() == child)
			return true;
	}
	return false;
}
std::optional<uint32_t> WIBase::FindChildIndex(WIBase &child) const
{
	auto it = std::find_if(m_children.begin(),m_children.end(),[&child](const WIHandle &hEl) -> bool {
		return hEl.get() == &child;
	});
	if(it == m_children.end())
		return {};
	return it -m_children.begin();
}
void WIBase::OnChildAdded(WIBase *child) {CallCallbacks<void,WIBase*>("OnChildAdded",child);}
void WIBase::OnChildRemoved(WIBase *child) {CallCallbacks<void,WIBase*>("OnChildRemoved",child);}
bool WIBase::PosInBounds(int x,int y) const
{
	return PosInBounds(Vector2i(x,y));
}
bool WIBase::PosInBounds(Vector2i pos) const
{
	pos -= GetAbsolutePos();
	const Vector2i &size = GetSize();
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y) ? false : true;
}
void WIBase::Resize() {SetSize(GetSize());}
const util::PBoolProperty &WIBase::GetMouseInBoundsProperty() const {return m_bMouseInBounds;}
bool WIBase::MouseInBounds() const
{
	auto &context = WGUI::GetInstance().GetContext();
	auto &window = context.GetWindow();
	auto cursorPos = window.GetCursorPos();
	return PosInBounds(static_cast<int>(cursorPos.x),static_cast<int>(cursorPos.y));
}
bool WIBase::GetMouseInputEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::AcceptMouseInputBit);}
bool WIBase::GetKeyboardInputEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::AcceptKeyboardInputBit);}
bool WIBase::GetScrollInputEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::AcceptScrollInputBit);}
void WIBase::UpdateMouseInBounds()
{
	bool old = *m_bMouseInBounds;
	if(umath::is_flag_set(m_stateFlags,StateFlags::AcceptMouseInputBit) == false)
		*m_bMouseInBounds = false;
	else
		*m_bMouseInBounds = MouseInBounds();
	if(old == *m_bMouseInBounds)
		return;
	if(*m_bMouseInBounds == true)
		OnCursorEntered();
	else OnCursorExited();
}
void WIBase::UpdateChildrenMouseInBounds()
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::AcceptMouseInputBit) == true)
		UpdateMouseInBounds();
	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIHandle &hChild = m_children[i];
		if(hChild.IsValid() && hChild->IsVisible())
			hChild->UpdateChildrenMouseInBounds();
	}
}
void WIBase::OnCursorEntered()
{
	CallCallbacks<void>("OnCursorEntered");
}
void WIBase::OnCursorExited()
{
	CallCallbacks<void>("OnCursorExited");
}
void WIBase::SetMouseInputEnabled(bool b)
{
	umath::set_flag(m_stateFlags,StateFlags::AcceptMouseInputBit,b);
	if(b == false && *m_bMouseInBounds == true)
	{
		OnCursorExited();
		*m_bMouseInBounds = false;
	}
}
void WIBase::SetKeyboardInputEnabled(bool b) {umath::set_flag(m_stateFlags,StateFlags::AcceptKeyboardInputBit,b);}
void WIBase::SetScrollInputEnabled(bool b) {umath::set_flag(m_stateFlags,StateFlags::AcceptScrollInputBit,b);}
util::EventReply WIBase::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto hThis = GetHandle();

	if(button == GLFW::MouseButton::Left && state == GLFW::KeyState::Press)
	{
		ChronoTimePoint now = util::Clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now -m_clickStart);
		if(duration.count() <= 300)
		{
			umath::set_flag(m_stateFlags,StateFlags::ClickedBit,false);
			if(OnDoubleClick() == util::EventReply::Handled)
				return util::EventReply::Handled;
		}
		else
		{
			umath::set_flag(m_stateFlags,StateFlags::ClickedBit,true);
			m_clickStart = util::Clock::now();
		}
	}

	util::EventReply reply;
	if(
		CallCallbacksWithOptionalReturn<util::EventReply,GLFW::MouseButton,GLFW::KeyState,GLFW::Modifier>("OnMouseEvent",reply,button,state,mods) == CallbackReturnType::HasReturnValue &&
		reply == util::EventReply::Handled
	)
		return util::EventReply::Handled;
	if(!hThis.IsValid())
		return util::EventReply::Unhandled;
	if(button == GLFW::MouseButton::Left)
	{
		if(state == GLFW::KeyState::Press)
		{
			auto reply = util::EventReply::Unhandled;
			CallCallbacksWithOptionalReturn<util::EventReply>("OnMousePressed",reply);
			if(!hThis.IsValid())
				return util::EventReply::Unhandled;
			if(reply == util::EventReply::Unhandled)
				return OnMousePressed();
			return reply;
		}
		else if(state == GLFW::KeyState::Release)
		{
			auto reply = util::EventReply::Unhandled;
			CallCallbacksWithOptionalReturn<util::EventReply>("OnMouseReleased",reply);
			if(!hThis.IsValid())
				return util::EventReply::Unhandled;
			if(reply == util::EventReply::Unhandled)
				return OnMouseReleased();
			return reply;
		}
	}
	//std::cout<<"MouseCallback: "<<button<<","<<action<<std::endl;
	return util::EventReply::Unhandled;
}
util::EventReply WIBase::OnMousePressed() {return util::EventReply::Unhandled;}
util::EventReply WIBase::OnMouseReleased() {return util::EventReply::Unhandled;}
util::EventReply WIBase::OnDoubleClick()
{
	auto reply = util::EventReply::Unhandled;
	CallCallbacksWithOptionalReturn<util::EventReply>("OnDoubleClick",reply);
	return reply;
}
util::EventReply WIBase::JoystickCallback(const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state)
{
	auto reply = util::EventReply::Unhandled;
	CallCallbacksWithOptionalReturn<util::EventReply,std::reference_wrapper<const GLFW::Joystick>,uint32_t,GLFW::KeyState>("OnJoystickEvent",reply,std::ref(joystick),key,state);
	return reply;
}
util::EventReply WIBase::KeyboardCallback(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	//std::cout<<"KeyboardCallback: "<<key<<","<<action<<std::endl;
	auto reply = util::EventReply::Unhandled;
	CallCallbacksWithOptionalReturn<util::EventReply,GLFW::Key,int,GLFW::KeyState,GLFW::Modifier>("OnKeyEvent",reply,key,scanCode,state,mods);
	return reply;
}
util::EventReply WIBase::CharCallback(unsigned int c,GLFW::Modifier mods)
{
	//std::cout<<"CharCallback: "<<char(c)<<","<<action<<std::endl;
	auto reply = util::EventReply::Unhandled;
	CallCallbacksWithOptionalReturn<util::EventReply,int,GLFW::Modifier>("OnCharEvent",reply,c,mods);
	return reply;
}
util::EventReply WIBase::ScrollCallback(Vector2 offset)
{
	//std::cout<<"ScrollCallback: "<<xoffset<<","<<yoffset<<std::endl;
	auto reply = util::EventReply::Unhandled;
	CallCallbacksWithOptionalReturn<util::EventReply,Vector2>("OnScroll",reply,offset);
	return reply;
}

WIBase *WIBase::FindDeepestChild(const std::function<bool(const WIBase&)> &predInspect,const std::function<bool(const WIBase&)> &predValidCandidate)
{
	auto largestZPos = std::numeric_limits<int32_t>::lowest();
	WIBase *r = nullptr;
	if(predValidCandidate(*this))
		r = this;
	WIBase *childToInspect = nullptr;
	for(auto &hChild : m_children)
	{
		if(hChild.IsValid() == false)
			continue;
		if(predInspect(*hChild.get()) == true && hChild.get()->GetZPos() > largestZPos)
		{
			childToInspect = hChild.get();
			largestZPos = childToInspect->GetZPos();
		}
	}
	if(childToInspect != nullptr)
	{
		auto *rChild = childToInspect->FindDeepestChild(predInspect,predValidCandidate);
		if(rChild != nullptr)
			r = rChild;
	}
	return r;
}
void WIBase::InjectMouseMoveInput(int32_t x,int32_t y)
{
	OnCursorMoved(x,y);
	UpdateChildrenMouseInBounds();
}
util::EventReply WIBase::InjectMouseInput(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto *el = FindDeepestChild([](const WIBase &el) {
		return el.MouseInBounds();
	},[](const WIBase &el) {
		return el.GetMouseInputEnabled();
	});
	if(el == nullptr)
		return util::EventReply::Handled;
	return InjectMouseButtonCallback(*el,button,state,mods);
}
util::EventReply WIBase::InjectKeyboardInput(GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods) {return KeyboardCallback(key,scanCode,state,mods);}
util::EventReply WIBase::InjectCharInput(unsigned int c,GLFW::Modifier mods) {return CharCallback(c,mods);}
util::EventReply WIBase::InjectScrollInput(Vector2 offset)
{
	auto *el = FindDeepestChild([](const WIBase &el) {
		return el.MouseInBounds();
	},[](const WIBase &el) {
		return el.GetScrollInputEnabled();
	});
	if(el != nullptr)
		return el->ScrollCallback(offset);
	return util::EventReply::Handled;
}

std::vector<std::string> &WIBase::GetStyleClasses() {return m_styleClasses;}
void WIBase::AddStyleClass(const std::string &className)
{
	auto lname = className;
	ustring::to_lower(lname);
	m_styleClasses.push_back(lname);
}
void WIBase::RemoveStyleClass(const std::string &className)
{
	auto lname = className;
	ustring::to_lower(lname);
	auto it = std::find(m_styleClasses.begin(),m_styleClasses.end(),lname);
	if(it == m_styleClasses.end())
		return;
	m_styleClasses.erase(it);
}
void WIBase::ClearStyleClasses()
{
	m_styleClasses.clear();
}

/////////////////

static std::unordered_map<GLFW::MouseButton,WIHandle> __lastMouseGUIElements;
util::EventReply WIBase::InjectMouseButtonCallback(WIBase &el,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto hEl = el.GetHandle();

	WIBase *pFocused = WGUI::GetInstance().GetFocusedElement();
	auto hFocused = pFocused ? pFocused->GetHandle() : WIHandle{};
	__lastMouseGUIElements.insert(std::unordered_map<GLFW::MouseButton,WIHandle>::value_type(button,el.GetHandle()));
	auto result = el.MouseCallback(button,state,mods);
	// Callback may have invoked mouse button release-event already, so we have to check if our element's still there
	auto it = std::find_if(__lastMouseGUIElements.begin(),__lastMouseGUIElements.end(),[&el](const std::pair<GLFW::MouseButton,WIHandle> &p) {
		return (p.second.get() == &el) ? true : false;
		});
	if(it != __lastMouseGUIElements.end() && !it->second.IsValid())
		__lastMouseGUIElements.erase(it);
	if(hFocused.IsValid() && hEl.IsValid() && &el != pFocused && !pFocused->MouseInBounds())
	{
		pFocused->KillFocus(); // Make sure to kill the focus AFTER the mouse callback.
		if(hEl.IsValid())
			el.RequestFocus();
	}
	return result;
}
bool WIBase::__wiMouseButtonCallback(GLFW::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto i = __lastMouseGUIElements.find(button);
	if(i != __lastMouseGUIElements.end())
	{
		if(i->second.IsValid() && i->second->IsVisible() && i->second->GetMouseInputEnabled())
			i->second->MouseCallback(button,GLFW::KeyState::Release,mods);
		__lastMouseGUIElements.erase(i);
	}
	if(state == GLFW::KeyState::Press)
	{
		auto *pContextMenu = WIContextMenu::GetActiveContextMenu();
		if(pContextMenu && pContextMenu->IsCursorInMenuBounds() == false)
			WIContextMenu::CloseContextMenu();

		auto cursorPos = window.GetCursorPos();
		WIBase *p = WGUI::GetInstance().GetFocusedElement();
		WIBase *gui = (p == nullptr || !p->IsFocusTrapped()) ? WGUI::GetInstance().GetBaseElement() : p;
		auto hP = p ? p->GetHandle() : WIHandle{};
		auto hGui = gui->GetHandle();
		if(hGui.IsValid() && gui->IsVisible())
		{
			gui = WGUI::GetInstance().GetGUIElement(gui,static_cast<int>(cursorPos.x),static_cast<int>(cursorPos.y),
				[&hGui,&hP,p,button,state,mods](WIBase *elChild) -> bool {
				if(elChild->GetMouseInputEnabled() == false)
					return false;
				hGui = elChild->GetHandle();
				auto result = InjectMouseButtonCallback(*elChild,button,state,mods);
				return (result == util::EventReply::Handled) ? true : false;
			});
			if(gui)
				return true;
		}
	}
	return false;
}

static std::unordered_map<GLFW::Key,WIHandle> __lastKeyboardGUIElements;
bool WIBase::__wiJoystickCallback(GLFW::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state)
{
	
	// TODO
	return false;
}
bool WIBase::__wiKeyCallback(GLFW::Window&,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto it = __lastKeyboardGUIElements.find(key);
	if(it != __lastKeyboardGUIElements.end())
	{
		if(it->second.IsValid() && it->second->GetKeyboardInputEnabled())
			it->second->KeyboardCallback(key,scanCode,GLFW::KeyState::Release,mods);
		__lastKeyboardGUIElements.erase(it);
	}
	if(state == GLFW::KeyState::Press || state == GLFW::KeyState::Repeat)
	{
		WIBase *gui = WGUI::GetInstance().GetFocusedElement();
		while(gui)
		{
			if(key == GLFW::Key::Escape && !gui->IsFocusTrapped())
			{
				gui->KillFocus();
				break;
			}
			else if(gui->GetKeyboardInputEnabled())
			{
				__lastKeyboardGUIElements.insert(std::unordered_map<GLFW::Key,WIHandle>::value_type(key,gui->GetHandle()));
				auto result = gui->KeyboardCallback(key,scanCode,state,mods);
				// Callback may have invoked mouse button release-event already, so we have to check if our element's still there
				auto it = std::find_if(__lastKeyboardGUIElements.begin(),__lastKeyboardGUIElements.end(),[gui](const std::pair<GLFW::Key,WIHandle> &p) {
					return (p.second.get() == gui) ? true : false;
					});
				if(it != __lastKeyboardGUIElements.end() && !it->second.IsValid())
					__lastKeyboardGUIElements.erase(it);
				if(result == util::EventReply::Handled)
					return true;
			}

			gui = gui->GetParent();
		}
	}
	return false;
}

bool WIBase::__wiCharCallback(GLFW::Window&,unsigned int c)
{
	WIBase *gui = WGUI::GetInstance().GetFocusedElement();
	if(gui != NULL)
	{
		if(gui->GetKeyboardInputEnabled())
			return gui->CharCallback(c) == util::EventReply::Handled;
	}
	return false;
}

bool WIBase::__wiScrollCallback(GLFW::Window &window,Vector2 offset)
{
	auto cursorPos = window.GetCursorPos();
	WIBase *gui = WGUI::GetInstance().GetBaseElement();
	if(gui->IsVisible())
	{
		gui = WGUI::GetInstance().GetGUIElement(gui,static_cast<int>(cursorPos.x),static_cast<int>(cursorPos.y),[](WIBase *elChild) -> bool {return elChild->GetScrollInputEnabled();});
		while(gui != nullptr)
		{
			if(gui->GetScrollInputEnabled())
				return gui->ScrollCallback(offset) == util::EventReply::Handled;
			gui = gui->GetParent();
		}
	}
	return false;
}

void WIBase::FadeIn(float tFade,float alphaTarget)
{
	m_fade = std::make_unique<WIFadeInfo>(tFade);
	m_fade->alphaTarget = alphaTarget;
	UpdateThink();
}
void WIBase::FadeOut(float tFade,bool removeOnFaded)
{
	m_fade = std::make_unique<WIFadeInfo>(tFade);
	m_fade->alphaTarget = 0.f;
	m_fade->removeOnFaded = removeOnFaded;
	UpdateThink();
}
bool WIBase::IsFading() const {return (m_fade != NULL) ? true : false;}
bool WIBase::IsFadingIn() const
{
	if(!IsFading())
		return false;
	return (m_fade->alphaTarget >= GetAlpha()) ? true : false;
}
bool WIBase::IsFadingOut() const
{
	if(!IsFading())
		return false;
	return (m_fade->alphaTarget < GetAlpha()) ? true : false;
}
void WIBase::SetIgnoreParentAlpha(bool ignoreParentAlpha) {umath::set_flag(m_stateFlags,StateFlags::IgnoreParentAlpha,ignoreParentAlpha);}
bool WIBase::ShouldIgnoreParentAlpha() const {return umath::is_flag_set(m_stateFlags,StateFlags::IgnoreParentAlpha);}
#pragma optimize("",on)
