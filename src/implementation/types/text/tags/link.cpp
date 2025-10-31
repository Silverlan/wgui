// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


module pragma.gui;

import :text_tags;

decltype(WITextTagLink::s_linkHandler) WITextTagLink::s_linkHandler = nullptr;
void WITextTagLink::set_link_handler(const std::function<void(const std::string &)> &linkHandler) { s_linkHandler = linkHandler; }
void WITextTagLink::InitializeOverlay(WIBase &overlay)
{
	WITextTagUnderline::InitializeOverlay(overlay);
	overlay.AddCallback("OnMousePressed", FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this](util::EventReply *reply) mutable -> CallbackReturnType {
		if(m_cbFunction.IsValid() == false)
			return CallbackReturnType::NoReturnValue;
		if(reply)
			*reply = util::EventReply::Handled;
		return CallbackReturnType::HasReturnValue;
	}));
	overlay.AddCallback("OnMouseReleased", FunctionCallback<util::EventReply>::CreateWithOptionalReturn([this](util::EventReply *reply) mutable -> CallbackReturnType {
		if(m_cbFunction.IsValid() == false)
			return CallbackReturnType::NoReturnValue;
		return m_cbFunction.Call<util::EventReply, std::reference_wrapper<const std::vector<std::string>>>(reply, m_strArgs);
	}));
	overlay.SetZPos(20);
	overlay.SetMouseInputEnabled(true);
	overlay.SetMouseMovementCheckEnabled(true);
	overlay.SetCursor(pragma::platform::Cursor::Shape::Hand);
}
void WITextTagLink::Initialize()
{
	auto type = m_args.front().type;
	switch(type) {
	case WITextTagArgument::Type::Function:
		m_cbFunction = *static_cast<CallbackHandle *>(m_args.front().value.get());
		break;
	case WITextTagArgument::Type::String:
		{
			if(s_linkHandler == nullptr)
				return;
			auto arg = *static_cast<std::string *>(m_args.front().value.get());
			m_cbFunction = FunctionCallback<util::EventReply, std::reference_wrapper<const std::vector<std::string>>>::CreateWithOptionalReturn([this, arg](util::EventReply *reply, std::reference_wrapper<const std::vector<std::string>> args) -> CallbackReturnType {
				m_text.CallCallbacksWithOptionalReturn<util::EventReply, std::string>("HandleLinkTagAction", *reply, arg);
				if(*reply == util::EventReply::Handled)
					return CallbackReturnType::HasReturnValue;
				s_linkHandler(arg);
				*reply = util::EventReply::Handled;
				return CallbackReturnType::HasReturnValue;
			});
			break;
		}
	default:
		return;
	}

	if(m_args.size() > 1u) {
		m_strArgs.reserve(m_args.size() - 1u);
		for(auto it = m_args.begin() + 1u; it != m_args.end(); ++it) {
			auto &arg = *it;
			if(arg.type != WITextTagArgument::Type::String)
				continue;
			m_strArgs.push_back(*static_cast<std::string *>(arg.value.get()));
		}
	}
}
void WITextTagLink::Apply()
{
	CreateOverlayElements();
	WITextTagUnderline::Apply();
}
