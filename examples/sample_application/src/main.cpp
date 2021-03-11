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
#include <buffers/prosper_buffer.hpp>
#include <shader/prosper_shader.hpp>
#include <prosper_command_buffer.hpp>
#include <debug/prosper_debug.hpp>
#include <image/prosper_render_target.hpp>
#include <sharedutils/util_library.hpp>
#include <sharedutils/util.h>

#pragma optimize("",off)

#include <shader/prosper_pipeline_create_info.hpp>
class ShaderTest
	: public prosper::ShaderBaseImageProcessing
{
public:
	ShaderTest(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader);
	ShaderTest(prosper::IPrContext &context,const std::string &identifier);
	static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;
	static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE2;
	bool Draw(const Mat4 &modelMatrix);
protected:
	virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
};

decltype(ShaderTest::DESCRIPTOR_SET_TEXTURE) ShaderTest::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding {
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		},
		prosper::DescriptorSetInfo::Binding {
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		},
		prosper::DescriptorSetInfo::Binding {
			prosper::DescriptorType::UniformBuffer,
			prosper::ShaderStageFlags::FragmentBit
		}
	}
};

decltype(ShaderTest::DESCRIPTOR_SET_TEXTURE2) ShaderTest::DESCRIPTOR_SET_TEXTURE2 = {
	{
		prosper::DescriptorSetInfo::Binding {
	prosper::DescriptorType::CombinedImageSampler,
	prosper::ShaderStageFlags::FragmentBit
},
prosper::DescriptorSetInfo::Binding {
	prosper::DescriptorType::CombinedImageSampler,
	prosper::ShaderStageFlags::FragmentBit
}
	}
};
ShaderTest::ShaderTest(prosper::IPrContext &context,const std::string &identifier,const std::string &vsShader,const std::string &fsShader)
	: ShaderBaseImageProcessing(context,identifier,vsShader,fsShader)
{}
ShaderTest::ShaderTest(prosper::IPrContext &context,const std::string &identifier)
	: ShaderTest(context,identifier,"screen/vs_screen_uv","screen/fs_test")
{}
void ShaderTest::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);
	AddVertexAttribute(pipelineInfo,VERTEX_ATTRIBUTE_POSITION);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(Vector4),prosper::ShaderStageFlags::FragmentBit);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE2);
}
bool ShaderTest::Draw(const Mat4 &modelMatrix)
{
	return ShaderBaseImageProcessing::Draw();
}


#if 0
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
#endif

