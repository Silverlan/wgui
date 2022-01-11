/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "texturemanager/texturemanager.h"
#include "wgui/types/wishape.h"
#include "cmaterialmanager.h"
#include "textureinfo.h"
#include "wgui/shaders/wishader_textured.hpp"
#include <sharedutils/scope_guard.h>
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_util.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>

LINK_WGUI_TO_CLASS(WIShape,WIShape);
LINK_WGUI_TO_CLASS(WITexturedShape,WITexturedShape);

class CachedBuffers
{
public:
	struct BufferInfo
	{
		std::shared_ptr<prosper::IBuffer> buffer = nullptr;
		std::shared_ptr<prosper::IBuffer> uvBuffer = nullptr;
		uint32_t vertexCount = 0;
	};
	BufferInfo &GetBuffer(const std::string &identifier);
private:
	std::unordered_map<std::string,BufferInfo> m_cache;
};

CachedBuffers::BufferInfo &CachedBuffers::GetBuffer(const std::string &identifier)
{
	auto it = m_cache.find(identifier);
	if(it != m_cache.end())
		return it->second;
	if(identifier == "circle")
	{
		std::vector<Vector2> verts;
		auto addSegment = [this,&verts](float deg0,float deg1,float minDist,float maxDist) {
			auto r0 = umath::deg_to_rad(deg0);
			auto r1 = umath::deg_to_rad(deg1);

			verts.push_back(Vector2{umath::sin(r0) *maxDist,-umath::cos(r0) *maxDist});
			verts.push_back(Vector2{umath::sin(r0) *minDist,-umath::cos(r0) *minDist});
			verts.push_back(Vector2{umath::sin(r1) *maxDist,-umath::cos(r1) *maxDist});

			verts.push_back(Vector2{umath::sin(r0) *minDist,-umath::cos(r0) *minDist});
			verts.push_back(Vector2{umath::sin(r1) *minDist,-umath::cos(r1) *minDist});
			verts.push_back(Vector2{umath::sin(r1) *maxDist,-umath::cos(r1) *maxDist});
		};

		auto degRange = 360.f;
		auto minDist = 0.f;
		auto maxDist = 1.f;

		float stepSize = 8.f;
		verts.reserve(umath::ceil(degRange /stepSize));
		for(auto i=-degRange /2.f;i<=(degRange /2.f -stepSize);i+=stepSize)
			addSegment(i,i +stepSize,minDist,maxDist);

		std::vector<Vector2> uvs;
		uvs.reserve(verts.size());
		for(auto &v : verts)
		{
			Vector2 uv{v.x +1.f,v.y +1.f};
			if(uv.x != 0.f) uv.x /= 2.f;
			if(uv.y != 0.f) uv.y /= 2.f;
			uvs.push_back(uv);
		}

		BufferInfo info {};
		info.vertexCount = verts.size();
		info.buffer = WIShape::CreateBuffer(verts);
		info.uvBuffer = WIShape::CreateBuffer(uvs);
		return (m_cache[identifier] = info);
	}
	throw std::runtime_error{"Invalid buffer type!"};
}

