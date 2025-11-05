// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <ft2build.h>
#include FT_FREETYPE_H
#include "freetype/ftglyph.h"

module pragma.gui;

import :font_manager;

import pragma.string.unicode;

class DynamicFontMap {
  public:
	static std::unique_ptr<DynamicFontMap> Create(const std::string &cpath, const FontSettings &fontSettings);
	static constexpr auto INVALID_GLYPH_INDEX = std::numeric_limits<uint32_t>::max();
#pragma pack(push, 1)
	struct GlyphBounds {
		int32_t left;
		int32_t top;
		int32_t width;
		int32_t height;
		int32_t advanceX;
		int32_t advanceY;
	};
#pragma pack(pop)

	struct GlyphExtent {
		uint32_t width;
		uint32_t height;
	};
	void AddCharacter(int32_t c);
	bool GenerateImageMap();
	void ClearMap();

	uint32_t GetMaxGlyphBitmapWidth() const { return m_maxBitmapWidth; }
	uint32_t GetMaxGlyphBitmapHeight() const { return m_maxBitmapHeight; }
	std::shared_ptr<prosper::Texture> GetGlyphMap() const { return m_glyphMap; }
	prosper::IDescriptorSet *GetGlyphMapDescriptorSet() const { return m_glyphMapDescSetGroup->GetDescriptorSet(); }
	std::shared_ptr<prosper::IBuffer> GetGlyphBoundsBuffer() const { return m_glyphBoundsBuffer; }
	prosper::IDescriptorSet *GetGlyphBoundsDescriptorSet() const { return m_glyphBoundsDsg ? m_glyphBoundsDsg->GetDescriptorSet() : nullptr; }
	int32_t GetMaxGlyphTop() const { return m_glyphTopMax; }
	uint32_t GetGlyphCountPerRow() const { return m_numGlyphsPerRow; }
	const FT_Face GetFace() const { return m_face.GetFtFace(); }
	const std::vector<std::shared_ptr<GlyphInfo>> &GetGlyphs() const { return m_glyphs; }
	const GlyphInfo *GetGlyphInfo(int32_t c) const
	{
		auto idx = CharToGlyphMapIndex(c);
		if(idx < 0 || idx >= m_glyphs.size())
			return nullptr;
		return m_glyphs[idx].get();
	}
	uint32_t CharToGlyphMapIndex(int32_t c) const
	{
		auto it = m_glyphIndexMap.find(c);
		if(it == m_glyphIndexMap.end())
			return INVALID_GLYPH_INDEX;
		return it->second;
	}
  private:
	DynamicFontMap();
	uint32_t m_curGlyphIndex = 0;
	std::vector<GlyphExtent> m_glyphExtents;
	std::vector<GlyphBounds> m_glyphCharacterBounds;
	std::vector<std::shared_ptr<GlyphInfo>> m_glyphs;
	std::unordered_map<uint32_t, uint32_t> m_glyphIndexMap;
	std::vector<uint8_t> m_glyphImageBufData;
	uint32_t m_maxBitmapWidth = 0;
	uint32_t m_maxBitmapHeight = 0;
	uint32_t m_maxGlyphHeight = 0;
	uint32_t m_maxGlyphSize = 0;

	FontFace m_face;
	std::vector<uint8_t> m_fontData;
	int32_t m_glyphTopMax = 0;
	uint32_t m_fontSize = 0;
	uint32_t m_numGlyphsPerRow = 0;

	bool m_dirtyGlyphMap = true;
	std::shared_ptr<prosper::Texture> m_glyphMap = nullptr;
	std::shared_ptr<prosper::IDescriptorSetGroup> m_glyphMapDescSetGroup = nullptr;
	std::shared_ptr<prosper::IBuffer> m_glyphBoundsBuffer = nullptr;
	std::shared_ptr<prosper::IDescriptorSetGroup> m_glyphBoundsDsg = nullptr;
};

FontFace::~FontFace()
{
	if(m_ftFace != nullptr)
		FT_Done_Face(m_ftFace);
}
const FT_Face &FontFace::GetFtFace() const { return m_ftFace; }

GlyphInfo::GlyphInfo()
{
	for(unsigned int i = 0; i < 4; i++)
		m_bbox[i] = 0;
}

