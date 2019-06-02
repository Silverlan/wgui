/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext.h"
#include "wgui/types/witext_tags.hpp"
#include "wgui/shaders/wishader_text.hpp"
#include "wgui/types/wirect.h"
#include <prosper_context.hpp>
#include <image/prosper_sampler.hpp>
#include <prosper_util.hpp>
#include <prosper_util_square_shape.hpp>
#include <shader/prosper_shader_blur.hpp>
#include <prosper_command_buffer.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <sharedutils/scope_guard.h>
#include <util_formatted_text.hpp>

void WIText::InitializeBlur(bool bReload)
{
	if(bReload == true)
		DestroyBlur();
	// Initialize blur temp buffers
	if(m_shadowRenderTarget != nullptr)
	{
		auto &dev = WGUI::GetInstance().GetContext().GetDevice();
		m_shadowBlurSet = prosper::BlurSet::Create(dev,m_shadowRenderTarget);
	}
	//
}

void WIText::DestroyBlur() {m_shadowBlurSet = nullptr;}

void WIText::InitializeShadow(bool bReload)
{
	if(bReload == true)
		DestroyShadow();
	auto &context = WGUI::GetInstance().GetContext();
	auto &dev = context.GetDevice();
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = m_wTexture;
	createInfo.height = m_hTexture;
	createInfo.format = Anvil::Format::R8_UNORM;
	createInfo.usage = Anvil::ImageUsageFlagBits::SAMPLED_BIT | Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT;
	createInfo.postCreateLayout = Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
	auto imgViewCreateInfo = prosper::util::ImageViewCreateInfo {};
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	samplerCreateInfo.addressModeU = samplerCreateInfo.addressModeV = Anvil::SamplerAddressMode::CLAMP_TO_EDGE;
	auto img = prosper::util::create_image(dev,createInfo);
	auto tex = prosper::util::create_texture(dev,{},std::move(img),&imgViewCreateInfo,&samplerCreateInfo);
	auto &shader = static_cast<wgui::ShaderText&>(*m_shader.get());
	auto &renderPass = shader.GetRenderPass();
	m_shadowRenderTarget = prosper::util::create_render_target(dev,{tex},renderPass);

	if(!m_baseTextShadow.IsValid())
	{
		m_baseTextShadow = CreateChild<WITexturedRect>();
		if(m_baseTextShadow.IsValid())
		{
			auto *rect = static_cast<WITexturedRect*>(m_baseTextShadow.get());
			if(rect != nullptr)
			{
				rect->SetZPos(0);
				rect->SetAlphaOnly(true);
				rect->SetColor(*GetShadowColor());
				rect->SetPos(m_shadow.offset);
			}
		}
	}
	if(m_baseTextShadow.IsValid())
	{
		auto *rect = static_cast<WITexturedRect*>(m_baseTextShadow.get());
		if(rect != nullptr)
			rect->SetTexture(*tex);
	}
}

void WIText::DestroyShadow() {m_shadowRenderTarget = nullptr;}

void WIText::ScheduleRenderUpdate(bool bFull)
{
	if(bFull)
		m_flags |= Flags::FullUpdateScheduled | Flags::ApplySubTextTags;
	else
		m_flags |= Flags::RenderTextScheduled | Flags::ApplySubTextTags;
}

