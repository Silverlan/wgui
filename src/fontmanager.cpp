/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/fontmanager.h"
#include <fsys/filesystem.h>
#include "wgui/shaders/wishader_text.hpp"
#include <sharedutils/util_file.h>
#include <prosper_util.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <prosper_command_buffer.hpp>
#include <image/prosper_sampler.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_context.hpp>

#include FT_GLYPH_H
#include FT_OUTLINE_H

enum class GlyphRange : uint32_t
{
	Start = 32,
	End = 255,
	Count = End -Start
};
//#define FONT_GLYPH_END 126

FontInfo::Face::~Face()
{
	if(m_ftFace != nullptr)
		FT_Done_Face(m_ftFace);
}
const FT_Face &FontInfo::Face::GetFtFace() const {return m_ftFace;}

GlyphInfo::GlyphInfo()
{
	for(unsigned int i=0;i<4;i++)
		m_bbox[i] = 0;
}

void GlyphInfo::GetAdvance(int32_t &advanceX,int32_t &advanceY) const
{
	advanceX = m_advanceX;
	advanceY = m_advanceY;
}

void GlyphInfo::Initialize(FT_GlyphSlot glyph)
{
	if(m_bInitialized)
		return;
	m_bInitialized = true;
	m_left = glyph->bitmap_left;
	m_top = glyph->bitmap_top;
	m_width = glyph->bitmap.width;
	m_height = glyph->bitmap.rows;
	m_advanceX = glyph->advance.x;
	m_advanceY = glyph->advance.y;

	FT_Glyph g;
	FT_Get_Glyph(glyph,&g);

	FT_BBox bbox;
	FT_Glyph_Get_CBox(g,FT_GLYPH_BBOX_TRUNCATE,&bbox);
	m_bbox[0] = bbox.xMin;
	m_bbox[1] = bbox.yMin;
	m_bbox[2] = bbox.xMax;
	m_bbox[3] = bbox.yMax;

	FT_Done_Glyph(g);
}

void GlyphInfo::GetBounds(int32_t *xMin,int32_t *yMin,int32_t *xMax,int32_t *yMax) const
{
	if(xMin != nullptr)
		*xMin = m_bbox[0];
	if(yMin != nullptr)
		*yMin = m_bbox[1];
	if(xMax != nullptr)
		*xMax = m_bbox[2];
	if(yMax != nullptr)
		*yMax = m_bbox[3];
}

void GlyphInfo::GetDimensions(int32_t &left,int32_t &top,int32_t &width,int32_t &height) const
{
	left = m_left;
	top = m_top;
	width = m_width;
	height = m_height;
}
int32_t GlyphInfo::GetTop() const {return m_top;}
int32_t GlyphInfo::GetLeft() const {return m_left;}
int32_t GlyphInfo::GetWidth() const {return m_width;}
int32_t GlyphInfo::GetHeight() const {return m_height;}

/////////////////////

FontInfo::~FontInfo()
{
	Clear();
}
const GlyphInfo *FontInfo::GetGlyphInfo(char c) const
{
	auto idx = CharToGlyphMapIndex(c);
	if(idx < 0 || idx > umath::to_integral(GlyphRange::Count)) //94)
		return nullptr;
	return m_glyphs[idx].get();
}
const std::vector<std::shared_ptr<GlyphInfo>> &FontInfo::GetGlyphs() const {return m_glyphs;}
uint32_t FontInfo::GetSize() const {return m_size;}

