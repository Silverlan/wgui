// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :text_tags;

decltype(pragma::gui::WITextTagColor::s_colorBuffer) pragma::gui::WITextTagColor::s_colorBuffer = nullptr;
void pragma::gui::WITextTagColor::ClearColorBuffer() { s_colorBuffer = nullptr; }
std::optional<Vector4> pragma::gui::WITextTagColor::GetColor() const
{
	if(m_args.empty())
		return {};
	auto &arg = m_args.front();
	Vector4 color;
	switch(arg.type) {
	case WITextTagArgument::Type::String:
		color = Color::CreateFromHexColor(*static_cast<std::string *>(arg.value.get())).ToVector4();
		break;
	case WITextTagArgument::Type::Color:
		color = static_cast<Color *>(arg.value.get())->ToVector4();
		break;
	case WITextTagArgument::Type::Vector4:
		color = *static_cast<Vector4 *>(arg.value.get());
		break;
	default:
		return {};
	}
	return color;
}
void pragma::gui::WITextTagColor::Apply()
{
	WITextDecorator::Apply();
	auto color = GetColor();
	if(color.has_value() == false)
		return;
	if(s_colorBuffer == nullptr) {
		const auto maxInstances = 2'048; // 1 MiB total space
		auto instanceSize = sizeof(Vector4) * types::WIText::MAX_CHARS_PER_BUFFER;
		prosper::util::BufferCreateInfo createInfo {};
		createInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit | prosper::BufferUsageFlags::TransferDstBit;
		createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
		createInfo.size = instanceSize * maxInstances;
		createInfo.flags |= prosper::util::BufferCreateInfo::Flags::Persistent;
		s_colorBuffer = WGUI::GetInstance().GetContext().CreateUniformResizableBuffer(createInfo, instanceSize, createInfo.size * 5u, 0.05f);
		s_colorBuffer->SetPermanentlyMapped(true, prosper::IBuffer::MapFlags::ReadBit);
		s_colorBuffer->SetDebugName("text_color_buf");
	}

	auto baseColor = m_text.GetColor().ToVector4();
	auto &context = WGUI::GetInstance().GetContext();
	auto &openingTag = *m_tag.GetOpeningTagComponent();

	string::LineIndex startLineIdx, endLineIdx;
	string::TextOffset absStartCharOffset, absEndCharOffset;
	GetTagRange(startLineIdx, endLineIdx, absStartCharOffset, absEndCharOffset);

	auto &startAnchorPoint = *openingTag.GetStartAnchorPoint();
	for(auto lineIdx = startLineIdx; lineIdx <= endLineIdx; ++lineIdx) {
		auto &line = *m_text.GetLine(lineIdx);

		string::CharOffset localStartOffset, localEndOffset;
		auto numChars = GetTagRange(line, localStartOffset, localEndOffset);
		if(numChars == -1)
			break;
		if(numChars == -2)
			continue;

		auto &lineInfo = m_text.GetLines().at(lineIdx);
		auto numBuffers = lineInfo.GetBufferCount();
		auto bufIdx = 0u;
		while(bufIdx < numBuffers) {
			auto &glyphBufferInfo = *lineInfo.GetBufferInfo(bufIdx++);
			auto bufStartOffset = glyphBufferInfo.charOffset;
			auto bufEndOffset = bufStartOffset + glyphBufferInfo.numChars - 1;
			if(bufEndOffset < localStartOffset)
				continue;
			if(bufStartOffset > localEndOffset)
				break;
			std::array<Vector4, types::WIText::MAX_CHARS_PER_BUFFER> colors;

			auto fullUpdate = false;
			if(glyphBufferInfo.colorBuffer == nullptr) {
				glyphBufferInfo.colorBuffer = s_colorBuffer->AllocateBuffer();
				std::fill(colors.begin(), colors.end(), baseColor);
				fullUpdate = true;
			}

			auto offsetRelativeToBuffer = (localStartOffset > bufStartOffset) ? (localStartOffset - bufStartOffset) : 0;
			auto endOffsetRelativeToBuffer = std::min<uint32_t>(localEndOffset - bufStartOffset, glyphBufferInfo.numChars - 1);
			std::fill(colors.begin() + offsetRelativeToBuffer, colors.begin() + endOffsetRelativeToBuffer + 1, *color);
			if(fullUpdate == true) {
				context.ScheduleRecordUpdateBuffer(glyphBufferInfo.colorBuffer, 0u, colors.size() * sizeof(colors.front()), colors.data());
			}
			else {
				context.ScheduleRecordUpdateBuffer(glyphBufferInfo.colorBuffer, offsetRelativeToBuffer * sizeof(colors.front()), (endOffsetRelativeToBuffer - offsetRelativeToBuffer + 1) * sizeof(colors.front()), colors.data() + offsetRelativeToBuffer);
			}
			context.GetWindow().GetDrawCommandBuffer()->RecordBufferBarrier(*glyphBufferInfo.colorBuffer, prosper::PipelineStageFlags::TransferBit, prosper::PipelineStageFlags::VertexInputBit, prosper::AccessFlags::TransferWriteBit, prosper::AccessFlags::VertexAttributeReadBit);
		}
	}
}