void WIText::SetShadowBlurSize(float size)
{
	if(size == m_shadow.blurSize)
		return;
	if(size > 0.f && IsCacheEnabled() == false)
	{
		SetCacheEnabled(true); // Cached images are required for blurred shadows
		m_flags |= Flags::FullUpdateScheduled;
	}
	ScheduleRenderUpdate();
	m_shadow.blurSize = size;
}
float WIText::GetShadowBlurSize() {return m_shadow.blurSize;}
void WIText::EnableShadow(bool b)
{
	if(b == m_shadow.enabled)
		return;
	m_shadow.enabled = b;
	ScheduleRenderUpdate();
}
bool WIText::IsShadowEnabled() {return m_shadow.enabled;}
void WIText::SetShadowOffset(Vector2i offset) {SetShadowOffset(offset.x,offset.y);}
void WIText::SetShadowOffset(int x,int y)
{
	if(m_shadow.offset.x == x && m_shadow.offset.y == y)
		return;
	ScheduleRenderUpdate();
	m_shadow.offset.x = x;
	m_shadow.offset.y = y;
	if(!m_baseTextShadow.IsValid())
		return;
	WITexturedRect *rect = static_cast<WITexturedRect*>(m_baseTextShadow.get());
	if(rect == NULL)
		return;
	rect->SetPos(m_shadow.offset);
}
Vector2i *WIText::GetShadowOffset() {return &m_shadow.offset;}
Vector4 *WIText::GetShadowColor() {return &m_shadow.color;}
void WIText::SetShadowColor(Vector4 col) {SetShadowColor(col.r,col.g,col.b,col.a);}
void WIText::SetShadowColor(const Color &col) {SetShadowColor(static_cast<float>(col.r) /255.f,static_cast<float>(col.g) /255.f,static_cast<float>(col.b) /255.f,static_cast<float>(col.a) /255.f);}
void WIText::SetShadowColor(float r,float g,float b,float a)
{
	if(m_shadow.color.r == r && m_shadow.color.g == g && m_shadow.color.b == b && m_shadow.color.a == a)
		return;
	ScheduleRenderUpdate();
	m_shadow.color.r = r;
	m_shadow.color.g = g;
	m_shadow.color.b = b;
	m_shadow.color.a = a;
	if(!m_baseTextShadow.IsValid())
		return;
	WITexturedRect *rect = static_cast<WITexturedRect*>(m_baseTextShadow.get());
	rect->SetColor(r,g,b,a);
}
void WIText::SetShadowAlpha(float alpha)
{
	if(m_shadow.color.a == alpha)
		return;
	ScheduleRenderUpdate();
	m_shadow.color.a = alpha;
}
float WIText::GetShadowAlpha() {return m_shadow.color.a;}

void WIText::SelectShader()
{
	// Deprecated?
}
Mat4 WIText::GetTransformedMatrix(const Vector2i &origin,int w,int h,Mat4 mat) const
{
	return WIBase::GetTransformedMatrix(origin,w,h,mat);
	/*
	mat = glm::translate(mat,Vector3(
		(m_pos.x /float(w)) *2,
		-((m_pos.y /float(h)) *2),
		0
	));
	mat = glm::translate(mat,Vector3(-1,1,0));
	//mat = glm::translate(mat,Vector3(-1,1 -((m_font->GetSize()) /float(h) *2.f),0));
	return mat;*/
}

void WIText::UpdateRenderTexture()
{
	if(IsCacheEnabled() == false)
	{
		if((m_flags &(Flags::RenderTextScheduled | Flags::FullUpdateScheduled)) != Flags::None)
		{
			m_flags &= ~(Flags::RenderTextScheduled | Flags::FullUpdateScheduled);
			UpdateSubLines();
			InitializeTextBuffers();
		}
		if((m_flags &Flags::ApplySubTextTags) != Flags::None)
		{
			m_flags &= ~Flags::ApplySubTextTags;
			ApplySubTextTags();
		}
		return;
	}
	if(m_shader.expired())
		return;
	if((m_flags &Flags::RenderTextScheduled) != Flags::None && (m_flags &Flags::FullUpdateScheduled) == Flags::None)
	{
		m_flags &= ~Flags::RenderTextScheduled;
		RenderText();
		return;
	}
	else if((m_flags &(Flags::RenderTextScheduled | Flags::FullUpdateScheduled)) == Flags::None)
		return;
	m_flags &= ~(Flags::FullUpdateScheduled | Flags::RenderTextScheduled);
	int w,h;
	GetTextSize(&w,&h);
	if(w <= 0)
		w = 1;
	if(h <= 0)
		h = 1;
	//int szMax = OpenGL::GetInt(GL_MAX_TEXTURE_SIZE);
	//if(w > szMax)
	//	w = szMax;
	//if(h > szMax)
	//	h = szMax; // Vulkan TODO
	m_wTexture = w;
	m_hTexture = h;
	if(m_renderTarget != nullptr)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_renderTarget);
	m_renderTarget = nullptr;
	DestroyShadow();
	DestroyBlur();

	if(m_wTexture == 0u || m_hTexture == 0u)
		return;

	auto &dev = WGUI::GetInstance().GetContext().GetDevice();
	auto imgCreateInfo = prosper::util::ImageCreateInfo {};
	imgCreateInfo.width = m_wTexture;
	imgCreateInfo.height = m_hTexture;
	imgCreateInfo.format = Anvil::Format::R8_UNORM;
	imgCreateInfo.usage = Anvil::ImageUsageFlagBits::SAMPLED_BIT | Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT;
	imgCreateInfo.postCreateLayout = Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
	
	auto img = prosper::util::create_image(dev,imgCreateInfo);
	auto imgViewCreateInfo = prosper::util::ImageViewCreateInfo {};
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	auto tex = prosper::util::create_texture(dev,{},std::move(img),&imgViewCreateInfo,&samplerCreateInfo);
	auto &shader = static_cast<wgui::ShaderText&>(*m_shader.get());
	auto &renderPass = shader.GetRenderPass();
	m_renderTarget = prosper::util::create_render_target(dev,{tex},renderPass);
	if(IsShadowEnabled())
		InitializeShadow();
	InitializeBlur();
	if(m_renderTarget == nullptr) // Dimensions are most likely 0
		return;
	if(m_baseEl.IsValid())
	{
		auto *el = static_cast<WITextBase*>(m_baseEl.get());
		el->InitializeTexture(*m_renderTarget->GetTexture(),w,h);
	}
	if(m_baseTextShadow.IsValid())
	{
		WITexturedRect *text = m_baseTextShadow.get<WITexturedRect>();
		if(text != NULL)
		{
			text->SetSize(w,h);
			text->SetTexture(*m_shadowRenderTarget->GetTexture());
		}
	}

	//if(OpenGL::GetFrameBufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	//{
	//	m_renderTexture = GLTexturePtr(nullptr);
	//	m_wTexture = 0;
	//	m_hTexture = 0;
	//	return;
	//}
	//bool scissor = OpenGL::GetBool(GL_SCISSOR_TEST);
	//OpenGL::Disable(GL_SCISSOR_TEST); // Vulkan TODO

	RenderText();

	//if(scissor)
	//	OpenGL::Enable(GL_SCISSOR_TEST);

	//OpenGL::BindTexture(texture);
	//OpenGL::BindFrameBuffer(frameBuffer);
}
void WIText::RenderText()
{
	Mat4 mat(1.f);
	mat = glm::translate(mat,Vector3(-1.f,0.f,0.f));
	RenderText(mat);
}
std::shared_ptr<prosper::Texture> WIText::GetTexture() const {return (m_renderTarget != nullptr) ? m_renderTarget->GetTexture() : nullptr;}