bool FontInfo::Initialize(const std::string &cpath,uint32_t fontSize)
{
	if(m_bInitialized || wgui::ShaderText::DESCRIPTOR_SET_TEXTURE.IsValid() == false)
		return true;
	auto f = FileManager::OpenFile(cpath.c_str(),"rb");
	if(f == nullptr)
		return false;
	auto size = f->GetSize();
	m_data.resize(size);
	f->Read(m_data.data(),size);

	auto lib = FontManager::GetFontLibrary();
	auto &face = m_face.GetFtFace();
	auto &ncFace = const_cast<FT_Face&>(face);
	if(FT_New_Memory_Face(lib,&m_data[0],static_cast<FT_Long>(size),0,&ncFace) != 0 || FT_Set_Pixel_Sizes(face,0,fontSize) != 0)
	{
		m_data.clear();
		return false;
	}
	//FT_Select_Charmap(m_face,FT_ENCODING_UNICODE);
	m_size = fontSize;

	auto numGlyphs = umath::to_integral(GlyphRange::Count) +1;
	m_glyphs.resize(numGlyphs);
	uint32_t hMax = 0;
	uint32_t szMax = 0;

	m_maxBitmapWidth = 0;
	m_maxBitmapHeight = 0;
	auto &context = WGUI::GetInstance().GetContext();
	auto &dev = context.GetDevice();
	std::vector<std::shared_ptr<prosper::Image>> glyphImages(m_glyphs.size(),nullptr);
	std::vector<std::pair<uint32_t,uint32_t>> glyphBounds(m_glyphs.size());
	for(auto i=umath::to_integral(GlyphRange::Start);i<=umath::to_integral(GlyphRange::End);i++)
	{
		if(FT_Load_Char(face,i,FT_LOAD_RENDER) == 0)
		{
			auto gslot = face->glyph;
			// Outline
			/*if(gslot->format == FT_GLYPH_FORMAT_OUTLINE)
			{
				std::vector<Span> spans;
				RenderSpans(lib,gslot->outline,spans);
			}*/
			//
			auto &glyph = m_glyphs[i -umath::to_integral(GlyphRange::Start)] = std::shared_ptr<GlyphInfo>(new GlyphInfo());
			if(gslot->bitmap.width > 0 && gslot->bitmap.rows > 0)
			{
				prosper::util::ImageCreateInfo createInfo {};
				createInfo.format = Anvil::Format::R8_UNORM;
				createInfo.width = gslot->bitmap.width;
				createInfo.height = umath::next_power_of_2(gslot->bitmap.rows); // With has to be a power of 2!
				createInfo.tiling = Anvil::ImageTiling::LINEAR;
				createInfo.usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;
				createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL;
				createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::HostAccessable;

				std::vector<uint8_t> data(createInfo.width *createInfo.height);
				m_maxBitmapWidth = umath::max(m_maxBitmapWidth,gslot->bitmap.width);
				m_maxBitmapHeight = umath::max(m_maxBitmapHeight,gslot->bitmap.rows);

				auto *ptr = data.data();
				size_t pos = 0;
				for(auto y=decltype(gslot->bitmap.rows){0};y<gslot->bitmap.rows;++y)
				{
					auto *row = ptr;
					for(auto x=decltype(gslot->bitmap.width){0};x<gslot->bitmap.width;++x)
					{
						auto *px = row;
						px[0] = gslot->bitmap.buffer[pos];
						++pos;
						++row;
					}
					ptr += createInfo.width;
				}

				auto &glyphImg = glyphImages.at(i -umath::to_integral(GlyphRange::Start)) = prosper::util::create_image(dev,createInfo,data.data());
				glyphImg->SetDebugName("tmp_glyph_img");
				glyphBounds.at(i -umath::to_integral(GlyphRange::Start)) = {gslot->bitmap.width,gslot->bitmap.rows};
			}

			glyph->Initialize(gslot);
			int32_t yMin,yMax;
			glyph->GetBounds(nullptr,&yMin,nullptr,&yMax);
			yMax -= yMin;
			if(yMax < 0)
				yMax = 0;
			auto u_yMax = static_cast<unsigned int>(yMax);
			if(u_yMax > hMax)
				hMax = u_yMax;
			auto u_szMax = static_cast<unsigned int>(u_yMax) +static_cast<unsigned int>(static_cast<int>(fontSize) -glyph->GetTop());
			if(u_szMax < 0)
				u_szMax = 0;
			if(u_szMax > szMax)
				szMax = u_szMax;
			int top = glyph->GetTop();
			if(top > m_glyphTopMax)
				m_glyphTopMax = top;
		}
	}

	// Initialize font image map
	const auto format = Anvil::Format::R8_UNORM;
	// TODO: Calculate proper width / height ratio
	auto imgCreateInfo = prosper::util::ImageCreateInfo {};
	imgCreateInfo.format = format;
	imgCreateInfo.width = numGlyphs *m_maxBitmapWidth;
	imgCreateInfo.height = m_maxBitmapHeight;
	imgCreateInfo.usage = Anvil::ImageUsageFlagBits::SAMPLED_BIT | Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT;
	imgCreateInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
	auto imgViewCreateInfo = prosper::util::ImageViewCreateInfo {};
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	auto glyphMapImage = std::shared_ptr<prosper::Image>(prosper::util::create_image(dev,imgCreateInfo));
	m_glyphMap = prosper::util::create_texture(dev,{},glyphMapImage,&imgViewCreateInfo,&samplerCreateInfo);
	m_glyphMap->SetDebugName("glyph_map_tex");

	auto setupCmd = context.GetSetupCommandBuffer();
	// Initialize glyph images
#pragma pack(push,1)
	struct GlyphBounds
	{
		int32_t left;
		int32_t top;
		int32_t width;
		int32_t height;
		int32_t advanceX;
		int32_t advanceY;
	};
#pragma pack(pop)
	//std::vector<std::shared_ptr<prosper::Image>> tmpGlyphImages {};
	//tmpGlyphImages.reserve(m_glyphs.size());
	std::vector<GlyphBounds> glyphCharacterBounds {};
	glyphCharacterBounds.reserve(m_glyphs.size());
	for(auto i=decltype(m_glyphs.size()){0};i<m_glyphs.size();++i)
	{
		auto &glyphImage = glyphImages[i];
		if(glyphImage == nullptr)
			continue;
		auto &bounds = glyphBounds.at(i);
		auto &glyph = m_glyphs[i];
		// Copy to image with optimal tiling and without host-visible memory
		auto imgCreateInfo = prosper::util::ImageCreateInfo {};
		imgCreateInfo.format = format;
		imgCreateInfo.width = bounds.first;
		imgCreateInfo.height = bounds.second;
		imgCreateInfo.usage = Anvil::ImageUsageFlagBits::SAMPLED_BIT | Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT;
		imgCreateInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;

		prosper::util::CopyInfo copyInfo {};
		copyInfo.srcSubresource = Anvil::ImageSubresourceLayers{Anvil::ImageAspectFlagBits::COLOR_BIT,0u,0u,1u};
		copyInfo.dstSubresource = Anvil::ImageSubresourceLayers{Anvil::ImageAspectFlagBits::COLOR_BIT,0u,0u,1u};
		copyInfo.width = bounds.first;
		copyInfo.height = bounds.second;

		// Write glyph to font image map
		auto xOffset = i *m_maxBitmapWidth;
		uint32_t yOffset = 0;
		copyInfo.dstOffset = vk::Offset3D(xOffset,yOffset,0);
		prosper::util::record_copy_image(setupCmd->GetAnvilCommandBuffer(),copyInfo,glyphImage->GetAnvilImage(),glyphMapImage->GetAnvilImage());

		glyphCharacterBounds.push_back(GlyphBounds{});
		auto &glyphCharBounds = glyphCharacterBounds.back();
		int32_t left,top,width,height,advanceX,advanceY;
		glyph->GetDimensions(left,top,width,height);
		glyph->GetAdvance(advanceX,advanceY);
		glyphCharBounds.left = left;
		glyphCharBounds.top = top;
		glyphCharBounds.width = width;
		glyphCharBounds.height = height;
		glyphCharBounds.advanceX = advanceX;
		glyphCharBounds.advanceY = advanceY;
	}
	prosper::util::BufferCreateInfo bufCreateInfo {};
	bufCreateInfo.size = glyphCharacterBounds.size() *sizeof(glyphCharacterBounds.front());
	bufCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
	bufCreateInfo.usageFlags = Anvil::BufferUsageFlagBits::STORAGE_BUFFER_BIT;
	m_glyphBoundsBuffer = prosper::util::create_buffer(dev,bufCreateInfo,glyphCharacterBounds.data());
	m_glyphBoundsBuffer->SetDebugName("font_glyph_bounds_buffer");
	m_glyphBoundsDsg = prosper::util::create_descriptor_set_group(dev,wgui::ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER);
	prosper::util::set_descriptor_set_binding_storage_buffer(*(*m_glyphBoundsDsg)->get_descriptor_set(0u),*m_glyphBoundsBuffer,0u);
	context.FlushSetupCommandBuffer();

	m_maxGlyphSize = szMax;
	m_maxGlyphHeight = hMax;
	m_bInitialized = true;

	auto wpShader = context.GetShader("wguitext");
	if(wpShader.expired() == false)
	{
		m_glyphMapDescSetGroup = prosper::util::create_descriptor_set_group(dev,wgui::ShaderText::DESCRIPTOR_SET_TEXTURE);
		prosper::util::set_descriptor_set_binding_texture(*(*m_glyphMapDescSetGroup)->get_descriptor_set(0u),*m_glyphMap,0u);
	}
	return true;
}

