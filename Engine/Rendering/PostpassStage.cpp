#include "PostpassStage.h"
#include "Core\StringTable.h"



PostpassStage::PostpassStage(RenderContext * Context): RenderStage(Context), ssaoMaterial(0), HDRShader(0), Frames(0) {
	Initiallize();
}


PostpassStage::~PostpassStage() {
}

void PostpassStage::InitPostSchema() {
	R_DEPTH_STENCIL_DESC depth = {};
	depth.ZTestEnable = 0;
	depth.ZWriteEnable = 0;
	depth.DepthFunc = R_CMP_FUNC::LESS;
	depth.StencilEnable = 0;
	depth.StencilFailFront = R_STENCIL_OP::KEEP;
	depth.DepthFailFront = R_STENCIL_OP::KEEP;
	depth.StencilPassFront = R_STENCIL_OP::KEEP;
	depth.StencilFuncFront = R_CMP_FUNC::NOT_EQUAL;
	depth.StencilFailBack = R_STENCIL_OP::KEEP;
	depth.DepthFailBack = R_STENCIL_OP::KEEP;
	depth.StencilPassBack = R_STENCIL_OP::KEEP;
	depth.StencilFuncBack = R_CMP_FUNC::NOT_EQUAL;
	depth.StencilRef = 0;
	DepthStat[0] = Interface->CreateDepthStencilStatus(&depth);
	Context->RegisterRenderState(String("NoZTest"), DepthStat[0]);
}

void PostpassStage::CreatePingPongBuffer() {
	R_TEXTURE2D_DESC desc = {};
	desc.Width = 1920;
	desc.Height = 1080;
	desc.ArraySize = 1;
	desc.CPUAccess = (R_CPU_ACCESS)0;
	desc.BindFlag = (R_BIND_FLAG)(BIND_RENDER_TARGET | BIND_SHADER_RESOURCE);
	desc.MipLevels = 1;
	desc.Usage = DEFAULT;
	desc.Format = FORMAT_R16G16B16A16_UNORM;
	desc.SampleDesc.Count = 1;
	PingPong[0] = Interface->CreateTexture2D(&desc, 0, 0, 0);
	desc.Format = FORMAT_R16G16B16A16_UNORM;
	PingPong[1] = Interface->CreateTexture2D(&desc, 0, 0, 0);

	//// resgister targets
	Context->RegisterRenderTarget(String("gPostBuffer"), PingPong[0]);
	Context->RegisterRenderTarget(String("gFinalBuffer"), PingPong[1]);

	Variant ScreenSize;
	ScreenSize.as<Vector2>() = Vector2(1920, 1080);
	Context->SetResource(String("gScreenSize"), ScreenSize);
}

void PostpassStage::InitSampleOffset() {
	// SampleOffset for downsample image by 1/4. with bilinear sampler
	int Width = 1920;
	int Height = 1080;
	for (int i = 0; i < MAX_HDR_LUM; i++) {
		int Index = 0;
		float tux = 1.0f / Width;
		float tuy = 1.0f / Height;
		for (int x = 0; x < 4; x += 2) {
			for (int y = 0; y < 4; y += 2) {
				ScaleOffset[i][Index] = (x - 1) * tux;
				ScaleOffset[i][Index+1] = (y - 1) * tuy;
				Index += 4;
			}
		}
		if (Width == 1 && Height == 1) {
			break;
		}
		Width /= 4;
		Height /= 4;
	}
	// bright pass offset
	float tux = 2.0f / 1920;
	float tuy = 2.0f / 1080;
	int Index = 0;
	for (int x = 0; x < 4; x += 2) {
		for (int y = 0; y < 4; y += 2) {
			BrightOffset[Index] = (x - 1) * tux;
			BrightOffset[Index + 1] = (y - 1) * tuy;
			Index += 4;
		}
	}
	// bloom gauss bloom offset
	tux = 8.0f / 1920;
	tuy = 8.0f / 1080;
	memset(BloomOffset, 0, sizeof(float) * 32);
	memset(BloomWeight, 0, sizeof(float) * 16);
	// horizion
	BloomOffset[0][4] = (1.0f + 0.5 / 1.3f) * tux;
	BloomOffset[0][8] = -BloomOffset[0][4];
	// vertical
	BloomOffset[1][5] = (1.0f + 0.5 / 1.3f) * tuy;
	BloomOffset[1][9] = -BloomOffset[1][5];
	// bloom weight
	BloomWeight[0] = 1.0f;
	BloomWeight[4] = BloomWeight[8] = 1.3f;
}

