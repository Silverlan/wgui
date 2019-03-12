/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/types/witext.h"
#include "wgui/types/wirect.h"
#include "wgui/shaders/wishader_text.hpp"
#include <prosper_context.hpp>
#include <image/prosper_sampler.hpp>
#include <prosper_util.hpp>
#include <prosper_util_square_shape.hpp>
#include <shader/prosper_shader_blur.hpp>
#include <prosper_command_buffer.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <sharedutils/property/util_property_color.hpp>

LINK_WGUI_TO_CLASS(WIText,WIText);

WIText::WIText()
	: WIBase(),m_font(nullptr),m_breakHeight(0),m_wTexture(0),m_hTexture(0),
	m_autoBreak(AutoBreak::NONE),m_renderTarget(nullptr),
	m_text(util::StringProperty::Create())
{
	SetColor(Color(0,0,0,255));

	SetText("");
	SetFont(FontManager::GetDefaultFont().get());

	auto &shaderManager = WGUI::GetInstance().GetContext().GetShaderManager();
	m_shader = shaderManager.GetShader("wguitext");

	RegisterCallback<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged");
	RegisterCallback<void,const FontInfo*>("OnFontChanged");
	RegisterCallback<void,std::reference_wrapper<const std::shared_ptr<prosper::RenderTarget>>>("OnTextRendered");

	InitializeTextBuffer(WGUI::GetInstance().GetContext());
}

WIText::~WIText()
{
	auto &context = WGUI::GetInstance().GetContext();
	context.KeepResourceAliveUntilPresentationComplete(m_renderTarget);
	context.KeepResourceAliveUntilPresentationComplete(m_shadowRenderTarget);
	context.KeepResourceAliveUntilPresentationComplete(m_shadowBlurSet);

	DestroyShadow();
	DestroyBlur();
}

std::string WIText::GetDebugInfo() const {return "Text: " +GetText();}

 void WIText::Initialize()
{
	WIBase::Initialize();
	m_baseText = CreateChild<WITexturedRect>();
	WITexturedRect *rect = m_baseText.get<WITexturedRect>();
	if(rect != NULL)
	{
		rect->SetAlphaOnly(true);
		rect->SetZPos(1);
		rect->GetColorProperty()->Link(*GetColorProperty());
	}
}

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
void WIText::SetSize(int x,int y)
{
	WIBase::SetSize(x,y);
	if(m_autoBreak != AutoBreak::NONE)
		SizeToContents();
}

