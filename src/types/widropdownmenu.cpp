/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/widropdownmenu.h"
#include "wgui/types/witext.h"
#include "wgui/types/wiscrollbar.h"
#include "wgui/types/wiarrow.h"

LINK_WGUI_TO_CLASS(WIDropDownMenu,WIDropDownMenu);

static const Vector4 COLOR_SELECTED {0.1176f,0.564f,1.f,1.f};
static float MARGIN = 5.f;
static int MARGIN_LEFT = 4;
static int OPTION_HEIGHT = 18;

WIDropDownMenu::WIDropDownMenu()
	: WITextEntry(),m_numListItems(15),m_listOffset(0),
	m_selected(-1)
{
	RegisterCallback<void,unsigned int>("OnOptionSelected");
	RegisterCallback<void>("OnValueChanged");
	RegisterCallback<void>("OnMenuOpened");
	RegisterCallback<void>("OnMenuClosed");
}

WIDropDownMenu::~WIDropDownMenu()
{
	if(m_hList.IsValid())
		m_hList->Remove();
}

void WIDropDownMenu::SetListItemCount(uint32_t n) {m_numListItems = n;}

void WIDropDownMenu::OnTextChanged(const std::string &text,bool changedByUser)
{
	WITextEntry::OnTextChanged(text,changedByUser);
	if(changedByUser)
		ClearSelectedOption();
}

void WIDropDownMenu::ClearSelectedOption()
{
	if(m_selected == -1)
		return;
	m_selected = -1;
	CallCallbacks<void,unsigned int>("OnOptionSelected",std::numeric_limits<uint32_t>::max());
}

void WIDropDownMenu::Initialize()
{
	WITextEntry::Initialize();
	SetEditable(false);

	if(m_hBase.IsValid() == true)
	{
		auto hThis = GetHandle();
		m_hBase->AddCallback("OnMousePressed",FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this,hThis](util::EventReply *reply) -> CallbackReturnType {
			*reply = util::EventReply::Handled;
			if(hThis.IsValid() == false)
				return CallbackReturnType::HasReturnValue;
			if(IsEditable())
				return CallbackReturnType::NoReturnValue; // Let text entry handle mouse input
			ToggleMenu();
			return CallbackReturnType::HasReturnValue;
		}));
	}
	AddCallback("OnTextEntered",FunctionCallback<void>::Create([this]() {
		CallCallbacks<void,unsigned int>("OnOptionSelected",std::numeric_limits<uint32_t>::max());
	}));

	m_hOutline = CreateChild<WIOutlinedRect>();
	WIOutlinedRect *pOutline = static_cast<WIOutlinedRect*>(m_hOutline.get());
	pOutline->SetOutlineWidth(1);
	pOutline->SetColor(0.f,0.f,0.f,1.f);

	auto *pArrowContainer = CreateChild<WIBase>().get();
	pArrowContainer->SetSize(20,GetHeight());
	pArrowContainer->SetX(GetWidth() -pArrowContainer->GetWidth());
	pArrowContainer->SetAnchor(1.f,0.f,1.f,1.f);
	pArrowContainer->SetMouseInputEnabled(true);
	pArrowContainer->AddCallback("OnMousePressed",FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this](util::EventReply *reply) -> CallbackReturnType {
		ToggleMenu();
		*reply = util::EventReply::Handled;
		return CallbackReturnType::HasReturnValue;
	}));
	
	m_hArrow = WGUI::GetInstance().Create<WIArrow>(pArrowContainer)->GetHandle();
	auto *pArrow = static_cast<WIArrow*>(m_hArrow.get());
	pArrow->CenterToParent();
	pArrow->SetAnchor(0.5f,0.5f,0.5f,0.5f);

	//m_hText = CreateChild<WIText>();
	//WIText *pText = m_hText.get<WIText>();
	//pText->SetX(static_cast<int>(MARGIN));

	WIRect *pList = WGUI::GetInstance().Create<WIRect>();
	m_hList = pList->GetHandle();
	pList->SetVisible(false);
	pList->SetZPos(10'000);
	pList->AddCallback("OnFocusKilled",FunctionCallback<>::Create(std::bind([](WIHandle hThis) {
		if(!hThis.IsValid())
			return;
		WIDropDownMenu *t = static_cast<WIDropDownMenu*>(hThis.get());
		t->CloseMenu();
	},this->GetHandle())));

	WIScrollBar *pScrollBar = WGUI::GetInstance().Create<WIScrollBar>(pList);
	m_hScrollBar = pScrollBar->GetHandle();
	pScrollBar->SetWidth(8);
	pScrollBar->AddCallback("OnScrollOffsetChanged",FunctionCallback<void,unsigned int>::Create(std::bind([](WIHandle hThis,unsigned int offset) {
		if(!hThis.IsValid())
			return;
		WIDropDownMenu *t = static_cast<WIDropDownMenu*>(hThis.get());
		t->SetOptionOffset(offset);
	},this->GetHandle(),std::placeholders::_1)));

	SetMouseInputEnabled(true);
	UpdateListWindow();
}

