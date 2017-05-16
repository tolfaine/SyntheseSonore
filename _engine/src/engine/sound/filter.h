#ifndef __FILTER_SOUND__
#define __FILTER_SOUND__

/**
*Filtre abstrait
*/
#include "engine/utils/types.h"
#include "engine/utils/ny_utils.h"
#include "engine/log/log.h"

class FilterAudio
{
public:

	FilterAudio()
	{
		_Frequency = 44100;
		_Active = true;
	}

	float filter(float ech)
	{
		if(_Active)
			return doFilter(ech);
		return ech;
	}

	void activate(bool active)
	{
		_Active = active;
	}

	void setFrequency(float frequency)
	{
		_Frequency = frequency;
	}

	virtual void init()
	{

	}

protected :
	
	/**
	  * Methode a redefinir pour votre filtre
	  */
	virtual float doFilter(float ech)
	{
		return ech;
	}	

private:
	float _Frequency;
	bool _Active;

};


#endif
