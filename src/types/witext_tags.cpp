/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <prosper_command_buffer.hpp>

#pragma optimize("",off)
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const TagArgument &arg)
{
	auto it = m_tagArgumentOverrides.find(tagLabel);
	if(it == m_tagArgumentOverrides.end())
		it = m_tagArgumentOverrides.insert(std::make_pair<std::string,std::unordered_map<uint32_t,TagArgument>>(std::string{tagLabel},std::unordered_map<uint32_t,TagArgument>{})).first;
	auto &tagArgOverrides = it->second;
	tagArgOverrides[argumentIndex] = arg;
	ApplySubTextTag(tagLabel);
}
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const std::string &arg)
{
	SetTagArgument(tagLabel,argumentIndex,TagArgument{TagArgument::Type::String,std::make_shared<std::string>(arg)});
}
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const Vector4 &arg)
{
	SetTagArgument(tagLabel,argumentIndex,TagArgument{TagArgument::Type::Vector4,std::make_shared<Vector4>(arg)});
}
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const CallbackHandle &arg)
{
	SetTagArgument(tagLabel,argumentIndex,TagArgument{TagArgument::Type::Function,std::make_shared<CallbackHandle>(arg)});
}
void WIText::SetTagArgument(const std::string &tagLabel,uint32_t argumentIndex,const Color &arg)
{
	SetTagArgument(tagLabel,argumentIndex,TagArgument{TagArgument::Type::Color,std::make_shared<Color>(arg)});
}

void WIText::SetTagsEnabled(bool bEnabled) {m_bTagsEnabled = bEnabled;}
bool WIText::AreTagsEnabled() const {return m_bTagsEnabled;}

std::pair<int32_t,int32_t> WIText::GetTagRange(const TagInfo &tag) const
{
	auto range = std::pair<int32_t,int32_t>{tag.rangeOpenTag.offset,tag.rangeClosingTag.GetEndOffset()};
	if(tag.rangeClosingTag.length == 0)
	{
		// Tag hasn't been closed (yet(?)), assume it applies to everything up to the last visible character
		auto it = std::find_if(m_lines.rbegin(),m_lines.rend(),[](const LineInfo &lineInfo) {
			return lineInfo.visTextRange.GetEndOffset() != -1;
		});
		if(it != m_lines.rend())
			range.second = m_visibleTextIndexToRawTextIndex.at(it->visTextRange.GetEndOffset());
		else
			range.second = range.first; // This should never happen!
	}
	return range;
}

std::optional<uint32_t> WIText::ParseCodeTag(const std::string_view &sv,uint32_t offset,TagInfo *outTagInfo,bool *outIsClosingTag) const
{
	if(AreTagsEnabled() == false)
		return {};
	auto *ptrData = sv.data() +offset;
	if(ptrData[0] != '[')
		return {};
	if(outIsClosingTag)
		*outIsClosingTag = false;
	auto i = 1u;
	enum class Stage : uint8_t
	{
		TagName = 0u,
		Label,
		Arguments
	};
	auto stage = Stage::TagName;
	TagInfo tagInfo {};
	std::string tagName {};
	auto bStringInQuotes = false;
	while(true)
	{
		auto token = ptrData[i];
		auto controlToken = token;
		if(bStringInQuotes && token != '\0' && token != '\"')
			controlToken = ' '; // Arbitrary token which must reach 'default' branch
		switch(controlToken)
		{
			case '\0':
				return {};
			case ':':
				stage = Stage::Arguments;
				break;
			case '#':
				if(stage == Stage::TagName)
					stage = Stage::Label;
				break;
			case ']':
				goto endLoop;
			case ',':
				if(stage == Stage::Arguments)
				{
					if(tagInfo.args.empty())
						tagInfo.args.push_back({TagArgument::Type::String,std::make_shared<std::string>()}); // First argument is empty
					tagInfo.args.push_back({TagArgument::Type::String,std::make_shared<std::string>()});
				}
				break;
			case '/':
				if(i == 1u && outIsClosingTag)
					*outIsClosingTag = true;
				break;
			case '"':
				bStringInQuotes = !bStringInQuotes;
				break;
			default:
			{
				switch(stage)
				{
					case Stage::TagName:
						tagName += token;
						break;
					case Stage::Label:
						tagInfo.label += token;
						break;
					default:
						if(tagInfo.args.empty())
							tagInfo.args.push_back({TagArgument::Type::String,std::make_shared<std::string>()});
						*static_cast<std::string*>(tagInfo.args.back().value.get()) += token;
						break;
				}
				break;
			}
		}
		++i;
	}
endLoop:
	tagInfo.tagType = TagType::None;
	if(ustring::compare(tagName,"c") || ustring::compare(tagName,"color"))
		tagInfo.tagType = TagType::Color;
	else if(ustring::compare(tagName,"l") || ustring::compare(tagName,"link"))
		tagInfo.tagType = TagType::Link;
	else if(ustring::compare(tagName,"u") || ustring::compare(tagName,"underline"))
		tagInfo.tagType = TagType::Underline;
	else if(ustring::compare(tagName,"t") || ustring::compare(tagName,"tooltip"))
		tagInfo.tagType = TagType::Tooltip;
	else if(ustring::compare(tagName,"template"))
		tagInfo.tagType = TagType::Template;
	else
		return {};
	if(outTagInfo)
		*outTagInfo = tagInfo;
	return offset +i +1u;
}

