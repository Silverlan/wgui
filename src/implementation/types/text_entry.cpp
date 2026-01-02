// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.text_entry;

import pragma.string.unicode;

pragma::gui::types::WITextEntry::WITextEntry() : WIBase()
{
	RegisterCallback<void>("OnTextEntered");
	RegisterCallback<void, std::reference_wrapper<const string::Utf8String>, bool>("OnTextChanged");
	RegisterCallback<void>("OnContentsChanged");
}

pragma::gui::types::WITextEntry::~WITextEntry() {}

void pragma::gui::types::WITextEntry::SetInputHidden(bool b)
{
	if(!m_hBase.IsValid())
		return;
	static_cast<WITextEntryBase *>(m_hBase.get())->SetInputHidden(b);
}

pragma::gui::types::WIText *pragma::gui::types::WITextEntry::GetTextElement()
{
	if(m_hBase.IsValid() == false)
		return nullptr;
	return static_cast<WITextEntryBase *>(m_hBase.get())->GetTextElement();
}

void pragma::gui::types::WITextEntry::RequestFocus()
{
	if(!m_hBase.IsValid())
		return;
	m_hBase->RequestFocus();
}

bool pragma::gui::types::WITextEntry::HasFocus()
{
	if(!m_hBase.IsValid())
		return false;
	return m_hBase->HasFocus();
}

void pragma::gui::types::WITextEntry::SetMouseInputEnabled(bool b)
{
	WIBase::SetMouseInputEnabled(b);
	if(!m_hBase.IsValid())
		return;
	m_hBase->SetMouseInputEnabled(b);
}
void pragma::gui::types::WITextEntry::SetKeyboardInputEnabled(bool b)
{
	WIBase::SetKeyboardInputEnabled(b);
	if(!m_hBase.IsValid())
		return;
	m_hBase->SetKeyboardInputEnabled(b);
}
void pragma::gui::types::WITextEntry::SizeToContents(bool x, bool y)
{
	if(m_hBase.IsValid()) {
		m_hBase->SizeToContents(x, y);
		SetSize(m_hBase->GetSize());
	}
	else
		WIBase::SizeToContents(x, y);
}
void pragma::gui::types::WITextEntry::SetColor(float r, float g, float b, float a)
{
	WIBase::SetColor(r, g, b, a);
	m_hBase->SetColor(r, g, b, a);
}
void pragma::gui::types::WITextEntry::Initialize()
{
	WIBase::Initialize();

	SetMouseInputEnabled(true);
	SetKeyboardInputEnabled(true);
	SetSize(128, 25);
	Vector2i size = GetSize();
	m_hBg = CreateChild<WIRect>();
	WIRect *pBg = static_cast<WIRect *>(m_hBg.get());
	pBg->SetName("background");
	pBg->SetColor(1, 1, 1, 1);
	pBg->SetSize(size.x, size.y);
	pBg->SetAnchor(0, 0, 1, 1);

	m_hBase = CreateChild<WITextEntryBase>();
	WITextEntryBase *pBase = static_cast<WITextEntryBase *>(m_hBase.get());
	pBase->SetEntryFieldElement(this);
	pBase->SetSize(size.x, size.y);
	pBase->AddCallback("OnTextEntered",
	  FunctionCallback<>::Create(std::bind(
	    [](WIHandle hTextEntry) {
		    if(!hTextEntry.IsValid())
			    return;
		    WITextEntry *te = static_cast<WITextEntry *>(hTextEntry.get());
		    te->OnTextEntered();
	    },
	    this->GetHandle())));
	pBase->AddCallback("OnTextChanged",
	  FunctionCallback<void, std::reference_wrapper<const string::Utf8String>, bool>::Create(std::bind(
	    [](WIHandle hTextEntry, std::reference_wrapper<const string::Utf8String> text, bool changedByUser) {
		    if(!hTextEntry.IsValid())
			    return;
		    WITextEntry *te = static_cast<WITextEntry *>(hTextEntry.get());
		    te->OnTextChanged(text, changedByUser);
	    },
	    this->GetHandle(), std::placeholders::_1, std::placeholders::_2)));
	pBase->AddCallback("OnContentsChanged",
	  FunctionCallback<void>::Create(std::bind(
	    [](WIHandle hTextEntry) {
		    if(!hTextEntry.IsValid())
			    return;
		    WITextEntry *te = static_cast<WITextEntry *>(hTextEntry.get());
		    te->OnContentsChanged();
	    },
	    this->GetHandle())));
	pBase->AddCallback("OnFocusGained",
	  FunctionCallback<>::Create(std::bind(
	    [](WIHandle hTextEntry) {
		    if(!hTextEntry.IsValid())
			    return;
		    WITextEntry *te = static_cast<WITextEntry *>(hTextEntry.get());
		    te->OnFocusGained();
	    },
	    this->GetHandle())));
	pBase->AddCallback("OnFocusKilled",
	  FunctionCallback<>::Create(std::bind(
	    [](WIHandle hTextEntry) {
		    if(!hTextEntry.IsValid())
			    return;
		    WITextEntry *te = static_cast<WITextEntry *>(hTextEntry.get());
		    te->OnFocusKilled();
	    },
	    this->GetHandle())));
	pBase->SetMouseInputEnabled(GetMouseInputEnabled());
	pBase->SetKeyboardInputEnabled(GetKeyboardInputEnabled());

	m_hOutline = CreateChild<WIOutlinedRect>();
	WIOutlinedRect *pORect = static_cast<WIOutlinedRect *>(m_hOutline.get());
	pORect->SetName("background_outline");
	pORect->SetOutlineWidth(1);
	pORect->WIBase::SetSize(size.x, size.y);
	pORect->SetZPos(10);
	pORect->SetColor(0, 0, 0, 1);
}