uint32_t FontInfo::GetMaxGlyphBitmapWidth() const {return m_maxBitmapWidth;}
uint32_t FontInfo::GetMaxGlyphBitmapHeight() const {return m_maxBitmapHeight;}
uint32_t FontInfo::CharToGlyphMapIndex(char c) {return static_cast<uint8_t>(c) -umath::to_integral(GlyphRange::Start);}

std::shared_ptr<prosper::Texture> FontInfo::GetGlyphMap() const {return m_glyphMap;}
std::shared_ptr<prosper::Buffer> FontInfo::GetGlyphBoundsBuffer() const {return m_glyphBoundsBuffer;}
Anvil::DescriptorSet *FontInfo::GetGlyphBoundsDescriptorSet() const
{
	return (m_glyphBoundsDsg != nullptr) ? (*m_glyphBoundsDsg)->get_descriptor_set(0u) : nullptr;
}
Anvil::DescriptorSet *FontInfo::GetGlyphMapDescriptorSet() const {return (*m_glyphMapDescSetGroup)->get_descriptor_set(0u);}

const FT_Face FontInfo::GetFace() const {return m_face.GetFtFace();}

int32_t FontInfo::GetMaxGlyphTop() const {return m_glyphTopMax;}

uint32_t FontInfo::GetMaxGlyphSize() const {return m_maxGlyphSize;}
uint32_t FontInfo::GetMaxGlyphHeight() const {return m_maxGlyphHeight;}