static std::optional<Vector4> get_color_from_tag(const WIText::TagInfo &tag)
{
	if(tag.args.empty())
		return {};
	auto &arg = tag.args.front();
	Vector4 color;
	switch(arg.type)
	{
		case WIText::TagArgument::Type::String:
			color = Color::CreateFromHexColor(*static_cast<std::string*>(arg.value.get())).ToVector4();
			break;
		case WIText::TagArgument::Type::Color:
			color = static_cast<Color*>(arg.value.get())->ToVector4();
			break;
		case WIText::TagArgument::Type::Vector4:
			color = *static_cast<Vector4*>(arg.value.get());
			break;
		default:
			return {};
	}
	return color;
}
void WIText::ApplySubTextColor(TagInfo &tag)
{
	auto color = get_color_from_tag(tag);
	if(color.has_value() == false)
		return;
	if(s_colorBuffer == nullptr)
	{
		const auto maxInstances = 2'048; // 1 MiB total space
		auto instanceSize = sizeof(Vector4) *MAX_CHARS_PER_BUFFER;
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
		createInfo.size = instanceSize *maxInstances;
		s_colorBuffer = prosper::util::create_uniform_resizable_buffer(WGUI::GetInstance().GetContext(),createInfo,instanceSize,createInfo.size *5u,0.05f);
		s_colorBuffer->SetPermanentlyMapped(true);
		s_colorBuffer->SetDebugName("text_color_buf");
	}

	auto baseColor = GetColor().ToVector4();
	auto range = GetTagRange(tag);
	auto charStart = m_rawTextIndexToVisibleTextIndex.at(range.first);
	auto charEnd = m_rawTextIndexToVisibleTextIndex.at(range.second);
	std::cout<<"Applying subtext color in range "<<charStart<<","<<charEnd<<"..."<<std::endl;
	auto &context = WGUI::GetInstance().GetContext();
	for(auto &lineInfo : m_lines)
	{
		auto &glyphBufferInfos = lineInfo.buffers;
		auto bufIdx = 0u;
		while(bufIdx < glyphBufferInfos.size())
		{
			auto &glyphBufferInfo = glyphBufferInfos.at(bufIdx++);
			auto glyphCharStart = m_rawTextIndexToVisibleTextIndex.at(glyphBufferInfo.range.offset);
			auto glyphCharEnd = m_rawTextIndexToVisibleTextIndex.at(glyphBufferInfo.range.GetEndOffset());
			if(charStart > glyphCharEnd || glyphBufferInfo.numChars == 0u)
				continue;
			if(glyphCharStart > charEnd)
				break;
			std::cout<<"Applying color to subtext: "<<ustring::substr(m_visibleText,glyphCharStart,glyphCharEnd)<<std::endl;
			std::array<Vector4,MAX_CHARS_PER_BUFFER> colors {};
			auto fullUpdate = false;
			if(glyphBufferInfo.colorBuffer == nullptr)
			{
				glyphBufferInfo.colorBuffer = s_colorBuffer->AllocateBuffer();
				std::fill(colors.begin(),colors.end(),baseColor);
				fullUpdate = true;
			}
			auto localCharStart = umath::max(static_cast<int32_t>(charStart) -static_cast<int32_t>(glyphCharStart),0);
			auto localCharEnd = umath::clamp(static_cast<int32_t>(charEnd) -static_cast<int32_t>(glyphCharStart),0,static_cast<int32_t>(colors.size()) -1);
			auto numLocalChars = localCharEnd -localCharStart +1u;
			//for(auto i=localCharStart;i<(localCharStart +numLocalChars);++i)
			//	colors.at(i) = color;
			std::fill(colors.begin() +localCharStart,colors.begin() +localCharStart +numLocalChars,*color);
			if(fullUpdate == true)
			{
				context.ScheduleRecordUpdateBuffer(
					glyphBufferInfo.colorBuffer,
					0u,colors.size() *sizeof(colors.front()),colors.data()
				);
			}
			else
			{
				context.ScheduleRecordUpdateBuffer(
					glyphBufferInfo.colorBuffer,
					localCharStart *sizeof(colors.front()),numLocalChars *sizeof(colors.front()),colors.data() +localCharStart
				);
			}
			prosper::util::record_buffer_barrier(
				**context.GetDrawCommandBuffer(),*glyphBufferInfo.colorBuffer,
				Anvil::PipelineStageFlagBits::TRANSFER_BIT,Anvil::PipelineStageFlagBits::VERTEX_INPUT_BIT,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,Anvil::AccessFlagBits::VERTEX_ATTRIBUTE_READ_BIT
			);
		}
	}
}

