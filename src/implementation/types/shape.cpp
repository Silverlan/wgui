// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.gui;

import :types.shape;

#undef DrawState

class CachedBuffers {
  public:
	struct BufferInfo {
		std::shared_ptr<prosper::IBuffer> buffer = nullptr;
		std::shared_ptr<prosper::IBuffer> uvBuffer = nullptr;
		uint32_t vertexCount = 0;
	};
	BufferInfo &GetBuffer(const std::string &identifier);
  private:
	std::unordered_map<std::string, BufferInfo> m_cache;
};

CachedBuffers::BufferInfo &CachedBuffers::GetBuffer(const std::string &identifier)
{
	auto it = m_cache.find(identifier);
	if(it != m_cache.end())
		return it->second;
	if(identifier == "circle") {
		std::vector<Vector2> verts;
		auto addSegment = [this, &verts](float deg0, float deg1, float minDist, float maxDist) {
			auto r0 = umath::deg_to_rad(deg0);
			auto r1 = umath::deg_to_rad(deg1);

			verts.push_back(Vector2 {umath::sin(r0) * maxDist, -umath::cos(r0) * maxDist});
			verts.push_back(Vector2 {umath::sin(r0) * minDist, -umath::cos(r0) * minDist});
			verts.push_back(Vector2 {umath::sin(r1) * maxDist, -umath::cos(r1) * maxDist});

			verts.push_back(Vector2 {umath::sin(r0) * minDist, -umath::cos(r0) * minDist});
			verts.push_back(Vector2 {umath::sin(r1) * minDist, -umath::cos(r1) * minDist});
			verts.push_back(Vector2 {umath::sin(r1) * maxDist, -umath::cos(r1) * maxDist});
		};

		auto degRange = 360.f;
		auto minDist = 0.f;
		auto maxDist = 1.f;

		float stepSize = 8.f;
		verts.reserve(umath::ceil(degRange / stepSize));
		for(auto i = -degRange / 2.f; i <= (degRange / 2.f - stepSize); i += stepSize)
			addSegment(i, i + stepSize, minDist, maxDist);

		std::vector<Vector2> uvs;
		uvs.reserve(verts.size());
		for(auto &v : verts) {
			Vector2 uv {v.x + 1.f, v.y + 1.f};
			if(uv.x != 0.f)
				uv.x /= 2.f;
			if(uv.y != 0.f)
				uv.y /= 2.f;
			uvs.push_back(uv);
		}

		BufferInfo info {};
		info.vertexCount = verts.size();
		info.buffer = pragma::gui::types::WIShape::CreateBuffer(verts);
		info.uvBuffer = pragma::gui::types::WIShape::CreateBuffer(uvs);
		return (m_cache[identifier] = info);
	}
	throw std::runtime_error {"Invalid buffer type!"};
}

