#ifndef __FILTER_REVERB__
#define __FILTER_REVERB__

/**
*Filtre abstrait
*/
#include "filter.h"

class FilterReverb : public FilterAudio
{
public:

	void setReverb(float reverb)
	{
		_reverb = reverb;
	}

	FilterReverb() : FilterAudio()
	{
		init();
	}

	void init()
	{
		for (int i = 0; i < _sizeBuffer; i++){
			_buffer[i] = 0;
		}
	}

	virtual float doFilter(float ech)
	{
		float finalValue = ech;
		if (_isFull){
			finalValue += _buffer[_currentIndex] * _reverb;
			finalValue /= 2;
		}

		_buffer[_currentIndex] = ech;
		_currentIndex++;

		if (_currentIndex == _sizeBuffer){
			_currentIndex = 0;
			_isFull = true;
		}

		return finalValue;
	}	

private :

	float _buffer[20000];
	int _sizeBuffer = 20000;
	int _currentIndex = 0;
	bool _isFull = false;

	float _reverb;

};


#endif
