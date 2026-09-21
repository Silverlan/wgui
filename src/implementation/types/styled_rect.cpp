// SPDX-FileCopyrightText: (c) 2026 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.gui;

import :types.styled_rect;

namespace pragma::gui {
	using StyleCacheMap = std::unordered_map<shaders::StyledRect::Style, std::weak_ptr<types::StyledRect::SharedStyleData>>;
}

static pragma::gui::StyleCacheMap s_styleCache;

static std::shared_ptr<prosper::IUniformResizableBuffer> s_styleCacheBuffer;
static void initialize_style_cache_buffer(prosper::IPrContext &context, bool increaseCapacity = false)
{
	if(!increaseCapacity && s_styleCacheBuffer)
		return;
	size_t maxInstances = 512; // Not an absolute limit, but exceeding it will trigger re-allocation
	if(s_styleCacheBuffer)
		maxInstances = s_styleCacheBuffer->GetTotalInstanceCount() * 2;
	constexpr auto instanceSize = sizeof(pragma::gui::shaders::StyledRect::Style);
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.usageFlags = prosper::BufferUsageFlags::UniformBufferBit | prosper::BufferUsageFlags::TransferDstBit;
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::CPUToGPU | prosper::MemoryFeatureFlags::HostCoherent;
	createInfo.size = prosper::util::get_aligned_size(instanceSize * maxInstances, context.CalcBufferAlignment(createInfo.usageFlags));
	createInfo.flags |= prosper::util::BufferCreateInfo::Flags::Persistent;
	createInfo.debugName = "style_cache_buffer";
	auto buf = context.CreateUniformResizableBuffer(createInfo, instanceSize);
	buf->SetPermanentlyMapped(true, prosper::IBuffer::MapFlags::WriteBit);
	buf->SetReallocationBehavior(prosper::IBaseResizableBuffer::ReallocationBehavior::SafelyFreeOldBuffer);
	buf->SetResizable(false);
	if(s_styleCacheBuffer)
		context.KeepResourceAliveUntilPresentationComplete(s_styleCacheBuffer);
	s_styleCacheBuffer = buf;
}

void pragma::gui::types::StyledRect::clear_style_cache_buffer(prosper::IPrContext &context)
{
	if(!s_styleCacheBuffer)
		return;
	context.KeepResourceAliveUntilPresentationComplete(s_styleCacheBuffer);
	s_styleCacheBuffer = nullptr;
}

pragma::gui::shaders::StyledRect *pragma::gui::types::StyledRect::GetShader()
{
	static auto *shader = WGUI::GetInstance().GetShader<shaders::ShaderType::StyledRect>();
	return shader;
}

pragma::gui::types::StyledRect::StyledRect() : WIBase {} {}
void pragma::gui::types::StyledRect::DoUpdate() { WIBase::DoUpdate(); }
void pragma::gui::types::StyledRect::Initialize()
{
	WIBase::Initialize();
	initialize_style_cache_buffer(WGUI::GetInstance().GetContext());
	SetStyleDataDirty();
}
void pragma::gui::types::StyledRect::OnRemove()
{
	if(m_sharedData)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_sharedData);
}

void pragma::gui::types::StyledRect::SetStyleDataDirty() { m_styleDirty = true; }
void pragma::gui::types::StyledRect::ClearGradient()
{
	m_numGradientStops = 0;
	SetStyleDataDirty();
}
void pragma::gui::types::StyledRect::SetLinearGradient(const Color &startColor, const Color &endColor, const Vector2 &startPos, const Vector2 &endPos) { SetGradient(std::array<ColorStop, 2> {ColorStop {startColor, 0.f}, ColorStop {endColor, 1.f}}, startPos, endPos, GradientType::Linear); }
void pragma::gui::types::StyledRect::SetGradient(std::span<const ColorStop> stops, const Vector2 &startPos, const Vector2 &endPos, GradientType type)
{
	SetGradientStops(stops);
	SetGradientStart(startPos);
	SetGradientEnd(endPos);
	SetGradientType(type);
	SetStyleDataDirty();
}
void pragma::gui::types::StyledRect::SetGradientStops(std::span<const ColorStop> stops)
{
	m_numGradientStops = std::min<std::size_t>(stops.size(), m_gradientStops.size());
	std::copy_n(stops.begin(), m_numGradientStops, m_gradientStops.begin());
}
void pragma::gui::types::StyledRect::SetGradientType(GradientType type)
{
	m_gradientType = type;
	SetStyleDataDirty();
}
void pragma::gui::types::StyledRect::SetBorderColor(const Color &color)
{
	m_borderColor = color.ToVector4();
	SetStyleDataDirty();
}
void pragma::gui::types::StyledRect::SetCornerRadii(const Vector4 &radii)
{
	m_cornerRadii = radii;
	SetStyleDataDirty();
}
void pragma::gui::types::StyledRect::SetGradientStart(const Vector2 &start)
{
	m_gradientStart = start;
	SetStyleDataDirty();
}
void pragma::gui::types::StyledRect::SetGradientEnd(const Vector2 &end)
{
	m_gradientEnd = end;
	SetStyleDataDirty();
}
void pragma::gui::types::StyledRect::SetBorderThickness(float thickness)
{
	m_borderThickness = thickness;
	SetStyleDataDirty();
}
pragma::gui::types::StyledRect::GradientType pragma::gui::types::StyledRect::GetGradientType() const { return m_gradientType; }
const std::array<pragma::gui::types::StyledRect::ColorStop, 4> &pragma::gui::types::StyledRect::GetGradientStops() const { return m_gradientStops; }
const Color &pragma::gui::types::StyledRect::GetBorderColor() const { return m_borderColor; }
const Vector4 &pragma::gui::types::StyledRect::GetCornerRadii() const { return m_cornerRadii; }
const Vector2 &pragma::gui::types::StyledRect::GetGradientStart() const { return m_gradientStart; }
const Vector2 &pragma::gui::types::StyledRect::GetGradientEnd() const { return m_gradientEnd; }
float pragma::gui::types::StyledRect::GetBorderThickness() const { return m_borderThickness; }