void WIText::ScheduleRenderUpdate(bool bFull)
{
	if(bFull)
		m_flags |= Flags::FullUpdateScheduled;
	else
		m_flags |= Flags::RenderTextScheduled;
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

const FontInfo *WIText::GetFont() const {return m_font.get();}
void WIText::GetLines(std::vector<std::string> **lines) {*lines = &m_lines;}
std::string *WIText::GetLine(int line)
{
	if(line < 0 || line >= m_lines.size())
		return NULL;
	return &m_lines[line];
}
int WIText::GetLineCount() {return static_cast<int>(m_lines.size());}
int WIText::GetLineHeight() {return m_font->GetSize() +m_breakHeight;}
int WIText::GetBreakHeight() {return m_breakHeight;}
void WIText::SetBreakHeight(int breakHeight) {m_breakHeight = breakHeight;}
const util::PStringProperty &WIText::GetTextProperty() const {return m_text;}
const std::string &WIText::GetText() const {return *m_text;}
void WIText::SetText(std::string text)
{
	if(text == **m_text)
		return;
	ScheduleRenderUpdate(true);
	std::string oldText = *m_text;
	*m_text = text;
	std::string cur = "";
	unsigned int pos = 0;
	unsigned int l = static_cast<unsigned int>(text.length());
	unsigned int inl = 1;
	while(pos < l)
	{
		if(text[pos] == '\n')
		{
			if(m_lines.size() < inl)
				m_lines.push_back(cur);
			else
				m_lines[inl -1] = cur;
			cur = "";
			inl++;
		}
		else
			cur += text[pos];
		pos++;
	}
	if(m_lines.size() < inl)
		m_lines.push_back(cur);
	else
		m_lines[inl -1] = cur;
	if(!m_lines.empty())
	{
		for(unsigned int i=static_cast<unsigned int>(m_lines.size()) -1;i>=inl;i--)
			m_lines.pop_back();
	}
	if(m_autoBreak != AutoBreak::NONE)
	{
		auto w = GetWidth();
		if(w > 0)
		{
			int wText = 0;
			int hText = 0;
			auto *font = GetFont();
			for(auto it=m_lines.begin();it!=m_lines.end();)
			{
				auto &line = *it;
				auto len = line.size() -1;
				auto i = line.size();
				auto breakMode = m_autoBreak;
				FontManager::GetTextSize(line.c_str(),font,&wText,&hText);
				while(wText > w && i > 0)
				{
					i--;
					if(breakMode == AutoBreak::ANY || line[i] == ' ' || line[i] == '\f' || line[i] == '\v')
						FontManager::GetTextSize(line.substr(0,i).c_str(),font,&wText,&hText);
					else if(i == 0 && breakMode == AutoBreak::WHITESPACE)
					{
						i = line.size();
						breakMode = AutoBreak::ANY;
					}
				}
				if(i < len && i > 0)
				{
					it = m_lines.insert(it,line.substr(0,i));
					++it;
					*it = it->substr((breakMode == AutoBreak::ANY) ? i : (i +1),it->length());
				}
				else
					++it;
			}
		}
	}
	CallCallbacks<void,std::reference_wrapper<const std::string>,std::reference_wrapper<const std::string>>("OnTextChanged",std::reference_wrapper<const std::string>(oldText),std::reference_wrapper<const std::string>(text));
}
void WIText::SetFont(const std::string &font) {SetFont(FontManager::GetFont(font.c_str()).get());}
void WIText::SetFont(const FontInfo *font)
{
	if(m_font.get() == font)
		return;
	m_font = (font != nullptr) ? font->shared_from_this() : nullptr;
	SetText(*m_text);
	CallCallbacks<void,const FontInfo*>("OnFontChanged",font);
}

void WIText::SelectShader()
{
	// Deprecated?
}

Mat4 WIText::GetTransformedMatrix(const Vector2i &origin,int w,int h,Mat4 mat)
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

void WIText::SetCacheEnabled(bool bEnabled)
{
	umath::set_flag(m_flags,Flags::Cache,bEnabled);
	if(bEnabled == true || m_renderTarget == nullptr)
		return;
	WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_renderTarget);
	m_renderTarget = nullptr;
}
bool WIText::IsCacheEnabled() const {return umath::is_flag_set(m_flags,Flags::Cache);}

int WIText::GetTextHeight()
{
	if(m_font == nullptr)
		return 0;
	auto h = m_font->GetSize();
	auto numLines=  static_cast<int>(m_lines.size());
	return numLines *h +(((numLines > 0) ? (numLines -1) : 0) *m_breakHeight);
}

void WIText::GetTextSize(int *w,int *h)
{
	if(m_font == nullptr)
	{
		*w = 0;
		*h = 0;
		return;
	}
	int wText = 0;
	for(unsigned int i=0;i<m_lines.size();i++)
	{
		auto &line = m_lines[i];
		int w;
		FontManager::GetTextSize(line.c_str(),m_font.get(),&w);
		if(w > wText)
			wText = w;
	}
	int hText = m_font->GetMaxGlyphSize();//m_font->GetSize();
	*w = wText;
	*h = hText *static_cast<int>(m_lines.size()) +1;
}