void GlyphInfo::GetAdvance(int32_t &advanceX, int32_t &advanceY) const
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
	FT_Get_Glyph(glyph, &g);

	FT_BBox bbox;
	FT_Glyph_Get_CBox(g, FT_GLYPH_BBOX_TRUNCATE, &bbox);
	m_bbox[0] = bbox.xMin;
	m_bbox[1] = bbox.yMin;
	m_bbox[2] = bbox.xMax;
	m_bbox[3] = bbox.yMax;

	FT_Done_Glyph(g);
}

void GlyphInfo::GetBounds(int32_t *xMin, int32_t *yMin, int32_t *xMax, int32_t *yMax) const
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

void GlyphInfo::GetDimensions(int32_t &left, int32_t &top, int32_t &width, int32_t &height) const
{
	left = m_left;
	top = m_top;
	width = m_width;
	height = m_height;
}
int32_t GlyphInfo::GetTop() const { return m_top; }
int32_t GlyphInfo::GetLeft() const { return m_left; }
int32_t GlyphInfo::GetWidth() const { return m_width; }
int32_t GlyphInfo::GetHeight() const { return m_height; }

/////////////////////

FontSettings::FontSettings() {}
FontSettings::~FontSettings() {}
FontInfo::~FontInfo() { Clear(); }
const GlyphInfo *FontInfo::InitializeGlyph(int32_t c) const
{
	m_dynamicFontMap->AddCharacter(c);
	return GetGlyphInfo(c);
}
const GlyphInfo *FontInfo::GetGlyphInfo(int32_t c) const { return m_dynamicFontMap->GetGlyphInfo(c); }
const std::vector<std::shared_ptr<GlyphInfo>> &FontInfo::GetGlyphs() const { return m_dynamicFontMap->GetGlyphs(); }
uint32_t FontInfo::GetSize() const { return m_size; }

