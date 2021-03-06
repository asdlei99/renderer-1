#include "RenderLight.h"
#include "RenderQueue.h"
#include "Core\StringTable.h"


USING_ALLOCATER(RenderLight);

//cbuffer Light : register(b5)
//{
//	float4 gLightPosition;
//	float4 gLightColor;
//	float4 gRadiusIntensity;
//};


RenderLight::RenderLight() : Radius(1.0f), Intensity(1.0f), Color(Vector3(0.0f, 0.1f, 0.0f)), ShadowCast(0)
{
	Type = Node::LIGHT;
	LightType = RenderLight::POINT;
	// lightCamera
	LightCamera = new RenderingCamera();
}


RenderLight::~RenderLight()
{
	delete LightCamera;
}

void RenderLight::SetLightType(int Type) {
	LightType = Type;
}

void RenderLight::SetRadius(float r) {
	Radius = r;
	//Parameter[hash_string::gRadiusIntensity].as<Vector3>() = Vector3(Radius, Intensity, 0);
	CullingObj.UniformScale(r);
}
// set color
void RenderLight::SetColor(Vector3& Color_) {
	Color = Color_;
	//Parameter[hash_string::gLightColor].as<Vector3>() = Color;
}
// set intensity
void RenderLight::SetIntensity(float Intensity_) {
	Intensity = Intensity_;
	//Parameter[hash_string::gRadiusIntensity].as<Vector3>() = Vector3(Radius, Intensity, 0);
}
// set direction
void RenderLight::SetDirection(Vector3& Direction_) {
	Direction = Direction_;
	//Parameter[hash_string::gLightDirection].as<Vector3>() = Direction;
}
// set shdowcast disable/enable
void RenderLight::SetShadowCast(int Flag) {
	ShadowCast = Flag;
}

void RenderLight::SetShadowMap(int id) {
	// Parameter[hash_string::gShadowMap].as<int>() = id;
	ShadowMap = id;
}

RenderingCamera* RenderLight::GetLightCamera() {
	Matrix4x4 Projection = Matrix4x4::PerspectiveFovLH(0.25f * 3.1415926f, 1.0f, 1, Radius);
	LightCamera->FromLight(Position, Rotation, Projection);
	return LightCamera;
}

int RenderLight::Compile(BatchCompiler* Compiler, int Stage, int Lod, Dict& StageParameter, RenderingCamera* Camera, RenderContext* Context) {
	Stage = 0;
	// process matrix
	Matrix4x4 Transform = GetWorldMatrix();
	Matrix4x4 Scale;
	Scale.Scale(Vector3(Radius, Radius, Radius));
	Transform = Scale * Transform;
	// perlight position
	Matrix4x4::Tranpose(Transform * Camera->GetViewProjection(), &StageParameter["gWorldViewProjection"].as<Matrix4x4>());
	Matrix4x4::Tranpose(Transform * Camera->GetViewMatrix(), &StageParameter["gWorldViewMatrix"].as<Matrix4x4>());
	// per-frame
	//Matrix4x4::Tranpose(Camera->GetInvertView(), &Parameter[hash_string::gInvertViewMaxtrix].as<Matrix4x4>());
	//Matrix4x4::Tranpose(Camera->GetProjection(), &Parameter[hash_string::gProjectionMatrix].as<Matrix4x4>());
	//Parameter[hash_string::gViewPoint].as<Vector3>() = Camera->GetViewPoint();
	//Parameter[hash_string::gScreenSize].as<Vector2>() = Vector2(static_cast<float>(Context->FrameWidth), static_cast<float>(Context->FrameHeight));
	// light parameters
	StageParameter["gLightPosition"].as<Vector3>() = Position * Camera->GetViewMatrix();
	StageParameter["gLightDirection"].as<Vector3>() = Direction.RotateBy(Camera->GetViewMatrix());
	StageParameter["gRadiusIntensity"].as<Vector3>() = Vector3(Radius, Intensity, 0);
	StageParameter["gLightColor"].as<Vector3>() = Color;
	StageParameter["gShadowMap"].as<int>() = ShadowMap;
	Matrix4x4::Tranpose(LightCamera->GetViewProjection(), &StageParameter["gLightViewProjection"].as<Matrix4x4>());
	// process material
	int Compiled = 0;
	// stencil-pass
	Shader* shader = 0;
	if (material) {
		Compiled += material->Compile(Compiler, Stage, Lod);
		// process shader
		shader = material->GetShader();
		Compiled += shader->Compile(Compiler, Stage, Lod, material->GetParameter(), StageParameter, Context);
	}
	int Geometry = GetRenderMesh(Stage, Lod);
	if (Geometry != -1 && LightType == POINT) {
		//Compiled += Compiler->SetTransform(Transform);
		Compiled += Compiler->RenderGeometry(Geometry);
	}
	// lighting-pass
	// get quad shader stage types
	switch (LightType) {
	case POINT:
		Stage = 1;
		//return Compiled;
		break;
	case DIRECTION:
		Stage = 2;
		// return Compiled;
		break;
	case ENV:
		Stage = 3;
		// return Compiled;
		break;
	default:
		Stage = 1;
	}
	Compiled += shader->Compile(Compiler, Stage, Lod, material->GetParameter(), StageParameter, Context);
	// full screen quad
	Compiled += Compiler->Quad();
	return Compiled;
}