void WIDropDownMenu::OnRemove()
{
	WITextEntry::OnRemove();
	if(m_cbListWindowUpdate.IsValid())
		m_cbListWindowUpdate.Remove();
}

void WIDropDownMenu::SelectOption(unsigned int idx)
{
	if(idx >= m_options.size())
		return;
	WIHandle &hOption = m_options[idx];
	if(!hOption.IsValid())
		return;
	WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption*>(hOption.get());
	m_selected = idx;
	SetText(pOption->GetText());
	CallCallbacks<void,unsigned int>("OnOptionSelected",idx);
	CallCallbacks<void>("OnValueChanged");
}

const WIDropDownMenuOption *WIDropDownMenu::FindOptionByValue(const std::string &value) const {return const_cast<WIDropDownMenu*>(this)->FindOptionByValue(value);}
WIDropDownMenuOption *WIDropDownMenu::FindOptionByValue(const std::string &value)
{
	for(auto it=m_options.begin();it!=m_options.end();it++)
	{
		if(it->IsValid())
		{
			auto *pOption = static_cast<WIDropDownMenuOption*>(it->get());
			if(pOption->GetValue() == value)
				return pOption;
		}
	}
	return nullptr;
}
bool WIDropDownMenu::HasOption(const std::string &value) const {return FindOptionByValue(value) != nullptr;}

void WIDropDownMenu::SelectOption(const std::string &value)
{
	auto *option = FindOptionByValue(value);
	if(option == nullptr)
		return;
	SelectOption(option->GetIndex());
}

void WIDropDownMenu::SelectOptionByText(const std::string &name)
{
	auto it = std::find_if(m_options.begin(),m_options.end(),[&name](const WIHandle &hOption) {
		return (hOption.IsValid() && static_cast<const WIDropDownMenuOption*>(hOption.get())->GetText() == name) ? true : false;
	});
	if(it == m_options.end())
		return;
	SelectOption(static_cast<WIDropDownMenuOption*>(it->get())->GetIndex());
}

std::string_view WIDropDownMenu::GetOptionText(uint32_t idx)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return {};
	WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption*>(m_options[idx].get());
	return pOption->GetText();
}

std::string WIDropDownMenu::GetOptionValue(uint32_t idx)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return "";
	WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption*>(m_options[idx].get());
	return pOption->GetValue();
}
void WIDropDownMenu::SetOptionText(uint32_t idx,const std::string &text)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return;
	static_cast<WIDropDownMenuOption*>(m_options[idx].get())->SetText(text);
}
void WIDropDownMenu::SetOptionValue(uint32_t idx,const std::string &val)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return;
	static_cast<WIDropDownMenuOption*>(m_options[idx].get())->SetValue(val);
}

std::string_view WIDropDownMenu::GetText() const {return WITextEntry::GetText();}

std::string WIDropDownMenu::GetValue()
{
	auto idx = m_selected;
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return IsEditable() ? std::string{GetText()} : "";
	WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption*>(m_options[idx].get());
	return pOption->GetValue();
}

int32_t WIDropDownMenu::GetSelectedOption() const {return m_selected;}

void WIDropDownMenu::SetText(const std::string_view &text)
{
	WITextEntry::SetText(text);
	//SetText(text);
	//SizeToContents();
	UpdateText();
}
void WIDropDownMenu::UpdateText()
{
	//SizeToContents();
	/*(m_hArrow.IsValid())
	{
		int w = GetWidth();
		int wMax = m_hArrow->GetX() -static_cast<int>(MARGIN) *2;
		if(w > wMax)
			SetWidth(wMax);
	}
	UpdateTextPos();*/
}
unsigned int WIDropDownMenu::GetOptionCount() {return static_cast<unsigned int>(m_options.size());}

void WIDropDownMenu::UpdateTextPos()
{
	//SetY(static_cast<int>(static_cast<float>(GetHeight()) *0.5f -static_cast<float>(GetHeight()) *0.5f));
}

void WIDropDownMenu::OnOptionSelected(WIDropDownMenuOption *option)
{
	SelectOption(option->GetIndex());
}

