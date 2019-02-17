/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
void WIElementVertexBufferData::SetBuffer(const std::shared_ptr<prosper::Buffer> &buffer) {m_buffer = buffer;}
void WIElementVertexBufferData::ClearBuffer() {m_buffer = nullptr;}
std::shared_ptr<prosper::Buffer> WIElementVertexBufferData::GetBuffer() const {return m_buffer;}