void PostpassStage::CreateHDRBuffer() {
	int Width = 1920;
	int Height = 1080;
	R_TEXTURE2D_DESC desc = {};
	desc.Width = Width/4;
	desc.Height = Height/4;
	desc.ArraySize = 1;
	desc.CPUAccess = (R_CPU_ACCESS)0;
	desc.BindFlag = (R_BIND_FLAG)(BIND_RENDER_TARGET | BIND_SHADER_RESOURCE);
	desc.MipLevels = 1;
	desc.Usage = DEFAULT;
	desc.Format = FORMAT_R16G16B16A16_FLOAT;
	desc.SampleDesc.Count = 1;
	LumScaleBy4 = Interface->CreateTexture2D(&desc, 0, 0, 0);
	// create scale array
	memset(LumScaleArray, -1, sizeof(int) * MAX_HDR_LUM);
	int  i = 0;
	int width = Width;
	int height = Height;
	for (int i = 0; i < MAX_HDR_LUM; i++) {
		width /= 4;
		height /= 4;
		desc.Width = width;
		desc.Height = height;
		desc.Format = FORMAT_R16_FLOAT;
		if (width==1 && height==1) {
			LumScaleArray[i] = Interface->CreateTexture2D(&desc, 0, 0, 0);
			AdaptLum[0] = LumScaleArray[i];
			LumScaleArray[i+1] = Interface->CreateTexture2D(&desc, 0, 0, 0);
			AdaptLum[1] = LumScaleArray[i + 1];
			AvgIter = i;
			break;
		} else {
			LumScaleArray[i] = Interface->CreateTexture2D(&desc, 0, 0, 0);
		}
	}
	// create Bright
	desc.Format = FORMAT_R16G16B16A16_FLOAT;
	desc.Width = Width/2;
	desc.Height = Height/2;
	Bright = Interface->CreateTexture2D(&desc, 0, 0, 0);
	// bloom 
	desc.Width = Width/8;
	desc.Height = Height/8;
	Bloom[0] = Interface->CreateTexture2D(&desc, 0, 0, 0);
	Bloom[1] = Interface->CreateTexture2D(&desc, 0, 0, 0);
	Bloom[2] = Interface->CreateTexture2D(&desc, 0, 0, 0);
	// Star
	Star = Interface->CreateTexture2D(&desc, 0, 0, 0);
}

void PostpassStage::Initiallize() {
	CreatePingPongBuffer();
	CreateHDRBuffer();
	InitSampleOffset();
	InitPostSchema();
	Time = GetCurrentTime();
}

unsigned int PostpassStage::GetFrameDelta() {
	// set time
	return GetCurrentTime() - Time;
}

void PostpassStage::SwapPingPong() {
	// nothing 
}

int PostpassStage::SSAO(BatchCompiler * Compiler) {
	int Compiled = 0;
	if (ssaoMaterial) {
		Shader * shader = ssaoMaterial->GetShader();
		Compiled += shader->Compile(Compiler, 0, 0, ssaoMaterial->GetParameter(), ssaoMaterial->GetParameter(), Context);
		// draw full screen quad
		Compiled += Compiler->Quad();
	}
	else {
		Variant * Value = Context->GetResource(String("Material\\Materials\\ssao.xml\\0"));
		if (Value) {
			ssaoMaterial = Value->as<Material*>();
		}
	}

	return Compiled;
}