#pragma pack(push,1)
struct GlyphBoundsInfo
{
	int32_t index;
	Vector4 bounds;
};
#pragma pack(pop)
void WIText::RenderText(Mat4&)
{
	if(m_font == nullptr || m_renderTarget == nullptr || m_shader.expired() || IsCacheEnabled() == false)
		return;
	auto &context = WGUI::GetInstance().GetContext();
	auto extents = (*m_renderTarget->GetTexture()->GetImage())->get_image_extent_2D(0u);
	auto w = extents.width;
	auto h = extents.height;
	auto sx = 2.f /float(w);
	auto sy = 2.f /float(h);
	//auto fontHeight = m_font->GetMaxGlyphTop();//m_font->GetMaxGlyphSize()
	auto fontSize = m_font->GetSize();

	// Initialize Buffers
	float x = 0;
	float y = 0;
	auto numChars = m_text->GetCharCount();
	std::vector<GlyphBoundsInfo> glyphBoundsInfos;
	glyphBoundsInfos.reserve(numChars);
	if(numChars == 0)
		return;
	for(unsigned int i=0;i<m_lineInfos.size();i++)
	{
		x = 0;
		auto &lineInfo = m_lineInfos.at(i);
		if(lineInfo.wpLine.expired())
			continue;
		auto &line = *lineInfo.wpLine.lock();
		auto &formattedLine = line.GetFormattedLine();
		for(unsigned int j=0;j<formattedLine.GetLength();j++)
		{
			auto c = static_cast<UChar>(formattedLine.At(j));
			auto *glyph = m_font->GetGlyphInfo(c);
			if(glyph != nullptr)
			{
				int32_t left,top,width,height;
				glyph->GetDimensions(left,top,width,height);
				int32_t advanceX,advanceY;
				glyph->GetAdvance(advanceX,advanceY);

				auto x2 = x +left *sx -1.f;
				auto y2 = y -1.f -((top -static_cast<int>(fontSize))) *sy;
				auto w = width *sx;
				auto h = height *sy;

				glyphBoundsInfos.push_back({});
				auto &info = glyphBoundsInfos.back();
				info.index = FontInfo::CharToGlyphMapIndex(c);
				info.bounds = {x2,y2,width,height};

				x += (advanceX >> 6) *sx;
				y += (advanceY >> 6) *sy;
			}
		}
		y += GetLineHeight() *sy;
	}
	numChars = glyphBoundsInfos.size();
	assert(numChars != 0);
	if(numChars == 0)
		return;
	auto &dev = context.GetDevice();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
	createInfo.size = glyphBoundsInfos.size() *sizeof(glyphBoundsInfos.front());

	auto bufBounds = prosper::util::create_buffer(dev,createInfo,glyphBoundsInfos.data());
	context.KeepResourceAliveUntilPresentationComplete(bufBounds);

	auto drawCmd = context.GetDrawCommandBuffer();
	auto &shader = static_cast<wgui::ShaderText&>(*m_shader.get());
	//prosper::util::record_set_viewport(*drawCmd,w,h);
	//prosper::util::record_set_scissor(*drawCmd,w,h);

	auto glyphMap = m_font->GetGlyphMap();
	auto glyphMapExtents = (*glyphMap->GetImage())->get_image_extent_2D(0u);
	auto maxGlyphBitmapWidth = m_font->GetMaxGlyphBitmapWidth();

	wgui::ShaderText::PushConstants pushConstants {
		sx,sy,glyphMapExtents.width,glyphMapExtents.height,maxGlyphBitmapWidth
	};
	const auto fDraw = [&context,&drawCmd,&dev,&shader,&bufBounds,&pushConstants,sx,sy,numChars,w,h,this](prosper::RenderTarget &rt,bool bClear,uint32_t vpWidth,uint32_t vpHeight) {
		auto &img = rt.GetTexture()->GetImage();

		auto &primCmd = static_cast<Anvil::PrimaryCommandBuffer&>(drawCmd->GetAnvilCommandBuffer());

		prosper::util::record_image_barrier(
			primCmd,img->GetAnvilImage(),
			Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT | Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
			Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
			Anvil::AccessFlagBits::SHADER_READ_BIT | Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT
		);

		prosper::util::record_begin_render_pass(primCmd,rt);
			prosper::util::record_clear_attachment(primCmd,**img,std::array<float,4>{0.f,0.f,0.f,0.f});
			if(shader.BeginDraw(drawCmd,w,h) == true)
			{
				prosper::util::record_set_viewport(primCmd,vpWidth,vpHeight);
				prosper::util::record_set_scissor(primCmd,vpWidth,vpHeight);
				auto descSet = m_font->GetGlyphMapDescriptorSet();
				shader.Draw(*bufBounds,*descSet,pushConstants,numChars);
				shader.EndDraw();
			}
		prosper::util::record_end_render_pass(primCmd);

		prosper::util::record_image_barrier(
			primCmd,img->GetAnvilImage(),
			Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT | Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
			Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
			Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT | Anvil::AccessFlagBits::SHADER_READ_BIT
		);
	};

	// Render Shadow
	if(m_shadow.enabled)
	{
		fDraw(*m_shadowRenderTarget,true,w,h); // TODO: Render text shadow shadow at the same time? (Single framebuffer)

		if(m_shadow.blurSize != 0.f && m_shadowBlurSet != nullptr)
		{
			prosper::util::record_blur_image(dev,drawCmd,*m_shadowBlurSet,{
				Vector4(2.f,1.f,1.f,1.f),
				m_shadow.blurSize,
				9
			});
		}
		//prosper::util::record_set_viewport(*drawCmd,m_wTexture,m_hTexture);
	}
	//
	
	// Render Text
	fDraw(*m_renderTarget,false,w,h);
	//

	CallCallbacks<void,std::reference_wrapper<const std::shared_ptr<prosper::RenderTarget>>>("OnTextRendered",std::reference_wrapper<const std::shared_ptr<prosper::RenderTarget>>(m_renderTarget));

	//uto windowSize = context.GetWindowSize();
	//prosper::util::record_set_viewport(*drawCmd,windowSize.at(0),windowSize.at(1));
	//prosper::util::record_set_scissor(*drawCmd,windowSize.at(0),windowSize.at(1));
}