void FontInfo::Clear()
{
	m_data.clear();
	if(m_bInitialized)
		m_glyphs.clear();
	m_maxGlyphSize = 0;
	m_maxGlyphHeight = 0;
	m_size = 0;
	m_bInitialized = false;
}

/////////////////////

decltype(FontManager::m_fontDefault) FontManager::m_fontDefault = nullptr;
decltype(FontManager::m_lib) FontManager::m_lib = {};
decltype(FontManager::m_fonts) FontManager::m_fonts;

FontManager::Library::~Library()
{
	if(m_ftLibrary != nullptr)
		FT_Done_FreeType(m_ftLibrary);
}
const FT_Library &FontManager::Library::GetFtLibrary() const {return m_ftLibrary;}

std::shared_ptr<const FontInfo> FontManager::GetFont(const std::string &cfontName)
{
	auto fontName = cfontName;
	ustring::to_lower(fontName);
	auto it = m_fonts.find(fontName);
	if(it == m_fonts.end())
	{
		if(m_fontDefault != nullptr)
			return m_fontDefault;
		return nullptr;
	}
	return it->second;
}

std::shared_ptr<const FontInfo> FontManager::LoadFont(const std::string &cidentifier,const std::string &cpath,uint32_t size,bool bForceReload)
{
	auto &lib = m_lib.GetFtLibrary();
	if(lib == nullptr)
		return nullptr;
	auto identifier = cidentifier;
	ustring::to_lower(identifier);
	if(bForceReload == false)
	{
		auto it = m_fonts.find(identifier);
		if(it != m_fonts.end())
			return it->second;
	}
	auto file = cpath;
	std::string ext;
	if(ufile::get_extension(file,&ext) == false || (ext != "ttf" && ext != "otf"))
		file += ".ttf";

	auto path = "fonts\\" +file;
	std::shared_ptr<FontInfo> font = nullptr;
	if(bForceReload == true)
	{
		auto it = m_fonts.find(identifier);
		if(it != m_fonts.end())
		{
			font = it->second;
			font->Clear();
		}
	}
	if(font == nullptr)
		font = std::shared_ptr<FontInfo>(new FontInfo());
	if(!font->Initialize(path.c_str(),size))
		return nullptr;
	m_fonts.insert(decltype(m_fonts)::value_type(identifier,font));
	return font;
}