std::unique_ptr<DynamicFontMap> DynamicFontMap::Create(const std::string &cpath, const FontSettings &fontSettings)
{
	auto fontMap = std::unique_ptr<DynamicFontMap> {new DynamicFontMap {}};
	auto f = FileManager::OpenFile(cpath.c_str(), "rb");
	if(f == nullptr)
		return {};
	auto size = f->GetSize();
	fontMap->m_fontData.resize(size);
	f->Read(fontMap->m_fontData.data(), size);

	auto lib = FontManager::GetFontLibrary();
	auto &face = fontMap->m_face.GetFtFace();
	auto &ncFace = const_cast<FT_Face &>(face);
	if(FT_New_Memory_Face(lib, &fontMap->m_fontData[0], static_cast<FT_Long>(size), 0, &ncFace) != 0 || FT_Set_Pixel_Sizes(face, 0, fontSettings.fontSize) != 0) {
		fontMap->m_fontData.clear();
		return {};
	}
	fontMap->m_fontSize = fontSettings.fontSize;
	return fontMap;
}
DynamicFontMap::DynamicFontMap() { FontManager::SetFontsDirty(); }
void DynamicFontMap::AddCharacter(int32_t c)
{
	if(m_glyphIndexMap.find(c) != m_glyphIndexMap.end())
		return;
	ClearMap();

	auto &face = m_face.GetFtFace();
	auto ftGlyphIdx = FT_Get_Char_Index(face, c);
	if(ftGlyphIdx == 0 || FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT) != 0) {
		m_glyphIndexMap[c] = INVALID_GLYPH_INDEX;
		return;
	}
	auto gslot = face->glyph;
	// Outline
	/*if(gslot->format == FT_GLYPH_FORMAT_OUTLINE)
				{
					std::vector<Span> spans;
					RenderSpans(lib,gslot->outline,spans);
				}*/
	//
	m_glyphs.push_back({});
	m_glyphExtents.push_back({});
	auto &glyph = m_glyphs[m_curGlyphIndex] = std::shared_ptr<GlyphInfo> {new GlyphInfo {}};
	auto &glyphInfo = m_glyphExtents[m_curGlyphIndex];
	if(gslot->bitmap.width > 0 && gslot->bitmap.rows > 0) {
		auto &data = m_glyphImageBufData;
		auto glyphDataSize = gslot->bitmap.width * gslot->bitmap.rows;
		if((data.size() + glyphDataSize) >= data.capacity())
			data.reserve((data.size() + glyphDataSize) * 1.5 + 1'000);
		auto offset = data.size();
		data.resize(offset + glyphDataSize);
		auto *ptr = data.data() + offset;
		m_maxBitmapWidth = umath::max(m_maxBitmapWidth, gslot->bitmap.width);
		m_maxBitmapHeight = umath::max(m_maxBitmapHeight, gslot->bitmap.rows);

		size_t pos = 0;
		for(auto y = decltype(gslot->bitmap.rows) {0}; y < gslot->bitmap.rows; ++y) {
			auto *row = ptr;
			for(auto x = decltype(gslot->bitmap.width) {0}; x < gslot->bitmap.width; ++x) {
				auto *px = row;
				px[0] = gslot->bitmap.buffer[pos];
				++pos;
				++row;
			}
			ptr += gslot->bitmap.width;
		}

		glyphInfo.width = gslot->bitmap.width;
		glyphInfo.height = gslot->bitmap.rows;
	}

	glyph->Initialize(gslot);
	int32_t yMin, yMax;
	glyph->GetBounds(nullptr, &yMin, nullptr, &yMax);
	yMax -= yMin;
	if(yMax < 0)
		yMax = 0;
	auto u_yMax = static_cast<unsigned int>(yMax);
	if(u_yMax > m_maxGlyphHeight)
		m_maxGlyphHeight = u_yMax;
	auto u_szMax = static_cast<unsigned int>(u_yMax) + static_cast<unsigned int>(static_cast<int>(m_fontSize) - glyph->GetTop());
	if(u_szMax < 0)
		u_szMax = 0;
	if(u_szMax > m_maxGlyphSize)
		m_maxGlyphSize = u_szMax;
	int top = glyph->GetTop();
	if(top > m_glyphTopMax)
		m_glyphTopMax = top;
	m_glyphIndexMap[c] = m_curGlyphIndex;
	++m_curGlyphIndex;
}
void DynamicFontMap::ClearMap()
{
	if(m_dirtyGlyphMap)
		return;
	FontManager::SetFontsDirty();
	m_dirtyGlyphMap = true;
	auto &context = WGUI::GetInstance().GetContext();
	context.WaitIdle();

	m_glyphMap = nullptr;
	m_glyphMapDescSetGroup = nullptr;
	m_glyphBoundsBuffer = nullptr;
	m_glyphBoundsDsg = nullptr;
}
bool DynamicFontMap::GenerateImageMap()
{
	if(!m_dirtyGlyphMap)
		return (m_glyphMapDescSetGroup != nullptr);
	m_dirtyGlyphMap = false;

	if(m_glyphImageBufData.empty())
		return false;

	prosper::util::BufferCreateInfo glyphBufCreateInfo {};
	glyphBufCreateInfo.size = m_glyphImageBufData.size();
	glyphBufCreateInfo.usageFlags = prosper::BufferUsageFlags::TransferSrcBit;
	glyphBufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::HostAccessable;

	auto &context = WGUI::GetInstance().GetContext();
	auto glyphBuf = context.CreateBuffer(glyphBufCreateInfo, m_glyphImageBufData.data());
	glyphBuf->SetDebugName("tmp_glyph_buf");

	// Initialize font image map
	const auto format = prosper::Format::R8_UNorm;
	auto imgCreateInfo = prosper::util::ImageCreateInfo {};
	imgCreateInfo.format = format;
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.usage = prosper::ImageUsageFlags::SampledBit | prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferDstBit;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::TransferDstOptimal;

	imgCreateInfo.width = m_glyphs.size() * m_maxBitmapWidth;
	imgCreateInfo.height = m_maxBitmapHeight;

	auto props = context.GetPhysicalDeviceImageFormatProperties(imgCreateInfo);
	if(!props.has_value())
		return false;

	if(imgCreateInfo.width > props->maxExtent.width) {
		auto levels = (imgCreateInfo.width / props->maxExtent.width) + 1;
		imgCreateInfo.height = umath::next_power_of_2(imgCreateInfo.height * levels);
		imgCreateInfo.width = props->maxExtent.width;
		if(imgCreateInfo.height > props->maxExtent.height)
			return false;
	}

	m_numGlyphsPerRow = imgCreateInfo.width / m_maxBitmapWidth;

	auto imgViewCreateInfo = prosper::util::ImageViewCreateInfo {};
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	auto glyphMapImage = context.CreateImage(imgCreateInfo);
	m_glyphMap = context.CreateTexture({}, *glyphMapImage, imgViewCreateInfo, samplerCreateInfo);
	m_glyphMap->SetDebugName("glyph_map_tex");

	auto setupCmd = context.GetSetupCommandBuffer();
	// Initialize glyph images

	m_glyphCharacterBounds.reserve(m_glyphs.size());
	size_t glyphBufOffset = 0;
	Vector2i glyphImageOffset {0, 0};
	for(auto i = decltype(m_glyphs.size()) {0}; i < m_glyphs.size(); ++i) {
		auto &glyphInfo = m_glyphExtents.at(i);
		auto glyphDataSize = glyphInfo.width * glyphInfo.height;

		auto curGlyphImageOffset = glyphImageOffset;
		glyphImageOffset.x += m_maxBitmapWidth;
		if((glyphImageOffset.x + m_maxBitmapWidth) > imgCreateInfo.width) {
			glyphImageOffset.x = 0;
			glyphImageOffset.y += m_maxBitmapHeight;
		}

		if(glyphDataSize == 0)
			continue;
		auto &glyph = m_glyphs[i];

		prosper::util::BufferImageCopyInfo cpyInfo {};
		cpyInfo.bufferOffset = glyphBufOffset;
		cpyInfo.imageExtent = {glyphInfo.width, glyphInfo.height};

		// Write glyph to font image map
		cpyInfo.imageOffset = curGlyphImageOffset;
		if((cpyInfo.imageOffset.x + cpyInfo.imageExtent->x) > imgCreateInfo.width || (cpyInfo.imageOffset.y + cpyInfo.imageExtent->y) > imgCreateInfo.height)
			throw std::logic_error {"Glyph map image offset out of bounds!"};
		setupCmd->RecordCopyBufferToImage(cpyInfo, *glyphBuf, *glyphMapImage);

		m_glyphCharacterBounds.push_back(GlyphBounds {});
		auto &glyphCharBounds = m_glyphCharacterBounds.back();
		int32_t left, top, width, height, advanceX, advanceY;
		glyph->GetDimensions(left, top, width, height);
		glyph->GetAdvance(advanceX, advanceY);
		glyphCharBounds.left = left;
		glyphCharBounds.top = top;
		glyphCharBounds.width = width;
		glyphCharBounds.height = height;
		glyphCharBounds.advanceX = advanceX;
		glyphCharBounds.advanceY = advanceY;

		glyphBufOffset += glyphDataSize;
	}
	setupCmd->RecordImageBarrier(*glyphMapImage, prosper::ImageLayout::TransferDstOptimal, prosper::ImageLayout::ShaderReadOnlyOptimal);

	prosper::util::BufferCreateInfo bufCreateInfo {};
	bufCreateInfo.size = m_glyphCharacterBounds.size() * sizeof(m_glyphCharacterBounds.front());
	bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	bufCreateInfo.usageFlags = prosper::BufferUsageFlags::StorageBufferBit;
	m_glyphBoundsBuffer = context.CreateBuffer(bufCreateInfo, m_glyphCharacterBounds.data());
	m_glyphBoundsBuffer->SetDebugName("font_glyph_bounds_buffer");
	m_glyphBoundsDsg = context.CreateDescriptorSetGroup(wgui::ShaderText::DESCRIPTOR_SET_GLYPH_BOUNDS_BUFFER);
	m_glyphBoundsDsg->GetDescriptorSet()->SetBindingStorageBuffer(*m_glyphBoundsBuffer, 0u);
	context.FlushSetupCommandBuffer();

	auto wpShader = context.GetShader("wguitext");
	if(wpShader.expired() == false) {
		m_glyphMapDescSetGroup = context.CreateDescriptorSetGroup(wgui::ShaderText::DESCRIPTOR_SET_TEXTURE);
		m_glyphMapDescSetGroup->GetDescriptorSet()->SetBindingTexture(*m_glyphMap, 0u);
	}
	return true;
}
FontInfo::FontInfo() {}
bool FontInfo::Initialize(const std::string &cpath, const std::string &name, const FontSettings &fontSettings)
{
	m_name = name;
	if(m_bInitialized || wgui::ShaderText::DESCRIPTOR_SET_TEXTURE.IsValid() == false)
		return true;
	auto fontMap = DynamicFontMap::Create(cpath, fontSettings);
	if(!fontMap)
		return false;
	m_size = fontSettings.fontSize;
	m_dynamicFontMap = std::move(fontMap);
	m_bInitialized = true;
	return true;
}