int PostpassStage::ScaleBy4(BatchCompiler * Compiler) {
	// set offset
	float * Offset = Parameter[hash_string::gSampleOffsets].as<float[16]>();
	Parameter[hash_string::gPostBuffer].as<int>() = PingPong[1];
	memcpy_s(Offset, sizeof(Variant), ScaleOffset[0], sizeof(float) * 16);
	Compiler->SetRenderTargets(1, LumScaleArray);
	HDRShader->Compile(Compiler, 0, 0, Parameter, Parameter, Context);
	// Quad
	Compiler->SetViewport(0, 0, 1920 / 4.0f, 1080 / 4.0f, 0, 1);
	Compiler->Quad();
	return 0;
}

int PostpassStage::CalcAvgLum(BatchCompiler * Compiler) {
	// set offset
	float * Offset = Parameter[hash_string::gSampleOffsets].as<float[16]>();
	int Width = 1920 / 4.0f;
	int Height = 1080 / 4.0f;
	for (int i = 1; i < AvgIter; i++) {
		Width /= 4;
		Height /= 4;
		// the previous lumbuffer as gPostBuffer
		Compiler->SetRenderTargets(1, &LumScaleArray[i]);
		Parameter[hash_string::gPostBuffer].as<int>() = LumScaleArray[i-1];
		memcpy_s(Offset, sizeof(Variant), &ScaleOffset[i], sizeof(float) * 16);
		HDRShader->Compile(Compiler, 1, 0, Parameter, Parameter, Context);
		// Quad
		Compiler->SetViewport(0, 0, Width, Height, 0, 1);
		Compiler->Quad();
	}
	return 0;
}

int PostpassStage::CalcAdaptLum(BatchCompiler * Compiler) {
	// swap adaptlum
//	if (Frames % 2) {
	int t = AdaptLum[0];
	AdaptLum[0] = AdaptLum[1];
	AdaptLum[1] = t;
//	}
	float * Offset = Parameter[hash_string::gSampleOffsets].as<float[16]>();
	Parameter[hash_string::gPostBuffer].as<int>() = LumScaleArray[AvgIter-1];
	Parameter[hash_string::gDiffuseMap0].as<int>() = AdaptLum[1];
	Parameter[hash_string::gTimeElapse].as<int>() = GetFrameDelta();
	Compiler->SetRenderTargets(1, &AdaptLum[0]);
	memcpy_s(Offset, sizeof(Variant), &ScaleOffset[AvgIter], sizeof(float) * 16);
	HDRShader->Compile(Compiler, 2, 0, Parameter, Parameter, Context);
	Compiler->SetViewport(0, 0, 1, 1, 0, 1);
	Compiler->Quad();
	Frames++;
	return 0;
}

int PostpassStage::BrightPass(BatchCompiler * Compiler){
	// bright pass to bloom 0
	int Width = 1920 / 2.0f;
	int Height = 1080 / 2.0f;
	Parameter[hash_string::gPostBuffer].as<int>() = PingPong[1];
	Parameter[hash_string::gDiffuseMap0].as<int>() = AdaptLum[0];
	Compiler->SetRenderTargets(1, &Bright);
	HDRShader->Compile(Compiler, 3, 0, Parameter, Parameter, Context);
	Compiler->SetViewport(0, 0, Width, Height, 0, 1);
	Compiler->Quad();
	// downsample to bloom0
	float * Offset = Parameter[hash_string::gSampleOffsets].as<float[16]>();
	Parameter[hash_string::gPostBuffer].as<int>() = Bright;
	memcpy_s(Offset, sizeof(Variant), BrightOffset, sizeof(float) * 16);
	Compiler->SetRenderTargets(1, &Bloom[0]);
	HDRShader->Compile(Compiler, 1, 0, Parameter, Parameter, Context);
	Compiler->SetViewport(0, 0, Width/4, Height/4, 0, 1);
	Compiler->Quad();
	return 0;
}

