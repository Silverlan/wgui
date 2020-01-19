#include <iostream>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#undef MemoryBarrier
#endif
#include <wgui/wgui.h>
#include <wgui/types/wirect.h>
#include <wgui/types/witext.h>
#include <wgui/types/wibutton.h>
#include <wgui/types/witextentry.h>
#include <cmaterialmanager.h>
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <shader/prosper_shader.hpp>
#include <prosper_command_buffer.hpp>
#include <debug/prosper_debug.hpp>
#include <image/prosper_render_target.hpp>

class RenderContext
	: public prosper::Context
{
public:
	static std::shared_ptr<RenderContext> Create(const std::string &programName,const RenderContext::CreateInfo &createInfo,bool validationEnabled=false)
	{
		auto context = std::shared_ptr<RenderContext>{new RenderContext{"GUI Demo",validationEnabled}};
		context->Initialize(createInfo);
		if(context == nullptr)
			return nullptr;

		auto &matManager = context->m_materialManager = std::make_shared<CMaterialManager>(*context);
		auto &wgui = WGUI::Open(*context,matManager);
		auto r = wgui.Initialize();
		if(r != WGUI::ResultCode::Ok)
		{
			context->Close();
			return nullptr;
		}
		return context;
	}
	void Initialize(const RenderContext::CreateInfo &createInfo)
	{
		prosper::Context::Initialize(createInfo);
		InitializeStagingTarget();
	}
	void Draw()
	{
		m_materialManager->Update();
		GLFW::poll_joystick_events();
		DrawFrame();
		GLFW::poll_events();
	}
protected:
	RenderContext(const std::string &appName,bool bEnableValidation=false)
		: prosper::Context{appName,bEnableValidation}
	{
		GetWindowCreationInfo().resizable = false;
		prosper::Shader::SetLogCallback([](prosper::Shader &shader,Anvil::ShaderStage stage,const std::string &infoLog,const std::string &debugInfoLog) {
			std::cerr<<"Unable to load shader '"<<shader.GetIdentifier()<<"':"<<std::endl;
			std::cerr<<"Shader Stage: "<<prosper::util::to_string(stage)<<std::endl;
			std::cerr<<infoLog<<std::endl<<std::endl;
			std::cerr<<debugInfoLog<<std::endl;
		});
		prosper::debug::set_debug_validation_callback([](vk::DebugReportObjectTypeEXT objectType,const std::string &msg) {
			std::cerr<<"[VK] "<<msg<<std::endl;
		});
		GLFW::initialize();
	}
	void InitializeStagingTarget()
	{
		auto &dev = GetDevice();
		prosper::util::ImageCreateInfo imgCreateInfo {};
		imgCreateInfo.usage = Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT;
		imgCreateInfo.format = Anvil::Format::R8G8B8A8_UNORM;
		imgCreateInfo.width = GetWindowWidth();
		imgCreateInfo.height = GetWindowHeight();
		imgCreateInfo.postCreateLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
		auto stagingImg = prosper::util::create_image(dev,imgCreateInfo);
		prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
		auto stagingTex = prosper::util::create_texture(dev,{},stagingImg,&imgViewCreateInfo);

		auto rp = prosper::util::create_render_pass(dev,
			prosper::util::RenderPassCreateInfo{{{Anvil::Format::R8G8B8A8_UNORM,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,Anvil::AttachmentLoadOp::DONT_CARE,
				Anvil::AttachmentStoreOp::STORE,Anvil::SampleCountFlagBits::_1_BIT,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL
			}}}
		);
		m_stagingRenderTarget = prosper::util::create_render_target(dev,{stagingTex},rp);
	}
	virtual void OnClose() override
	{
		WaitIdle();
		m_materialManager = nullptr;
		m_stagingRenderTarget = nullptr;
		WGUI::Close();
		prosper::Context::OnClose();
	}
	virtual VkBool32 ValidationCallback(
		Anvil::DebugMessageSeverityFlags severityFlags,
		const char *message
	) override
	{
		if((severityFlags &Anvil::DebugMessageSeverityFlagBits::ERROR_BIT) != Anvil::DebugMessageSeverityFlagBits::NONE)
		{
			std::string strMsg = message;
			prosper::debug::add_debug_object_information(strMsg);
			std::cerr<<"[VK] "<<strMsg<<std::endl;
		}
		return prosper::Context::ValidationCallback(severityFlags,message);
	}
private:
	using prosper::Context::DrawFrame;
	std::shared_ptr<prosper::RenderTarget> m_stagingRenderTarget = nullptr;
	std::shared_ptr<CMaterialManager> m_materialManager = nullptr;
	virtual void DrawFrame(prosper::PrimaryCommandBuffer &drawCmd,uint32_t iCurrentSwapchainImage) override
	{
		auto &gui = WGUI::GetInstance();
		// Update GUI Logic
		gui.Think();

		prosper::util::record_image_barrier(
			*drawCmd,**m_stagingRenderTarget->GetTexture()->GetImage(),
			Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL
		);
		auto &outputImg = *m_stagingRenderTarget->GetTexture()->GetImage();
		prosper::util::record_begin_render_pass(*drawCmd,*m_stagingRenderTarget);
			// Draw GUI elements to staging image
			gui.Draw();
		prosper::util::record_end_render_pass(*drawCmd);

		prosper::util::record_image_barrier(
			*drawCmd,outputImg.GetAnvilImage(),
			Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL
		);

		auto &swapchainImg = *m_swapchainPtr->get_image(iCurrentSwapchainImage);

		prosper::util::BarrierImageLayout srcInfo {Anvil::PipelineStageFlagBits::TRANSFER_BIT,Anvil::ImageLayout::UNDEFINED,Anvil::AccessFlagBits{}};
		prosper::util::BarrierImageLayout dstInfo {Anvil::PipelineStageFlagBits::TRANSFER_BIT,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT};
		prosper::util::record_image_barrier(*drawCmd,swapchainImg,srcInfo,dstInfo);
			// Blit staging image to swapchain image
			prosper::util::record_blit_image(*drawCmd,{},outputImg.GetAnvilImage(),swapchainImg);
		srcInfo = {Anvil::PipelineStageFlagBits::TRANSFER_BIT,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::AccessFlagBits::MEMORY_READ_BIT};
		dstInfo = {Anvil::PipelineStageFlagBits::TRANSFER_BIT,Anvil::ImageLayout::PRESENT_SRC_KHR,Anvil::AccessFlagBits::TRANSFER_READ_BIT};
		prosper::util::record_image_barrier(*drawCmd,swapchainImg,srcInfo,dstInfo);
	}
};

