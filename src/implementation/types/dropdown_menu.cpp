// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.dropdown_menu;
import :types.text;

import pragma.string.unicode;

pragma::gui::types::WIDropDownMenu::WIDropDownMenu() : WITextEntry(), m_numListItems(15), m_listOffset(0), m_selected(-1)
{
	RegisterCallback<void, unsigned int>("OnOptionSelected");
	RegisterCallback<void>("OnValueChanged");
	RegisterCallback<void>("OnMenuOpened");
	RegisterCallback<void>("OnMenuClosed");
}

pragma::gui::types::WIDropDownMenu::~WIDropDownMenu()
{
	if(m_hList.IsValid())
		m_hList->Remove();
}

void pragma::gui::types::WIDropDownMenu::SetListItemCount(uint32_t n) { m_numListItems = n; }

void pragma::gui::types::WIDropDownMenu::OnTextChanged(const string::Utf8String &text, bool changedByUser)
{
	WITextEntry::OnTextChanged(text, changedByUser);
	if(changedByUser)
		ClearSelectedOption();
}

void pragma::gui::types::WIDropDownMenu::ClearSelectedOption()
{
	if(m_selected == -1)
		return;
	m_selected = -1;
	CallCallbacks<void, unsigned int>("OnOptionSelected", std::numeric_limits<uint32_t>::max());
}

void pragma::gui::types::WIDropDownMenu::Initialize()
{
	WITextEntry::Initialize();
	SetEditable(false);

	if(m_hBase.IsValid() == true) {
		auto hThis = GetHandle();
		m_hBase->AddCallback("OnMousePressed", FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this, hThis](util::EventReply *reply) -> CallbackReturnType {
			*reply = util::EventReply::Handled;
			if(hThis.IsValid() == false)
				return CallbackReturnType::HasReturnValue;
			if(IsEditable())
				return CallbackReturnType::NoReturnValue; // Let text entry handle mouse input
			ToggleMenu();
			return CallbackReturnType::HasReturnValue;
		}));
	}
	AddCallback("OnTextEntered", FunctionCallback<void>::Create([this]() { CallCallbacks<void, unsigned int>("OnOptionSelected", std::numeric_limits<uint32_t>::max()); }));

	//m_hText = CreateChild<WIText>();
	//WIText *pText = m_hText.get<WIText>();
	//pText->SetX(static_cast<int>(MARGIN));

	auto *pList = WGUI::GetInstance().Create<VBox>();
	m_hList = pList->GetHandle();
	pList->AddStyleClass("dropdown_menu_list");
	pList->SetVisible(false);
	pList->SetZPos(10'000);
	pList->AddCallback("OnFocusKilled",
	  FunctionCallback<>::Create(std::bind(
	    [](WIHandle hThis) {
		    if(!hThis.IsValid())
			    return;
		    WIDropDownMenu *t = static_cast<WIDropDownMenu *>(hThis.get());
		    t->CloseMenu();
	    },
	    this->GetHandle())));

	WIScrollBar *pScrollBar = WGUI::GetInstance().Create<WIScrollBar>(pList);
	m_hScrollBar = pScrollBar->GetHandle();
	pList->SetBackgroundElement(true);
	pScrollBar->SetWidth(8);
	pScrollBar->AddCallback("OnScrollOffsetChanged",
	  FunctionCallback<void, unsigned int>::Create(std::bind(
	    [](WIHandle hThis, unsigned int offset) {
		    if(!hThis.IsValid())
			    return;
		    WIDropDownMenu *t = static_cast<WIDropDownMenu *>(hThis.get());
		    t->SetOptionOffset(offset);
	    },
	    this->GetHandle(), std::placeholders::_1)));

	SetMouseInputEnabled(true);
	UpdateListWindow();
}

void pragma::gui::types::WIDropDownMenu::OnRemove()
{
	WITextEntry::OnRemove();
	if(m_cbListWindowUpdate.IsValid())
		m_cbListWindowUpdate.Remove();
}

