#ifndef __ANIMATED_CAM_H__
#define __ANIMATED_CAM_H__

#include "external/gl/glew.h"
#include "external/gl/freeglut.h" 
#include "engine/utils/types_3d.h"
#include "engine/utils/ny_utils.h"

//Une animation de camera
class CamAnim
{
public:
	NYVert3Df StartPos;
	NYVert3Df Pos;
	NYVert3Df EndPos;
	NYVert3Df StartUp;
	NYVert3Df Up;
	NYVert3Df EndUp;
	NYVert3Df StartLookAt;
	NYVert3Df LookAt;
	NYVert3Df EndLookAt;
	float Duration;
	float Elapsed;
	bool Running;

	CamAnim()
	{
		Duration = 3.0f;
		Elapsed = 0;
		Running = false;
	}

	void start(NYVert3Df startPos,	NYVert3Df endPos,
				NYVert3Df startUp,	NYVert3Df endUp,
				NYVert3Df startLookAt, NYVert3Df endLookAt,
				float duration)
	{
		this->StartPos = startPos;
		this->EndPos = endPos;
		this->StartUp = startUp;
		this->EndUp = endUp;
		this->StartLookAt = startLookAt;
		this->EndLookAt = endLookAt;
		this->Duration = duration;
		this->Elapsed = 0;
		this->Running = true;
		this->update(0);
	}

	void update(float elapsed)
	{
		Elapsed += elapsed;
		float part = Elapsed/Duration;
		if(part >= 1.0f)
		{
			part = 1.0f;
			Running = false;
		}

		Pos = StartPos*(1.0f-part) + EndPos*(part);
		Up = StartUp*(1.0f-part) + EndUp*(part);
		LookAt = StartLookAt*(1.0f-part) + EndLookAt*(part);

		float distpos = (Pos-EndPos).getMagnitude();
		float distup = (Up-EndUp).getMagnitude();
		float distlookat = (LookAt-EndLookAt).getMagnitude();

		if(max(distpos,max(distup,distlookat)) == 0.0f)
		{
			part = 1.0f;
			Running = false;
			Pos = EndPos;
			Up = EndUp;
			LookAt = EndLookAt;
		}
	}
};

class NYCameraAnimated : public NYCamera
{
public:
	CamAnim _CamAnim;

	//Permet de faire tourner les animations
	void update(float elapsed)
	{
		if(_CamAnim.Running)
		{
			_CamAnim.update(elapsed);
			this->_Position = _CamAnim.Pos;
			this->_UpVec = _CamAnim.Up;
			this->_LookAt = _CamAnim.LookAt;
			updateVecs();
		}
	}

	void startAnimTo(NYVert3Df pos,NYVert3Df up,NYVert3Df lookat, float duration )
	{
		_CamAnim.start(_Position,pos,_UpVec,up,_LookAt,lookat,duration);
	}
};




#endif