void WIText::InitializeTextBuffers()
{
	util::text::LineIndex lineIndex = 0;
	for(auto &lineInfo : m_lineInfos)
	{
		if(lineInfo.bufferUpdateRequired == true)
			InitializeTextBuffers(lineInfo,lineIndex);
		if(lineInfo.subLines.empty() == false)
			lineIndex += lineInfo.subLines.size();
		else
			++lineIndex;
	}
}

void WIText::InitializeTextBuffers(LineInfo &lineInfo,util::text::LineIndex lineIndex)
{
	lineInfo.bufferUpdateRequired = false;
	if(lineInfo.wpLine.expired() == true)
	{
		lineInfo.buffers.clear();
		return;
	}
	auto &context = WGUI::GetInstance().GetContext();
	//auto fontHeight = m_font->GetMaxGlyphTop();//m_font->GetMaxGlyphSize()

	// Initialize Buffers
	struct SubStringInfo
	{
		std::string_view subString;
		size_t hash;
		uint32_t width;
		uint32_t height;
		float sx;
		float sy;
		util::text::CharOffset charOffset;
		std::vector<Vector4> glyphBounds;
		std::vector<uint32_t> glyphIndices;
		util::text::LineIndex absLineIndex;
	};

	std::vector<SubStringInfo> subStrings {};
	auto numChars = m_text->GetCharCount();

	const auto fAddSubString = [this,&subStrings](const std::string_view &substr,util::text::LineIndex lineIndex,util::text::CharOffset charOffset,uint32_t &inOutX,uint32_t &inOutY) {
		if(substr.empty())
			return;
		if(subStrings.size() == subStrings.capacity())
			subStrings.reserve(subStrings.size() +20);
		subStrings.push_back({});
		auto &info = subStrings.back();
		info.subString = substr;
		info.charOffset = charOffset;
		info.hash = std::hash<std::string_view>{}(info.subString);
		info.absLineIndex = lineIndex;

		int w,h;
		GetTextSize(&w,&h,&info.subString);
		if(w <= 0)
			w = 1;
		if(h <= 0)
			h = 1;
		info.width = w;
		info.height = h;
		auto sx = info.sx = 2.f /float(w);
		auto sy = info.sy = 2.f /float(h);

		info.glyphBounds.reserve(substr.length());
		info.glyphIndices.reserve(substr.length());

		// Populate glyph bounds and indices
		auto fontSize = m_font->GetSize();
		auto offset = 0u;
		for(auto c : info.subString)
		{
			auto multiplier = 1u;
			if(c == '\t')
			{
				auto tabSpaceCount = FontManager::TAB_WIDTH_SPACE_COUNT -(offset %FontManager::TAB_WIDTH_SPACE_COUNT);
				multiplier = tabSpaceCount;
				c = ' ';
			}
			auto *glyph = m_font->GetGlyphInfo(c);
			if(glyph == nullptr)
				continue;
			int32_t left,top,width,height;
			glyph->GetDimensions(left,top,width,height);
			width *= multiplier;
			int32_t advanceX,advanceY;
			glyph->GetAdvance(advanceX,advanceY);

			auto x2 = (inOutX +left) *sx -1.f;
			auto y2 = (inOutY -(top -static_cast<int>(fontSize))) *sy -1.f;
			auto w = width *sx;
			auto h = height *sy;

			info.glyphBounds.push_back({x2,y2,width,height});
			info.glyphIndices.push_back(FontInfo::CharToGlyphMapIndex(c));

			advanceX >>= 6;
			advanceX *= multiplier;
			advanceY >>= 6;
			inOutX += advanceX;
			inOutY += advanceY;
			offset += multiplier;
		}
	};
	auto y = 0u; // lineIndex *GetLineHeight();
	auto x = 0u;
	auto pLine = lineInfo.wpLine.lock();
	auto lineView = std::string_view{pLine->GetFormattedLine()};
	auto subString = lineView;
	util::text::CharOffset offset = 0;
	auto subLines = lineInfo.subLines;
	while(subString.empty() == false)
	{
		auto curLineIndex = lineIndex;
		auto numChars = std::min<uint32_t>(MAX_CHARS_PER_BUFFER,subString.length());
		auto newLine = false;
		if(subLines.empty() == false)
		{
			auto &numCharsSubLine = subLines.front();
			numChars = std::min<uint32_t>(numCharsSubLine,numChars);
			numCharsSubLine -= numChars;
			if(numCharsSubLine == 0)
			{
				newLine = true;
				subLines.erase(subLines.begin());
				++lineIndex;
			}
		}
		fAddSubString(subString.substr(0ull,numChars),curLineIndex,offset,x,y);
		subString = subString.substr(numChars);
		offset += numChars;
		if(newLine)
			x = 0;
	}

	lineInfo.buffers.clear();
	auto bufferOffset = 0u;
	for(auto &subStrInfo : subStrings)
	{
		ScopeGuard sg{[this,&subStrInfo,&bufferOffset]() {
			++bufferOffset;
		}};

		auto bExistingBuffer = bufferOffset < lineInfo.buffers.size();
		if(bExistingBuffer == true)
		{
			/*auto &curBufInfo = lineInfo.buffers.at(bufferOffset);
			if(
				curBufInfo.subStringHash == subStrInfo.hash &&
				curBufInfo.width == subStrInfo.width && curBufInfo.height == subStrInfo.height &&
				curBufInfo.sx == subStrInfo.sx && curBufInfo.sy == subStrInfo.sy &&
				curBufInfo.range == subStrInfo.range
			)
				continue; // No need to update the buffer
				*/
		}
		std::vector<GlyphBoundsInfo> glyphBoundsData {};
		glyphBoundsData.reserve(subStrInfo.glyphBounds.size());
		for(auto i=decltype(subStrInfo.glyphBounds.size()){0u};i<subStrInfo.glyphBounds.size();++i)
		{
			auto &bounds = subStrInfo.glyphBounds.at(i);
			auto index = subStrInfo.glyphIndices.at(i);
			glyphBoundsData.push_back({static_cast<int32_t>(index),bounds});
		}
		if(bExistingBuffer == false)
			lineInfo.buffers.push_back({s_textBuffer->AllocateBuffer()});
		// Update existing buffer
		auto &bufInfo = lineInfo.buffers.at(bufferOffset);
		// bufInfo.subStringHash = subStrInfo.hash;
		bufInfo.numChars = subStrInfo.subString.length();
		bufInfo.charOffset = subStrInfo.charOffset;
		bufInfo.absLineIndex = subStrInfo.absLineIndex;
		bufInfo.colorBuffer = nullptr;

		bufInfo.width = subStrInfo.width;
		bufInfo.height = subStrInfo.height;
		bufInfo.sx = subStrInfo.sx;
		bufInfo.sy = subStrInfo.sy;
		context.ScheduleRecordUpdateBuffer(
			bufInfo.buffer,
			0ull,glyphBoundsData.size() *sizeof(glyphBoundsData.front()),glyphBoundsData.data()
		);

		prosper::util::record_buffer_barrier(
			**context.GetDrawCommandBuffer(),*bufInfo.buffer,
			Anvil::PipelineStageFlagBits::TRANSFER_BIT,Anvil::PipelineStageFlagBits::VERTEX_INPUT_BIT,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,Anvil::AccessFlagBits::VERTEX_ATTRIBUTE_READ_BIT
		);
	}
	lineInfo.buffers.resize(bufferOffset);
}