class GUIProgram
{
public:
	static std::shared_ptr<GUIProgram> Create()
	{
		auto p = std::shared_ptr<GUIProgram>{new GUIProgram{}};
		if(p->Initialize() == false)
			return nullptr;
		auto &context = p->GetContext();
		while(context.GetWindow().ShouldClose() == false)
			p->Draw();
		return p;
	}
	~GUIProgram()
	{
		m_stagingRenderTarget = nullptr;
		if(m_context)
		{
			m_context->Close();
			m_context = nullptr;
		}

		m_libRenderAPI = nullptr; // Needs to be destroyed last!
	}
	void Draw()
	{
		m_context->DrawFrame();
		GLFW::poll_events();
		m_context->EndFrame();
	}
	prosper::IPrContext &GetContext() const {return *m_context;}
private:
	GUIProgram()=default;
	bool Initialize()
	{
		auto lib = util::Library::Load(util::get_program_path() +"/pr_prosper_vulkan.dll");//pr_prosper_opengl.dll");
		if(lib == nullptr)
			return false;
		m_libRenderAPI = lib;
		auto *initRenderAPI = lib->FindSymbolAddress<bool(*)(const std::string&,bool,std::shared_ptr<prosper::IPrContext>&,std::string&)>("initialize_render_api");
		if(initRenderAPI == nullptr)
			return false;
		std::shared_ptr<prosper::IPrContext> context;
		std::string err;
		if(initRenderAPI("WGUI Demo",false /* validation */,context,err) == false)
		{
			std::cout<<err<<std::endl;
			return false;
		}
		prosper::Callbacks callbacks {};
		/*callbacks.validationCallback = [](prosper::DebugMessageSeverityFlags severityFlags,const std::string &message) {
			std::cout<<"Validation message: "<<message<<std::endl;
		};*/
		callbacks.onWindowInitialized = []() {};
		callbacks.onClose = []() {};
		callbacks.onResolutionChanged = [](uint32_t w,uint32_t h) {};
		callbacks.drawFrame = [this](prosper::IPrimaryCommandBuffer &drawCmd,uint32_t swapchainImageIdx) {
			DrawFrame(drawCmd,swapchainImageIdx);
		};
		context->SetCallbacks(callbacks);

		context->GetWindowCreationInfo().resizable = false;
		prosper::Shader::SetLogCallback([](prosper::Shader &shader,prosper::ShaderStage stage,const std::string &infoLog,const std::string &debugInfoLog) {
			std::cout<<"Unable to load shader '"<<shader.GetIdentifier()<<"':"<<std::endl;
			std::cout<<"Shader Stage: "<<prosper::util::to_string(stage)<<std::endl;
			std::cout<<infoLog<<std::endl<<std::endl;
			std::cout<<debugInfoLog<<std::endl;
			});
		/*prosper::debug::set_debug_validation_callback([](prosper::DebugReportObjectTypeEXT objectType,const std::string &msg) {
			std::cerr<<"[VK] "<<msg<<std::endl;
			});*/
		GLFW::initialize();

		prosper::IPrContext::CreateInfo contextCreateInfo {};
		contextCreateInfo.width = 1'280;
		contextCreateInfo.height = 1'024;
		contextCreateInfo.presentMode = prosper::PresentModeKHR::Mailbox;
		context->GetWindowCreationInfo().decorated = true;
		context->Initialize(contextCreateInfo);

		auto matManager = std::make_shared<CMaterialManager>(*context);
		m_matManager = matManager;

		TextureManager::LoadInfo loadInfo {};
		loadInfo.flags = TextureLoadFlags::LoadInstantly;
		matManager->GetTextureManager().Load(*context,"error",loadInfo);

		auto &gui = WGUI::Open(*context,matManager);
		gui.SetMaterialLoadHandler([this](const std::string &path) -> Material* {
			return m_matManager->Load(path,nullptr,nullptr,false,nullptr,true);
		});
		auto r = gui.Initialize(Vector2i{contextCreateInfo.width,contextCreateInfo.height});
		if(r != WGUI::ResultCode::Ok)
		{
			std::cerr<<"ERROR: Unable to initialize GUI library: ";
			switch(r)
			{
			case WGUI::ResultCode::UnableToInitializeFontManager:
				std::cerr<<"Error initializing font manager!";
				break;
			case WGUI::ResultCode::ErrorInitializingShaders:
				std::cerr<<"Error initializing shaders!";
				break;
			case WGUI::ResultCode::FontNotFound:
				std::cerr<<"Font not found!";
				break;
			default:
				std::cout<<"Unknown error!";
				break;
			}
			matManager = nullptr;
			context->Close();
			std::this_thread::sleep_for(std::chrono::seconds(5));
			return false;
		}
		m_context = context;
		m_context->CalcBufferAlignment(prosper::BufferUsageFlags::IndexBufferBit);
		InitializeStagingTarget();

		auto el = gui.Create<WIRect>();
		el->SetSize(512,512);
		el->SetColor(Color::Lime);

		auto elTex = gui.Create<WITexturedRect>();
		elTex->SetMaterial("vulkan_logo");
		elTex->SetSize(256,256);
		m_elTest = elTex->GetHandle();

		auto el2 = gui.Create<WIRect>();
		el2->SetColor(Color::Aqua);

		auto elText = gui.Create<WIText>();
		elText->SetText("HellooOOoo");
		elText->SetPos(200,200);
		elText->SetColor(Color::White);
		elText->SizeToContents();

		el2->SetSize(elText->GetSize());
		el2->SetPos(elText->GetPos());

		m_context->GetShaderManager().RegisterShader("test",[](prosper::IPrContext &context,const std::string &identifier) {return new ShaderTest(context,identifier);});
		return true;
	}
	void DrawFrame(prosper::IPrimaryCommandBuffer &drawCmd,uint32_t swapchainImageIdx)
	{
#if 0
		auto &gui = WGUI::GetInstance();
		// Update GUI Logic
		gui.Think();

		drawCmd.RecordImageBarrier(
			m_stagingRenderTarget->GetTexture().GetImage(),
			prosper::ImageLayout::TransferSrcOptimal,prosper::ImageLayout::ColorAttachmentOptimal
		);
		auto &outputImg = m_stagingRenderTarget->GetTexture().GetImage();


		//uimg::ImageBuffer::
		static std::shared_ptr<prosper::Texture> texTest = nullptr;
		static std::shared_ptr<prosper::IFramebuffer> framebuffer = nullptr;
		static std::shared_ptr<prosper::IBuffer> buffer = nullptr;
		if(texTest == nullptr)
		{
			prosper::util::ImageCreateInfo imgCreateInfo {};
			imgCreateInfo.width = 128;
			imgCreateInfo.height = 128;
			imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
			imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
			imgCreateInfo.usage = prosper::ImageUsageFlags::SampledBit;
			auto img = m_context->CreateImage(imgCreateInfo);
			texTest = m_context->CreateTexture({},*img,prosper::util::ImageViewCreateInfo{},prosper::util::SamplerCreateInfo{});
			framebuffer = m_context->CreateFramebuffer(imgCreateInfo.width,imgCreateInfo.height,1,{texTest->GetImageView()});

			prosper::util::BufferCreateInfo bufCreateInfo {};
			bufCreateInfo.size = sizeof(Vector4);
			bufCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::CPUToGPU;//prosper::MemoryFeatureFlags::GPUBulk;
			bufCreateInfo.usageFlags = prosper::BufferUsageFlags::UniformBufferBit | prosper::BufferUsageFlags::TransferSrcBit;
			Vector4 color = Color::Red.ToVector4();
			buffer = m_context->CreateBuffer(bufCreateInfo,&color);
			color = {5,6,7,8};
			buffer->Read(0,sizeof(color),&color);
			std::cout<<"Color: "<<color<<std::endl;

			//static_cast<WITexturedRect*>(m_elTest.get())->SetTexture(*texTest);
		}

		static std::shared_ptr<prosper::Texture> texTest2 = nullptr;
		static std::shared_ptr<prosper::IFramebuffer> framebuffer2 = nullptr;
		if(texTest2 == nullptr)
		{
			prosper::util::ImageCreateInfo imgCreateInfo {};
			imgCreateInfo.width = 128;
			imgCreateInfo.height = 128;
			imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
			imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
			imgCreateInfo.usage = prosper::ImageUsageFlags::SampledBit;
			auto img = m_context->CreateImage(imgCreateInfo);
			texTest2 = m_context->CreateTexture({},*img,prosper::util::ImageViewCreateInfo{},prosper::util::SamplerCreateInfo{});
			framebuffer2 = m_context->CreateFramebuffer(imgCreateInfo.width,imgCreateInfo.height,1,{texTest2->GetImageView()});

			auto glyphMap = FontManager::GetFont("default")->GetGlyphMap();
			texTest2 = glyphMap;
			drawCmd.RecordClearImage(glyphMap->GetImage(),prosper::ImageLayout::TransferDstOptimal,std::array<float,4>{1.f,1.f,1.f,1.f});
			static_cast<WITexturedRect*>(m_elTest.get())->SetTexture(*glyphMap);
		}

		auto *mat = m_matManager->Load("error");
		if(mat)
		{
			static std::shared_ptr<prosper::IImage> imgTest3 = nullptr;
			if(imgTest3 == nullptr)
			{
				prosper::util::ImageCreateInfo imgCreateInfo {};
				imgCreateInfo.width = 128;
				imgCreateInfo.height = 128;
				imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
				imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
				imgCreateInfo.usage = prosper::ImageUsageFlags::TransferDstBit | prosper::ImageUsageFlags::TransferSrcBit | prosper::ImageUsageFlags::SampledBit;
				imgCreateInfo.layers = 1;
				imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
				imgCreateInfo.format = prosper::Format::BC1_RGBA_UNorm_Block;
				imgTest3 = m_context->CreateImage(imgCreateInfo);
			}

			auto *albedoMap = mat->GetAlbedoMap();
			auto &tex = std::static_pointer_cast<Texture>(albedoMap->texture)->GetVkTexture();
			//drawCmd.RecordClearImage(*imgTest3,prosper::ImageLayout::TransferDstOptimal,std::array<float,4>{1.f,1.f,1.f,1.f});
			
			static auto test = false;
			if(test == false)
			{
				test = true;
				//static_cast<WITexturedRect*>(m_elTest.get())->SetTexture(*tex);
			}
			// Is Black??
		}
		//drawCmd.RecordClearImage(texTest->GetImage(),prosper::ImageLayout::TransferDstOptimal,std::array<float,4>{1.f,0.f,0.f,1.f});
		/*glBindFramebuffer(GL_FRAMEBUFFER,static_cast<prosper::GLFramebuffer&>(*framebuffer).GetGLFramebuffer());
		glClearColor(1.0f, 1.0f, 0.0f, 1.0f );
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER,0);*/



#if 0
		glBindFramebuffer(GL_FRAMEBUFFER,static_cast<prosper::GLFramebuffer&>(*framebuffer2).GetGLFramebuffer());
		glClearColor(1.0f, 0.0f, 1.0f, 1.0f );
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER,0);
#endif