void WIText::ApplySubTextUnderline(TagInfo &tag)
{
	auto color = GetColor();
	auto range = GetTagRange(tag);
	auto startOffset = range.first;
	auto itTag = std::find_if(m_subTextTags.begin(),m_subTextTags.end(),[this,startOffset](const WIText::TagInfo &tagInfo) -> bool {
		if(tagInfo.tagType != WIText::TagType::Color)
			return false;
		auto rangeOther = GetTagRange(tagInfo);
		return startOffset >= rangeOther.first && startOffset <= rangeOther.second;
	});
	if(itTag != m_subTextTags.end())
	{
		auto tagColor = get_color_from_tag(*itTag);
		if(tagColor.has_value())
			color = Color{*tagColor}; // Use color of first character as color for underline
	}

	auto *overlays = CreateSubTextOverlayElement(tag);
	if(!overlays)
		return;
	for(auto &hOverlay : *overlays)
	{
		if(hOverlay.IsValid() == false)
			continue;
		auto *pOverlay = hOverlay.get();
		auto *pUnderline = pOverlay->FindDescendantByName("underline");
		if(pUnderline == nullptr)
			pUnderline = WGUI::GetInstance().Create<WIRect>(pOverlay);
		pUnderline->SetName("underline");
		pUnderline->SetPos(Vector2i{0,pOverlay->GetHeight() -1});
		pUnderline->SetSize(Vector2i{pOverlay->GetWidth(),1});
		pUnderline->SetColor(color);
	}
}

void WIText::ApplySubTextLink(TagInfo &tag)
{
	if(tag.args.empty())
		return;
	auto type = tag.args.front().type;
	CallbackHandle cb {};
	switch(type)
	{
		case TagArgument::Type::Function:
			cb = *static_cast<CallbackHandle*>(tag.args.front().value.get());
			break;
		case TagArgument::Type::String:
		{
			if(s_linkHandler == nullptr)
				return;
			auto arg = *static_cast<std::string*>(tag.args.front().value.get());
			cb = FunctionCallback<util::EventReply,std::reference_wrapper<const std::vector<std::string>>>::CreateWithOptionalReturn(
				[arg](util::EventReply *reply,std::reference_wrapper<const std::vector<std::string>> args) -> CallbackReturnType {
				s_linkHandler(arg);
				*reply = util::EventReply::Handled;
				return CallbackReturnType::HasReturnValue;
			});
			break;
		}
		default:
			return;
	}

	std::vector<std::string> strArgs {};
	if(tag.args.size() > 1u)
	{
		strArgs.reserve(tag.args.size() -1u);
		for(auto it=tag.args.begin() +1u;it!=tag.args.end();++it)
		{
			auto &arg = *it;
			if(arg.type != TagArgument::Type::String)
				continue;
			strArgs.push_back(*static_cast<std::string*>(arg.value.get()));
		}
	}

	auto *overlays = CreateSubTextOverlayElement(tag);
	if(!overlays)
		return;
	for(auto &hOverlay : *overlays)
	{
		if(hOverlay.IsValid() == false)
			continue;
		auto *pOverlay = hOverlay.get();
		pOverlay->AddCallback("OnMouseReleased",FunctionCallback<util::EventReply>::CreateWithOptionalReturn([cb,strArgs](util::EventReply *reply) mutable -> CallbackReturnType {
			if(cb.IsValid() == false)
				return CallbackReturnType::NoReturnValue;
			return cb.Call<util::EventReply,std::reference_wrapper<const std::vector<std::string>>>(reply,strArgs);
		}));
		pOverlay->SetMouseInputEnabled(true);
		pOverlay->SetMouseMovementCheckEnabled(true);
		pOverlay->SetCursor(GLFW::Cursor::Shape::Hand);
	}
	ApplySubTextUnderline(tag);
}