void pragma::gui::types::WITextEntry::OnTextEntered() { CallCallbacks<void>("OnTextEntered"); }

void pragma::gui::types::WITextEntry::OnTextChanged(const string::Utf8String &text, bool changedByUser) { CallCallbacks<void, std::reference_wrapper<const string::Utf8String>, bool>("OnTextChanged", text, changedByUser); }

void pragma::gui::types::WITextEntry::OnContentsChanged()
{
	if(m_hBase.IsValid()) {
		auto *pBase = static_cast<WITextEntryBase *>(m_hBase.get());
		auto *pText = pBase->GetTextElement();
		if(pText && pText->ShouldAutoSizeToText()) {
			//pBase->SizeToContents();
			if(pText->GetAutoBreakMode() != WIText::AutoBreak::NONE)
				SetHeight(pBase->GetHeight());
		}
	}
	CallCallbacks<void>("OnContentsChanged");
}

void pragma::gui::types::WITextEntry::SetSize(int x, int y)
{
	WIBase::SetSize(x, y);
	if(m_hBase.IsValid()) {
		auto *pBase = static_cast<WITextEntryBase *>(m_hBase.get());
		auto *pText = pBase->GetTextElement();
		if(IsMultiLine() == false) {
			auto yBase = y - 10;
			if(pText != nullptr) {
				auto *font = pText->GetFont();
				if(font != nullptr)
					yBase = math::max(static_cast<uint32_t>(yBase), font->GetSize());
			}
			pBase->SetSize(x - 10, yBase);
			pBase->SetPos(5, (y - yBase) / 2);
		}
		else {
			pBase->SetSize(x, y);
			pBase->SetPos(0, 0);
		}
	}
	if(m_hOutline.IsValid()) {
		WIOutlinedRect *pRect = static_cast<WIOutlinedRect *>(m_hOutline.get());
		pRect->SetSize(x, y);
	}
	if(m_hBg.IsValid()) {
		WIRect *pBg = static_cast<WIRect *>(m_hBg.get());
		pBg->SetSize(x, y);
	}
}

void pragma::gui::types::WITextEntry::SetMaxLength(int length)
{
	if(!m_hBase.IsValid())
		return;
	static_cast<WITextEntryBase *>(m_hBase.get())->SetMaxLength(length);
}
int pragma::gui::types::WITextEntry::GetMaxLength() const
{
	if(!m_hBase.IsValid())
		return -1;
	return static_cast<const WITextEntryBase *>(m_hBase.get())->GetMaxLength();
}

