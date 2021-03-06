#include "Animator.h"
#include "AnimationSystem.h"
#include "Rendering\Renderer.h"
#include "Rendering\MeshRenderer.h"
#include "Physics/CharacterController.h"


USING_ALLOCATER(Animator)

Animator::Animator(Context* context) : Component(context), BlendNode(nullptr), BlendShape_(nullptr), BlendShapeDescStage_{ 0 }, BlendShapeDesc_{ 0 }, Stage(nullptr), skeleton(nullptr)
{
}


Animator::~Animator() {
}

void Animator::Update(float time) {
	// advance in time
	//Stage->Advance(time);
	//Stage->Apply();
	MeshRenderer* renderer = (MeshRenderer*)Owner->GetComponent("Renderer");
	if (BlendNode) {
		BlendNode->Advance(time);
		BlendNode->Apply();
		// get the result
		AnimationCache* Cache = BlendNode->GetAnimationCache();
		// set palette
		Cache->GeneratePalette(skeleton);
		renderer->SetMatrixPalette(Cache->Palette, Cache->Result.Size());
		// apply root motion, get translation delta
		// set postion delta and rotation for charactr controller
		auto Controller = (CharacterController*)Owner->GetComponent("CharacterController");
		// get delta transformation from prev-updated pos
		auto& Motion = BlendNode->GetMotion();
		Matrix4x4 CurrentTrans = Owner->GetTransform();
		Matrix4x4 TargetTrans = Motion * CurrentTrans;
		Vector3 Walk = Vector3(0, 0, 0) * TargetTrans - Vector3(0, 0, 0) * CurrentTrans;
		Quaternion Rotation;
		Rotation.FromMatrix(TargetTrans);

		// update character controller and node
		Controller->SetWalkDirection(Walk);
		// Owner->SetTranslation(Owner->GetTranslation() + Walk);
		Owner->SetRotation(Rotation);
	}
	// apply blendshape description to renderer
	if (BlendShape_) {
		// pack blendshape weight buffer
		auto valid = -1;
		for (auto i = 0; i < MAX_BS_NUMBER; i++) {
			if (BlendShapeDescStage_.entries[i].used) {
				valid++;
				BlendShapeDesc_.entries[valid] = BlendShapeDescStage_.entries[i];
			}
		}
		// set the packed description
		BlendShapeDesc_.num_weiths = (float)(valid + 1);
		renderer->SetBlendShapeDesc(&BlendShapeDesc_);
	}

}

void Animator::SetAnimationStage(int Layer, AnimationClip* Clip, unsigned char StartBone, float Scale) {
	Stage = new AnimationStage();
	Stage->SetAnimationClip(Clip);
	Stage->SetTime(0.0f);
	Stage->SetScale(Scale);
}

void Animator::SetBlendingNode(BlendingNode* Node) {
	BlendNode = Node;
}

BlendingNode* Animator::GetBlendingNode() const {
	return BlendNode;
}

void Animator::ListBlendShapes() const {

}

int Animator::OnAttach(GameObject* GameObj) {

	AnimationSystem* animationSys = context->GetSubsystem<AnimationSystem>();
	animationSys->AddAnimator(this);
	return 0;
}

int Animator::OnDestroy(GameObject* GameObj) {
	// call parent
	AnimationSystem* animationSys = context->GetSubsystem<AnimationSystem>();
	animationSys->RemoveAnimator(this);
	Component::OnDestroy(GameObj);
	if (BlendNode) {
		BlendNode->DecRef();
	}
	return 0;
}


void Animator::SetBlendShape(BlendShape* blendshape) {
	// assign blendshape
	BlendShape_ = blendshape;
	// file blendshape description
	BlendShapeDesc_.num_shapes = (float)blendshape->ShapeCount_;
	BlendShapeDesc_.buffer_stride = (float)blendshape->ShapeStride_;
	// get shape to rederer
	MeshRenderer* renderer = (MeshRenderer*)Owner->GetComponent("Renderer");
	renderer->SetBlendShape(blendshape);
}


void Animator::SetBlendShapeWeight(const String& name, float weight) {
	// get shape index by name
	String names[3];
	int index = 0;
	for (auto iter = BlendShape_->BlendShapes_.Begin(); iter != BlendShape_->BlendShapes_.End(); iter++) {
		auto mesh = *iter;
		auto url = mesh->GetUrl();
		// split the url to get the filename (mesh_name)
		url.Split('\\', names, 3);
		if (names[2] == name) {
			break;
		}
		index++;
	}
	if (index > BlendShape_->ShapeCount_ - 1) {
		// can not find the mesh by name
		return;
	}
	// set the weight by index
	SetBlendShapeWeight(index, weight);
}

void Animator::SetBlendShapeWeight(int index, float weight) {
	BlendShapeDescStage_.entries[index].index = (float)index;
	BlendShapeDescStage_.entries[index].weight = (float)weight;
	BlendShapeDescStage_.entries[index].used = 1;
}