static std::unique_ptr<CachedBuffers> g_cache = nullptr;
static uint32_t g_count = 0;
WIShape::WIShape()
	: WIBufferBase(),m_vertexBufferUpdateRequired(false)
{
	if(g_count++ == 0)
		g_cache = std::make_unique<CachedBuffers>();
}
WIShape::~WIShape()
{
	if(--g_count == 0)
		g_cache = nullptr;
}
unsigned int WIShape::AddVertex(Vector2 vert)
{
	m_vertices.push_back(vert);
	m_vertexBufferUpdateRequired |= 1;
	return static_cast<unsigned int>(m_vertices.size());
}
void WIShape::SetVertexPos(unsigned int vertID,Vector2 pos)
{
	if(m_vertices.size() <= vertID)
		return;
	m_vertices[vertID] = pos;
	m_vertexBufferUpdateRequired |= 1;
}
void WIShape::ClearVertices() {m_vertices.clear(); m_vertexBufferUpdateRequired |= 1;}
bool WIShape::DoPosInBounds(const Vector2i &pos) const
{
	if(m_checkInBounds)
		return m_checkInBounds(*this,pos);
	return WIBufferBase::DoPosInBounds(pos);
}
void WIShape::SetShape(BasicShape shape)
{
	switch(shape)
	{
	case BasicShape::Rectangle:
		SetBoundsCheckFunction(nullptr);
		ClearVertices();
		ClearBuffer();
		break;
	case BasicShape::Circle:
		SetBoundsCheckFunction([](const WIShape &el,const Vector2i &pos) -> bool {
			Vector2 lpos {pos};
			auto ratio = el.GetWidth() /static_cast<float>(el.GetHeight());
			lpos -= Vector2{el.GetHalfWidth(),el.GetHalfHeight()};
			lpos.y *= ratio;

			auto d = glm::length2(lpos);
			auto r = el.GetHalfWidth();
			return d < (r *r);
		});

		auto &info = g_cache->GetBuffer("circle");
		ClearVertices();
		SetBuffer(*info.buffer,info.vertexCount);
		auto *texShape = dynamic_cast<WITexturedShape*>(this);
		if(texShape)
			texShape->SetUVBuffer(*info.uvBuffer);
		break;
	}
}
void WIShape::SetBoundsCheckFunction(const std::function<bool(const WIShape&,const Vector2i&)> &func) {m_checkInBounds = func;}
void WIShape::DoUpdate()
{
	WIBase::DoUpdate();
	if(!(m_vertexBufferUpdateRequired &1) || m_vertices.size() == 0)
		return;
	m_vertexBufferUpdateRequired &= ~1;
	
	auto buf = CreateBuffer(m_vertices);
	InitializeBufferData(*buf);
}
std::shared_ptr<prosper::IBuffer> WIShape::CreateBuffer(const std::vector<Vector2> &verts)
{
	auto &context = WGUI::GetInstance().GetContext();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.size = verts.size() *sizeof(Vector2);
	createInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit;
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	auto buf = context.CreateBuffer(createInfo,verts.data());
	buf->SetDebugName("gui_shape_vertex_buf");
	return buf;
}
void WIShape::SetBuffer(prosper::IBuffer &buffer,uint32_t numVerts)
{
	WIBufferBase::SetBuffer(buffer);
	m_bufferVertexCount = numVerts;
}
void WIShape::ClearBuffer()
{
	WIBufferBase::ClearBuffer();
	m_bufferVertexCount = {};
}
unsigned int WIShape::GetVertexCount() {return m_bufferVertexCount.has_value() ? *m_bufferVertexCount : static_cast<unsigned int>(m_vertices.size());}
void WIShape::InvertVertexPositions(bool x,bool y)
{
	for(auto &v : m_vertices)
	{
		if(x == true)
			v.x = 2.f -(v.x +1.f) -1.f;
		if(y == true)
			v.y = 2.f -(v.y +1.f) -1.f;
	}
	m_vertexBufferUpdateRequired |= 1;
	Update();
}

///////////////////

WIOutlinedShape::WIOutlinedShape()
	: WIShape(),WILineBase()
{}

///////////////////

