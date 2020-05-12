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

#pragma optimize("",off)
WIDropDownMenu::WIDropDownMenu()
	: WITextEntry(),m_numListItems(5),m_listOffset(0),
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

void WIDropDownMenu::OnTextChanged(const std::string &text,bool changedByUser)
{
	WITextEntry::OnTextChanged(text,changedByUser);
	if(changedByUser)
		m_selected = -1;
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
	WIOutlinedRect *pOutline = m_hOutline.get<WIOutlinedRect>();
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
	auto *pArrow = m_hArrow.get<WIArrow>();
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
		WIDropDownMenu *t = hThis.get<WIDropDownMenu>();
		t->CloseMenu();
	},this->GetHandle())));

	WIScrollBar *pScrollBar = WGUI::GetInstance().Create<WIScrollBar>(pList);
	m_hScrollBar = pScrollBar->GetHandle();
	pScrollBar->SetWidth(8);
	pScrollBar->AddCallback("OnScrollOffsetChanged",FunctionCallback<void,unsigned int>::Create(std::bind([](WIHandle hThis,unsigned int offset) {
		if(!hThis.IsValid())
			return;
		WIDropDownMenu *t = hThis.get<WIDropDownMenu>();
		t->SetOptionOffset(offset);
	},this->GetHandle(),std::placeholders::_1)));

	SetMouseInputEnabled(true);
}

void WIDropDownMenu::SelectOption(unsigned int idx)
{
	if(idx >= m_options.size())
		return;
	WIHandle &hOption = m_options[idx];
	if(!hOption.IsValid())
		return;
	WIDropDownMenuOption *pOption = hOption.get<WIDropDownMenuOption>();
	m_selected = idx;
	SetText(pOption->GetText());
	CallCallbacks<void,unsigned int>("OnOptionSelected",idx);
	CallCallbacks<void>("OnValueChanged");
}

void WIDropDownMenu::SelectOption(const std::string &value)
{
	for(auto it=m_options.begin();it!=m_options.end();it++)
	{
		if(it->IsValid())
		{
			auto *pOption = it->get<WIDropDownMenuOption>();
			if(pOption->GetValue() == value)
			{
				SelectOption(pOption->GetIndex());
				break;
			}
		}
	}
}

void WIDropDownMenu::SelectOptionByText(const std::string &name)
{
	auto it = std::find_if(m_options.begin(),m_options.end(),[&name](const WIHandle &hOption) {
		return (hOption.IsValid() && static_cast<WIDropDownMenuOption*>(hOption.get())->GetText() == name) ? true : false;
	});
	if(it == m_options.end())
		return;
	SelectOption(static_cast<WIDropDownMenuOption*>(it->get())->GetIndex());
}

std::string_view WIDropDownMenu::GetOptionText(uint32_t idx)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return {};
	WIDropDownMenuOption *pOption = m_options[idx].get<WIDropDownMenuOption>();
	return pOption->GetText();
}

std::string WIDropDownMenu::GetOptionValue(uint32_t idx)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return "";
	WIDropDownMenuOption *pOption = m_options[idx].get<WIDropDownMenuOption>();
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
	WIDropDownMenuOption *pOption = m_options[idx].get<WIDropDownMenuOption>();
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
	pOption->AddCallback("OnScroll",FunctionCallback<util::EventReply,Vector2>::CreateWithOptionalReturn([hMenu](util::EventReply *reply,Vector2 offset) -> CallbackReturnType {
		if(!hMenu.IsValid())
		{
			*reply = util::EventReply::Handled;
			return CallbackReturnType::HasReturnValue;
		}
		WIDropDownMenu *dm = hMenu.get<WIDropDownMenu>();
		dm->InjectScrollInput(offset);
		*reply = util::EventReply::Handled;
		return CallbackReturnType::HasReturnValue;
	}));
	pOption->AddCallback("OnMouseEvent",FunctionCallback<util::EventReply,GLFW::MouseButton,GLFW::KeyState,GLFW::Modifier>::CreateWithOptionalReturn(
		[hOption](util::EventReply *reply,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier) -> CallbackReturnType {
		if(!hOption.IsValid())
		{
			*reply = util::EventReply::Handled;
			return CallbackReturnType::HasReturnValue;
		}
		WIDropDownMenuOption *pOption = hOption.get<WIDropDownMenuOption>();
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
		m_hScrollBar.get<WIScrollBar>()->SetUp(m_numListItems,static_cast<unsigned int>(m_options.size()));
	return pOption;
}

WIDropDownMenuOption *WIDropDownMenu::AddOption(const std::string &option)
{
	auto idx = m_options.size();
	return AddOption(option,std::to_string(idx));
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
			WIDropDownMenuOption *pOption = hOption.get<WIDropDownMenuOption>();
			pOption->SetVisible(true);
			pOption->SetY(y);

			y += OPTION_HEIGHT;
		}
	}
	m_listOffset = offset;
}

