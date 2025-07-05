// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "stdafx_wgui.h"
#include "wgui/wielementbufferdata.hpp"
#include <prosper_context.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_command_buffer.hpp>
#include <prosper_util.hpp>

WIElementVertexBufferData::~WIElementVertexBufferData()
{
	if(m_buffer != nullptr)
		WGUI::GetInstance().GetContext().KeepResourceAliveUntilPresentationComplete(m_buffer);
}
void WIElementVertexBufferData::SetBuffer(const std::shared_ptr<prosper::IBuffer> &buffer) { m_buffer = buffer; }
void WIElementVertexBufferData::ClearBuffer() { m_buffer = nullptr; }
std::shared_ptr<prosper::IBuffer> WIElementVertexBufferData::GetBuffer() const { return m_buffer; }
