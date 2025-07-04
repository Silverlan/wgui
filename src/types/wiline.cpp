// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "stdafx_wgui.h"
#include "wgui/types/wiline.h"
#include "wgui/shaders/wishader_coloredline.hpp"
#include "wgui/wielementdata.hpp"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <buffers/prosper_buffer.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>

LINK_WGUI_TO_CLASS(WILine, WILine);

unsigned int WILineBase::GetLineWidth() { return m_lineWidth; }
void WILineBase::SetLineWidth(unsigned int width) { m_lineWidth = width; }
WILineBase::WILineBase() : m_lineWidth(1) {}

////////////////////

static std::shared_ptr<prosper::IBuffer> s_lineBuffer = nullptr;
static uint32_t s_lineCount = 0u;

WILine::WILine() : WIBufferBase(), WILineBase(), m_posStart {util::Vector2iProperty::Create({})}, m_posEnd {util::Vector2iProperty::Create({})}, m_dot(0.f), m_bufColor(nullptr)
{
	++s_lineCount;
	SetShouldScissor(false);
	const std::array<Vector2, 2> verts = {Vector2(0.f, 0.f), Vector2(1.f, 1.f)};
	auto &instance = WGUI::GetInstance();
	auto &context = instance.GetContext();
	auto *pShader = instance.GetColoredLineShader();
	if(pShader != nullptr)
		SetShader(*pShader);

	if(s_lineBuffer == nullptr) {
		prosper::util::BufferCreateInfo bufCreateInfo {};
		bufCreateInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit;
		bufCreateInfo.size = util::size_of_container(verts);
		bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
		s_lineBuffer = context.CreateBuffer(bufCreateInfo, verts.data());
		s_lineBuffer->SetDebugName("gui_line_vertex_buf");
	}
	InitializeBufferData(*s_lineBuffer);

	const std::array<Vector4, 2> colors = {Vector4 {1.f, 1.f, 1.f, 1.f}, Vector4 {1.f, 1.f, 1.f, 1.f}};
	prosper::util::BufferCreateInfo bufCreateInfo {};
	bufCreateInfo.usageFlags = prosper::BufferUsageFlags::VertexBufferBit | prosper::BufferUsageFlags::TransferDstBit;
	bufCreateInfo.size = util::size_of_container(colors);
	bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	m_bufColor = context.CreateBuffer(bufCreateInfo, colors.data());
	m_bufColor->SetDebugName("gui_line_color_buf");

	auto col = Color::White.ToVector4();
	SetColor(col.x, col.y, col.z, col.w);
}

WILine::~WILine()
{
	if(m_bufColor != nullptr)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_bufColor);
	if(--s_lineCount == 0u)
		s_lineBuffer = nullptr;
}

void WILine::UpdateColorBuffer()
{
	auto &colStart = GetStartColor();
	auto &colEnd = GetEndColor();
	std::array<Vector4, 2> colors = {colStart.ToVector4(), colEnd.ToVector4()};
	/*prosper::util::record_update_buffer(
		*WGUI::GetInstance().GetContext().GetDrawCommandBuffer(),*m_bufColor,0ull,colors,
		vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eGeometryShader,
		vk::AccessFlagBits::eShaderRead
	);*/
	m_bufColor->Write(0u, colors.size() * sizeof(colors.front()), colors.data());
}

void WILine::SetColor(float r, float g, float b, float a)
{
	WIBase::SetColor(r, g, b, a);
	Color col {static_cast<int16_t>(umath::round(r * 255.f)), static_cast<int16_t>(umath::round(g * 255.f)), static_cast<int16_t>(umath::round(b * 255.f)), static_cast<int16_t>(umath::round(a * 255.f))};
	m_colStart = col;
	m_colEnd = col;
	UpdateColorBuffer();
}

void WILine::SetLineWidth(unsigned int width) { WILineBase::SetLineWidth(width); }
unsigned int WILine::GetLineWidth() { return WILineBase::GetLineWidth(); }

void WILine::SetStartColor(const Color &col)
{
	m_colStart = col;
	UpdateColorBuffer();
}
void WILine::SetEndColor(const Color &col)
{
	m_colEnd = col;
	UpdateColorBuffer();
}
const Color &WILine::GetStartColor() const { return m_colStart; }
const Color &WILine::GetEndColor() const { return m_colEnd; }

