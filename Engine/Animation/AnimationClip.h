#ifndef __ANIMATION_CLIP__
#define __ANIMATION_CLIP__

#include "Core\Str.h"
#include "AnimationCache.h"

class Animation;
class AnimationCache;

class AnimationClip {

	friend Animation;

public:
	// start frame
	int StartFrame;
	// end frame
	int EndFrame;
	// start time
	float TimeOffset;
	// end time
	float EndTime;
	// total translation in position
	Vector3 Tanslation;
	// start rootmotion
	Vector3 RootStart;
	// animation source data of this clip
	Animation * Source;
	// name
	String Name;
	// is looped
	bool Looped;
public:
	AnimationClip();
	~AnimationClip();
	// sample the animation by time, store the result to animationcache
	void Sample(float time, AnimationCache * Cache);

};
#endif