int main()
{
	RenderContext::CreateInfo createInfo {};
	createInfo.width = 1'280;
	createInfo.height = 1'024;
	createInfo.presentMode = Anvil::PresentModeKHR::FIFO_KHR;
	auto renderContext = RenderContext::Create("GUI Demo",createInfo,true);
	if(renderContext == nullptr)
		return EXIT_FAILURE;

	auto &wgui = WGUI::GetInstance();

	// Populate window with some basic GUI elements
	auto *p = wgui.Create<WIRect>();
	p->SetAutoAlignToParent(true);
	p->SetColor(Color::AliceBlue);

	//WIText::set_link_handler([](const std::string &link) {
	//	std::cout<<"Clicked Link!"<<std::endl;
	//});
	auto *pText = wgui.Create<WIText>(p);
	// pText->SetTagsEnabled(true);
	pText->SetText("This is a demo showcasing the GUI and materialmanager libraries!");
	pText->SetColor(Color::DarkOrange);
	// pText->SetAutoBreakMode(WIText::AutoBreak::WHITESPACE);
	pText->SetWidth(128);
	pText->SetFont("default_large");
	pText->EnableShadow(true);
	pText->SetShadowOffset(Vector2i{2,2});
	//pText->InsertText("InsertedText",1);
	pText->SizeToContents();
	pText->SetHeight(512);
	
	auto *pLogo = wgui.Create<WITexturedRect>(p);
	pLogo->SetMaterial("vulkan_logo.wmi");
	pLogo->SetSize(512,136);

	/*auto *pTextShadow = wgui.Create<WIText>(p);
	pTextShadow->SetText("Text with shadow");
	pTextShadow->SetPos(512,64);
	pTextShadow->SetColor(Color::LimeGreen);
	pTextShadow->EnableShadow(true);
	pTextShadow->SetShadowOffset(1,1);
	pTextShadow->SetShadowColor(Color::Black);
	pTextShadow->SizeToContents();*/

	auto *pTextEntry = wgui.Create<WITextEntry>(p);
	pTextEntry->SetSize(64,24);
	pTextEntry->SetPos(512,128);
	pTextEntry->SetMouseInputEnabled(true);
	pTextEntry->SetKeyboardInputEnabled(true);
	pTextEntry->SetMultiLine(true);
	pTextEntry->RequestFocus();

	Vector2i logoMovementSpeed {2,2};
	auto windowBounds = renderContext->GetWindowSize();
	while(renderContext->GetWindow().ShouldClose() == false)
	{
		auto pos = pLogo->GetPos();
		if(
			(logoMovementSpeed.x > 0 && pLogo->GetRight() +logoMovementSpeed.x >= windowBounds.at(0)) ||
			(logoMovementSpeed.x < 0 && pLogo->GetLeft() +logoMovementSpeed.x <= 0u)
		)
			logoMovementSpeed.x = -logoMovementSpeed.x;

		if(
			(logoMovementSpeed.y > 0 && pLogo->GetBottom() +logoMovementSpeed.y >= windowBounds.at(1)) ||
			(logoMovementSpeed.y < 0 && pLogo->GetTop() +logoMovementSpeed.y <= 0u)
		)
			logoMovementSpeed.y = -logoMovementSpeed.y;

		pos.x += logoMovementSpeed.x;
		pos.y += logoMovementSpeed.y;
		pLogo->SetPos(pos);

		renderContext->Draw();
	}
	renderContext->Close();
	renderContext = nullptr;
	return EXIT_SUCCESS;
}