WIDropDownMenuOption *WIDropDownMenu::AddOption(const std::string &option,const std::string &value)
{
	if(!m_hList.IsValid())
		return nullptr;
	WIDropDownMenuOption *pOption = WGUI::GetInstance().Create<WIDropDownMenuOption>(m_hList.get());
	WIHandle hOption = pOption->GetHandle();
	pOption->SetDropDownMenu(this);
	pOption->SetText(option);
	pOption->SetHeight(OPTION_HEIGHT);
	pOption->SetIndex(static_cast<int>(m_options.size()));
	pOption->SetValue(value);
	pOption->SetVisible(false);
	auto hMenu = GetHandle();
	pOption->AddCallback("OnScroll",FunctionCallback<util::EventReply,Vector2>::CreateWithOptionalReturn([hMenu](util::EventReply *reply,Vector2 offset) mutable -> CallbackReturnType {
		if(!hMenu.IsValid())
		{
			*reply = util::EventReply::Handled;
			return CallbackReturnType::HasReturnValue;
		}
		WIDropDownMenu *dm = static_cast<WIDropDownMenu*>(hMenu.get());
		dm->InjectScrollInput(offset);
		*reply = util::EventReply::Handled;
		return CallbackReturnType::HasReturnValue;
	}));
	pOption->AddCallback("OnMouseEvent",FunctionCallback<util::EventReply,GLFW::MouseButton,GLFW::KeyState,GLFW::Modifier>::CreateWithOptionalReturn(
		[hOption](util::EventReply *reply,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier) mutable -> CallbackReturnType {
		if(!hOption.IsValid())
		{
			*reply = util::EventReply::Handled;
			return CallbackReturnType::HasReturnValue;
		}
		WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption*>(hOption.get());
		if(button == GLFW::MouseButton::Left && state == GLFW::KeyState::Press)
		{
			WIDropDownMenu *dm = pOption->GetDropDownMenu();
			if(dm != nullptr)
			{
				dm->OnOptionSelected(pOption);
				dm->CloseMenu();
			}
		}
		*reply = util::EventReply::Handled;
		return CallbackReturnType::HasReturnValue;
	}));
	m_options.push_back(hOption);
	if(m_hScrollBar.IsValid())
		static_cast<WIScrollBar*>(m_hScrollBar.get())->SetUp(m_numListItems,static_cast<unsigned int>(m_options.size()));
	return pOption;
}

WIDropDownMenuOption *WIDropDownMenu::AddOption(const std::string &option)
{
	auto idx = m_options.size();
	return AddOption(option,std::to_string(idx));
}

WIDropDownMenuOption *WIDropDownMenu::GetOptionElement(uint32_t idx)
{
	if(idx >= m_options.size())
		return nullptr;
	return static_cast<WIDropDownMenuOption*>(m_options.at(idx).get());
}

WIDropDownMenuOption *WIDropDownMenu::FindOptionSelectedByCursor()
{
	for(auto &hOpt : m_options)
	{
		if(hOpt.IsValid() == false || static_cast<WIDropDownMenuOption*>(hOpt.get())->IsSelected() == false)
			continue;
		return static_cast<WIDropDownMenuOption*>(hOpt.get());
	}
	return nullptr;
}

void WIDropDownMenu::ClearOptions()
{
	for(auto it=m_options.begin();it!=m_options.end();it++)
	{
		auto &hOption = *it;
		if(hOption.IsValid())
			hOption->Remove();
	}
	m_options.clear();
}

void WIDropDownMenu::SetOptions(const std::vector<std::string> &options)
{
	ClearOptions();
	for(auto it=options.begin();it!=options.end();it++)
	{
		auto &option = *it;
		AddOption(option);
	}
}

void WIDropDownMenu::SetOptions(const std::unordered_map<std::string,std::string> &options)
{
	ClearOptions();
	for(auto it=options.begin();it!=options.end();++it)
		AddOption(it->first,it->second);
}

void WIDropDownMenu::SetOptionOffset(unsigned int offset)
{
	unsigned int curOffset = m_listOffset;
	int numList = curOffset +m_numListItems;
	int numOptions = static_cast<int>(m_options.size());
	if(numOptions < numList)
		numList = numOptions;
	for(int i=curOffset;i<numList;i++)
	{
		WIHandle &hOption = m_options[i];
		if(hOption.IsValid())
			hOption->SetVisible(false);
	}

	int y = 0;
	numList = offset +m_numListItems;
	if(numOptions < numList)
		numList = numOptions;
	for(int i=offset;i<numList;i++)
	{
		WIHandle &hOption = m_options[i];
		if(hOption.IsValid())
		{
			WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption*>(hOption.get());
			pOption->SetVisible(true);
			pOption->SetY(y);

			y += OPTION_HEIGHT;
		}
	}
	m_listOffset = offset;
}