void pragma::gui::types::WIDropDownMenu::DoUpdate()
{
	WITextEntry::DoUpdate();
	UpdateOptionItems({});
}

void pragma::gui::types::WIDropDownMenu::SelectOption(unsigned int idx)
{
	if(idx >= m_options.size() || idx == m_selected)
		return;
	WIHandle &hOption = m_options[idx];
	if(!hOption.IsValid())
		return;
	WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption *>(hOption.get());
	m_selected = idx;
	SetText(pOption->GetText());
	CallCallbacks<void, unsigned int>("OnOptionSelected", idx);
	CallCallbacks<void>("OnValueChanged");
}

const pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::FindOptionByValue(const std::string &value) const { return const_cast<WIDropDownMenu *>(this)->FindOptionByValue(value); }
pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::FindOptionByValue(const std::string &value)
{
	for(auto it = m_options.begin(); it != m_options.end(); it++) {
		if(it->IsValid()) {
			auto *pOption = static_cast<WIDropDownMenuOption *>(it->get());
			if(pOption->GetValue() == value)
				return pOption;
		}
	}
	return nullptr;
}
bool pragma::gui::types::WIDropDownMenu::HasOption(const std::string &value) const { return FindOptionByValue(value) != nullptr; }

void pragma::gui::types::WIDropDownMenu::SelectOption(const std::string &value)
{
	auto *option = FindOptionByValue(value);
	if(option == nullptr)
		return;
	SelectOption(option->GetIndex());
}

void pragma::gui::types::WIDropDownMenu::SelectOptionByText(const string::Utf8StringArg &name)
{
	auto it = std::find_if(m_options.begin(), m_options.end(), [&name](const WIHandle &hOption) { return (hOption.IsValid() && static_cast<const WIDropDownMenuOption *>(hOption.get())->GetText() == *name) ? true : false; });
	if(it == m_options.end())
		return;
	SelectOption(static_cast<WIDropDownMenuOption *>(it->get())->GetIndex());
}

pragma::string::Utf8StringView pragma::gui::types::WIDropDownMenu::GetOptionText(uint32_t idx)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return {};
	WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption *>(m_options[idx].get());
	return pOption->GetText();
}

std::string pragma::gui::types::WIDropDownMenu::GetOptionValue(uint32_t idx)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return "";
	WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption *>(m_options[idx].get());
	return pOption->GetValue();
}
void pragma::gui::types::WIDropDownMenu::SetOptionText(uint32_t idx, const std::string &text)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return;
	static_cast<WIDropDownMenuOption *>(m_options[idx].get())->SetText(text);
}
void pragma::gui::types::WIDropDownMenu::SetOptionValue(uint32_t idx, const std::string &val)
{
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return;
	static_cast<WIDropDownMenuOption *>(m_options[idx].get())->SetValue(val);
}

pragma::string::Utf8StringView pragma::gui::types::WIDropDownMenu::GetText() const { return WITextEntry::GetText(); }

std::string pragma::gui::types::WIDropDownMenu::GetValue()
{
	auto idx = m_selected;
	if(idx >= m_options.size() || !m_options[idx].IsValid())
		return IsEditable() ? GetText().cpp_str() : "";
	WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption *>(m_options[idx].get());
	return pOption->GetValue();
}

int32_t pragma::gui::types::WIDropDownMenu::GetSelectedOption() const { return m_selected; }

void pragma::gui::types::WIDropDownMenu::SetText(const string::Utf8StringArg &text)
{
	WITextEntry::SetText(text);
	//SetText(text);
	//SizeToContents();
	UpdateText();
}
void pragma::gui::types::WIDropDownMenu::UpdateText()
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
unsigned int pragma::gui::types::WIDropDownMenu::GetOptionCount() { return static_cast<unsigned int>(m_options.size()); }

