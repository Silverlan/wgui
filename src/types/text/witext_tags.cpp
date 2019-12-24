/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext.h"
#include "wgui/types/witext_tags.hpp"
#include "wgui/types/wirect.h"
#include <util_formatted_text.hpp>
#include <util_formatted_text_tag.hpp>
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <prosper_command_buffer.hpp>

void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const WITextTagArgument &arg)
{
	auto it = m_tagArgumentOverrides.find(tagLabel);
	if(it == m_tagArgumentOverrides.end())
		it = m_tagArgumentOverrides.insert(std::make_pair<std::string,std::unordered_map<uint32_t,WITextTagArgument>>(std::string{tagLabel},std::unordered_map<uint32_t,WITextTagArgument>{})).first;
	auto &tagArgOverrides = it->second;
	tagArgOverrides[argumentIndex] = arg;

	auto itTag = m_labelToDecorators.find(tagLabel);
	if(itTag != m_labelToDecorators.end())
	{
		auto &tags = itTag->second;
		for(auto &wpTag : tags)
		{
			if(wpTag.expired())
				continue;
			auto tag = wpTag.lock();
			if(tag->IsValid() == false)
				continue;
			auto &tagArgs = tag->GetArguments();
			if(argumentIndex >= tagArgs.size())
				tagArgs.resize(argumentIndex +1,WITextTagArgument{WITextTagArgument::Type::String,std::make_shared<std::string>("")});
			tagArgs.at(argumentIndex) = arg;

			tag->SetDirty();
		}
	}
	m_flags |= Flags::ApplySubTextTags;
}
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const std::string &arg)
{
	SetTagArgument(tagLabel,argumentIndex,WITextTagArgument{WITextTagArgument::Type::String,std::make_shared<std::string>(arg)});
}
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const Vector4 &arg)
{
	SetTagArgument(tagLabel,argumentIndex,WITextTagArgument{WITextTagArgument::Type::Vector4,std::make_shared<Vector4>(arg)});
}
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const CallbackHandle &arg)
{
	SetTagArgument(tagLabel,argumentIndex,WITextTagArgument{WITextTagArgument::Type::Function,std::make_shared<CallbackHandle>(arg)});
}
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const Color &arg)
{
	SetTagArgument(tagLabel,argumentIndex,WITextTagArgument{WITextTagArgument::Type::Color,std::make_shared<Color>(arg)});
}

void WIText::SetTagsEnabled(bool bEnabled) {m_text->SetTagsEnabled(bEnabled);}
bool WIText::AreTagsEnabled() const {return m_text->AreTagsEnabled();}

bool WIText::IsTextHidden() const {return umath::is_flag_set(m_flags,Flags::HideText);}
void WIText::HideText(bool hide) {umath::set_flag(m_flags,Flags::HideText,hide);}

Color WIText::GetCharColor(util::text::TextOffset offset) const
{
	auto itTag = std::find_if(m_tagInfos.begin(),m_tagInfos.end(),[offset](const std::shared_ptr<WITextDecorator> &pTag) {
		if(pTag->IsValid() == false || pTag->IsTag() == false)
			return false;
		auto &tag = static_cast<WITextTag&>(*pTag).GetTag();
		return tag.IsValid() &&
			offset >= tag.GetOpeningTagComponent()->GetStartAnchorPoint()->GetTextCharOffset() &&
			(tag.IsClosed() == false || offset <= tag.GetClosingTagComponent()->GetStartAnchorPoint()->GetTextCharOffset()) &&
			typeid(*pTag) == typeid(WITextTagColor)
		;
	});
	if(itTag == m_tagInfos.end())
		return GetColor();
	auto color = static_cast<WITextTagColor&>(**itTag).GetColor();
	return color.has_value() ? *color : GetColor();
}

void WIText::InitializeTagArguments(const WITextTag &tag,std::vector<WITextTagArgument> &args) const
{
	auto &openingTag = *tag.GetTag().GetOpeningTagComponent();
	auto &strArgs = openingTag.GetTagAttributes();
	args.resize(strArgs.size());
	auto itTagOverrides = m_tagArgumentOverrides.find(openingTag.GetLabel());
	if(itTagOverrides != m_tagArgumentOverrides.end())
	{
		auto &tagOverrides = itTagOverrides->second;
		for(auto &pair : tagOverrides)
		{
			auto argIdx = pair.first;
			if(argIdx >= args.size())
				args.resize(argIdx +1);
			args.at(argIdx) = pair.second;
		}
	}
	for(auto i=decltype(strArgs.size()){0u};i<strArgs.size();++i)
	{
		auto &strArg = strArgs.at(i);
		auto &arg = args.at(i);
		if(arg.value == nullptr)
			arg = WITextTagArgument{WITextTagArgument::Type::String,std::make_shared<std::string>(strArg)};
	}
	for(auto i=strArgs.size();i<args.size();++i)
	{
		auto &arg = args.at(i);
		if(arg.value == nullptr)
			arg = WITextTagArgument{WITextTagArgument::Type::String,std::make_shared<std::string>("")};
	}
}

void WIText::ApplySubTextTag(WITextDecorator &tag)
{
	tag.Apply();
}

void WIText::ApplySubTextTags()
{
	for(auto &tag : m_tagInfos)
	{
		if(tag->IsValid() == false || tag->IsDirty() == false)
			continue;
		ApplySubTextTag(*tag);
	}
}