WITexturedShape::WITexturedShape()
	: WIShape(),m_hMaterial(),m_texture(),
	m_uvBuffer(nullptr),m_shader(),m_descSetTextureGroup(nullptr),m_texLoadCallback(nullptr)
{
	auto &instance = WGUI::GetInstance();
	auto *pShader = instance.GetTexturedShader();
	auto *pShaderCheap = instance.GetTexturedRectShader();
	if(pShader != nullptr)
		SetShader(*pShader,pShaderCheap);
	ReloadDescriptorSet();
}
void WITexturedShape::ClearBuffer()
{
	WIShape::ClearBuffer();
	m_uvBuffer = nullptr;
}
void WITexturedShape::SetShader(prosper::Shader &shader,prosper::Shader *shaderCheap)
{
	if(dynamic_cast<wgui::ShaderTextured*>(&shader) != nullptr)
		m_shader = shader.GetHandle();
	else
		m_shader = {};
	
	if(dynamic_cast<wgui::ShaderTexturedRect*>(shaderCheap) != nullptr)
		m_shaderCheap = shaderCheap->GetHandle();
	else
		m_shaderCheap = {};
}
void WITexturedShape::ReloadDescriptorSet()
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error{"Attempted to reload GUI element descriptor set during rendering, this is not allowed!"};
	m_descSetTextureGroup = nullptr;
	if(wgui::ShaderTextured::DESCRIPTOR_SET_TEXTURE.IsValid() == false)
		return;
	m_descSetTextureGroup = WGUI::GetInstance().GetContext().CreateDescriptorSetGroup(wgui::ShaderTextured::DESCRIPTOR_SET_TEXTURE);
	//m_descSetTextureGroup = prosper::util::create_descriptor_set_group(WGUI::GetInstance().GetContext().GetDevice(),m_shader.get()->GetDescriptorSetGroup(),false); // TODO: FIXME
}
prosper::IBuffer &WITexturedShape::GetUVBuffer() const {return *m_uvBuffer;}
void WITexturedShape::SetUVBuffer(prosper::IBuffer &buffer)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error{"Attempted to change GUI element UV buffer during rendering, this is not allowed!"};
	m_uvBuffer = buffer.shared_from_this();
}
void WITexturedShape::SetAlphaOnly(bool b) {umath::set_flag(m_stateFlags,StateFlags::AlphaOnly,b);}
bool WITexturedShape::GetAlphaOnly() const {return umath::is_flag_set(m_stateFlags,StateFlags::AlphaOnly);}
float WITexturedShape::GetLOD() const {return m_lod;}
void WITexturedShape::SetLOD(float lod) {m_lod = lod;}
void WITexturedShape::ClearTextureLoadCallback()
{
	if(m_texLoadCallback != nullptr)
		*m_texLoadCallback = false;
	m_texLoadCallback = nullptr;
}
void WITexturedShape::InvertVertexUVCoordinates(bool x,bool y)
{
	for(auto &v : m_uvs)
	{
		if(x == true)
			v.x = 1.f -v.x;
		if(y == true)
			v.y = 1.f -v.y;
	}
	m_vertexBufferUpdateRequired |= 2;
	Update();
}
void WITexturedShape::InitializeTextureLoadCallback(const std::shared_ptr<Texture> &texture)
{
	auto hThis = GetHandle();
	m_texLoadCallback = std::make_shared<bool>(true);
	auto bLoadCallback = m_texLoadCallback;
	texture->CallOnLoaded([hThis,bLoadCallback](std::shared_ptr<Texture> texture) mutable {
		if(!hThis.IsValid() || static_cast<WITexturedShape*>(hThis.get())->m_descSetTextureGroup == nullptr)
			return;

		if((bLoadCallback != nullptr && *bLoadCallback == false) || texture->HasValidVkTexture() == false)
		{
			static_cast<WITexturedShape*>(hThis.get())->m_descSetTextureGroup->GetDescriptorSet()->SetBindingTexture(*WGUI::GetInstance().GetContext().GetDummyTexture(),0u);
			return;
		}
		WGUI::GetInstance().GetContext().WaitIdle(); // Mustn't update the descriptor set if it may still be in use by a command buffer
		auto &descSet = *static_cast<WITexturedShape*>(hThis.get())->m_descSetTextureGroup->GetDescriptorSet();
		descSet.SetBindingTexture(*texture->GetVkTexture(),0u);
		descSet.Update();
		static_cast<WITexturedShape*>(hThis.get())->m_texUpdateCountRef = texture->GetUpdateCount();
	});
}
void WITexturedShape::UpdateMaterialDescriptorSetTexture()
{
	// if(WGUI::GetInstance().IsLockedForDrawing())
	// 	throw std::runtime_error{"Attempted to update material descriptor set texture during rendering, this is not allowed!"};
	if(!m_descSetTextureGroup)
		return;
	util::ScopeGuard sgDummy {[this]() {
		 // Mustn't update the descriptor set if it may still be in use by a command buffer
		auto &descSet = *m_descSetTextureGroup->GetDescriptorSet();
		descSet.SetBindingTexture(*WGUI::GetInstance().GetContext().GetDummyTexture(),0u);
		descSet.Update();
	}};
	if(!m_hMaterial || !m_descSetTextureGroup)
		return;
	auto *diffuseMap = m_hMaterial->GetDiffuseMap();
	if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
		return;
	auto diffuseTexture = std::static_pointer_cast<Texture>(diffuseMap->texture);
	if(diffuseTexture == nullptr)
		return;
	sgDummy.dismiss();
	auto &prTex = diffuseTexture->GetVkTexture();
	 // Mustn't update the descriptor set if it may still be in use by a command buffer
	auto &descSet = *m_descSetTextureGroup->GetDescriptorSet();
	descSet.SetBindingTexture(*prTex,0u);
	descSet.Update();
	m_texUpdateCountRef = diffuseTexture->GetUpdateCount();
	m_matUpdateCountRef = m_hMaterial->GetUpdateIndex();
}
void WITexturedShape::SetMaterial(Material *material)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error{"Attempted to change GUI element material during rendering, this is not allowed!"};
	util::ScopeGuard sg {[this]() {UpdateTransparencyState();}};
	ClearTexture();
	m_hMaterial = material ? material->GetHandle() : msys::MaterialHandle{};
	if(!m_hMaterial)
		return;
	auto *diffuseMap = m_hMaterial->GetDiffuseMap();
	if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
		return;
	auto diffuseTexture = std::static_pointer_cast<Texture>(diffuseMap->texture);
	if(diffuseTexture == nullptr)
		return;
	InitializeTextureLoadCallback(diffuseTexture);
}
void WITexturedShape::SetMaterial(const std::string &material)
{
	auto *mat = WGUI::GetInstance().GetMaterialLoadHandler()(material);
	if(mat == nullptr)
		SetMaterial(nullptr);
	else
		SetMaterial(mat);
}
Material *WITexturedShape::GetMaterial()
{
	if(!m_hMaterial)
		return nullptr;
	return m_hMaterial.get();
}
void WITexturedShape::ClearTexture()
{
	ClearTextureLoadCallback();
	m_hMaterial = nullptr;
	m_texture = nullptr;
}
void WITexturedShape::SetTexture(prosper::Texture &tex,std::optional<uint32_t> layerIndex)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error{"Attempted to change GUI element material during rendering, this is not allowed!"};
	ClearTexture();
	m_texture = tex.shared_from_this();
	
	// Mustn't update the descriptor set if it may still be in use by a command buffer
	ReloadDescriptorSet(); // Need to generate a new descriptor set and keep the old one alive, in case it was still in use
	if(layerIndex.has_value())
		m_descSetTextureGroup->GetDescriptorSet()->SetBindingTexture(tex,0u,*layerIndex);
	else
		m_descSetTextureGroup->GetDescriptorSet()->SetBindingTexture(tex,0u);
	m_descSetTextureGroup->GetDescriptorSet()->Update();
}
const std::shared_ptr<prosper::Texture> &WITexturedShape::GetTexture() const
{
	if(m_texture || !m_hMaterial)
		return m_texture;
	auto *diffuseMap = m_hMaterial.get()->GetDiffuseMap();
	if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
		return m_texture;
	auto diffuseTexture = std::static_pointer_cast<Texture>(diffuseMap->texture);
	if(diffuseTexture == nullptr)
		return m_texture;
	return diffuseTexture->GetVkTexture();
}