uint32_t FontInfo::GetMaxGlyphBitmapWidth() const { return m_dynamicFontMap->GetMaxGlyphBitmapWidth(); }
uint32_t FontInfo::GetMaxGlyphBitmapHeight() const { return m_dynamicFontMap->GetMaxGlyphBitmapHeight(); }
uint32_t FontInfo::CharToGlyphMapIndex(int32_t c) const { return m_dynamicFontMap->CharToGlyphMapIndex(c); }

std::shared_ptr<prosper::Texture> FontInfo::GetGlyphMap() const { return m_dynamicFontMap->GetGlyphMap(); }
std::shared_ptr<prosper::IBuffer> FontInfo::GetGlyphBoundsBuffer() const { return m_dynamicFontMap->GetGlyphBoundsBuffer(); }
prosper::IDescriptorSet *FontInfo::GetGlyphBoundsDescriptorSet() const { return m_dynamicFontMap->GetGlyphBoundsDescriptorSet(); }
prosper::IDescriptorSet *FontInfo::GetGlyphMapDescriptorSet() const { return m_dynamicFontMap->GetGlyphMapDescriptorSet(); }
void FontInfo::AddFallbackFont(FontInfo &font) { m_fallbackFonts.push_back(font.shared_from_this()); }

const FT_Face FontInfo::GetFace() const { return m_dynamicFontMap->GetFace(); }