void pragma::gui::types::WIDropDownMenu::UpdateTextPos()
{
	//SetY(static_cast<int>(static_cast<float>(GetHeight()) *0.5f -static_cast<float>(GetHeight()) *0.5f));
}

void pragma::gui::types::WIDropDownMenu::OnOptionSelected(WIDropDownMenuOption *option) { SelectOption(option->GetIndex()); }

pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::AddTextOption(const DisplayText &text, const std::string &value)
{
	if(!m_hList.IsValid())
		return nullptr;
	WIDropDownMenuOption *pOption = WGUI::GetInstance().Create<WIDropDownMenuOption>(m_hList.get());
	WIHandle hOption = pOption->GetHandle();
	pOption->SetDropDownMenu(this);
	std::visit([pOption](auto &&text) { pOption->SetText(text); }, text);
	pOption->SetIndex(static_cast<int>(m_options.size()));
	pOption->SetValue(value);
	pOption->SetVisible(false);
	auto hMenu = GetHandle();
	pOption->AddCallback("OnScroll", FunctionCallback<util::EventReply, Vector2>::CreateWithOptionalReturn([hMenu](util::EventReply *reply, Vector2 offset) mutable -> CallbackReturnType {
		if(!hMenu.IsValid()) {
			*reply = util::EventReply::Handled;
			return CallbackReturnType::HasReturnValue;
		}
		WIDropDownMenu *dm = static_cast<WIDropDownMenu *>(hMenu.get());
		dm->InjectScrollInput(offset);
		*reply = util::EventReply::Handled;
		return CallbackReturnType::HasReturnValue;
	}));
	pOption->AddCallback("OnMouseEvent",
	  FunctionCallback<util::EventReply, platform::MouseButton, platform::KeyState, platform::Modifier>::CreateWithOptionalReturn([hOption](util::EventReply *reply, platform::MouseButton button, platform::KeyState state, platform::Modifier) mutable -> CallbackReturnType {
		  if(!hOption.IsValid()) {
			  *reply = util::EventReply::Handled;
			  return CallbackReturnType::HasReturnValue;
		  }
		  WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption *>(hOption.get());
		  if(button == platform::MouseButton::Left && state == platform::KeyState::Press) {
			  WIDropDownMenu *dm = pOption->GetDropDownMenu();
			  if(dm != nullptr) {
				  dm->OnOptionSelected(pOption);
				  dm->CloseMenu();
			  }
		  }
		  *reply = util::EventReply::Handled;
		  return CallbackReturnType::HasReturnValue;
	  }));
	m_options.push_back(hOption);
	if(m_hScrollBar.IsValid())
		static_cast<WIScrollBar *>(m_hScrollBar.get())->SetUp(m_numListItems, static_cast<unsigned int>(m_options.size()));
	m_hList->ScheduleUpdate();
	ScheduleUpdate();
	return pOption;
}
pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::AddOption(const string::Utf8StringArg &option, const std::string &value) { return AddTextOption(DisplayText {option->to_str()}, value); }

pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::AddOption(const string::Utf8StringArg &option)
{
	auto idx = m_options.size();
	return AddOption(option, util::to_string(idx));
}

pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::AddOption(const LocalizedString &str, const std::string &value) { return AddTextOption(DisplayText {str}, value); }
pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::AddOption(const LocalizedString &str)
{
	auto idx = m_options.size();
	return AddOption(str, util::to_string(idx));
}

pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::GetOptionElement(uint32_t idx)
{
	if(idx >= m_options.size())
		return nullptr;
	return static_cast<WIDropDownMenuOption *>(m_options.at(idx).get());
}

pragma::gui::types::WIDropDownMenuOption *pragma::gui::types::WIDropDownMenu::FindOptionSelectedByCursor()
{
	for(auto &hOpt : m_options) {
		if(hOpt.IsValid() == false || static_cast<WIDropDownMenuOption *>(hOpt.get())->IsSelected() == false)
			continue;
		return static_cast<WIDropDownMenuOption *>(hOpt.get());
	}
	return nullptr;
}

