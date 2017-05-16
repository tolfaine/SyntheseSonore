#ifndef __SINUS_SOUND__
#define __SINUS_SOUND__

/**
* Synthé sinusoidal.
*/
#include "continuous_sound.h"
#include <math.h>

class SinusSound : public ContinuousSound
{
public:
	
	SinusSound() : ContinuousSound()
	{
		_time = 0;
	}

	void setFreq(float freq, float stepFreqPerPeriod)
	{
		_stepFreqPerPeriod = stepFreqPerPeriod;

		_previousSpeed = _frequency;

	//	_frequency = freq;

		_changeSpeed = true;
		_requestedSpeed = freq;
		_currentInterpolationTime = 0;

	}
private:

	float _frequency =440;
	float _previousSpeed;
	float _requestedSpeed;

	bool _changeSpeed = false;

	float _interpolationTime =1;
	float _currentInterpolationTime = 0;

	float _time;

	float _stepFreqPerPeriod;

	float _previousSample;

	float _phase = 0;

protected :
	virtual void init()
	{

	}

	/**
	  * Remplissage du buffer de synthèse, jusqu'au pointeur de lecture
	  */
	virtual float getNextSample()
	{
		_time += 1 / _Frequency;
		_currentInterpolationTime += 1 / _Frequency;


		float newSample = sin((2 * M_PI* _time*_frequency + _phase));

		if (_previousSample < 0 && newSample >= 0){

			

			if (_changeSpeed){
				if (_currentInterpolationTime >= _interpolationTime){
					_changeSpeed = false;
					_currentInterpolationTime = 0;
				}
				else{
					float alpha = _currentInterpolationTime / _interpolationTime;
					//_frequency = 1 - (3 * pow(alpha, 2) - 2 * pow(alpha, 3)) * _previousSpeed + (3 * pow(alpha, 2) - 2 * pow(alpha, 3)) * _requestedSpeed;

					_frequency = _previousSpeed + (3 * pow(alpha, 2) - 2 * pow(alpha, 3)) * (_requestedSpeed - _previousSpeed);
					_time = 0;

					

					_phase = asin(newSample); //- 2 * M_PI * _time*_frequency;

					newSample = sin((2 * M_PI* _time*_frequency + _phase));

				}
			}
		}

		_previousSample = newSample;
		return newSample * 0.8;
	}	

};


#endif