static std::unique_ptr<CachedBuffers> g_cache = nullptr;
static uint32_t g_count = 0;
pragma::gui::types::WIShape::WIShape() : WIBufferBase(), m_vertexBufferUpdateRequired(false)
{
	if(g_count++ == 0)
		g_cache = std::make_unique<CachedBuffers>();
}
pragma::gui::types::WIShape::~WIShape()
{
	if(--g_count == 0)
		g_cache = nullptr;
}
unsigned int pragma::gui::types::WIShape::AddVertex(Vector2 vert)
{
	m_vertices.push_back(vert);
	m_vertexBufferUpdateRequired |= 1;
	return static_cast<unsigned int>(m_vertices.size());
}
void pragma::gui::types::WIShape::SetVertexPos(unsigned int vertID, Vector2 pos)
{
	if(m_vertices.size() <= vertID)
		return;
	m_vertices[vertID] = pos;
	m_vertexBufferUpdateRequired |= 1;
}
void pragma::gui::types::WIShape::ClearVertices()
{
	m_vertices.clear();
	m_vertexBufferUpdateRequired |= 1;
}
bool pragma::gui::types::WIShape::DoPosInBounds(const Vector2i &pos) const
{
	if(m_checkInBounds)
		return m_checkInBounds(*this, pos);
	return WIBufferBase::DoPosInBounds(pos);
}
void pragma::gui::types::WIShape::SetShape(BasicShape shape)
{
	switch(shape) {
	case BasicShape::Rectangle:
		SetBoundsCheckFunction(nullptr);
		ClearVertices();
		ClearBuffer();
		break;
	case BasicShape::Circle:
		SetBoundsCheckFunction([](const WIShape &el, const Vector2i &pos) -> bool {
			Vector2 lpos {pos};
			auto ratio = el.GetWidth() / static_cast<float>(el.GetHeight());
			lpos -= Vector2 {el.GetHalfWidth(), el.GetHalfHeight()};
			lpos.y *= ratio;

			auto d = glm::gtx::length2(lpos);
			auto r = el.GetHalfWidth();
			return d < (r * r);
		});

		auto &info = g_cache->GetBuffer("circle");
		ClearVertices();
		SetBuffer(*info.buffer, info.vertexCount);
		auto *texShape = dynamic_cast<WITexturedShape *>(this);
		if(texShape)
			texShape->SetUVBuffer(*info.uvBuffer);
		break;
	}
}
void pragma::gui::types::WIShape::SetBoundsCheckFunction(const std::function<bool(const WIShape &, const Vector2i &)> &func) { m_checkInBounds = func; }
void pragma::gui::types::WIShape::DoUpdate()
{
	WIBase::DoUpdate();
	if(!(m_vertexBufferUpdateRequired & 1) || m_vertices.size() == 0)
		return;
	m_vertexBufferUpdateRequired &= ~1;

	auto buf = CreateBuffer(m_vertices);
	InitializeBufferData(*buf);
}
std::shared_ptr<prosper::IBuffer> pragma::gui::types::WIShape::CreateBuffer(const std::vector<Vector2> &verts)
{
	auto &context = WGUI::GetInstance().GetContext();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.size = verts.size() * sizeof(Vector2);
	createInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit;
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	auto buf = context.CreateBuffer(createInfo, verts.data());
	buf->SetDebugName("gui_shape_vertex_buf");
	return buf;
}
void pragma::gui::types::WIShape::SetBuffer(prosper::IBuffer &buffer, uint32_t numVerts)
{
	WIBufferBase::SetBuffer(buffer);
	m_bufferVertexCount = numVerts;
}
void pragma::gui::types::WIShape::ClearBuffer()
{
	WIBufferBase::ClearBuffer();
	m_bufferVertexCount = {};
}
unsigned int pragma::gui::types::WIShape::GetVertexCount() { return m_bufferVertexCount.has_value() ? *m_bufferVertexCount : static_cast<unsigned int>(m_vertices.size()); }
void pragma::gui::types::WIShape::InvertVertexPositions(bool x, bool y)
{
	for(auto &v : m_vertices) {
		if(x == true)
			v.x = 2.f - (v.x + 1.f) - 1.f;
		if(y == true)
			v.y = 2.f - (v.y + 1.f) - 1.f;
	}
	m_vertexBufferUpdateRequired |= 1;
	Update();
}

///////////////////

pragma::gui::types::WIOutlinedShape::WIOutlinedShape() : WIShape(), WILineBase() {}

///////////////////