int32_t FontInfo::GetMaxGlyphTop() const { return m_dynamicFontMap->GetMaxGlyphTop(); }
uint32_t FontInfo::GetGlyphCountPerRow() const { return m_dynamicFontMap->GetGlyphCountPerRow(); }

uint32_t FontInfo::GetMaxGlyphSize() const { return m_maxGlyphSize; }
uint32_t FontInfo::GetMaxGlyphHeight() const { return m_maxGlyphHeight; }

void FontInfo::Clear()
{
	m_dynamicFontMap = {};
	m_maxGlyphSize = 0;
	m_maxGlyphHeight = 0;
	m_size = 0;
	m_bInitialized = false;
}

/////////////////////

decltype(FontManager::m_fontDefault) FontManager::m_fontDefault = nullptr;
decltype(FontManager::m_fallbackFonts) FontManager::m_fallbackFonts = {};
decltype(FontManager::m_lib) FontManager::m_lib = {};
decltype(FontManager::m_fonts) FontManager::m_fonts;

FontManager::Library::~Library()
{
	if(m_ftLibrary != nullptr)
		FT_Done_FreeType(m_ftLibrary);
}
const FT_Library &FontManager::Library::GetFtLibrary() const { return m_ftLibrary; }

std::shared_ptr<const FontInfo> FontManager::GetFont(const std::string &cfontName)
{
	auto fontName = cfontName;
	ustring::to_lower(fontName);
	auto it = m_fonts.find(fontName);
	if(it == m_fonts.end()) {
		if(m_fontDefault != nullptr)
			return m_fontDefault;
		return nullptr;
	}
	return it->second;
}

static std::string get_font_file_path(const std::string &cpath)
{
	auto file = cpath;
	std::string ext;
	if(ufile::get_extension(file, &ext) == false || (ext != "ttf" && ext != "otf"))
		file += ".ttf";

	return "fonts/" + file;
}
std::shared_ptr<const FontInfo> FontManager::LoadFont(const std::string &cidentifier, const std::string &cpath, const FontSettings &fontSettings, bool bForceReload)
{
	auto &lib = m_lib.GetFtLibrary();
	if(lib == nullptr)
		return nullptr;
	auto identifier = cidentifier;
	ustring::to_lower(identifier);
	if(bForceReload == false) {
		auto it = m_fonts.find(identifier);
		if(it != m_fonts.end())
			return it->second;
	}

	auto path = get_font_file_path(cpath);
	std::shared_ptr<FontInfo> font = nullptr;
	if(bForceReload == true) {
		auto it = m_fonts.find(identifier);
		if(it != m_fonts.end()) {
			font = it->second;
			font->Clear();
		}
	}
	if(font == nullptr)
		font = std::shared_ptr<FontInfo>(new FontInfo());
	if(!font->Initialize(path, identifier, fontSettings))
		return nullptr;
	for(auto &fallbackFont : FontManager::GetFallbackFonts()) {
		auto path = get_font_file_path(fallbackFont);
		auto fontFallback = std::shared_ptr<FontInfo>(new FontInfo());
		if(!fontFallback->Initialize(path, identifier, fontSettings))
			continue;
		font->AddFallbackFont(*fontFallback);
	}
	m_fonts.insert(decltype(m_fonts)::value_type(identifier, font));
	return font;
}

