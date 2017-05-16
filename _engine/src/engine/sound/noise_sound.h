#ifndef __NOISE_SOUND__
#define __NOISE_SOUND__

/**
* Bruit de variabilité ajustable
*/
#include "continuous_sound.h"

class NoiseSound : public ContinuousSound
{
public:
	
	NoiseSound() : ContinuousSound()
	{		
		_previousSample = 0;
		_currentFrequence = 0;
		for (int i = 0; i < _nbSamples; i++){
			_vCurrentFreqAndSample.push_back(make_pair(0, (float)(randf()*2.0f - 1)*0.5f));
		}
		setFreq(40);
	}

	void setFreq(float freq)
	{
		for each(std::pair<float, float> pair in _vCurrentFreqAndSample){
			pair.first = 0;
		}
		_frequence = freq;
	}

private:
	float _frequence;
	float _currentFrequence;
	float _previousSample;
	std::vector<std::pair<float, float>>_vCurrentFreqAndSample;
	int _nbSamples = 8;
	float _freqMultiplicator = 0.1;
	
protected :

	/**
	  * Remplissage du buffer de synthèse, jusqu'au pointeur de lecture
	  */
	virtual float getNextSample()
	{	
		//_currentFrequence++;

		std::pair<float, float> currentPair;
		float curentSample = 0;

		for (int i = 0; i < _nbSamples; i++){

			_vCurrentFreqAndSample[i].first++;

			float freqmax = _frequence*_freqMultiplicator* 0.5* (i + 1);

			if (_vCurrentFreqAndSample[i].first >= freqmax){
				_vCurrentFreqAndSample[i].first = 0;
				_vCurrentFreqAndSample[i].second = (float)(randf()*2.0f - 1)*0.5f;
			}
			curentSample += _vCurrentFreqAndSample[i].second;
		}

		return curentSample;
	}	

};

#endif