int32_t WIText::FindLine(int32_t charOffset)
{
	auto it = std::find_if(m_lines.begin(),m_lines.end(),[charOffset](const LineInfo &lineInfo) {
		return charOffset >= lineInfo.textRange.offset && charOffset <= lineInfo.textRange.GetEndOffset();
	});
	return (it != m_lines.end()) ? (it -m_lines.begin()) : -1;
}

const std::vector<WIHandle> *WIText::CreateSubTextOverlayElement(TagInfo &tag)
{
	auto range = GetTagRange(tag);
	auto lineStartIdx = FindLine(range.first);
	auto lineEndIdx = FindLine(range.second);
	if(lineStartIdx == -1 || lineEndIdx == -1)
		return nullptr;
	auto numElements = lineEndIdx -lineStartIdx +1u;
	tag.overlays.reserve(numElements);
	while(tag.overlays.size() > numElements)
	{
		auto &hOverlay = tag.overlays.back();
		if(hOverlay.IsValid())
			hOverlay->Remove();
		tag.overlays.pop_back();
	}
	for(auto i=tag.overlays.size();i<numElements;++i)
	{
		auto *p = WGUI::GetInstance().Create<WIBase>(this);
		tag.overlays.push_back(p->GetHandle());
	}

	auto idxOverlay = 0u;
	for(auto i=lineStartIdx;i<=lineEndIdx;++i)
	{
		auto &hOverlay = tag.overlays.at(idxOverlay++);
		if(hOverlay.IsValid() == false)
			continue;
		auto &lineInfo = m_lines.at(i);
		auto startBounds = GetCharacterPixelBounds(m_rawTextIndexToVisibleTextIndex.at(umath::max(range.first,lineInfo.textRange.offset)));
		auto endBounds = GetCharacterPixelBounds(m_rawTextIndexToVisibleTextIndex.at(umath::min(range.second,lineInfo.textRange.GetEndOffset())));

		auto *p = hOverlay.get();
		p->SetSize(endBounds.second -startBounds.first +Vector2i{0,2});
		p->SetPos(startBounds.first);
	}
	return &tag.overlays;
}

void WIText::ApplySubTextTag(const std::string &label)
{
	for(auto &tagInfo : m_subTextTags)
	{
		if(ustring::compare(tagInfo.label,label) == false)
			continue;
		ApplySubTextTag(tagInfo);
	}
}
void WIText::ApplySubTextTag(TagInfo &tag)
{
	auto itTagOverrides = m_tagArgumentOverrides.find(tag.label);
	if(itTagOverrides != m_tagArgumentOverrides.end())
	{
		auto &tagOverrides = itTagOverrides->second;
		for(auto &pair : tagOverrides)
		{
			if(pair.first >= tag.args.size())
				tag.args.resize(pair.first +1u);
			tag.args.at(pair.first) = pair.second;
		}
	}
	switch(tag.tagType)
	{
		case TagType::Color:
			ApplySubTextColor(tag);
			break;
		case TagType::Link:
			ApplySubTextLink(tag);
			break;
		case TagType::Underline:
			ApplySubTextUnderline(tag);
			break;
		case TagType::Tooltip:
		{
			if(tag.args.empty() == false && tag.args.front().type == TagArgument::Type::String)
			{
				auto *overlays = CreateSubTextOverlayElement(tag);
				if(overlays)
				{
					for(auto &hOverlay : *overlays)
					{
						if(hOverlay.IsValid() == false)
							continue;
						hOverlay.get()->SetTooltip(*static_cast<std::string*>(tag.args.front().value.get()));
					}
				}
			}
			break;
		}
		case TagType::Template:
		{
			// TODO: Not yet implemented!
			// Allow insertion of custom tags after the fact
			break;
		}
	}
}

void WIText::ApplySubTextTags()
{
	for(auto it=m_subTextTags.begin();it!=m_subTextTags.end();++it)
	{
		auto &subTextTag = *it;
		ApplySubTextTag(subTextTag);
	}
}
#pragma optimize("",on)