void WIDropDownMenu::ScrollToOption(uint32_t offset,bool center)
{
	if(center)
	{
		unsigned int numOptions = static_cast<unsigned int>(m_options.size());
		offset = std::max<int>(std::min<int>(
			offset -static_cast<int>(ceilf(float(m_numListItems) /2.f)) +1,
			static_cast<int>(numOptions) -static_cast<int>(m_numListItems)
		),0);
	}
	if(m_hScrollBar.IsValid())
	{
		WIScrollBar *pScrollBar = static_cast<WIScrollBar*>(m_hScrollBar.get());
		pScrollBar->SetScrollOffset(offset);
		SetOptionOffset(offset);
	}
}

void WIDropDownMenu::UpdateListWindow()
{
	if(!m_hList.IsValid())
		return;
	auto *pList = static_cast<WIRect*>(m_hList.get());
	auto *wMenu = GetRootWindow();
	auto *wList = pList->GetRootWindow();
	if(wList == wMenu && m_cbListWindowUpdate.IsValid())
		return;
	auto *elBase = WGUI::GetInstance().GetBaseElement(wMenu);
	if(wList != wMenu)
	{
		// We'll have to move the list to the same window as the menu
		pList->SetParentAndUpdateWindow(elBase);
	}

	if(m_cbListWindowUpdate.IsValid())
		m_cbListWindowUpdate.Remove();
	if(!elBase)
		return;
	m_cbListWindowUpdate = elBase->AddCallback("OnPreRemove",FunctionCallback<void>::Create([this]() {UpdateListWindow();}));
}