const std::unordered_map<std::string, std::shared_ptr<FontInfo>> &FontManager::GetFonts() { return m_fonts; }
std::shared_ptr<const FontInfo> FontManager::GetDefaultFont() { return m_fontDefault; }
void FontManager::SetDefaultFont(const FontInfo &font) { m_fontDefault = font.shared_from_this(); }

bool FontManager::Initialize()
{
	auto &lib = m_lib.GetFtLibrary();
	if(lib != nullptr)
		return true;
	auto &ncLib = const_cast<FT_Library &>(lib);
	if(FT_Init_FreeType(&ncLib) != 0) {
		m_lib = {};
		return false;
	}
	return true;
}

const FT_Library FontManager::GetFontLibrary() { return m_lib.GetFtLibrary(); }

static bool g_fontsDirty = false;
void FontManager::SetFontsDirty() { g_fontsDirty = true; }
void FontManager::UpdateDirtyFonts()
{
	if(!g_fontsDirty)
		return;
	g_fontsDirty = false;
	for(auto &[name, fontInfo] : m_fonts) {
		fontInfo->m_dynamicFontMap->GenerateImageMap();
		for(auto &fallbackFont : fontInfo->GetFallbackFonts())
			fallbackFont->m_dynamicFontMap->GenerateImageMap();
	}
}
void FontManager::Close()
{
	m_fontDefault = nullptr;
	m_fonts.clear();
	m_lib = {};
}
void FontManager::InitializeFontGlyphs(const pragma::string::Utf8StringArg &text, const FontInfo &font)
{
	for(auto c : *text) {
		auto *glyph = font.InitializeGlyph(c);
		if(glyph == nullptr) {
			for(auto &fallbackFont : font.GetFallbackFonts()) {
				glyph = fallbackFont->InitializeGlyph(c);
				if(glyph)
					break;
			}
		}
	}
}
void FontManager::AddFallbackFont(const std::string &fallbackFont) { m_fallbackFonts.push_back(fallbackFont); }
const std::vector<std::string> &FontManager::GetFallbackFonts() { return m_fallbackFonts; }
uint32_t FontManager::GetTextSize(const pragma::string::Utf8StringArg &text, uint32_t charOffset, const FontInfo *font, int32_t *width, int32_t *height)
{
	if(font == nullptr) {
		if(width != nullptr)
			*width = 0;
		if(height != nullptr)
			*height = 0;
		return 0;
	}
	auto fWidth = 0.f;
	auto fHeight = 0.f;
	int32_t scrW, scrH;

	auto &context = WGUI::GetInstance().GetContext();
	scrW = context.GetWindowWidth();
	scrH = context.GetWindowHeight();
	auto sx = 2.f / static_cast<float>(scrW);
	auto sy = 2.f / static_cast<float>(scrH);
	auto offset = charOffset;
	for(auto c : *text) {
		auto multiplier = 1u;
		if(c == '\t') {
			c = ' ';
			auto tabSpaceCount = TAB_WIDTH_SPACE_COUNT - (offset % TAB_WIDTH_SPACE_COUNT);
			multiplier = tabSpaceCount;
		}
		auto *glyph = font->InitializeGlyph(c);
		if(glyph == nullptr) {
			for(auto &fallbackFont : font->GetFallbackFonts()) {
				glyph = fallbackFont->InitializeGlyph(c);
				if(glyph)
					break;
			}
		}
		if(glyph != nullptr) {
			int32_t advanceX, advanceY;
			glyph->GetAdvance(advanceX, advanceY);

			int32_t left, top, width, height;
			glyph->GetDimensions(left, top, width, height);
			width *= multiplier;
			auto fadvanceX = static_cast<float>(advanceX >> 6) * multiplier;
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
	return offset - charOffset;
}

uint32_t FontManager::GetTextSize(const pragma::string::Utf8StringArg &text, uint32_t charOffset, const std::string &font, int32_t *width, int32_t *height) { return GetTextSize(text, charOffset, GetFont(font).get(), width, height); }
uint32_t FontManager::GetTextSize(int32_t c, uint32_t charOffset, const FontInfo *font, int32_t *width, int32_t *height)
{
	pragma::string::Utf8String str {static_cast<char16_t>(c)};
	return GetTextSize(str, charOffset, font, width, height);
}
uint32_t FontManager::GetTextSize(int32_t c, uint32_t charOffset, const std::string &font, int32_t *width, int32_t *height)
{
	pragma::string::Utf8String str {static_cast<char16_t>(c)};
	return GetTextSize(str, charOffset, font, width, height);
}