void pragma::gui::types::WIDropDownMenu::ClearOptions()
{
	for(auto it = m_options.begin(); it != m_options.end(); it++) {
		auto &hOption = *it;
		if(hOption.IsValid())
			hOption->Remove();
	}
	m_options.clear();
}

void pragma::gui::types::WIDropDownMenu::SetOptions(const std::vector<std::string> &options)
{
	ClearOptions();
	for(auto it = options.begin(); it != options.end(); it++) {
		auto &option = *it;
		AddOption(option);
	}
}

void pragma::gui::types::WIDropDownMenu::SetOptions(const std::unordered_map<std::string, std::string> &options)
{
	ClearOptions();
	for(auto it = options.begin(); it != options.end(); ++it)
		AddOption(it->first, it->second);
}

void pragma::gui::types::WIDropDownMenu::UpdateOptionItems(std::optional<uint32_t> oldOffset)
{
	int numOptions = static_cast<int>(m_options.size());
	if(oldOffset.has_value()) {
		unsigned int curOffset = *oldOffset;
		int numList = curOffset + m_numListItems;
		if(numOptions < numList)
			numList = numOptions;
		for(int i = curOffset; i < numList; i++) {
			WIHandle &hOption = m_options[i];
			if(hOption.IsValid())
				hOption->SetVisible(false);
		}
	}

	int y = 0;
	auto numList = m_listOffset + m_numListItems;
	if(numOptions < numList)
		numList = numOptions;
	for(int i = m_listOffset; i < numList; i++) {
		WIHandle &hOption = m_options[i];
		if(hOption.IsValid()) {
			WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption *>(hOption.get());
			pOption->SetVisible(true);
		}
	}
}

void pragma::gui::types::WIDropDownMenu::SetOptionOffset(unsigned int offset)
{
	if(offset >= m_options.size())
		return;
	auto oldOffset = m_listOffset;
	m_listOffset = offset;
	UpdateOptionItems(oldOffset);
}

void pragma::gui::types::WIDropDownMenu::ScrollToOption(uint32_t offset, bool center)
{
	if(offset >= m_options.size())
		return;
	if(center) {
		unsigned int numOptions = static_cast<unsigned int>(m_options.size());
		offset = std::max<int>(std::min<int>(offset - static_cast<int>(ceilf(float(m_numListItems) / 2.f)) + 1, static_cast<int>(numOptions) - static_cast<int>(m_numListItems)), 0);
	}
	if(m_hScrollBar.IsValid()) {
		WIScrollBar *pScrollBar = static_cast<WIScrollBar *>(m_hScrollBar.get());
		pScrollBar->SetScrollOffset(offset);
		SetOptionOffset(offset);
	}
}

void pragma::gui::types::WIDropDownMenu::UpdateListWindow()
{
	if(!m_hList.IsValid())
		return;
	auto *pList = m_hList.get();
	auto *wMenu = GetRootWindow();
	auto *wList = pList->GetRootWindow();
	if(wList == wMenu && m_cbListWindowUpdate.IsValid())
		return;
	auto *elBase = WGUI::GetInstance().GetBaseElement(wMenu);
	if(wList != wMenu) {
		// We'll have to move the list to the same window as the menu
		pList->SetParentAndUpdateWindow(elBase);
	}

	if(m_cbListWindowUpdate.IsValid())
		m_cbListWindowUpdate.Remove();
	if(!elBase)
		return;
	m_cbListWindowUpdate = elBase->AddCallback("OnPreRemove", FunctionCallback<void>::Create([this]() { UpdateListWindow(); }));
}