void WIDropDownMenu::OpenMenu()
{
	if(!m_hList.IsValid())
		return;
	WIRect *pList = static_cast<WIRect*>(m_hList.get());
	if(pList->IsVisible())
		return;
	SetScrollInputEnabled(true);
	UpdateListWindow();

	int y = GetHeight();
	pList->SetVisible(true);
	auto pos = GetAbsolutePos();
	pList->SetPos(pos.x,pos.y +y);
	pList->SetWidth(GetWidth());
	pList->RequestFocus();

	unsigned int numOptions = static_cast<unsigned int>(m_options.size());
	unsigned int numList = numOptions;
	if(m_numListItems < numList)
		numList = m_numListItems;
	pList->SetHeight(OPTION_HEIGHT *numList);

	auto *elBase = WGUI::GetInstance().GetBaseElement();
	// If menu bounds exceed screen bounds, put it on top of the drop down
	// field instead
	if(elBase && pos.y +y +pList->GetHeight() >= elBase->GetHeight())
		pList->SetY(pos.y -pList->GetHeight());

	int marginRight = 0;
	if(m_hScrollBar.IsValid())
	{
		WIScrollBar *pScrollBar = static_cast<WIScrollBar*>(m_hScrollBar.get());
		pScrollBar->SetHeight(pList->GetHeight());
		pScrollBar->SetX(pList->GetWidth() -pScrollBar->GetWidth());
		if(pScrollBar->IsVisible())
			marginRight = pScrollBar->GetWidth();
	}
	auto text = GetText();
	int w = GetWidth() -marginRight;
	for(unsigned int i=0;i<numOptions;i++)
	{
		WIHandle &hOption = m_options[i];
		if(hOption.IsValid())
		{
			WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption*>(hOption.get());
			pOption->SetWidth(w);
		}
	}
	ScrollToOption(m_selected,true);
	CallCallbacks("OnMenuOpened");
}
void WIDropDownMenu::CloseMenu()
{
	SetScrollInputEnabled(false);
	if(m_hList.IsValid())
	{
		WIRect *pList = static_cast<WIRect*>(m_hList.get());
		if(pList->IsVisible())
			pList->SetVisible(false);
	}
	CallCallbacks("OnMenuClosed");
}
bool WIDropDownMenu::IsMenuOpen()
{
	if(!m_hList.IsValid())
		return false;
	return m_hList->IsVisible();
}
void WIDropDownMenu::ToggleMenu()
{
	if(IsMenuOpen()) CloseMenu();
	else OpenMenu();
}
util::EventReply WIDropDownMenu::MouseCallback(GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	if(WITextEntry::MouseCallback(button,state,mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(button == GLFW::MouseButton::Left && state == GLFW::KeyState::Press)
	{
		if(!IsMenuOpen())
		{
			if(!HasFocus())
				OpenMenu();
		}
	}
	return util::EventReply::Handled;
}
util::EventReply WIDropDownMenu::ScrollCallback(Vector2 offset)
{
	if(WITextEntry::ScrollCallback(offset) == util::EventReply::Handled || !m_hScrollBar.IsValid())
		return util::EventReply::Handled;
	static_cast<WIScrollBar*>(m_hScrollBar.get())->ScrollCallback(offset);
	return util::EventReply::Handled;
}
void WIDropDownMenu::SetSize(int x,int y)
{
	WITextEntry::SetSize(x,y);
	if(m_hOutline.IsValid())
	{
		WIOutlinedRect *pOutline = static_cast<WIOutlinedRect*>(m_hOutline.get());
		pOutline->SetSize(x,y);
	}
	/*if(m_hArrow.IsValid())
	{
		auto *pArrow = m_hArrow.get();
		pArrow->SetPos(x -pArrow->GetWidth() -static_cast<int>(MARGIN),static_cast<int>(static_cast<float>(y) *0.55f -MARGIN));
	}*/
	UpdateText();
}

//////////////////////////////

WIDropDownMenuOption::WIDropDownMenuOption()
	: WIBase(),m_index(-1)
{
	RegisterCallback<void,bool>("OnSelectionChanged");
}

void WIDropDownMenuOption::SetValue(const std::string &val) {m_value = val;}
const std::string &WIDropDownMenuOption::GetValue() {return m_value;}

int WIDropDownMenuOption::GetIndex() {return m_index;}
void WIDropDownMenuOption::SetIndex(int idx) {m_index = idx;}

WIDropDownMenuOption::~WIDropDownMenuOption()
{}

void WIDropDownMenuOption::SetDropDownMenu(WIDropDownMenu *menu) {m_dropDownMenu = menu->GetHandle();}
bool WIDropDownMenuOption::IsSelected() const {return m_selected;}
WIDropDownMenu *WIDropDownMenuOption::GetDropDownMenu()
{
	if(!m_dropDownMenu.IsValid())
		return NULL;
	return static_cast<WIDropDownMenu*>(m_dropDownMenu.get());
}

void WIDropDownMenuOption::Initialize()
{
	WIBase::Initialize();
	m_hBackground = CreateChild<WIRect>();
	WIRect *pBackground = static_cast<WIRect*>(m_hBackground.get());
	pBackground->SetColor(COLOR_SELECTED);
	pBackground->SetVisible(false);

	m_hText = CreateChild<WIText>();

	SetMouseInputEnabled(true);
	SetScrollInputEnabled(true);
}

void WIDropDownMenuOption::SetText(const std::string_view &text)
{
	if(!m_hText.IsValid())
		return;
	WIText *pText = static_cast<WIText*>(m_hText.get());
	pText->SetText(text);
	pText->SetX(MARGIN_LEFT);
	pText->SizeToContents();
	UpdateTextPos();
}

WIText *WIDropDownMenuOption::GetTextElement() {return static_cast<WIText*>(m_hText.get());}

std::string_view WIDropDownMenuOption::GetText() const
{
	if(!m_hText.IsValid())
		return {};
	return static_cast<const WIText*>(m_hText.get())->GetText();
}

void WIDropDownMenuOption::UpdateTextPos()
{
	if(!m_hText.IsValid())
		return;
	m_hText.get()->SetY(static_cast<int>(static_cast<float>(GetHeight()) *0.5f -static_cast<float>(m_hText.get()->GetHeight()) *0.5f));
}
void WIDropDownMenuOption::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	if(m_hBackground.IsValid())
	{
		WIRect *pBackground = static_cast<WIRect*>(m_hBackground.get());
		pBackground->SetSize(x,y);
	}
	UpdateTextPos();
}
void WIDropDownMenuOption::OnCursorEntered()
{
	WIBase::OnCursorEntered();
	if(m_hBackground.IsValid())
		m_hBackground->SetVisible(true);
	m_selected = true;
	CallCallbacks<void,bool>("OnSelectionChanged",true);
}
void WIDropDownMenuOption::OnCursorExited()
{
	WIBase::OnCursorExited();
	if(m_hBackground.IsValid())
		m_hBackground->SetVisible(false);
	m_selected = false;
	CallCallbacks<void,bool>("OnSelectionChanged",false);
}
void WIDropDownMenuOption::OnVisibilityChanged(bool bVisible)
{
	WIBase::OnVisibilityChanged(bVisible);
	if(m_selected)
		OnCursorExited();
}