void WIDropDownMenu::OpenMenu()
{
	if(!m_hList.IsValid())
		return;
	WIRect *pList = m_hList.get<WIRect>();
	if(pList->IsVisible())
		return;
	SetScrollInputEnabled(true);
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

	int marginRight = 0;
	if(m_hScrollBar.IsValid())
	{
		WIScrollBar *pScrollBar = m_hScrollBar.get<WIScrollBar>();
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
			WIDropDownMenuOption *pOption = hOption.get<WIDropDownMenuOption>();
			pOption->SetWidth(w);
		}
	}
	unsigned int idxSelected = std::max<int>(std::min<int>(
		m_selected -static_cast<int>(ceilf(float(m_numListItems) /2.f)) +1,
		static_cast<int>(numOptions) -static_cast<int>(m_numListItems)
	),0);
	if(m_hScrollBar.IsValid())
	{
		WIScrollBar *pScrollBar = m_hScrollBar.get<WIScrollBar>();
		pScrollBar->SetScrollOffset(idxSelected);
		SetOptionOffset(idxSelected);
	}
	CallCallbacks("OnMenuOpened");
}
void WIDropDownMenu::CloseMenu()
{
	SetScrollInputEnabled(false);
	if(m_hList.IsValid())
	{
		WIRect *pList = m_hList.get<WIRect>();
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
	m_hScrollBar.get<WIScrollBar>()->ScrollCallback(offset);
	return util::EventReply::Handled;
}
void WIDropDownMenu::SetSize(int x,int y)
{
	WITextEntry::SetSize(x,y);
	if(m_hOutline.IsValid())
	{
		WIOutlinedRect *pOutline = m_hOutline.get<WIOutlinedRect>();
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
{}

void WIDropDownMenuOption::SetValue(const std::string &val) {m_value = val;}
const std::string &WIDropDownMenuOption::GetValue() {return m_value;}

int WIDropDownMenuOption::GetIndex() {return m_index;}
void WIDropDownMenuOption::SetIndex(int idx) {m_index = idx;}

WIDropDownMenuOption::~WIDropDownMenuOption()
{}

void WIDropDownMenuOption::SetDropDownMenu(WIDropDownMenu *menu) {m_dropDownMenu = menu->GetHandle();}
WIDropDownMenu *WIDropDownMenuOption::GetDropDownMenu()
{
	if(!m_dropDownMenu.IsValid())
		return NULL;
	return m_dropDownMenu.get<WIDropDownMenu>();
}

void WIDropDownMenuOption::Initialize()
{
	WIBase::Initialize();
	m_hBackground = CreateChild<WIRect>();
	WIRect *pBackground = m_hBackground.get<WIRect>();
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
	WIText *pText = m_hText.get<WIText>();
	pText->SetText(text);
	pText->SetX(MARGIN_LEFT);
	pText->SizeToContents();
	UpdateTextPos();
}

std::string_view WIDropDownMenuOption::GetText() const
{
	if(!m_hText.IsValid())
		return {};
	return static_cast<WIText*>(m_hText.get())->GetText();
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
		WIRect *pBackground = m_hBackground.get<WIRect>();
		pBackground->SetSize(x,y);
	}
	UpdateTextPos();
}
void WIDropDownMenuOption::OnCursorEntered()
{
	WIBase::OnCursorEntered();
	if(m_hBackground.IsValid())
		m_hBackground->SetVisible(true);
}
void WIDropDownMenuOption::OnCursorExited()
{
	WIBase::OnCursorExited();
	if(m_hBackground.IsValid())
		m_hBackground->SetVisible(false);
}
#pragma optimize("",on)