void pragma::gui::types::WIDropDownMenu::OpenMenu()
{
	if(IsMenuOpen())
		return;
	if(!m_hList.IsValid())
		return;
	auto *pList = m_hList.get();
	auto *elRoot = GetRootElement();
	if(elRoot)
		pList->SetParent(elRoot);
	if(pList->IsVisible())
		return;
	SetScrollInputEnabled(true);
	UpdateListWindow();

	int y = GetHeight();
	pList->SetVisible(true);
	auto pos = GetAbsolutePos();
	pList->SetPos(pos.x, pos.y + y);
	pList->SetWidth(GetWidth());
	pList->RequestFocus();

	auto *elBase = WGUI::GetInstance().GetBaseElement();
	// If menu bounds exceed screen bounds, put it on top of the drop down
	// field instead
	if(elBase && pos.y + y + pList->GetHeight() >= elBase->GetHeight())
		pList->SetY(pos.y - pList->GetHeight());

	int marginRight = 0;
	if(m_hScrollBar.IsValid()) {
		WIScrollBar *pScrollBar = static_cast<WIScrollBar *>(m_hScrollBar.get());
		pScrollBar->SetHeight(pList->GetHeight());
		pScrollBar->SetX(pList->GetWidth() - pScrollBar->GetWidth());
		if(pScrollBar->IsVisible())
			marginRight = pScrollBar->GetWidth();
	}
	auto text = GetText();
	int w = GetWidth() - marginRight;
	unsigned int numOptions = static_cast<unsigned int>(m_options.size());
	for(unsigned int i = 0; i < numOptions; i++) {
		WIHandle &hOption = m_options[i];
		if(hOption.IsValid()) {
			WIDropDownMenuOption *pOption = static_cast<WIDropDownMenuOption *>(hOption.get());
			pOption->SetWidth(w);
		}
	}
	ScrollToOption(m_selected, true);
	CallCallbacks("OnMenuOpened");
}
void pragma::gui::types::WIDropDownMenu::CloseMenu()
{
	if(!IsMenuOpen())
		return;
	SetScrollInputEnabled(false);
	if(m_hList.IsValid()) {
		auto *pList = m_hList.get();
		if(pList->IsVisible())
			pList->SetVisible(false);
	}
	CallCallbacks("OnMenuClosed");
}
bool pragma::gui::types::WIDropDownMenu::IsMenuOpen()
{
	if(!m_hList.IsValid())
		return false;
	return m_hList->IsVisible();
}
void pragma::gui::types::WIDropDownMenu::ToggleMenu()
{
	if(IsMenuOpen())
		CloseMenu();
	else
		OpenMenu();
}
pragma::util::EventReply pragma::gui::types::WIDropDownMenu::MouseCallback(platform::MouseButton button, platform::KeyState state, platform::Modifier mods)
{
	if(WITextEntry::MouseCallback(button, state, mods) == util::EventReply::Handled)
		return util::EventReply::Handled;
	if(button == platform::MouseButton::Left && state == platform::KeyState::Press) {
		if(!IsMenuOpen()) {
			if(!HasFocus())
				OpenMenu();
		}
	}
	return util::EventReply::Handled;
}
pragma::util::EventReply pragma::gui::types::WIDropDownMenu::ScrollCallback(Vector2 offset, bool offsetAsPixels)
{
	if(WITextEntry::ScrollCallback(offset, offsetAsPixels) == util::EventReply::Handled || !m_hScrollBar.IsValid())
		return util::EventReply::Handled;
	static_cast<WIScrollBar *>(m_hScrollBar.get())->ScrollCallback(offset, offsetAsPixels);
	return util::EventReply::Handled;
}
void pragma::gui::types::WIDropDownMenu::OnSizeChanged(const Vector2i &oldSize, ChangeSource changeSource)
{
	/*if(m_hArrow.IsValid())
	{
		auto *pArrow = m_hArrow.get();
		pArrow->SetPos(x -pArrow->GetWidth() -static_cast<int>(MARGIN),static_cast<int>(static_cast<float>(y) *0.55f -MARGIN));
	}*/
	UpdateText();
}

//////////////////////////////