pragma::gui::types::WITexturedShape::WITexturedShape() : WIShape(), m_hMaterial(), m_texture(), m_uvBuffer(nullptr), m_shader(), m_descSetTextureGroup(nullptr), m_texLoadCallback(nullptr)
{
	auto &instance = WGUI::GetInstance();
	auto *pShader = instance.GetTexturedShader();
	auto *pShaderCheap = instance.GetTexturedRectShader();
	if(pShader != nullptr)
		SetShader(*pShader, pShaderCheap);
	ReloadDescriptorSet();

	RegisterCallback<void>("OnMaterialChanged");
	RegisterCallback<void>("OnTextureChanged");
	RegisterCallback<void>("OnTextureApplied");
}
void pragma::gui::types::WITexturedShape::ClearBuffer()
{
	pragma::gui::types::WIShape::ClearBuffer();
	m_uvBuffer = nullptr;
}
void pragma::gui::types::WITexturedShape::SetShader(prosper::Shader &shader, prosper::Shader *shaderCheap)
{
	if(dynamic_cast<shaders::ShaderTextured *>(&shader) != nullptr)
		m_shader = shader.GetHandle();
	else
		m_shader = {};

	if(dynamic_cast<shaders::ShaderTexturedRect *>(shaderCheap) != nullptr)
		m_shaderCheap = shaderCheap->GetHandle();
	else
		m_shaderCheap = {};
}
void pragma::gui::types::WITexturedShape::ReloadDescriptorSet()
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error {"Attempted to reload GUI element descriptor set during rendering, this is not allowed!"};
	if(m_descSetTextureGroup)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_descSetTextureGroup);
	m_descSetTextureGroup = nullptr;
	if(shaders::ShaderTextured::DESCRIPTOR_SET_TEXTURE.IsValid() == false)
		return;
	m_descSetTextureGroup = WGUI::GetInstance().GetContext().CreateDescriptorSetGroup(shaders::ShaderTextured::DESCRIPTOR_SET_TEXTURE);
	//m_descSetTextureGroup = prosper::util::create_descriptor_set_group(WGUI::GetInstance().GetContext().GetDevice(),m_shader.get()->GetDescriptorSetGroup(),false); // TODO: FIXME
}
prosper::IBuffer &pragma::gui::types::WITexturedShape::GetUVBuffer() const { return *m_uvBuffer; }
void pragma::gui::types::WITexturedShape::SetUVBuffer(prosper::IBuffer &buffer)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error {"Attempted to change GUI element UV buffer during rendering, this is not allowed!"};
	m_uvBuffer = buffer.shared_from_this();
}
void pragma::gui::types::WITexturedShape::SetAlphaOnly(bool b) { umath::set_flag(m_stateFlags, StateFlags::AlphaOnly, b); }
bool pragma::gui::types::WITexturedShape::GetAlphaOnly() const { return umath::is_flag_set(m_stateFlags, StateFlags::AlphaOnly); }
float pragma::gui::types::WITexturedShape::GetLOD() const { return m_lod; }
void pragma::gui::types::WITexturedShape::SetLOD(float lod) { m_lod = lod; }
void pragma::gui::types::WITexturedShape::ClearTextureLoadCallback()
{
	if(m_texLoadCallback != nullptr)
		*m_texLoadCallback = false;
	m_texLoadCallback = nullptr;
}
void pragma::gui::types::WITexturedShape::InvertVertexUVCoordinates(bool x, bool y)
{
	for(auto &v : m_uvs) {
		if(x == true)
			v.x = 1.f - v.x;
		if(y == true)
			v.y = 1.f - v.y;
	}
	m_vertexBufferUpdateRequired |= 2;
	Update();
}
void pragma::gui::types::WITexturedShape::InitializeTextureLoadCallback(const std::shared_ptr<msys::Texture> &texture)
{
	auto hThis = GetHandle();
	m_texLoadCallback = std::make_shared<bool>(true);
	auto bLoadCallback = m_texLoadCallback;
	texture->CallOnLoaded([hThis, bLoadCallback](std::shared_ptr<msys::Texture> texture) mutable {
		if(!hThis.IsValid() || static_cast<WITexturedShape *>(hThis.get())->m_descSetTextureGroup == nullptr)
			return;

		if((bLoadCallback != nullptr && *bLoadCallback == false) || texture->HasValidVkTexture() == false) {
			static_cast<WITexturedShape *>(hThis.get())->m_descSetTextureGroup->GetDescriptorSet()->SetBindingTexture(*WGUI::GetInstance().GetContext().GetDummyTexture(), 0u);
			return;
		}
		WGUI::GetInstance().GetContext().WaitIdle(false); // Mustn't update the descriptor set if it may still be in use by a command buffer
		auto &descSet = *static_cast<WITexturedShape *>(hThis.get())->m_descSetTextureGroup->GetDescriptorSet();
		descSet.SetBindingTexture(*texture->GetVkTexture(), 0u);
		descSet.Update();
		static_cast<WITexturedShape *>(hThis.get())->m_texUpdateCountRef = texture->GetUpdateCount();
		static_cast<WITexturedShape *>(hThis.get())->CallCallbacks<void>("OnTextureApplied");
	});
}
void pragma::gui::types::WITexturedShape::UpdateMaterialDescriptorSetTexture()
{
	// if(WGUI::GetInstance().IsLockedForDrawing())
	// 	throw std::runtime_error{"Attempted to update material descriptor set texture during rendering, this is not allowed!"};
	if(!m_descSetTextureGroup)
		return;
	util::ScopeGuard sgDummy {[this]() {
		// Mustn't update the descriptor set if it may still be in use by a command buffer
		auto &descSet = *m_descSetTextureGroup->GetDescriptorSet();
		descSet.SetBindingTexture(*WGUI::GetInstance().GetContext().GetDummyTexture(), 0u);
		descSet.Update();
	}};
	if(!m_hMaterial || !m_descSetTextureGroup)
		return;
	auto *diffuseMap = m_hMaterial->GetDiffuseMap();
	if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
		return;
	auto diffuseTexture = std::static_pointer_cast<msys::Texture>(diffuseMap->texture);
	if(diffuseTexture == nullptr)
		return;
	sgDummy.dismiss();
	auto &prTex = diffuseTexture->GetVkTexture();
	// Mustn't update the descriptor set if it may still be in use by a command buffer
	auto &descSet = *m_descSetTextureGroup->GetDescriptorSet();
	descSet.SetBindingTexture(*prTex, 0u);
	descSet.Update();
	m_texUpdateCountRef = diffuseTexture->GetUpdateCount();
	m_matUpdateCountRef = m_hMaterial->GetUpdateIndex();
}
void pragma::gui::types::WITexturedShape::SetMaterial(msys::Material *material)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error {"Attempted to change GUI element material during rendering, this is not allowed!"};
	util::ScopeGuard sg {[this]() {
		UpdateTransparencyState();
		CallCallbacks<void>("OnMaterialChanged");
	}};
	ClearTexture();
	m_hMaterial = material ? material->GetHandle() : msys::MaterialHandle {};
	if(!m_hMaterial)
		return;
	m_materialColor = material->GetProperty<Vector4>("color_factor", Vector4 {1.f, 1.f, 1.f, 1.f});
	auto *diffuseMap = m_hMaterial->GetDiffuseMap();
	if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
		return;
	auto diffuseTexture = std::static_pointer_cast<msys::Texture>(diffuseMap->texture);
	if(diffuseTexture == nullptr)
		return;
	InitializeTextureLoadCallback(diffuseTexture);
}
void pragma::gui::types::WITexturedShape::SetMaterial(const std::string &material)
{
	auto *mat = WGUI::GetInstance().GetMaterialLoadHandler()(material);
	if(mat == nullptr)
		SetMaterial(nullptr);
	else
		SetMaterial(mat);
}
msys::Material *pragma::gui::types::WITexturedShape::GetMaterial()
{
	if(!m_hMaterial)
		return nullptr;
	return m_hMaterial.get();
}
void pragma::gui::types::WITexturedShape::ClearTexture()
{
	ClearTextureLoadCallback();
	m_materialColor = {1.f, 1.f, 1.f, 1.f};
	m_hMaterial = nullptr;
	if(m_texture)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_texture);
	m_texture = nullptr;
}
void pragma::gui::types::WITexturedShape::SetTexture(prosper::Texture &tex, std::optional<uint32_t> layerIndex)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error {"Attempted to change GUI element material during rendering, this is not allowed!"};
	ClearTexture();
	m_texture = tex.shared_from_this();

	// Mustn't update the descriptor set if it may still be in use by a command buffer
	ReloadDescriptorSet(); // Need to generate a new descriptor set and keep the old one alive, in case it was still in use
	if(layerIndex.has_value())
		m_descSetTextureGroup->GetDescriptorSet()->SetBindingTexture(tex, 0u, *layerIndex);
	else
		m_descSetTextureGroup->GetDescriptorSet()->SetBindingTexture(tex, 0u);
	m_descSetTextureGroup->GetDescriptorSet()->Update();

	CallCallbacks<void>("OnTextureChanged");
	CallCallbacks<void>("OnTextureApplied");
}
const std::shared_ptr<prosper::Texture> &pragma::gui::types::WITexturedShape::GetTexture() const
{
	if(m_texture || !m_hMaterial)
		return m_texture;
	auto *diffuseMap = m_hMaterial.get()->GetDiffuseMap();
	if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
		return m_texture;
	auto diffuseTexture = std::static_pointer_cast<msys::Texture>(diffuseMap->texture);
	if(diffuseTexture == nullptr)
		return m_texture;
	return diffuseTexture->GetVkTexture();
}