pragma::string::Utf8StringView pragma::gui::types::WITextEntry::GetText() const
{
	if(!m_hBase.IsValid())
		return {};
	return static_cast<const WITextEntryBase *>(m_hBase.get())->GetText();
}
void pragma::gui::types::WITextEntry::SetText(const string::Utf8StringArg &text)
{
	if(!m_hBase.IsValid())
		return;
	static_cast<WITextEntryBase *>(m_hBase.get())->SetText(*text);
}
void pragma::gui::types::WITextEntry::InsertText(string::Utf8StringView instext, int pos)
{
	if(!m_hBase.IsValid())
		return;
	static_cast<WITextEntryBase *>(m_hBase.get())->InsertText(instext, pos);
}
void pragma::gui::types::WITextEntry::InsertText(string::Utf8StringView text)
{
	if(!m_hBase.IsValid())
		return;
	static_cast<WITextEntryBase *>(m_hBase.get())->InsertText(text);
}
int pragma::gui::types::WITextEntry::GetCaretPos() const
{
	if(!m_hBase.IsValid())
		return 0;
	return static_cast<const WITextEntryBase *>(m_hBase.get())->GetCaretPos();
}
void pragma::gui::types::WITextEntry::SetCaretPos(int pos)
{
	if(!m_hBase.IsValid())
		return;
	static_cast<WITextEntryBase *>(m_hBase.get())->SetCaretPos(pos);
}
pragma::gui::types::WIRect *pragma::gui::types::WITextEntry::GetCaretElement() { return m_hBase.IsValid() ? static_cast<WITextEntryBase *>(m_hBase.get())->GetCaretElement() : nullptr; }
bool pragma::gui::types::WITextEntry::IsNumeric() const { return false; }
bool pragma::gui::types::WITextEntry::IsMultiLine() const
{
	if(!m_hBase.IsValid())
		return false;
	return static_cast<const WITextEntryBase *>(m_hBase.get())->IsMultiLine();
}
void pragma::gui::types::WITextEntry::SetMultiLine(bool bMultiLine)
{
	if(!m_hBase.IsValid())
		return;
	static_cast<WITextEntryBase *>(m_hBase.get())->SetMultiLine(bMultiLine);
}
bool pragma::gui::types::WITextEntry::IsEditable() const
{
	if(!m_hBase.IsValid())
		return false;
	return static_cast<const WITextEntryBase *>(m_hBase.get())->IsEditable();
}
void pragma::gui::types::WITextEntry::SetEditable(bool bEditable)
{
	if(!m_hBase.IsValid())
		return;
	return static_cast<WITextEntryBase *>(m_hBase.get())->SetEditable(bEditable);
}
bool pragma::gui::types::WITextEntry::IsSelectable() const
{
	if(!m_hBase.IsValid())
		return false;
	return static_cast<const WITextEntryBase *>(m_hBase.get())->IsSelectable();
}
void pragma::gui::types::WITextEntry::SetSelectable(bool bSelectable)
{
	if(!m_hBase.IsValid())
		return;
	return static_cast<WITextEntryBase *>(m_hBase.get())->SetSelectable(bSelectable);
}
void pragma::gui::types::WITextEntry::GetSelectionBounds(int *start, int *end) const
{
	if(!m_hBase.IsValid()) {
		*start = 0;
		*end = 0;
		return;
	}
	return static_cast<const WITextEntryBase *>(m_hBase.get())->GetSelectionBounds(start, end);
}
void pragma::gui::types::WITextEntry::SetSelectionBounds(int start, int end)
{
	if(!m_hBase.IsValid())
		return;
	return static_cast<WITextEntryBase *>(m_hBase.get())->SetSelectionBounds(start, end);
}
void pragma::gui::types::WITextEntry::ClearSelection()
{
	if(!m_hBase.IsValid())
		return;
	return static_cast<WITextEntryBase *>(m_hBase.get())->ClearSelection();
}
void pragma::gui::types::WITextEntry::RemoveSelection()
{
	if(!m_hBase.IsValid())
		return;
	static_cast<WITextEntryBase *>(m_hBase.get())->RemoveSelectedText();
}