		glBindFramebuffer(GL_FRAMEBUFFER,0);
		glClearColor(0.0f, 0.0f, 1.0f, 1.0f );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);  

		drawCmd.RecordBeginRenderPass(*m_stagingRenderTarget);

		{
			auto *shader = static_cast<ShaderTest*>(m_context->GetShaderManager().FindShader<ShaderTest>());

			static std::shared_ptr<prosper::IDescriptorSetGroup> dsg = nullptr;
			static std::shared_ptr<prosper::IDescriptorSetGroup> dsg2 = nullptr;
			if(dsg == nullptr)
			{
				dsg = shader->CreateDescriptorSetGroup(0);
				dsg->GetDescriptorSet(0)->SetBindingTexture(*texTest,0);
				dsg->GetDescriptorSet(0)->SetBindingTexture(*texTest2,1);
				dsg->GetDescriptorSet(0)->SetBindingUniformBuffer(*buffer,2);

				dsg2 = shader->CreateDescriptorSetGroup(1);
				dsg2->GetDescriptorSet(0)->SetBindingTexture(*texTest2,0);
				dsg2->GetDescriptorSet(0)->SetBindingTexture(*texTest,1);
			}

			if(shader->BeginDraw(std::dynamic_pointer_cast<prosper::IPrimaryCommandBuffer>(drawCmd.shared_from_this())))
			{
				Vector4 color2 = Vector4{0.f,1.f,0.f,1.f};
				shader->RecordPushConstants(color2);
				auto &pcBuffer = static_cast<prosper::GLContext&>(GetContext()).GetPushConstantBuffer();

				shader->RecordBindDescriptorSet(*dsg->GetDescriptorSet());
				shader->RecordBindDescriptorSet(*dsg2->GetDescriptorSet(),1);
				auto shaderPipelineId = dynamic_cast<prosper::GLCommandBuffer&>(drawCmd).GetBoundPipelineId();

				auto &vertBuffer = dynamic_cast<prosper::GLBuffer&>(*prosper::util::get_square_vertex_buffer(GetContext()));
				auto vertexbuffer = vertBuffer.GetGLBuffer();
				//drawCmd.RecordBindVertexBuffers(*shader,{&vertBuffer});

				shader->Draw(umat::identity());
				glDisableVertexAttribArray(0); // TODO: Disable other vertex attrib arrays
				shader->EndDraw();
			}
		}

		// Draw GUI elements to staging image
		gui.Draw();

		drawCmd.RecordEndRenderPass();

		drawCmd.RecordImageBarrier(
			outputImg,
			prosper::ImageLayout::ColorAttachmentOptimal,prosper::ImageLayout::TransferSrcOptimal
		);

		//glBindFramebuffer(GL_FRAMEBUFFER,static_cast<prosper::GLFramebuffer&>(m_stagingRenderTarget->GetFramebuffer()).GetGLFramebuffer());

		auto &tex = m_stagingRenderTarget->GetTexture();
		auto &img = tex.GetImage();

		glBindFramebuffer(GL_FRAMEBUFFER,static_cast<prosper::GLFramebuffer&>(m_stagingRenderTarget->GetFramebuffer()).GetGLFramebuffer());
		//drawCmd.RecordClearImage(m_stagingRenderTarget->GetTexture().GetImage(),prosper::ImageLayout::TransferDstOptimal,std::array<float,4>{1.f,0.f,0.f,1.f});
		//drawCmd.RecordClearImage(m_stagingRenderTarget->GetTexture().GetImage(),prosper::ImageLayout::TransferDstOptimal,std::array<float,4>{0.f,1.f,0.f,1.f});
		//glClearColor(1.f,0.f,0.f,1.f);
		//glClear(GL_COLOR_BUFFER_BIT);
		auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBlitNamedFramebuffer(
			static_cast<prosper::GLFramebuffer&>(m_stagingRenderTarget->GetFramebuffer()).GetGLFramebuffer(),0,
			0,0,img.GetWidth(),img.GetHeight(),
			0,0,img.GetWidth(),img.GetHeight(),
			GL_COLOR_BUFFER_BIT,GL_LINEAR
		);
		// TODO?