void WIText::RemoveDecorator(const WITextDecorator &decorator)
{
	auto it = std::find_if(m_tagInfos.begin(),m_tagInfos.end(),[&decorator](const std::shared_ptr<WITextDecorator> &decoOther) {
		return decoOther.get() == &decorator;
	});
	if(it == m_tagInfos.end())
		return;
	m_tagInfos.erase(it);
}

void WIText::InitializeTextBuffer(prosper::Context &context)
{
	if(s_textBuffer != nullptr)
		return;
	const auto maxInstances = 8'192; // 5 MiB total space
	auto instanceSize = sizeof(GlyphBoundsInfo) *MAX_CHARS_PER_BUFFER;
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.usageFlags = Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
	createInfo.size = instanceSize *maxInstances;
	s_textBuffer = prosper::util::create_uniform_resizable_buffer(context,createInfo,instanceSize,createInfo.size *5u,0.05f);
	s_textBuffer->SetPermanentlyMapped(true);
	s_textBuffer->SetDebugName("text_glyph_bounds_info_buf");
}
void WIText::ClearTextBuffer()
{
	s_textBuffer = nullptr;
	WITextTagColor::ClearColorBuffer();
}
void WITextBase::SetTextElement(WIText &elText)
{
	m_hText = elText.GetHandle();
}
void WITextBase::InitializeTexture(prosper::Texture &tex,int32_t w,int32_t h)
{
	if(m_hTexture.IsValid())
	{
		static_cast<WITexturedRect*>(m_hTexture.get())->SetTexture(tex);
		return;
	}
	auto *pEl = WGUI::GetInstance().Create<WITexturedRect>(this);
	pEl->SetAlphaOnly(true);
	pEl->SetZPos(1);
	pEl->SetAlpha(0.f);
	pEl->GetColorProperty()->Link(*GetColorProperty());
	pEl->SetAutoAlignToParent(true);
	pEl->SetSize(w,h);
	m_hTexture = pEl->GetHandle();
}

