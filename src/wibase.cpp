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
#include "wgui/shaders/wishader_colored.hpp"
#include "wgui/types/wicontextmenu.hpp"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <sharedutils/scope_guard.h>
#include <atomic>
#include <prosper_window.hpp>
#include <sharedutils/property/util_property_color.hpp>
#include <mathutil/umath_geometry.hpp>
#pragma optimize("",off)
LINK_WGUI_TO_CLASS(WIBase,WIBase);

Vector4 WIBase::DrawInfo::GetColor(WIBase &el,const wgui::DrawState &drawState) const
{
	Vector4 color;
	if(this->color.has_value())
		color = *this->color;
	else
		color = el.GetColor().ToVector4();
	color.a *= drawState.renderAlpha;
	return color;
}

/////////////

bool is_valid(const WIHandle &hEl)
{
	return hEl.IsValid() && !hEl->IsRemovalScheduled();
}

WIBase::WIBase()
	: CallbackHandler(),m_cursor(GLFW::Cursor::Shape::Default),
	m_color(util::ColorProperty::Create(Color::White)),
	m_pos(util::Vector2iProperty::Create()),
	m_size(util::Vector2iProperty::Create()),
	m_bVisible(util::BoolProperty::Create(true)),
	m_bHasFocus(util::BoolProperty::Create(false)),
	m_bMouseInBounds(util::BoolProperty::Create(false)),
	m_scale{util::Vector2Property::Create(Vector2{1.f,1.f})}
{
	RegisterCallback<void>("OnFocusGained");
	RegisterCallback<void>("OnFocusKilled");
	RegisterCallback<void>("SetPos");
	RegisterCallback<void>("SetSize");
	RegisterCallback<void,int32_t,int32_t>("OnCursorMoved");
	RegisterCallback<void>("Think");
	RegisterCallback<void>("OnPreRemove");
	RegisterCallback<void>("OnRemove");
	RegisterCallback<void,WIBase*>("OnChildAdded");
	RegisterCallback<void,WIBase*>("OnChildRemoved");
	RegisterCallback<void>("OnCursorEntered");
	RegisterCallback<void>("OnCursorExited");
	RegisterCallback<void>("OnUpdated");
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
			if(is_valid(hChild) == false)
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
	for(unsigned int i=0;i<m_children.size();i++)
	{
		if(m_children[i].IsValid())
			m_children[i]->Remove();
	}
	m_fade = nullptr;
}

void WIBase::SetLocalRenderTransform(const umath::ScaledTransform &transform) {m_localRenderTransform = std::make_unique<umath::ScaledTransform>(transform);}
void WIBase::ClearLocalRenderTransform() {m_localRenderTransform = nullptr;}
umath::ScaledTransform *WIBase::GetLocalRenderTransform() {return m_localRenderTransform.get();}

