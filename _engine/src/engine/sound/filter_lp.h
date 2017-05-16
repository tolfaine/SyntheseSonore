#ifndef __FILTER_LP__
#define __FILTER_LP__

/**
*Filtre abstrait
*/
#include "filter.h"


class FilterLP : public FilterAudio
{
public:

	void setAlpha(float alpha)
	{
		_alpha = alpha;
	}

	FilterLP() : FilterAudio()
	{
		
	}

	virtual float doFilter(float ech)
	{
		float valueIn = ech;

		float returnedValue = _oldValueOut + _alpha * (valueIn - _oldValueOut);
		_oldValueOut = returnedValue;

		return returnedValue;
	}	

private :
	float _alpha;
	float _oldValueOut = 0;
};


#endif