bool WITextBase::RenderLines(
	wgui::ShaderTextRect &shader,int32_t width,int32_t height,
	const Vector2i &absPos,const Mat4 &transform,const Vector2i &origin,
	const Mat4 &matParent,Vector2i &inOutSize,
	wgui::ShaderTextRect::PushConstants &inOutPushConstants,
	const std::function<void(const SubBufferInfo&,Anvil::DescriptorSet&)> &fDraw,
	bool colorPass
) const
{
	auto &textEl = static_cast<WIText&>(*m_hText.get());
	auto &context = WGUI::GetInstance().GetContext();
	auto &drawCmd = context.GetDrawCommandBuffer();
	if(shader.BeginDraw(drawCmd,width,height) == false)
		return false;
	uint32_t xScissor,yScissor,wScissor,hScissor;
	WGUI::GetInstance().GetScissor(xScissor,yScissor,wScissor,hScissor);

	auto bHasColorBuffers = false;
	auto *descSet = textEl.m_font->GetGlyphMapDescriptorSet();
	auto lineHeight = textEl.GetLineHeight();
	uint32_t lineIndex = 0;
	for(auto &lineInfo : textEl.m_lineInfos)
	{
		if(lineInfo.buffers.empty())
			continue;
		auto &firstBufferInfo = lineInfo.buffers.front();
		auto firstLineIndex = firstBufferInfo.absLineIndex;

		auto &lastBufferInfo = lineInfo.buffers.back();
		auto lastLineIndex = lastBufferInfo.absLineIndex;

		int32_t yLineStartPxOffset = absPos.y +(static_cast<int32_t>(firstLineIndex) +1) *lineHeight;
		int32_t yLineEndPxOffset = absPos.y +(static_cast<int32_t>(lastLineIndex) +1) *lineHeight;

		if(yLineEndPxOffset < static_cast<int32_t>(yScissor))
			continue; // Line is out of scissor bounds; We don't need to render it
		if(yLineStartPxOffset >= static_cast<int32_t>(yScissor +hScissor))
			break; // Line is out of scissor bounds; We can skip all remaining lines altogether

		for(auto &bufInfo : lineInfo.buffers)
		{
			if(bufInfo.colorBuffer != nullptr)
			{
				bHasColorBuffers = true;
				if(colorPass == false)
					continue;
			}
			else if(colorPass)
				continue;
			inOutPushConstants.fontInfo.widthScale = bufInfo.sx;
			inOutPushConstants.fontInfo.heightScale = bufInfo.sy;

			inOutSize.x = bufInfo.width;
			inOutSize.y = bufInfo.height;

			// Temporarily change size to that of the text (instead of the element) to make sure GetTransformedMatrix returns the right matrix.
			// This will be reset further below.
			auto matText = GetTransformedMatrix(origin,width,height,matParent);
			inOutPushConstants.elementData.modelMatrix = matText;

			inOutPushConstants.fontInfo.yOffset = bufInfo.absLineIndex *lineHeight;

			fDraw(bufInfo,*descSet);
		}
	}
	shader.EndDraw();
	return bHasColorBuffers;
}