pragma::gui::types::WIDropDownMenuOption::WIDropDownMenuOption() : WIBase(), m_index(-1)
{
	AddStyleClass("dropdown_menu_option");
	RegisterCallback<void, bool>("OnSelectionChanged");
}

void pragma::gui::types::WIDropDownMenuOption::SetValue(const std::string &val) { m_value = val; }
const std::string &pragma::gui::types::WIDropDownMenuOption::GetValue() { return m_value; }

int pragma::gui::types::WIDropDownMenuOption::GetIndex() { return m_index; }
void pragma::gui::types::WIDropDownMenuOption::SetIndex(int idx) { m_index = idx; }

pragma::gui::types::WIDropDownMenuOption::~WIDropDownMenuOption() {}

void pragma::gui::types::WIDropDownMenuOption::SetDropDownMenu(WIDropDownMenu *menu) { m_dropDownMenu = menu->GetHandle(); }
bool pragma::gui::types::WIDropDownMenuOption::IsSelected() const { return m_selected; }
pragma::gui::types::WIDropDownMenu *pragma::gui::types::WIDropDownMenuOption::GetDropDownMenu()
{
	if(!m_dropDownMenu.IsValid())
		return nullptr;
	return static_cast<WIDropDownMenu *>(m_dropDownMenu.get());
}

void pragma::gui::types::WIDropDownMenuOption::Initialize()
{
	WIBase::Initialize();

	m_hText = CreateChild<WIText>();

	SetMouseInputEnabled(true);
	SetScrollInputEnabled(true);
}

void pragma::gui::types::WIDropDownMenuOption::SetText(const string::Utf8StringArg &text)
{
	if(!m_hText.IsValid())
		return;
	WIText *pText = static_cast<WIText *>(m_hText.get());
	pText->SetText(text);
	UpdateTextPos();
}

void pragma::gui::types::WIDropDownMenuOption::SetText(const LocalizedString &str)
{
	if(!m_hText.IsValid())
		return;
	WIText *pText = static_cast<WIText *>(m_hText.get());
	pText->SetText(str);
	UpdateTextPos();
}
const pragma::gui::LocalizedString *pragma::gui::types::WIDropDownMenuOption::GetLocaleText() const
{
	if(!m_hText.IsValid())
		return nullptr;
	return &static_cast<const WIText *>(m_hText.get())->GetLocaleText();
}

pragma::gui::types::WIText *pragma::gui::types::WIDropDownMenuOption::GetTextElement() { return static_cast<WIText *>(m_hText.get()); }

pragma::string::Utf8StringView pragma::gui::types::WIDropDownMenuOption::GetText() const
{
	if(!m_hText.IsValid())
		return {};
	return static_cast<const WIText *>(m_hText.get())->GetText();
}

void pragma::gui::types::WIDropDownMenuOption::UpdateTextPos()
{
	if(!m_hText.IsValid())
		return;
	m_hText.get()->SetY(static_cast<int>(static_cast<float>(GetHeight()) * 0.5f - static_cast<float>(m_hText.get()->GetHeight()) * 0.5f));
}
void pragma::gui::types::WIDropDownMenuOption::OnSizeChanged(const Vector2i &oldSize, ChangeSource changeSource) { UpdateTextPos(); }
void pragma::gui::types::WIDropDownMenuOption::OnCursorEntered()
{
	WIBase::OnCursorEntered();
	m_selected = true;
	CallCallbacks<void, bool>("OnSelectionChanged", true);
}
void pragma::gui::types::WIDropDownMenuOption::OnCursorExited()
{
	WIBase::OnCursorExited();
	m_selected = false;
	CallCallbacks<void, bool>("OnSelectionChanged", false);
}
void pragma::gui::types::WIDropDownMenuOption::OnVisibilityChanged(bool bVisible)
{
	WIBase::OnVisibilityChanged(bVisible);
	if(m_selected)
		OnCursorExited();
}