///////////////////////////////////

void pragma::gui::types::WINumericEntry::Initialize()
{
	WITextEntry::Initialize();

	static_cast<WITextEntryBase *>(m_hBase.get())->SetNumeric(true);
	m_numeric.min = nullptr;
	m_numeric.max = nullptr;
	if(!m_numeric.hUpArrow.IsValid()) {
		m_numeric.hUpArrow = CreateChild<WIArrow>();
		m_numeric.hUpArrow->SetSize(6, 6);
		m_numeric.hUpArrow->SetMouseInputEnabled(true);
		auto hThis = GetHandle();
		m_numeric.hUpArrow->AddCallback("OnMousePressed", FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this, hThis](util::EventReply *reply) -> CallbackReturnType {
			*reply = util::EventReply::Handled;
			if(!hThis.IsValid())
				return CallbackReturnType::HasReturnValue;
			auto text = GetText();
			auto i = string::to_int(text.cpp_str());
			if(m_numeric.max != nullptr)
				i = math::min(++i, *m_numeric.max);
			SetText(string::int_to_string(i));
			return CallbackReturnType::HasReturnValue;
		}));
		static_cast<WIArrow *>(m_numeric.hUpArrow.get())->SetDirection(WIArrow::Direction::Up);
	}
	if(!m_numeric.hDownArrow.IsValid()) {
		m_numeric.hDownArrow = CreateChild<WIArrow>();
		m_numeric.hDownArrow->SetSize(6, 6);
		auto hThis = GetHandle();
		m_numeric.hDownArrow->AddCallback("OnMousePressed", FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this, hThis](util::EventReply *reply) -> CallbackReturnType {
			*reply = util::EventReply::Handled;
			if(!hThis.IsValid())
				return CallbackReturnType::HasReturnValue;
			auto text = GetText();
			auto i = string::to_int(text.cpp_str());
			if(m_numeric.min != nullptr)
				i = math::max(--i, *m_numeric.min);
			SetText(string::int_to_string(i));
			return CallbackReturnType::HasReturnValue;
		}));
	}
	UpdateArrowPositions();
}

bool pragma::gui::types::WINumericEntry::IsNumeric() const { return true; }

const int32_t *pragma::gui::types::WINumericEntry::GetMinValue() const { return m_numeric.min.get(); }
const int32_t *pragma::gui::types::WINumericEntry::GetMaxValue() const { return m_numeric.max.get(); }

void pragma::gui::types::WINumericEntry::SetRange(int32_t min, int32_t max)
{
	SetMinValue(min);
	SetMaxValue(max);
}

void pragma::gui::types::WINumericEntry::SetMinValue(int32_t min) { m_numeric.min = std::make_unique<int32_t>(min); }
void pragma::gui::types::WINumericEntry::SetMinValue() { m_numeric.min = nullptr; }
void pragma::gui::types::WINumericEntry::SetMaxValue(int32_t max) { m_numeric.max = std::make_unique<int32_t>(max); }
void pragma::gui::types::WINumericEntry::SetMaxValue() { m_numeric.max = nullptr; }

void pragma::gui::types::WINumericEntry::UpdateArrowPositions()
{
	if(m_numeric.hDownArrow.IsValid() == false || m_numeric.hUpArrow.IsValid() == false)
		return;
	auto w = GetWidth();
	m_numeric.hUpArrow->SetPos(w - m_numeric.hUpArrow->GetWidth() - 4, 2);
	m_numeric.hDownArrow->SetPos(w - m_numeric.hDownArrow->GetWidth() - 4, GetHeight() - m_numeric.hDownArrow->GetHeight() - 2);
}

void pragma::gui::types::WINumericEntry::SetSize(int x, int y)
{
	WITextEntry::SetSize(x, y);
	UpdateArrowPositions();
}