void WITextBase::RenderLines(
	int32_t width,int32_t height,
	const Vector2i &absPos,const Mat4 &transform,const Vector2i &origin,
	const Mat4 &matParent,Vector2i &inOutSize,
	wgui::ShaderTextRect::PushConstants &inOutPushConstants
) const
{
	auto *pShaderTextRect = WGUI::GetInstance().GetTextRectShader();
	auto bHasColorBuffers = RenderLines(*pShaderTextRect,width,height,absPos,transform,origin,matParent,inOutSize,inOutPushConstants,[&inOutPushConstants,pShaderTextRect](const SubBufferInfo &bufInfo,Anvil::DescriptorSet &descSet) {
		pShaderTextRect->Draw(*bufInfo.buffer,descSet,inOutPushConstants,bufInfo.numChars);
	},false);
	if(bHasColorBuffers == false)
		return;
	auto *pShaderTextRectColor = WGUI::GetInstance().GetTextRectColorShader();
	RenderLines(*pShaderTextRectColor,width,height,absPos,transform,origin,matParent,inOutSize,inOutPushConstants,[&inOutPushConstants,pShaderTextRectColor](const SubBufferInfo &bufInfo,Anvil::DescriptorSet &descSet) {
		pShaderTextRectColor->Draw(*bufInfo.buffer,*bufInfo.colorBuffer,descSet,inOutPushConstants,bufInfo.numChars);
	},true);
}

