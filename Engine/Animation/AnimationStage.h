#ifndef __ANIMATION_STAGE__
#define __ANIMATION_STAGE__

#include "AnimationClip.h"
#include "AnimationCache.h"

class Animation;


class AnimationStage {

private:
	// current animation time
	float Time;
	// time scale
	float Scale;
	// the clip to use
	AnimationClip * Clip;
	// the animation cache
	AnimationCache * Cache;
	// start bone
	unsigned char StartBone;
	// actived
	bool Actived;
public:
	AnimationStage();
	~AnimationStage();

	// set clip
	void SetAnimationClip(AnimationClip * Clip_) { Clip = Clip_; }
	//settime
	void SetTime(float time) { Time = time; }
	// set scale
	void SetScale(float scale) { Scale = scale; }
	// add time
	void Advance(float time);
	// applly
	void Apply();
	// get cache
	AnimationCache * GetAnimationCache() { return Cache; };

};


#endif