unsigned int WILine::GetVertexCount() { return 2; }
Mat4 WILine::GetTransformPose(const Vector2i &origin, int w, int h, const Mat4 &poseParent, const Vector2 &scale) const
{
	auto bounds = GetNormalizedLineBounds();
	const auto &posStart = bounds.first;
	const auto &posEnd = bounds.second;
	Vector3 size {(posEnd.x - posStart.x), (posEnd.y - posStart.y), 0};
	Vector2 offset {origin.x, origin.y};
	if(posStart.x > posEnd.x)
		offset.x += posStart.x - posEnd.x;
	if(posStart.y > posEnd.y)
		offset.y += posStart.y - posEnd.y;
	Vector3 normOrigin {offset * 2.f, 0.f};
	Vector3 normScale {size.x * scale.x, size.y * scale.y, 0};

	if(!m_rotationMatrix)
		return poseParent * glm::scale(glm::translate(normOrigin), normScale);

	auto t = glm::translate(normOrigin);
	auto s = glm::scale(normScale);
	auto &r = *m_rotationMatrix;
	auto m = t * r * s;
	return poseParent * m;
}

void WILine::Render(const DrawInfo &drawInfo, wgui::DrawState &drawState, const Mat4 &matDraw, const Vector2 &scale, uint32_t testStencilLevel, wgui::StencilPipeline stencilPipeline)
{
	auto *pShader = GetShader();
	auto col = drawInfo.GetColor(*this, drawState);
	if(col.a <= 0.f)
		return;

	if(s_lineBuffer == nullptr || m_bufColor == nullptr)
		return;
	auto &shader = static_cast<wgui::ShaderColoredLine &>(*pShader);
	auto &context = WGUI::GetInstance().GetContext();
	prosper::ShaderBindState bindState {*drawInfo.commandBuffer};
	if(shader.RecordBeginDraw(bindState, drawState, drawInfo.size.x, drawInfo.size.y, stencilPipeline, umath::is_flag_set(drawInfo.flags, DrawInfo::Flags::Msaa)) == true) {
		wgui::ElementData pushConstants {matDraw, col, wgui::ElementData::ToViewportSize(drawInfo.size)};
		shader.RecordDraw(bindState, s_lineBuffer, m_bufColor, GetVertexCount(), GetLineWidth(), pushConstants, testStencilLevel);
		shader.RecordEndDraw(bindState);
	}
}

void WILine::SizeToContents(bool x, bool y)
{
	auto bounds = GetNormalizedLineBounds();
	const Vector2i &pos = bounds.first;
	const Vector2i &posEnd = bounds.second;
	int xL, xR, yU, yB;
	if(pos.x < posEnd.x) {
		xL = pos.x;
		xR = posEnd.x;
	}
	else {
		xL = posEnd.x;
		xR = pos.x;
	}
	if(pos.y < posEnd.y) {
		yU = pos.y;
		yB = posEnd.y;
	}
	else {
		yU = posEnd.y;
		yB = pos.y;
	}
	SetPos(xL, yU);
	auto w = xR - xL;
	auto h = yB - yU + 1;
	if(x && y)
		SetSize(w, h);
	else if(x)
		SetWidth(w);
	else if(y)
		SetHeight(h);
	Vector2 dir = Vector2(posEnd.x, posEnd.y) - Vector2(posEnd.x, posEnd.y);
	dir = glm::normalize(dir);
	m_dot = glm::dot(Vector2(1.f, 0.f), dir);
}

std::pair<Vector2i, Vector2i> WILine::GetNormalizedLineBounds() const { return {*m_posStart, *m_posEnd}; }

const util::PVector2iProperty &WILine::GetStartPosProperty() const { return m_posStart; }
const util::PVector2iProperty &WILine::GetEndPosProperty() const { return m_posEnd; }
Vector2i &WILine::GetStartPos() const { return *m_posStart; }
void WILine::SetStartPos(Vector2i pos) { SetStartPos(pos.x, pos.y); }
void WILine::SetStartPos(int x, int y) { *m_posStart = Vector2i {x, y}; }

Vector2i &WILine::GetEndPos() const { return *m_posEnd; }
void WILine::SetEndPos(Vector2i pos) { SetEndPos(pos.x, pos.y); }
void WILine::SetEndPos(int x, int y) { *m_posEnd = Vector2i {x, y}; }