void WITextBase::Render(int width,int height,const Mat4 &mat,const Vector2i &origin,const Mat4 &matParent)
{
	WIBase::Render(width,height,mat,origin,matParent);
	if(m_hText.IsValid() == false)
		return;
	auto &textEl = static_cast<WIText&>(*m_hText.get());
	if(textEl.m_renderTarget != nullptr && textEl.IsCacheEnabled() == true)
		return;
	auto *pShaderTextRect = WGUI::GetInstance().GetTextRectShader();
	auto *pShaderTextRectColor = WGUI::GetInstance().GetTextRectColorShader();
	if(pShaderTextRect != nullptr)
	{
		auto *pFont = textEl.GetFont();
		if(pFont == nullptr)
			return;
		auto &context = WGUI::GetInstance().GetContext();
		for(auto &lineInfo : textEl.m_lineInfos)
		{
			for(auto &bufInfo : lineInfo.buffers)
			{
				context.KeepResourceAliveUntilPresentationComplete(bufInfo.buffer);
				if(bufInfo.colorBuffer)
					context.KeepResourceAliveUntilPresentationComplete(bufInfo.colorBuffer);
			}
		}

		auto drawCmd = context.GetDrawCommandBuffer();
		auto glyphMap = pFont->GetGlyphMap();
		auto glyphMapExtents = (*glyphMap->GetImage())->get_image_extent_2D(0u);
		auto maxGlyphBitmapWidth = pFont->GetMaxGlyphBitmapWidth();

		auto col = GetColor().ToVector4();
		col.a *= WIBase::RENDER_ALPHA;
		if(col.a <= 0.f && umath::is_flag_set(m_stateFlags,StateFlags::RenderIfZeroAlpha) == false)
			return;

		auto currentSize = GetSizeProperty()->GetValue();
		auto &size = GetSizeProperty()->GetValue();

		//auto matText = GetTransformedMatrix(origin,width,height,matParent);

		wgui::ShaderTextRect::PushConstants pushConstants {
			wgui::ElementData{Mat4{},col},
			0.f,0.f,glyphMapExtents.width,glyphMapExtents.height,maxGlyphBitmapWidth
		};
		Vector2i absPos,absSize;
		CalcBounds(mat,width,height,absPos,absSize);
		const auto fDraw = [&context,&drawCmd,&pushConstants,&size,&origin,&mat,&matParent,width,height,pFont,this,&textEl,&absPos,&absSize](bool bClear) {
			RenderLines(width,height,absPos,mat,origin,matParent,size,pushConstants);
		};

		// Render Shadow
		if(textEl.m_shadow.enabled)
		{
			auto *pShadowColor = textEl.GetShadowColor();
			if(pShadowColor != nullptr && pShadowColor->w > 0.f)
			{
				auto *pOffset = textEl.GetShadowOffset();
				auto currentPos = GetPosProperty()->GetValue();
				auto &pos = GetPosProperty()->GetValue();
				if(pOffset != nullptr)
				{
					pos.x += pOffset->x;
					pos.y += pOffset->y;
				}
				auto tmpMatrix = pushConstants.elementData.modelMatrix;
				auto tmpColor = pushConstants.elementData.color;
				pushConstants.elementData.modelMatrix = GetTransformedMatrix(origin,width,height,matParent);
				if(pShadowColor != nullptr)
					pushConstants.elementData.color = *pShadowColor;
				fDraw(true); // TODO: Render text shadow shadow at the same time? (Single framebuffer)
				pos = currentPos;
				pushConstants.elementData.modelMatrix = tmpMatrix;
				pushConstants.elementData.color = tmpColor;

				/*if(m_shadow.blurSize != 0.f && m_shadowBlurSet != nullptr)
				{
					prosper::util::record_blur_image(context.GetDevice(),drawCmd,*m_shadowBlurSet,{
						Vector4(2.f,1.f,1.f,1.f),
						m_shadow.blurSize,
						9
					});
				}*/
				//prosper::util::record_set_viewport(*drawCmd,m_wTexture,m_hTexture);
			}
		}
		//
	
		// Render Text
		fDraw(false);
		//

		// Reset size
		size = currentSize;
	}
}