void WITexturedShape::SetVertexUVCoord(unsigned int vertID,Vector2 uv)
{
	if(m_uvs.size() <= vertID)
		return;
	m_uvs[vertID] = uv;
	m_vertexBufferUpdateRequired |= 2;
}
void WITexturedShape::ClearVertices()
{
	WIShape::ClearVertices();
	m_uvs.clear();
}
WITexturedShape::~WITexturedShape()
{
	if(m_uvBuffer != nullptr)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_uvBuffer);
	if(m_descSetTextureGroup)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_descSetTextureGroup);
	if(m_texture)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_texture);
	ClearTextureLoadCallback();
}
std::ostream &WITexturedShape::Print(std::ostream &stream) const
{
	WIShape::Print(stream);
	stream<<"[Mat:";
	if(m_hMaterial)
		stream<<m_hMaterial.get()->GetName();
	else
		stream<<"NULL";
	stream<<"]";

	stream<<"[Tex:";
	if(m_texture)
		stream<<m_texture->GetDebugName();
	else
		stream<<"NULL";
	stream<<"]";
	return stream;
}
unsigned int WITexturedShape::AddVertex(Vector2 vert)
{
	Vector2 uv(vert.x +1.f,vert.y +1.f);
	if(uv.x != 0.f) uv.x /= 2.f;
	if(uv.y != 0.f) uv.y /= 2.f;
	return AddVertex(vert,uv);
}
unsigned int WITexturedShape::AddVertex(Vector2 vert,Vector2 uv)
{
	m_uvs.push_back(uv);
	m_vertexBufferUpdateRequired |= 2;
	return WIShape::AddVertex(vert);
}
std::shared_ptr<prosper::IBuffer> WITexturedShape::CreateUvBuffer(const std::vector<Vector2> &uvs)
{
	if(WGUI::GetInstance().IsLockedForDrawing())
		throw std::runtime_error{"Attempted to update GUI element UV buffer during rendering, this is not allowed!"};
	auto &context = WGUI::GetInstance().GetContext();
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit;
	createInfo.size = uvs.size() *sizeof(Vector2);
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	return context.CreateBuffer(createInfo,uvs.data());
}
void WITexturedShape::DoUpdate()
{
	WIShape::DoUpdate();
	if(!(m_vertexBufferUpdateRequired &2) || m_uvs.size() == 0)
		return;
	m_vertexBufferUpdateRequired &= ~2;
	m_uvBuffer = CreateUvBuffer(m_uvs);
}
void WITexturedShape::SetChannelSwizzle(wgui::ShaderTextured::Channel dst,wgui::ShaderTextured::Channel src)
{
	m_channels.at(umath::to_integral(src)) = dst;
}
wgui::ShaderTextured::Channel WITexturedShape::GetChannelSwizzle(wgui::ShaderTextured::Channel channel) const
{
	return m_channels.at(umath::to_integral(channel));
}
void WITexturedShape::SetShader(wgui::ShaderTextured &shader)
{
	m_shader = shader.GetHandle();
	m_stateFlags |= StateFlags::ShaderOverride;
}
void WITexturedShape::SetAlphaMode(AlphaMode alphaMode) {m_alphaMode = alphaMode; UpdateShaderState();}
void WITexturedShape::SetAlphaCutoff(float alphaCutoff) {m_alphaCutoff = alphaCutoff;}
AlphaMode WITexturedShape::GetAlphaMode() const {return m_alphaMode;}
float WITexturedShape::GetAlphaCutoff() const {return m_alphaCutoff;}
void WITexturedShape::UpdateTransparencyState()
{
	WIBase::UpdateTransparencyState();
	if(umath::is_flag_set(WIBase::m_stateFlags,WIBase::StateFlags::FullyTransparent))
		return;
	auto *mat = GetMaterial();
	if(!mat)
		return;
	const char *transparent = "transparent.";
	auto fullyTransparent = ustring::compare(mat->GetName().c_str(),transparent,false,strlen(transparent));
	umath::set_flag(WIBase::m_stateFlags,WIBase::StateFlags::FullyTransparent,fullyTransparent);
}
void WITexturedShape::UpdateShaderState()
{
	umath::set_flag(m_stateFlags,StateFlags::ExpensiveShaderRequired,m_alphaMode != AlphaMode::Blend);
}
void WITexturedShape::SizeToTexture()
{
	if(m_texture == nullptr && m_hMaterial == nullptr)
		return;
	uint32_t width,height;
	if(m_texture)
	{
		auto extents = m_texture->GetImage().GetExtents();
		width = extents.width;
		height = extents.height;
	}
	else
	{
		auto *diffuseMap = m_hMaterial.get()->GetDiffuseMap();
		if(diffuseMap == nullptr || diffuseMap->texture == nullptr)
			return;
		width = diffuseMap->width;
		height = diffuseMap->height;
	}
	SetSize(width,height);
}
void WITexturedShape::Render(const DrawInfo &drawInfo,wgui::DrawState &drawState,const Mat4 &matDraw,const Vector2 &scale,uint32_t testStencilLevel,wgui::StencilPipeline stencilPipeline)
{
	if(m_hMaterial == nullptr && m_texture == nullptr)
		return;
	auto col = drawInfo.GetColor(*this,drawState);
	if(col.a <= 0.f)
		return;
	col.a *= GetLocalAlpha();
	if(m_hMaterial)
	{
		auto *map = m_hMaterial->GetDiffuseMap();
		if(!map || !map->texture)
			return;
		if(m_hMaterial->GetUpdateIndex() != m_matUpdateCountRef)
			UpdateMaterialDescriptorSetTexture();
		else
		{
			auto *tex = static_cast<Texture*>(map->texture.get());
			if(tex->GetUpdateCount() != m_texUpdateCountRef)
				UpdateMaterialDescriptorSetTexture();
		}
	}

	// Try to use cheap shader if no custom vertex buffer was used
	if(umath::is_flag_set(m_stateFlags,StateFlags::ShaderOverride) == false && ((m_vertexBufferData == nullptr && m_uvBuffer == nullptr) || m_shader.expired()))
	{
		if(umath::is_flag_set(m_stateFlags,StateFlags::ExpensiveShaderRequired))
		{
			auto *pShaderExpensive = static_cast<wgui::ShaderTexturedRectExpensive*>(WGUI::GetInstance().GetTexturedRectExpensiveShader());
			if(pShaderExpensive == nullptr)
				return;
			auto &context = WGUI::GetInstance().GetContext();
			prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
			if(pShaderExpensive->RecordBeginDraw(bindState,drawState,drawInfo.size.x,drawInfo.size.y,stencilPipeline,drawInfo.msaa) == true)
			{
				pShaderExpensive->RecordDraw(bindState,{
					matDraw,col,wgui::ElementData::ToViewportSize(drawInfo.size),std::array<uint32_t,3>{},umath::is_flag_set(m_stateFlags,StateFlags::AlphaOnly) ? 1 : 0,m_lod,
					m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Red)),
					m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Green)),
					m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Blue)),
					m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Alpha)),
					GetAlphaMode(),GetAlphaCutoff()
				},*m_descSetTextureGroup->GetDescriptorSet(0u),testStencilLevel);
				pShaderExpensive->RecordEndDraw(bindState);
			}
			return;
		}
		auto *pShaderCheap = static_cast<wgui::ShaderTexturedRect*>(GetCheapShader());
		if(pShaderCheap == nullptr)
			return;
		auto &context = WGUI::GetInstance().GetContext();
		prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
		if(pShaderCheap->RecordBeginDraw(bindState,drawState,drawInfo.size.x,drawInfo.size.y,stencilPipeline,drawInfo.msaa) == true)
		{
			pShaderCheap->RecordDraw(bindState,{
				matDraw,col,wgui::ElementData::ToViewportSize(drawInfo.size),std::array<uint32_t,3>{},umath::is_flag_set(m_stateFlags,StateFlags::AlphaOnly) ? 1 : 0,m_lod,
				m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Red)),
				m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Green)),
				m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Blue)),
				m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Alpha))
			},*m_descSetTextureGroup->GetDescriptorSet(0u),testStencilLevel);
			pShaderCheap->RecordEndDraw(bindState);
		}
		return;
	}
	//

	if(m_shader.expired())
		return;
	auto &shader = static_cast<wgui::ShaderTextured&>(*m_shader.get());
	auto &context = WGUI::GetInstance().GetContext();
	auto vbuf = (m_vertexBufferData != nullptr) ? m_vertexBufferData->GetBuffer() : context.GetCommonBufferCache().GetSquareVertexBuffer();
	auto uvBuf = (m_uvBuffer != nullptr) ? m_uvBuffer : context.GetCommonBufferCache().GetSquareUvBuffer();
	if(vbuf == nullptr || uvBuf == nullptr)
		return;
	prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
	if(shader.RecordBeginDraw(bindState,drawState,drawInfo.size.x,drawInfo.size.y,stencilPipeline,drawInfo.msaa) == true)
	{
		wgui::ShaderTextured::PushConstants pushConstants {};
		pushConstants.elementData.modelMatrix = matDraw;
		pushConstants.elementData.color = col;
		pushConstants.elementData.viewportSize = wgui::ElementData::ToViewportSize(drawInfo.size);
		pushConstants.lod = m_lod;
		pushConstants.red = m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Red));
		pushConstants.green = m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Green));
		pushConstants.blue = m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Blue));
		pushConstants.alpha = m_channels.at(umath::to_integral(wgui::ShaderTextured::Channel::Alpha));
		shader.RecordDraw(
			bindState,
			vbuf,
			uvBuf,
			GetVertexCount(),*m_descSetTextureGroup->GetDescriptorSet(0u),pushConstants,testStencilLevel
		);
		shader.RecordEndDraw(bindState);
	}
}