int PostpassStage::BloomPass(BatchCompiler * Compiler) {
	// bright pass
	int Width = 1920 / 8.0f;
	int Height = 1080 / 8.0f;
	float * Weight = Parameter[hash_string::gSampleWeights].as<float[16]>();
	memcpy_s(Weight, sizeof(Variant), BloomWeight, sizeof(float) * 16);
	// horizion bloom0 -> bloom1
	float * Offset = Parameter[hash_string::gSampleOffsets].as<float[16]>();
	memcpy_s(Offset, sizeof(Variant), &BloomOffset[0], sizeof(float) * 16);
	Parameter[hash_string::gPostBuffer].as<int>() = Bloom[0];
	Compiler->SetRenderTargets(1, &Bloom[1]);
	HDRShader->Compile(Compiler, 4, 0, Parameter, Parameter, Context);
	Compiler->SetViewport(0, 0, Width, Height, 0, 1);
	Compiler->Quad();
	// horizion bloom1->bloom2
	Offset = Parameter[hash_string::gSampleOffsets].as<float[16]>();
	memcpy_s(Offset, sizeof(Variant), &BloomOffset[1], sizeof(float) * 16);
	Parameter[hash_string::gPostBuffer].as<int>() = Bloom[1];
	Compiler->SetRenderTargets(1, &Bloom[2]);
	HDRShader->Compile(Compiler, 4, 0, Parameter, Parameter, Context);
	Compiler->SetViewport(0, 0, Width, Height, 0, 1);
	Compiler->Quad();
	return 0;
}

int PostpassStage::ToneMapping(BatchCompiler * Compiler) {
	int Target = 0;
	Parameter[hash_string::gPostBuffer].as<int>() = PingPong[1];
	Parameter[hash_string::gDiffuseMap0].as<int>() = AdaptLum[0];
	Parameter[hash_string::gDiffuseMap1].as<int>() = Bloom[2];
	Compiler->SetRenderTargets(1, &Target);
	HDRShader->Compile(Compiler, 5, 0, Parameter, Parameter, Context);
	// restore viewport
	Compiler->SetViewport(0, 0, 1920, 1080, 0, 1);
	Compiler->Quad();
	Compiler->Present();
	return 0;
}

int PostpassStage::HDR(BatchCompiler * Compiler) {
	int Compiled = 0;
	if (HDRShader) {
		//Compiled += Compiler->SetDepthBuffer(-1);
		Compiled += ScaleBy4(Compiler);
		Compiled += CalcAvgLum(Compiler);
		Compiled += CalcAdaptLum(Compiler);
		Compiled += BrightPass(Compiler);
		Compiled += BloomPass(Compiler);
		Compiled += ToneMapping(Compiler);
	} else {
		Variant * Value = Context->GetResource(String("Shader\\shaders\\HDR\\0"));
		if (Value) {
			HDRShader = Value->as<Shader*>();
		}
	}
	return Compiled;
}

int PostpassStage::Execute(RenderingCamera * Camera, Spatial * spatial, RenderQueue* renderQueue, WorkQueue * Queue, Vector<OsEvent*>& Events) {
	RenderView * renderview = RenderView::Create();
	// setup render view
	renderview->Camera = Camera;
	renderview->Depth = -1;
	renderview->Type = R_STAGE_POST;
	renderview->Index = 0;
	renderview->Queue = renderQueue;
	// set render target
	renderview->TargetCount = 1;
	renderview->Targets[0] = PingPong[1];
	//renderview->Depth = Context->GetRenderTarget(String("Depth"));
	renderview->ClearDepth = 0;
	BatchCompiler * Compiler = renderview->Compiler;
	char * Buffer = (char*)renderview->CommandBuffer;
	Compiler->SetBuffer(Buffer);
	renderview->Compile();
	SSAO(Compiler);
	HDR(Compiler);
	// submit to queue
	renderview->QueueCommand();
	renderview->Recycle();
	return 0;
}

int PostpassStage::End() {
	// set time
	Time = GetCurrentTime();
	return 0;
}