void WIText::UpdateRenderTexture()
{
	if(IsCacheEnabled() == false)
	{
		if((m_flags &(Flags::RenderTextScheduled | Flags::FullUpdateScheduled)) != Flags::None)
		{
			m_flags &= ~(Flags::RenderTextScheduled | Flags::FullUpdateScheduled);
			InitializeTextBuffers();
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
	if(m_baseText.IsValid())
	{
		WITexturedRect *text = m_baseText.get<WITexturedRect>();
		if(text != NULL)
		{
			text->SetSize(w,h);
			text->SetTexture(*m_renderTarget->GetTexture());
		}
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
	auto numChars = (*m_text)->size();
	std::vector<GlyphBoundsInfo> glyphBoundsInfos;
	glyphBoundsInfos.reserve(numChars);
	if(numChars == 0)
		return;
	for(unsigned int i=0;i<m_lines.size();i++)
	{
		x = 0;
		auto &text = m_lines[i];
		for(unsigned int j=0;j<text.size();j++)
		{
			auto c = static_cast<UChar>(text[j]);
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
	//

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

static const auto MAX_CHARS_PER_BUFFER = 32u;
static std::shared_ptr<prosper::UniformResizableBuffer> s_textBuffer = nullptr;
void WIText::InitializeTextBuffers()
{
	auto &context = WGUI::GetInstance().GetContext();
	int w,h;
	GetTextSize(&w,&h);
	if(w <= 0)
		w = 1;
	if(h <= 0)
		h = 1;
	m_textBufferInfo.width = w;
	m_textBufferInfo.height = h;
	auto sx = m_textBufferInfo.sx = 2.f /float(w);
	auto sy = m_textBufferInfo.sy = 2.f /float(h);
	//auto fontHeight = m_font->GetMaxGlyphTop();//m_font->GetMaxGlyphSize()
	auto fontSize = m_font->GetSize();

	// Initialize Buffers
	float x = 0;
	float y = 0;
	std::vector<Vector4> glyphBounds;
	std::vector<uint32_t> glyphIndices;
	std::vector<std::pair<std::string,size_t>> subStrings {};
	auto numChars = (*m_text)->size();
	subStrings.reserve((numChars /MAX_CHARS_PER_BUFFER) +1u);
	glyphBounds.reserve(numChars);
	glyphIndices.reserve(numChars);
	if(numChars == 0)
		return;
	for(unsigned int i=0;i<m_lines.size();i++)
	{
		x = 0;
		auto &text = m_lines[i];
		for(unsigned int j=0;j<text.size();j++)
		{
			auto c = static_cast<UChar>(text[j]);
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

				glyphBounds.push_back({x2,y2,width,height});
				glyphIndices.push_back(FontInfo::CharToGlyphMapIndex(c));
				if(subStrings.empty() || subStrings.back().first.size() >= MAX_CHARS_PER_BUFFER)
				{
					subStrings.push_back({});
					subStrings.back().first.reserve(MAX_CHARS_PER_BUFFER);
				}
				subStrings.back().first += c;

				x += (advanceX >> 6) *sx;
				y += (advanceY >> 6) *sy;
			}
		}
		y += GetLineHeight() *sy;
	}
	numChars = glyphBounds.size();
	assert(numChars != 0);
	if(numChars == 0)
		return;
	m_textBufferInfo.numChars = numChars;

	if(subStrings.size() != (numChars /MAX_CHARS_PER_BUFFER) +((numChars %MAX_CHARS_PER_BUFFER) > 0 ? 1u : 0u))
		throw std::logic_error("Unexpected size mismatch!");
	for(auto &info : subStrings)
		info.second = std::hash<std::string>{}(info.first);

	if(m_textBufferInfo.glyphInfoBufferInfos.size() > subStrings.size())
		m_textBufferInfo.glyphInfoBufferInfos.resize(subStrings.size());
	m_textBufferInfo.glyphInfoBufferInfos.reserve(subStrings.size());
	for(auto i=decltype(numChars){0u};i<numChars;i+=MAX_CHARS_PER_BUFFER)
	{
		auto bufferIdx = i /MAX_CHARS_PER_BUFFER;
		auto &subStrInfo = subStrings.at(bufferIdx);
		auto hash = subStrInfo.second;
		auto bExistingBuffer = bufferIdx < m_textBufferInfo.glyphInfoBufferInfos.size();
		if(bExistingBuffer == true)
		{
			auto &curBufInfo = m_textBufferInfo.glyphInfoBufferInfos.at(bufferIdx);
			if(curBufInfo.subStringHash == hash)
				continue; // No need to update the buffer
		}
		auto numCharsInstance = umath::min(static_cast<uint32_t>(numChars -i),MAX_CHARS_PER_BUFFER);
		std::vector<GlyphBoundsInfo> glyphBoundsData {};
		glyphBoundsData.resize(numCharsInstance);

		for(auto j=i;j<(i +numCharsInstance);++j)
		{
			auto &boundsInfo = glyphBoundsData.at(j -i);
			boundsInfo.bounds = glyphBounds.at(j);
			boundsInfo.index = glyphIndices.at(j);
		}
		if(bExistingBuffer == false)
			m_textBufferInfo.glyphInfoBufferInfos.push_back({s_textBuffer->AllocateBuffer(),hash});
		// Update existing buffer
		auto &buf = m_textBufferInfo.glyphInfoBufferInfos.at(bufferIdx).buffer;
		context.ScheduleRecordUpdateBuffer(
			buf,
			0ull,glyphBoundsData.size() *sizeof(glyphBoundsData.front()),glyphBoundsData.data()
		);
		prosper::util::record_buffer_barrier(
			**context.GetDrawCommandBuffer(),*buf,
			Anvil::PipelineStageFlagBits::TRANSFER_BIT,Anvil::PipelineStageFlagBits::VERTEX_INPUT_BIT,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,Anvil::AccessFlagBits::INDEX_READ_BIT
		);
	}
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
}

void WIText::Render(int width,int height,const Mat4 &mat,const Vector2i &origin,const Mat4 &matParent)
{
	WIBase::Render(width,height,mat,origin,matParent);
	if(m_renderTarget != nullptr && IsCacheEnabled() == true)
		return;
	auto *pShaderTextRect = WGUI::GetInstance().GetTextRectShader();
	if(pShaderTextRect != nullptr)
	{
		auto *pFont = GetFont();
		if(m_textBufferInfo.glyphInfoBufferInfos.empty() || pFont == nullptr)
			return;
		auto &context = WGUI::GetInstance().GetContext();
		for(auto &bufInfo : m_textBufferInfo.glyphInfoBufferInfos)
			context.KeepResourceAliveUntilPresentationComplete(bufInfo.buffer);

		auto drawCmd = context.GetDrawCommandBuffer();
		auto &shader = *pShaderTextRect;

		auto glyphMap = pFont->GetGlyphMap();
		auto glyphMapExtents = (*glyphMap->GetImage())->get_image_extent_2D(0u);
		auto maxGlyphBitmapWidth = pFont->GetMaxGlyphBitmapWidth();

		auto col = GetColor().ToVector4();
		col.a *= WIBase::RENDER_ALPHA;
		if(col.a <= 0.f)
			return;

		auto currentSize = GetSizeProperty()->GetValue();
		auto &size = GetSizeProperty()->GetValue();
		// Temporarily change size to that of the text (instead of the element) to make sure GetTransformedMatrix returns the right matrix.
		// This will be reset further below.
		size.x = m_textBufferInfo.width;
		size.y = m_textBufferInfo.height;

		auto matText = GetTransformedMatrix(origin,width,height,matParent);

		wgui::ShaderTextRect::PushConstants pushConstants {
			wgui::ElementData{matText,col},
			m_textBufferInfo.sx,m_textBufferInfo.sy,glyphMapExtents.width,glyphMapExtents.height,maxGlyphBitmapWidth
		};
		const auto fDraw = [&context,&drawCmd,&shader,&pushConstants,width,height,pFont,this](bool bClear,uint32_t vpWidth,uint32_t vpHeight) {
			if(shader.BeginDraw(drawCmd,width,height) == true)
			{
				auto descSet = m_font->GetGlyphMapDescriptorSet();
				for(auto &bufInfo : m_textBufferInfo.glyphInfoBufferInfos)
					shader.Draw(*bufInfo.buffer,*descSet,pushConstants,m_textBufferInfo.numChars);
				shader.EndDraw();
			}
		};

		// Render Shadow
		if(m_shadow.enabled)
		{
			auto *pShadowColor = GetShadowColor();
			if(pShadowColor != nullptr && pShadowColor->w > 0.f)
			{
				auto *pOffset = GetShadowOffset();
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
				fDraw(true,m_textBufferInfo.width,m_textBufferInfo.height); // TODO: Render text shadow shadow at the same time? (Single framebuffer)
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
		fDraw(false,m_textBufferInfo.width,m_textBufferInfo.height);
		//

		// Reset size
		size = currentSize;
	}
}

void WIText::Think()
{
	WIBase::Think();
	UpdateRenderTexture();
}

void WIText::Update()
{
	WIBase::Update();
	
}

void WIText::SizeToContents()
{
	if(m_autoBreak != AutoBreak::NONE)
	{
		SetText(GetText());
		WIBase::SetSize(GetWidth(),GetTextHeight());
		return;
	}
	int w,h;
	GetTextSize(&w,&h);
	if(IsShadowEnabled() == true)
	{
		auto &shadowOffset = *GetShadowOffset();
		w += shadowOffset.x;
		h += shadowOffset.y;
	}
	SetSize(w,h);
}

void WIText::SetAutoBreakMode(AutoBreak b)
{
	if(b == m_autoBreak)
		return;
	m_autoBreak = b;
	SizeToContents();
}