void WIBase::UpdateParentAutoSizeToContents()
{
	if(m_parent.expired() || umath::is_flag_set(m_parent->m_stateFlags,StateFlags::AutoSizeToContentsX | StateFlags::AutoSizeToContentsY) == false || IsBackgroundElement())
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
void WIBase::UpdateParentThink()
{
	auto *parent = GetParent();
	umath::set_flag(m_stateFlags,StateFlags::ParentUpdateIfInvisibleBit,parent ? parent->ShouldThinkIfInvisible() : false);

	for(auto &hChild : *GetChildren())
	{
		if(is_valid(hChild) == false)
			continue;
		hChild->UpdateParentThink();
	}
}
void WIBase::UpdateVisibility()
{
	auto *parent = GetParent();
	umath::set_flag(m_stateFlags,StateFlags::ParentVisible,parent ? parent->IsVisible() : true);

	for(auto &hChild : *GetChildren())
	{
		if(is_valid(hChild) == false)
			continue;
		hChild->UpdateVisibility();
	}

	// If we just became visible and we have an update schedule, we need to inform the GUI instance
	if(IsVisible() && (umath::is_flag_set(m_stateFlags,StateFlags::UpdateScheduledBit) || ShouldThinkIfInvisible()))
		WGUI::GetInstance().m_bGUIUpdateRequired = true;

	UpdateThink();
}
void WIBase::SetShouldScissor(bool b) {umath::set_flag(m_stateFlags,StateFlags::ShouldScissorBit,b);}
bool WIBase::GetShouldScissor() const {return umath::is_flag_set(m_stateFlags,StateFlags::ShouldScissorBit);}
std::ostream &WIBase::Print(std::ostream &stream) const
{
	auto &t = typeid(*this);
	stream<<t.name();
	return stream;
}
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
		if(is_valid(hOther) == false)
			return;
		const_cast<WIBase*>(hOther.get())->Remove();
	}));
}
void WIBase::ScheduleUpdate()
{
	//if(umath::is_flag_set(m_stateFlags,StateFlags::UpdateScheduledBit))
	//	return;
	WGUI::GetInstance().ScheduleElementForUpdate(*this);
}
bool WIBase::IsUpdateScheduled() const {return umath::is_flag_set(m_stateFlags,StateFlags::UpdateScheduledBit);}
bool WIBase::IsRemovalScheduled() const {return umath::is_flag_set(m_stateFlags,StateFlags::RemoveScheduledBit);}
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
	m_cbAutoAlign = parent->AddCallback("SetSize",FunctionCallback<>::Create([hEl]() mutable {
		if(!is_valid(hEl))
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
	auto cb = [hEl]() mutable {
		if(!is_valid(hEl))
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
	auto cb = [hEl]() mutable {
		if(!is_valid(hEl))
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
	m_handle = util::to_shared_handle<WIBase>(std::shared_ptr<WIBase>{this,[](WIBase*) {}}); // Deletion is handled by WGUI class
}
const std::shared_ptr<void> &WIBase::GetUserData() {return m_userData.at(0);}
const std::shared_ptr<void> &WIBase::GetUserData2() {return m_userData.at(1);}
const std::shared_ptr<void> &WIBase::GetUserData3() {return m_userData.at(2);}
const std::shared_ptr<void> &WIBase::GetUserData4() {return m_userData.at(3);}
void WIBase::SetUserData(const std::shared_ptr<void> &userData) {m_userData.at(0) = userData;}
void WIBase::SetUserData2(const std::shared_ptr<void> &userData) {m_userData.at(1) = userData;}
void WIBase::SetUserData3(const std::shared_ptr<void> &userData) {m_userData.at(2) = userData;}
void WIBase::SetUserData4(const std::shared_ptr<void> &userData) {m_userData.at(3) = userData;}
WIHandle WIBase::GetHandle() const {return util::TWeakSharedHandle<WIBase>{m_handle};}
void WIBase::TrapFocus(bool b)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::TrapFocusBit) == b)
		return;
	umath::set_flag(m_stateFlags,StateFlags::TrapFocusBit,b);
	
	auto *window = GetRootWindow();
	if(!window)
		return;
	auto *pair = WGUI::GetInstance().FindWindowRootPair(*window);
	if(!pair)
		return;
	if(b == true)
		pair->focusTrapStack.push_back(this->GetHandle());
	else
	{
		for(auto it=pair->focusTrapStack.begin();it!=pair->focusTrapStack.end();)
		{
			auto &hEl = *it;
			if(!is_valid(hEl))
				it = pair->focusTrapStack.erase(it);
			else if(hEl.get() == this)
			{
				pair->focusTrapStack.erase(it);
				break;
			}
			else
				++it;
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
	const WIBase *pChild = hElement.get();
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
			if(is_valid(elements[i]))
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
void WIBase::SetLocalAlpha(float a) {m_localAlpha = a; UpdateTransparencyState();}
void WIBase::UpdateTransparencyState() {umath::set_flag(m_stateFlags,StateFlags::FullyTransparent,(GetAlpha() *m_localAlpha) == 0.f);}
bool WIBase::IsFullyTransparent() const {return umath::is_flag_set(m_stateFlags,StateFlags::FullyTransparent);}
void WIBase::SetAlpha(float alpha)
{
	auto &col = GetColor();
	*m_color = Color(col.r,col.g,col.b,alpha *255.f);
}
void WIBase::GetMousePos(int *x,int *y) const
{
	auto *window = GetRootWindow();
	Vector2 cursorPos {};
	if(window)
	{
		cursorPos = (*window)->GetCursorPos();
		Vector2i cur = GetAbsolutePos();
		cursorPos.x -= cur.x;
		cursorPos.y -= cur.y;
	}
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
		if(is_valid(child) == false || child->IsBackgroundElement())
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
void WIBase::SetStencilEnabled(bool enabled) {umath::set_flag(m_stateFlags,StateFlags::StencilEnabled,enabled);}
bool WIBase::IsStencilEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::StencilEnabled);}
void WIBase::SetBackgroundElement(bool backgroundElement,bool autoAlignToParent)
{
	umath::set_flag(m_stateFlags,StateFlags::IsBackgroundElement,backgroundElement);
	if(backgroundElement && autoAlignToParent)
	{
		//SetPos(0,0);
		//SetSize(GetParent()->GetSize());
		//SetAnchor(0.f,0.f,1.f,1.f);
		if(HasAnchor() == false)
			SetAutoAlignToParent(true);
		SetZPos(-100);
	}
}
bool WIBase::IsBackgroundElement() const {return umath::is_flag_set(m_stateFlags,StateFlags::IsBackgroundElement);}
bool WIBase::HasFocus() {return *m_bHasFocus;}
void WIBase::RequestFocus()
{
	auto *window = GetRootWindow();
	if(HasFocus())
	{
		WGUI::GetInstance().IncrementFocusCount(window);
		return;
	}
	if(window && !WGUI::GetInstance().SetFocusedElement(this,window))
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
	auto *window = GetRootWindow();
	if(window)
		WGUI::GetInstance().SetFocusedElement(NULL,window);
	if(bForceKill == false)
	{
		auto *window = GetRootWindow();
		auto *pair = window ? WGUI::GetInstance().FindWindowRootPair(*window) : nullptr;
		if(pair)
			pair->RestoreTrappedFocus(this);
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
	WIBase *root = el->GetRootElement();
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
WIBase *WIBase::GetRootElement()
{
	auto *parent = GetParent();
	if(parent)
		return parent->GetRootElement();
	return this;
}
void WIBase::SetParentAndUpdateWindow(WIBase *base,std::optional<uint32_t> childIndex)
{
	if(!base)
	{
		SetParent(nullptr,childIndex);
		return;
	}
	auto *window = base->GetRootWindow();
	auto *curWindow = GetRootWindow();
	if(curWindow == window)
		return SetParent(base,childIndex);
	
	WIHandle hFocused {};
	auto trapped = false;
	WIBase *elFocused = nullptr;
	std::unordered_set<WIBase*> traversed; // Used to prevent potential infinite loop
	for(;;)
	{
		elFocused = WGUI::GetInstance().GetFocusedElement(curWindow);
		if(!elFocused)
			break;
		if(traversed.find(elFocused) != traversed.end())
			break;
		traversed.insert(elFocused);
		if(elFocused == this || elFocused->IsDescendantOf(this))
		{
			// The focused element is part of the new window, so we have to
			// kill the focus
			auto wasTrapped = elFocused->IsFocusTrapped();
			elFocused->TrapFocus(false);
			elFocused->KillFocus();

			if(!hFocused.IsValid())
			{
				// We want to re-apply the focus once the element
				// has been assigned to the new window
				hFocused = elFocused->GetHandle();
				trapped = wasTrapped;
			}
		}
		else
		{
			// The focused element isn't part of the new window, so it can
			// keep the focus
			hFocused = WIHandle{};
			break;
		}
	}
	WGUI::GetInstance().ClearFocus(*this); // Force clear focus for this element and all descendants

	SetParent(base);
	SetAnchor(0,0,1,1);
	
	if(hFocused.IsValid())
	{
		if(trapped)
			hFocused->TrapFocus(true);
		hFocused->RequestFocus();
	}
}
prosper::Window *WIBase::GetRootWindow()
{
	auto *el = GetRootElement();
	if(!el)
		return nullptr;
	return WGUI::GetInstance().FindWindow(*el);
}
void WIBase::SetVisible(bool b)
{
	if(*m_bVisible == b || umath::is_flag_set(m_stateFlags,StateFlags::RemoveScheduledBit))
		return;
	*m_bVisible = b;
	OnVisibilityChanged(b);
	if(b == false)
	{
		UpdateMouseInBounds();
		WIBase *el = WGUI::GetInstance().GetFocusedElement(GetRootWindow());
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

	CallCallbacks("OnUpdated");
}
void WIBase::OnFirstThink() {}
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
Mat4 WIBase::GetRelativePose(float x,float y) const
{
	Vector4 pos {x *2.f,y *2.f,0.f,1.f};
	if(m_rotationMatrix)
		return (*m_rotationMatrix *glm::translate(Vector3{pos.x,pos.y,pos.z}));
	return  glm::translate(Vector3{pos.x,pos.y,pos.z});
}
Mat4 WIBase::GetAbsolutePose(float x,float y) const
{
	auto *parent = GetParent();
	if(!parent)
		return GetRelativePose(x,y);
	return parent->GetAbsolutePose(GetX(),GetY()) *GetRelativePose(x,y);
}
Mat4 WIBase::GetAbsolutePose() const {return GetAbsolutePose(0.f,0.f);}
Vector2 WIBase::GetRelativePos(const Vector2 &absPos) const
{
	auto tmp = absPos;
	AbsolutePosToRelative(tmp);
	return tmp;
}
Vector2 WIBase::GetAbsolutePos(const Vector2 &localPos,bool includeRotation) const
{
	if(!includeRotation)
		return GetAbsolutePos(includeRotation) +localPos;
	auto pose = GetAbsolutePose() *glm::translate(Vector3{localPos *2.f,0.f});
	return Vector2{pose[3][0],pose[3][1]} /2.f;
}
Vector2 WIBase::GetAbsolutePos(bool includeRotation) const
{
	if(!includeRotation)
	{
		auto pos = GetPos();
		auto *parent = GetParent();
		if(parent)
			pos += parent->GetAbsolutePos(includeRotation);
		return pos;
	}
	auto pose = GetAbsolutePose();
	return Vector2{pose[3][0],pose[3][1]} /2.f;
}
void WIBase::SetAbsolutePos(const Vector2 &pos)
{
	auto tmp = pos;
	if(m_parent.IsValid())
		m_parent->AbsolutePosToRelative(tmp);
	SetPos(tmp);
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
umath::intersection::Intersect WIBase::IsInBounds(int x,int y,int w,int h) const
{
	Vector3 minA {x,y,0.f};
	Vector3 maxA {x +w,y +h,0.f};
	Vector3 minB {0.f,0.f,0.f};
	Vector3 maxB {GetWidth(),GetHeight(),0.f};
	return umath::intersection::aabb_aabb(minA,maxA,minB,maxB);
}
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
Mat4 WIBase::GetTransformPose(const Vector2i &origin,int w,int h,const Mat4 &poseParent,const Vector2 &scale) const
{
	Vector3 normOrigin {(origin.x) *2,(origin.y) *2,0.f};
	Vector3 normScale {(*m_size)->x *scale.x,(*m_size)->y *scale.y,0};

	if(!m_rotationMatrix)
		return poseParent *glm::scale(glm::translate(normOrigin),normScale);

	auto t = glm::translate(normOrigin);
	auto s = glm::scale(normScale);
	auto &r = *m_rotationMatrix;
	auto m = t *r *s;
	return poseParent *m;
}
void WIBase::SetScale(const Vector2 &scale) {*m_scale = scale;}
void WIBase::SetScale(float x,float y) {SetScale(Vector2{x,y});}
const Vector2 &WIBase::GetScale() const {return *m_scale;}
const util::PVector2Property &WIBase::GetScaleProperty() const {return m_scale;}
Mat4 WIBase::GetTransformPose(int w,int h,const Mat4 &poseParent) const {return GetTransformPose(*m_pos,w,h,poseParent);}
Mat4 WIBase::GetTransformPose(int w,int h) const
{
	return GetTransformPose(*m_pos,w,h,umat::identity());
}
Mat4 WIBase::GetTranslationPose(const Vector2i &origin,int w,int h,const Mat4 &poseParent) const
{
	umath::ScaledTransform pose {};
	pose.SetOrigin(Vector3{(origin.x) *2,(origin.y) *2,0.f});
	return poseParent *pose.ToMatrix();
}
Mat4 WIBase::GetTranslationPose(int w,int h,const Mat4 &poseParent) const
{
	return GetTranslationPose(*m_pos,w,h,poseParent);
}
Mat4 WIBase::GetTranslationPose(int w,int h) const
{
	return GetTranslationPose(w,h,umat::identity());
}
Mat4 WIBase::GetScaledMatrix(int w,int h,const Mat4 &poseParent) const
{
	Vector3 scale((*m_size)->x,(*m_size)->y,0);
	return glm::scale(poseParent,scale);
}
Mat4 WIBase::GetScaledMatrix(int w,int h) const
{
	Mat4 mat(1.0f);
	return GetScaledMatrix(w,h,mat);
}
void WIBase::Render(const DrawInfo &drawInfo,wgui::DrawState &drawState,const Mat4 &matDraw,const Vector2 &scale,uint32_t testStencilLevel,wgui::StencilPipeline stencilPipeline)
{
	if(stencilPipeline == wgui::StencilPipeline::Test)
		return;

	// We don't actually render the element, but it still needs to be drawn to the stencil buffer
	auto *shader = WGUI::GetInstance().GetStencilShader();
	assert(shader != nullptr);
	auto &context = WGUI::GetInstance().GetContext();
	prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
	if(shader->RecordBeginDraw(bindState,drawState,drawInfo.size.x,drawInfo.size.y,stencilPipeline,drawInfo.msaa) == true)
	{
		shader->RecordDraw(bindState,{matDraw,Vector4{},wgui::ElementData::ToViewportSize(drawInfo.size)},testStencilLevel);
		shader->RecordEndDraw(bindState);
	}
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
void WIBase::SetRemoveOnParentRemoval(bool b) {umath::set_flag(m_stateFlags,StateFlags::DontRemoveOnParentRemoval,!b);}
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
void WIBase::SetThinkIfInvisible(bool bThinkIfInvisible)
{
	umath::set_flag(m_stateFlags,StateFlags::UpdateIfInvisibleBit,bThinkIfInvisible);
	UpdateParentThink();
}
bool WIBase::ShouldThinkIfInvisible() const {return umath::is_flag_set(m_stateFlags,StateFlags::UpdateIfInvisibleBit | StateFlags::ParentUpdateIfInvisibleBit);}
void WIBase::SetRenderIfZeroAlpha(bool renderIfZeroAlpha) {umath::set_flag(m_stateFlags,StateFlags::RenderIfZeroAlpha,renderIfZeroAlpha);}
void WIBase::UpdateCursorMove(int x,int y)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::MouseCheckEnabledBit) == false)
		return;
	if(x != m_lastMouseX || y != m_lastMouseY)
	{
		OnCursorMoved(x,y);
		m_lastMouseX = x;
		m_lastMouseY = y;
	}
}
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
		auto *window = GetRootWindow();
		if(window && (*window)->IsFocused())
			UpdateChildrenMouseInBounds();
	}
	
	if(umath::is_flag_set(m_stateFlags,StateFlags::MouseCheckEnabledBit) && IsVisible())
	{
		int x,y;
		GetMousePos(&x,&y);
		UpdateCursorMove(x,y);
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
	if(umath::is_flag_set(m_stateFlags,StateFlags::FirstThink) == false)
	{
		umath::set_flag(m_stateFlags,StateFlags::FirstThink);
		OnFirstThink();
	}
}
void WIBase::CalcBounds(const Mat4 &mat,int32_t w,int32_t h,Vector2i &outPos,Vector2i &outSize)
{
	outSize = Vector2i(mat[0][0],mat[1][1]);
	outPos = Vector2i(
		mat[3][0] /2.f,
		mat[3][1] /2.f
	);
}
void WIBase::ResetRotation() {m_rotationMatrix = nullptr;}
void WIBase::SetRotation(umath::Degree angle,const Vector2 &pivot)
{
	Mat4 r = glm::translate(Vector3{pivot *2.f,0.f});
	r = r *umat::create(uquat::create(EulerAngles{0.f,0.f,angle}));
	r = r *glm::translate(Vector3{-pivot *2.f,0.f});
	SetRotation(r);
}
void WIBase::SetRotation(const Mat4 &rotationMatrix) {m_rotationMatrix = std::make_unique<Mat4>(rotationMatrix);}
const Mat4 *WIBase::GetRotationMatrix() const {return m_rotationMatrix.get();}

void WIBase::Draw(const DrawInfo &drawInfo,wgui::DrawState &drawState,const Vector2i &offsetParent,const Vector2i &scissorOffset,const Vector2i &scissorSize,const Vector2 &scale,uint32_t testStencilLevel)
{
	const auto w = drawInfo.size.x;
	const auto h = drawInfo.size.y;
	const auto &origin = drawInfo.offset;

	auto rootPose = drawInfo.transform;
	auto useScissor = drawInfo.useScissor;
	auto useStencilGlobal = drawInfo.useStencil;
	if(m_rotationMatrix)
	{
		useScissor = false;
		useStencilGlobal = true;
	}
	const auto &postTransform = drawInfo.postTransform;

	auto poseDraw = GetTransformPose(origin,w,h,rootPose,scale);
	if(postTransform.has_value())
		poseDraw = *postTransform *poseDraw;

	auto matDraw = poseDraw;
	if(useScissor == true)
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
	
	auto &context = WGUI::GetInstance().GetContext();
	uint32_t stencilLevel = testStencilLevel;
	auto newStencilLevel = stencilLevel;
	std::array<uint32_t,4> oldScissor;
	if(IsStencilEnabled() || useStencilGlobal)
	{
		drawState.GetScissor(oldScissor[0],oldScissor[1],oldScissor[2],oldScissor[3]);
		drawState.SetScissor(0u,0u,drawInfo.size.x,drawInfo.size.y);
		++newStencilLevel;
	}
	auto useStencil = (newStencilLevel > stencilLevel);
	auto fullyTransparent = IsFullyTransparent();
	if(useStencil || !fullyTransparent)
		Render(drawInfo,drawState,matDraw,scale,stencilLevel,useStencil ? wgui::StencilPipeline::Increment : wgui::StencilPipeline::Test);

	rootPose = GetTranslationPose(origin,w,h,rootPose);
	if(m_rotationMatrix)
		rootPose = rootPose **m_rotationMatrix;

	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIBase *child = m_children[i].get();
		if(child != NULL && child->IsSelfVisible())
		{
			auto useScissorChild = (useScissor && child->GetShouldScissor()) ? true : false;
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
			if(useScissor == false)
				drawState.SetScissor(0u,0u,drawInfo.size.x,drawInfo.size.y);
			else if(useScissorChild == false)
				drawState.SetScissor(umath::max(posScissor[0],0),umath::max(posScissor[1],0),umath::max(szScissor[0],0),umath::max(szScissor[1],0)); // Use parent scissor values

			posScissor = posChild;
			szScissor = posChildEnd -posChild;

			//auto yScissor = context.GetHeight() -(posScissor[1] +szScissor[1]); // Move origin to top left (OpenGL)
			//drawCmd->SetScissor(posScissor[0],yScissor,szScissor[0],szScissor[1]);
			if(useScissor == true && useScissorChild == true)
				drawState.SetScissor(umath::max(posScissor[0],0),umath::max(posScissor[1],0),umath::max(szScissor[0],0),umath::max(szScissor[1],0));

			float aOriginal = drawState.renderAlpha;
			if(child->ShouldIgnoreParentAlpha())
				drawState.renderAlpha = 1.f;
			else
			{
				float alpha = GetAlpha();
				drawState.renderAlpha = aOriginal *alpha;
				if(drawState.renderAlpha > 1.f)
					drawState.renderAlpha = 1.f;
				else if(drawState.renderAlpha < 0.f)
					drawState.renderAlpha = 0.f;
			}
			auto childDrawInfo = drawInfo;
			childDrawInfo.offset = *child->m_pos;
			childDrawInfo.transform = rootPose;
			childDrawInfo.useScissor = useScissorChild;
			childDrawInfo.useStencil = useStencilGlobal;
			auto scaleChild = scale *child->GetScale();
			child->Draw(childDrawInfo,drawState,offsetParentNew,posScissor,szScissor,scaleChild,newStencilLevel);
			drawState.renderAlpha = aOriginal;
		}
	}
	if(useStencil) // Undo stencil
	{
		// Reset scissor
		drawState.SetScissor(oldScissor[0],oldScissor[1],oldScissor[2],oldScissor[3]);
		Render(drawInfo,drawState,matDraw,scale,newStencilLevel,wgui::StencilPipeline::Decrement);
	}
}
void WIBase::Draw(const DrawInfo &drawInfo,wgui::DrawState &drawState)
{
	auto scissorPos = GetPos();
	auto scissorSize = drawInfo.size;
	if(drawInfo.useScissor)
	{
		scissorPos = {};
		scissorSize = GetSize();
	}
	drawState.SetScissor(scissorPos.x,scissorPos.y,scissorSize.x,scissorSize.y);
	Draw(drawInfo,drawState,GetPos(),scissorPos,scissorSize,GetScale());
}
void WIBase::Draw(int w,int h,std::shared_ptr<prosper::ICommandBuffer> &cmdBuf)
{
	if(!IsVisible())
		return;
	DrawInfo drawInfo {cmdBuf};
	drawInfo.offset = GetPos();
	drawInfo.useScissor = GetShouldScissor();
	drawInfo.size = {w,h};
	wgui::DrawState drawState {};
	Draw(drawInfo,drawState);
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
void WIBase::AnchorWithMargin(uint32_t left,uint32_t top,uint32_t right,uint32_t bottom)
{
	ClearAnchor();
	SetPos(left,top);
	auto *p = GetParent();
	if(!p)
		return;
	SetSize(p->GetWidth() -(right +left),p->GetHeight() -(top +bottom));
	SetAnchor(0.f,0.f,1.f,1.f);
}
void WIBase::AnchorWithMargin(uint32_t margin) {AnchorWithMargin(margin,margin,margin,margin);}
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
	CallCallbacks<void>("OnPreRemove");
	for(auto &hChild : *GetChildren())
	{
		if(hChild.IsValid() == false)
			continue;
		if(umath::is_flag_set(hChild->m_stateFlags,StateFlags::DontRemoveOnParentRemoval))
		{
			hChild->ClearParent();
			continue;
		}
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
		return m_fade != nullptr || umath::is_flag_set(m_stateFlags,StateFlags::UpdateIfInvisibleBit | StateFlags::ParentUpdateIfInvisibleBit | StateFlags::RenderIfZeroAlpha);
	return IsParentVisible() || umath::is_flag_set(m_stateFlags,StateFlags::ParentUpdateIfInvisibleBit);
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
		UpdateParentThink();
		return;
	}

	m_parent = base->GetHandle();
	base->AddChild(this,childIndex);

	if((m_stateFlags &(StateFlags::AutoAlignToParentXBit | StateFlags::AutoAlignToParentYBit)) != StateFlags::None)
		SetAutoAlignToParent(umath::is_flag_set(m_stateFlags,StateFlags::AutoAlignToParentXBit),umath::is_flag_set(m_stateFlags,StateFlags::AutoAlignToParentYBit),true);

	if(umath::is_flag_set(m_stateFlags,StateFlags::AutoCenterToParentXBit) == true)
		SetAutoCenterToParentX(true,true);
	if(umath::is_flag_set(m_stateFlags,StateFlags::AutoCenterToParentYBit) == true)
		SetAutoCenterToParentY(true,true);

	UpdateVisibility();
	UpdateParentThink();
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
bool WIBase::PosInBounds(const Vector2i &pos) const
{
	Vector2 tmp {pos};
	AbsolutePosToRelative(tmp);
	return DoPosInBounds(tmp);
}
bool WIBase::DoPosInBounds(const Vector2i &pos) const
{
	const Vector2i &size = GetSize();
	return (pos.x < 0 || pos.y < 0 || pos.x >= size.x || pos.y >= size.y) ? false : true;
}
void WIBase::Resize() {SetSize(GetSize());}
const util::PBoolProperty &WIBase::GetMouseInBoundsProperty() const {return m_bMouseInBounds;}
bool WIBase::MouseInBounds() const
{
	auto &context = WGUI::GetInstance().GetContext();
	auto *window = GetRootWindow();
	auto cursorPos = window ? (*window)->GetCursorPos() : Vector2{};
	return PosInBounds(static_cast<int>(cursorPos.x),static_cast<int>(cursorPos.y));
}
bool WIBase::GetMouseInputEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::AcceptMouseInputBit);}
bool WIBase::GetKeyboardInputEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::AcceptKeyboardInputBit);}
bool WIBase::GetScrollInputEnabled() const {return umath::is_flag_set(m_stateFlags,StateFlags::AcceptScrollInputBit);}
void WIBase::AbsolutePosToRelative(Vector2 &pos) const
{
	pos = glm::inverse(GetAbsolutePose()) *Vector4{pos *2.f,0.f,1.f};
	pos /= 2.f;
}
void WIBase::UpdateMouseInBounds(const Vector2 &relPos,bool forceFalse)
{
	bool old = *m_bMouseInBounds;
	if(forceFalse)
		*m_bMouseInBounds = false;
	else
		*m_bMouseInBounds = DoPosInBounds(relPos);
	if(old == *m_bMouseInBounds || !umath::is_flag_set(m_stateFlags,StateFlags::AcceptMouseInputBit))
		return;
	if(*m_bMouseInBounds == true)
		OnCursorEntered();
	else OnCursorExited();
}
void WIBase::UpdateMouseInBounds(bool forceFalse)
{
	auto &context = WGUI::GetInstance().GetContext();
	auto *window = GetRootWindow();
	auto cursorPos = window ? (*window)->GetCursorPos() : Vector2{};
	AbsolutePosToRelative(cursorPos);
	return UpdateMouseInBounds(cursorPos,forceFalse);
}
void WIBase::DoUpdateChildrenMouseInBounds(const Mat4 &parentPose,const Vector2 &cursorPos,bool ignoreVisibility,bool forceFalse)
{
	if(ignoreVisibility == false && IsVisible() == false)
		return;
	auto localPose = parentPose *GetRelativePose(0.f,0.f);
	auto relCursorPos = glm::inverse(localPose) *Vector4{cursorPos *2.f,0.f,1.f};
	auto wasInBounds = **m_bMouseInBounds;
	UpdateMouseInBounds(Vector2{relCursorPos} /2.f,forceFalse);
	if(*m_bMouseInBounds == wasInBounds && *m_bMouseInBounds == false)
		return;
	if(*m_bMouseInBounds == false)
		forceFalse = true;
	for(unsigned int i=0;i<m_children.size();i++)
	{
		WIHandle &hChild = m_children[i];
		if(is_valid(hChild) && (hChild->IsVisible() || hChild->ShouldThinkIfInvisible()))
			hChild->DoUpdateChildrenMouseInBounds(parentPose *GetRelativePose(hChild->GetX(),hChild->GetY()),cursorPos,ignoreVisibility,forceFalse);
	}
}
void WIBase::UpdateChildrenMouseInBounds(bool ignoreVisibility,bool forceFalse)
{
	auto &context = WGUI::GetInstance().GetContext();
	auto *window = GetRootWindow();
	auto cursorPos = window ? (*window)->GetCursorPos() : Vector2{};
	DoUpdateChildrenMouseInBounds(GetAbsolutePose(),cursorPos,ignoreVisibility,forceFalse);
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

	auto reply = util::EventReply::Unhandled;
	CallCallbacksWithOptionalReturn<util::EventReply,GLFW::MouseButton,GLFW::KeyState,GLFW::Modifier>("OnMouseEvent",reply,button,state,mods);
	if(reply == util::EventReply::Handled)
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
	for(auto &hChild : m_children)
	{
		if(hChild.IsValid() == false)
			continue;
		if(predInspect(*hChild.get()) == true && hChild.get()->GetZPos() > largestZPos)
		{
			auto *rChild = hChild->FindDeepestChild(predInspect,predValidCandidate);
			if(rChild)
			{
				r = rChild;
				largestZPos = hChild->GetZPos();
			}
		}
	}
	return r;
}
void WIBase::InjectMouseMoveInput(int32_t x,int32_t y)
{
	std::function<void(WIBase&,int,int)> iterateChildren = nullptr;
	iterateChildren = [&iterateChildren](WIBase &el,int x,int y) {
		el.UpdateCursorMove(x,y);
		for(auto &hChild : el.m_children)
		{
			if(hChild.IsValid() == false)
				continue;
			iterateChildren(*hChild.get(),x -hChild->GetX(),y -hChild->GetY());
		}
	};
	iterateChildren(*this,x,y);

	OnCursorMoved(x,y);
	UpdateChildrenMouseInBounds(true);
}
util::EventReply WIBase::InjectMouseInput(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto *el = FindDeepestChild([](const WIBase &el) {
		return el.IsSelfVisible() && el.MouseInBounds();
	},[](const WIBase &el) {
		return el.GetMouseInputEnabled() && el.IsSelfVisible();
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
		return el.IsSelfVisible() && el.MouseInBounds();
	},[](const WIBase &el) {
		return el.GetScrollInputEnabled() && el.IsSelfVisible();
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

	auto &wgui = WGUI::GetInstance();
	auto *window = el.GetRootWindow();
	WIBase *pFocused = wgui.GetFocusedElement(window);
	auto c = wgui.GetFocusCount(window);
	auto hFocused = pFocused ? pFocused->GetHandle() : WIHandle{};
	__lastMouseGUIElements[button] = el.GetHandle();
	auto result = el.MouseCallback(button,state,mods);
	{
		auto it = __lastMouseGUIElements.find(button);
		if(hEl.IsValid() == false || it == __lastMouseGUIElements.end() || it->second.get() != &el)
			return result; // Mouse button press was hijacked by another element
	}
	// Callback may have invoked mouse button release-event already, so we have to check if our element's still there
	auto it = std::find_if(__lastMouseGUIElements.begin(),__lastMouseGUIElements.end(),[&el](const std::pair<GLFW::MouseButton,WIHandle> &p) {
		return (p.second.get() == &el) ? true : false;
		});
	if(it != __lastMouseGUIElements.end() && (!is_valid(it->second) || state == GLFW::KeyState::Release))
		__lastMouseGUIElements.erase(it);

	if(wgui.GetFocusCount() != c)
		; // Focus was already changed by callback
	else
	{
		if(is_valid(hFocused) && is_valid(hEl) && &el != pFocused && !pFocused->MouseInBounds())
		{
			pFocused->KillFocus(); // Make sure to kill the focus AFTER the mouse callback.
			if(is_valid(hEl))
				el.RequestFocus();
		}
	}
	return result;
}
bool WIBase::__wiMouseButtonCallback(prosper::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto i = __lastMouseGUIElements.find(button);
	if(i != __lastMouseGUIElements.end())
	{
		auto hEl = i->second;
		__lastMouseGUIElements.erase(i);
		if(is_valid(hEl) && hEl->IsVisible() && hEl->GetMouseInputEnabled())
			hEl->MouseCallback(button,GLFW::KeyState::Release,mods);
	}
	if(state == GLFW::KeyState::Press)
	{
		auto *pContextMenu = WIContextMenu::GetActiveContextMenu();
		if(pContextMenu && pContextMenu->IsCursorInMenuBounds() == false)
			WIContextMenu::CloseContextMenu();

		auto cursorPos = window->GetCursorPos();
		WIBase *p = WGUI::GetInstance().GetFocusedElement(&window);
		WIBase *gui = (p == nullptr || !p->IsFocusTrapped()) ? WGUI::GetInstance().GetBaseElement(&window) : p;
		auto hP = p ? p->GetHandle() : WIHandle{};
		auto hGui = gui->GetHandle();
		if(is_valid(hGui) && gui->IsVisible())
		{
			gui = WGUI::GetInstance().GetGUIElement(gui,static_cast<int>(cursorPos.x),static_cast<int>(cursorPos.y),
				[&hGui,&hP,p,button,state,mods](WIBase *elChild) -> bool {
				if(elChild->GetMouseInputEnabled() == false)
					return false;
				hGui = elChild->GetHandle();
				auto result = InjectMouseButtonCallback(*elChild,button,state,mods);
				return (result == util::EventReply::Handled) ? true : false;
			},&window);
			if(gui)
				return true;
		}
	}
	return false;
}

static std::unordered_map<GLFW::Key,WIHandle> __lastKeyboardGUIElements;
bool WIBase::__wiJoystickCallback(prosper::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state)
{
	
	// TODO
	return false;
}
bool WIBase::__wiKeyCallback(prosper::Window &window,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	auto it = __lastKeyboardGUIElements.find(key);
	if(it != __lastKeyboardGUIElements.end())
	{
		if(is_valid(it->second) && it->second->GetKeyboardInputEnabled())
			it->second->KeyboardCallback(key,scanCode,GLFW::KeyState::Release,mods);
		__lastKeyboardGUIElements.erase(it);
	}
	if(state == GLFW::KeyState::Press || state == GLFW::KeyState::Repeat)
	{
		WIBase *gui = WGUI::GetInstance().GetFocusedElement(&window);
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
				if(it != __lastKeyboardGUIElements.end() && (!is_valid(it->second) || state == GLFW::KeyState::Release))
					__lastKeyboardGUIElements.erase(it);
				if(result == util::EventReply::Handled)
					return true;
			}

			gui = gui->GetParent();
		}
	}
	return false;
}

bool WIBase::__wiCharCallback(prosper::Window &window,unsigned int c)
{
	WIBase *gui = WGUI::GetInstance().GetFocusedElement(&window);
	if(gui != NULL)
	{
		if(gui->GetKeyboardInputEnabled())
			return gui->CharCallback(c) == util::EventReply::Handled;
	}
	return false;
}

bool WIBase::__wiScrollCallback(prosper::Window &window,Vector2 offset)
{
	auto cursorPos = window->GetCursorPos();
	WIBase *gui = WGUI::GetInstance().GetBaseElement(&window);
	if(gui->IsVisible())
	{
		gui = WGUI::GetInstance().GetGUIElement(gui,static_cast<int>(cursorPos.x),static_cast<int>(cursorPos.y),[](WIBase *elChild) -> bool {return elChild->GetScrollInputEnabled();},&window);
		while(gui != nullptr)
		{
			if(gui->GetScrollInputEnabled() && gui->ScrollCallback(offset) == util::EventReply::Handled)
				return true;
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

std::ostream &operator<<(std::ostream &stream,WIBase &el)
{
	el.Print(stream);
	return stream;
}
#pragma optimize("",on)