pragma::gui::shaders::StyledRect::Style pragma::gui::types::StyledRect::ToStyleData() const
{
	shaders::StyledRect::Style styleData {};
	styleData.borderThickness = m_borderThickness;
	if(m_borderThickness > 0.f)
		styleData.borderColor = m_borderColor.ToVector4();
	styleData.cornerRadii = m_cornerRadii;
	styleData.gradientColorMode = static_cast<shaders::StyledRect::Style::GradientColorMode>(m_numGradientStops + 1);
	if(m_numGradientStops > 0) {
		styleData.gradientType = m_gradientType;
		styleData.gradientStart = m_gradientStart;
		styleData.gradientEnd = m_gradientEnd;
		for(size_t i = 0; i < m_numGradientStops; ++i) {
			styleData.gradientColors[i] = m_gradientStops[i].color.ToVector4();
			styleData.gradientColorStops[i] = m_gradientStops[i].position;
		}
	}
	return styleData;
}

void pragma::gui::types::StyledRect::UpdateDescriptorSet()
{
	if(!m_styleDirty)
		return;
	m_styleDirty = false;

	auto styleData = ToStyleData();

	// Check if style is already cached
	auto it = s_styleCache.find(styleData);
	if(it != s_styleCache.end()) {
		if(auto sharedData = it->second.lock()) {
			m_sharedData = sharedData;
			return;
		}
	}

	if(m_sharedData) {
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_sharedData);
		m_sharedData = nullptr;
	}

	if(!s_styleCacheBuffer)
		return;

	static uint32_t s_pruneCounter = 0;
	auto prune = []() {
		std::erase_if(s_styleCache, [](const auto &pair) { return pair.second.expired(); });
		s_pruneCounter = 0;
	};

	auto &context = WGUI::GetInstance().GetContext();
	if(s_styleCacheBuffer->GetFreeInstanceCount() == 0)
		prune();
	if(s_styleCacheBuffer->GetFreeInstanceCount() == 0) {
		initialize_style_cache_buffer(context, true);
		for(auto &[style, data] : s_styleCache) {
			if(data.expired())
				continue;
			data.lock()->invalidated = true;
		}
		s_styleCache.clear();
	}
	auto buf = s_styleCacheBuffer->AllocateBuffer(&styleData);
	if(!buf)
		return;

	m_sharedData = std::make_shared<SharedStyleData>();

	m_sharedData->buffer = buf;
	m_sharedData->dsg = context.CreateDescriptorSetGroup(shaders::StyledRect::DESCRIPTOR_SET_STYLE);
	m_sharedData->dsg->GetDescriptorSet()->SetBindingUniformBuffer(*m_sharedData->buffer, 0u);

	s_styleCache[styleData] = m_sharedData;

	if(++s_pruneCounter > 100)
		prune();
}

void pragma::gui::types::StyledRect::Render(const DrawInfo &drawInfo, DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale, uint32_t testStencilLevel, StencilPipeline stencilPipeline)
{
	auto col = drawInfo.GetColor(*this, drawState);
	if(col.a <= 0.f)
		return;

	col.a *= GetLocalAlpha();
	auto *shader = GetShader();
	if(!shader)
		return;

	if(m_sharedData && m_sharedData->invalidated) {
		m_sharedData = nullptr;
		m_styleDirty = true;
	}

	if(m_styleDirty)
		UpdateDescriptorSet();
	if(!m_sharedData)
		return;

	prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
	if(shader->RecordBeginDraw(bindState, drawState, drawInfo.size.x, drawInfo.size.y, stencilPipeline, math::is_flag_set(drawInfo.flags, DrawInfo::Flags::Msaa))) {
		shader->RecordDraw(bindState, ElementData {matDraw, col, ElementData::ToViewportSize(drawInfo.size)}, *m_sharedData->dsg->GetDescriptorSet(), testStencilLevel);

		shader->RecordEndDraw(bindState);
	}
}