#if 0
		auto &swapchainImg = *m_swapchainPtr->get_image(swapchainImageIdx);

		prosper::util::BarrierImageLayout srcInfo {prosper::PipelineStageFlags::TransferBit,prosper::ImageLayout::Undefined,prosper::AccessFlags{}};
		prosper::util::BarrierImageLayout dstInfo {prosper::PipelineStageFlags::TransferBit,prosper::ImageLayout::TransferDstOptimal,prosper::AccessFlags::TransferWriteBit};
		drawCmd.RecordImageBarrier(swapchainImg,srcInfo,dstInfo);
		// Blit staging image to swapchain image
		drawCmd.RecordBlitImage({},outputImg,swapchainImg);
		srcInfo = {prosper::PipelineStageFlags::TransferBit,prosper::ImageLayout::TransferDstOptimal,prosper::AccessFlags::MemoryReadBit};
		dstInfo = {prosper::PipelineStageFlags::TransferBit,prosper::ImageLayout::PresentSrcKHR,prosper::AccessFlags::TransferReadBit};
		drawCmd.RecordImageBarrier(swapchainImg,srcInfo,dstInfo);
#endif
#endif
	}
	void InitializeStagingTarget()
	{
		prosper::util::ImageCreateInfo imgCreateInfo {};
		imgCreateInfo.usage = prosper::ImageUsageFlags::TransferDstBit | prosper::ImageUsageFlags::TransferSrcBit | prosper::ImageUsageFlags::ColorAttachmentBit;
		imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
		imgCreateInfo.width = m_context->GetWindowWidth();
		imgCreateInfo.height = m_context->GetWindowHeight();
		imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
		auto stagingImg = m_context->CreateImage(imgCreateInfo);
		prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
		auto stagingTex = m_context->CreateTexture({},*stagingImg,imgViewCreateInfo);

		auto rp = m_context->CreateRenderPass(
			prosper::util::RenderPassCreateInfo{{{prosper::Format::R8G8B8A8_UNorm,prosper::ImageLayout::ColorAttachmentOptimal,prosper::AttachmentLoadOp::DontCare,
			prosper::AttachmentStoreOp::Store,prosper::SampleCountFlags::e1Bit,prosper::ImageLayout::ColorAttachmentOptimal
		}}}
		);
		m_stagingRenderTarget = m_context->CreateRenderTarget({stagingTex},rp);
	}
	std::shared_ptr<prosper::RenderTarget> m_stagingRenderTarget = nullptr;
	std::shared_ptr<prosper::IPrContext> m_context = nullptr;
	std::shared_ptr<CMaterialManager> m_matManager = nullptr;
	std::shared_ptr<util::Library> m_libRenderAPI = nullptr;
	WIHandle m_elTest {};
};

int main()
{
	auto program = GUIProgram::Create();
	program = nullptr;
	return EXIT_SUCCESS;
}
#pragma optimize("",on)