const std::unordered_map<std::string,std::shared_ptr<FontInfo>> &FontManager::GetFonts() {return m_fonts;}
std::shared_ptr<const FontInfo> FontManager::GetDefaultFont() {return m_fontDefault;}
void FontManager::SetDefaultFont(const FontInfo &font) {m_fontDefault = font.shared_from_this();}

bool FontManager::Initialize()
{
	auto &lib = m_lib.GetFtLibrary();
	if(lib != nullptr)
		return true;
	auto &ncLib = const_cast<FT_Library&>(lib);
	if(FT_Init_FreeType(&ncLib) != 0)
	{
		m_lib = {};
		return false;
	}
	return true;
}

const FT_Library FontManager::GetFontLibrary() {return m_lib.GetFtLibrary();}

void FontManager::Close()
{
	m_fontDefault = nullptr;
	m_fonts.clear();
	m_lib = {};
}
uint32_t FontManager::GetTextSize(const std::string_view &text,uint32_t charOffset,const FontInfo *font,int32_t *width,int32_t *height)
{
	if(font == nullptr)
	{
		if(width != nullptr)
			*width = 0;
		if(height != nullptr)
			*height = 0;
		return 0;
	}
	auto fWidth = 0.f;
	auto fHeight = 0.f;
	int32_t scrW,scrH;

	auto &context = WGUI::GetInstance().GetContext();
	scrW = context.GetWindowWidth();
	scrH = context.GetWindowHeight();
	auto sx = 2.f /static_cast<float>(scrW);
	auto sy = 2.f /static_cast<float>(scrH);
	auto offset = charOffset;
	for(auto c : text)
	{
		auto multiplier = 1u;
		if(c == '\t')
		{
			c = ' ';
			auto tabSpaceCount = TAB_WIDTH_SPACE_COUNT -(offset %TAB_WIDTH_SPACE_COUNT);
			multiplier = tabSpaceCount;
		}
		auto *glyph = font->GetGlyphInfo(c);
		if(glyph != nullptr)
		{
			int32_t advanceX,advanceY;
			glyph->GetAdvance(advanceX,advanceY);

			int32_t left,top,width,height;
			glyph->GetDimensions(left,top,width,height);
			width *= multiplier;
			auto fadvanceX = static_cast<float>(advanceX >> 6) *multiplier;
			auto fadvanceY = static_cast<float>(height);
			fadvanceX *= sx;
			fadvanceY *= sy;
			fWidth += fadvanceX;
			if(fadvanceY > fHeight)
				fHeight = fadvanceY;
			offset += multiplier;
		}
		if(c == '\n')
			offset = 0u;
	}
	fWidth *= 0.5f;
	fHeight *= 0.5f;
	fWidth *= scrW;
	fHeight *= scrH;
	if(width != nullptr)
		*width = static_cast<int32_t>(fWidth);
	if(height != nullptr)
		*height = static_cast<int32_t>(fHeight);
	return offset -charOffset;
}

uint32_t FontManager::GetTextSize(const std::string_view &text,uint32_t charOffset,const std::string &font,int32_t *width,int32_t *height) {return  GetTextSize(text,charOffset,GetFont(font).get(),width,height);}
uint32_t FontManager::GetTextSize(char c,uint32_t charOffset,const FontInfo *font,int32_t *width,int32_t *height)
{
	std::string str{c,'\0'};
	return GetTextSize(str,charOffset,font,width,height);
}
uint32_t FontManager::GetTextSize(char c,uint32_t charOffset,const std::string &font,int32_t *width,int32_t *height)
{
	std::string str{c,'\0'};
	return GetTextSize(str,charOffset,font,width,height);
}