void pragma::gui::types::WITexturedShape::SetVertexUVCoord(unsigned int vertID, Vector2 uv)
{
	if(m_uvs.size() <= vertID)
		return;
	m_uvs[vertID] = uv;
	m_vertexBufferUpdateRequired |= 2;
}
void pragma::gui::types::WITexturedShape::ClearVertices()
{
	pragma::gui::types::WIShape::ClearVertices();
	m_uvs.clear();
}
pragma::gui::types::WITexturedShape::~WITexturedShape()
{
	if(m_uvBuffer != nullptr)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_uvBuffer);
	if(m_descSetTextureGroup)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_descSetTextureGroup);
	if(m_texture)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_texture);
	ClearTextureLoadCallback();
}
std::ostream &pragma::gui::types::WITexturedShape::Print(std::ostream &stream) const
{
	pragma::gui::types::WIShape::Print(stream);
	stream << "[Mat:";
	if(m_hMaterial)
		stream << const_cast<msys::Material *>(m_hMaterial.get())->GetName();
	else
		stream << "NULL";
	stream << "]";

	stream << "[Tex:";
	if(m_texture)
		stream << m_texture->GetDebugName();
	else
		stream << "NULL";
	stream << "]";
	return stream;
}
unsigned int pragma::gui::types::WITexturedShape::AddVertex(Vector2 vert)
{
	Vector2 uv(vert.x + 1.f, vert.y + 1.f);
	if(uv.x != 0.f)
		uv.x /= 2.f;
	if(uv.y != 0.f)
		uv.y /= 2.f;
	return AddVertex(vert, uv);
}
unsigned int pragma::gui::types::WITexturedShape::AddVertex(Vector2 vert, Vector2 uv)
{
	m_uvs.push_back(uv);
	m_vertexBufferUpdateRequired |= 2;
	return pragma::gui::types::WIShape::AddVertex(vert);
}
std::shared_ptr<prosper::IBuffer> pragma::gui::types::WITexturedShape::CreateUvBuffer(const std::vector<Vector2> &uvs)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error {"Attempted to update GUI element UV buffer during rendering, this is not allowed!"};
	auto &context = WGUI::GetInstance().GetContext();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit;
	createInfo.size = uvs.size() * sizeof(Vector2);
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	return context.CreateBuffer(createInfo, uvs.data());
}
void pragma::gui::types::WITexturedShape::DoUpdate()
{
	pragma::gui::types::WIShape::DoUpdate();
	if(!(m_vertexBufferUpdateRequired & 2) || m_uvs.size() == 0)
		return;
	m_vertexBufferUpdateRequired &= ~2;
	m_uvBuffer = CreateUvBuffer(m_uvs);
}
void pragma::gui::types::WITexturedShape::SetChannelSwizzle(shaders::ShaderTextured::Channel dst, shaders::ShaderTextured::Channel src) { m_channels.at(umath::to_integral(dst)) = src; }
pragma::gui::shaders::ShaderTextured::Channel pragma::gui::types::WITexturedShape::GetChannelSwizzle(shaders::ShaderTextured::Channel channel) const { return m_channels.at(umath::to_integral(channel)); }
void pragma::gui::types::WITexturedShape::SetShader(shaders::ShaderTextured &shader)
{
	m_shader = shader.GetHandle();
	m_stateFlags |= StateFlags::ShaderOverride;
}
void pragma::gui::types::WITexturedShape::SetAlphaMode(AlphaMode alphaMode)
{
	m_alphaMode = alphaMode;
	UpdateShaderState();
}
void pragma::gui::types::WITexturedShape::SetAlphaCutoff(float alphaCutoff) { m_alphaCutoff = alphaCutoff; }
AlphaMode pragma::gui::types::WITexturedShape::GetAlphaMode() const { return m_alphaMode; }
float pragma::gui::types::WITexturedShape::GetAlphaCutoff() const { return m_alphaCutoff; }
void pragma::gui::types::WITexturedShape::UpdateTransparencyState()
{
	WIBase::UpdateTransparencyState();
	if(umath::is_flag_set(WIBase::m_stateFlags, WIBase::StateFlags::FullyTransparent))
		return;
	auto *mat = GetMaterial();
	if(!mat)
		return;
	const char *transparent = "transparent.";
	auto fullyTransparent = ustring::compare(mat->GetName().c_str(), transparent, false, strlen(transparent));
	umath::set_flag(WIBase::m_stateFlags, WIBase::StateFlags::FullyTransparent, fullyTransparent);
}
void pragma::gui::types::WITexturedShape::UpdateShaderState() { umath::set_flag(m_stateFlags, StateFlags::ExpensiveShaderRequired, m_alphaMode != AlphaMode::Blend); }
Vector2i pragma::gui::types::WITexturedShape::GetTextureSize() const
{
	if(m_texture == nullptr && m_hMaterial == nullptr)
		return {};
	uint32_t width, height;
	if(m_texture) {
		auto extents = m_texture->GetImage().GetExtents();
		width = extents.width;
		height = extents.height;
	}
	else {
		auto *diffuseMap = m_hMaterial.get()->GetDiffuseMap();
		if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
			return {};
		width = diffuseMap->width;
		height = diffuseMap->height;
	}
	return {width, height};
}
void pragma::gui::types::WITexturedShape::SizeToTexture()
{
	auto size = GetTextureSize();
	SetSize(size.x, size.y);
}
bool pragma::gui::types::WITexturedShape::PrepareRender(const DrawInfo &drawInfo, DrawState &drawState, Vector4 &outColor)
{
	if(m_hMaterial == nullptr && m_texture == nullptr)
		return false;
	auto col = drawInfo.GetColor(*this, drawState);
	if(col.a <= 0.f)
		return false;
	col.a *= GetLocalAlpha();
	if(m_hMaterial) {
		auto *map = m_hMaterial->GetDiffuseMap();
		if(!map || !map->texture)
			return false;
		if(m_hMaterial->GetUpdateIndex() != m_matUpdateCountRef)
			UpdateMaterialDescriptorSetTexture();
		else {
			auto *tex = static_cast<msys::Texture *>(map->texture.get());
			if(tex->GetUpdateCount() != m_texUpdateCountRef)
				UpdateMaterialDescriptorSetTexture();
		}
	}
	col *= m_materialColor;
	outColor = col;
	return true;
}
void pragma::gui::types::WITexturedShape::Render(const DrawInfo &drawInfo, DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale, uint32_t testStencilLevel, StencilPipeline stencilPipeline)
{
	Vector4 col;
	if(!PrepareRender(drawInfo, drawState, col))
		return;

	// Try to use cheap shader if no custom vertex buffer was used
	if(umath::is_flag_set(m_stateFlags, StateFlags::ShaderOverride) == false && ((m_vertexBufferData == nullptr && m_uvBuffer == nullptr) || m_shader.expired())) {
		if(umath::is_flag_set(m_stateFlags, StateFlags::ExpensiveShaderRequired)) {
			auto *pShaderExpensive = static_cast<shaders::ShaderTexturedRectExpensive *>(WGUI::GetInstance().GetTexturedRectExpensiveShader());
			if(pShaderExpensive == nullptr)
				return;
			auto &context = WGUI::GetInstance().GetContext();
			prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
			if(pShaderExpensive->RecordBeginDraw(bindState, drawState, drawInfo.size.x, drawInfo.size.y, stencilPipeline, umath::is_flag_set(drawInfo.flags, DrawInfo::Flags::Msaa)) == true) {
				pShaderExpensive->RecordDraw(bindState,
				  {matDraw, col, ElementData::ToViewportSize(drawInfo.size), std::array<uint32_t, 3> {}, umath::is_flag_set(m_stateFlags, StateFlags::AlphaOnly) ? 1 : 0, m_lod, m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Red)),
				    m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Green)), m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Blue)), m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Alpha)), GetAlphaMode(), GetAlphaCutoff()},
				  *m_descSetTextureGroup->GetDescriptorSet(0u), testStencilLevel);
				pShaderExpensive->RecordEndDraw(bindState);
			}
			return;
		}
		auto *pShaderCheap = static_cast<shaders::ShaderTexturedRect *>(GetCheapShader());
		if(pShaderCheap == nullptr)
			return;
		auto &context = WGUI::GetInstance().GetContext();
		prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
		if(pShaderCheap->RecordBeginDraw(bindState, drawState, drawInfo.size.x, drawInfo.size.y, stencilPipeline, umath::is_flag_set(drawInfo.flags, DrawInfo::Flags::Msaa)) == true) {
			pShaderCheap->RecordDraw(bindState,
			  {matDraw, col, ElementData::ToViewportSize(drawInfo.size), std::array<uint32_t, 3> {}, umath::is_flag_set(m_stateFlags, StateFlags::AlphaOnly) ? 1 : 0, m_lod, m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Red)),
			    m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Green)), m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Blue)), m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Alpha))},
			  *m_descSetTextureGroup->GetDescriptorSet(0u), testStencilLevel);
			pShaderCheap->RecordEndDraw(bindState);
		}
		return;
	}
	//

	if(m_shader.expired())
		return;
	auto &shader = static_cast<shaders::ShaderTextured &>(*m_shader.get());
	auto &context = WGUI::GetInstance().GetContext();
	auto vbuf = (m_vertexBufferData != nullptr) ? m_vertexBufferData->GetBuffer() : context.GetCommonBufferCache().GetSquareVertexBuffer();
	auto uvBuf = (m_uvBuffer != nullptr) ? m_uvBuffer : context.GetCommonBufferCache().GetSquareUvBuffer();
	if(vbuf == nullptr || uvBuf == nullptr)
		return;
	prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
	if(shader.RecordBeginDraw(bindState, drawState, drawInfo.size.x, drawInfo.size.y, stencilPipeline, umath::is_flag_set(drawInfo.flags, DrawInfo::Flags::Msaa)) == true) {
		BindShader(shader, bindState, drawState);

		shaders::ShaderTextured::PushConstants pushConstants {};
		pushConstants.elementData.modelMatrix = matDraw;
		pushConstants.elementData.color = col;
		pushConstants.elementData.viewportSize = ElementData::ToViewportSize(drawInfo.size);
		pushConstants.lod = m_lod;
		pushConstants.red = m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Red));
		pushConstants.green = m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Green));
		pushConstants.blue = m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Blue));
		pushConstants.alpha = m_channels.at(umath::to_integral(shaders::ShaderTextured::Channel::Alpha));
		shader.RecordDraw(bindState, vbuf, uvBuf, GetVertexCount(), *m_descSetTextureGroup->GetDescriptorSet(0u), pushConstants, testStencilLevel);
		shader.RecordEndDraw(bindState);
	}
}

void pragma::gui::types::WITexturedShape::BindShader(shaders::ShaderTextured &shader, prosper::ShaderBindState &bindState, DrawState &drawState) {}
