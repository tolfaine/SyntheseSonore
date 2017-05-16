#ifndef __BASIC_FFT__
#define __BASIC_FFT__

#include "engine/utils/types.h"
#include "engine/utils/ny_utils.h"
#include "engine/log/log.h"
#include "external/ffft/FFTReal.h"

class BasicFFT
{
public:

	BasicFFT()
	{
		_In = NULL;
		_Out = NULL;
		_N = 0;
	}
	
	void updateInputSint16(sint16 * input, int nbVals)
	{
		checkPlan(nbVals);
		for(int i=0;i<nbVals;i++)
			_In[i] = ((float)input[i]) / (65535.0f/2.0f);
	}

	void init(int nbVals)
	{
		checkPlan(nbVals);
	}

	float const * getValues(void)
	{
		_FFT_object->do_fft(_Out,_In);
		return _Out;
	}

	int getNb(void)
	{
		return _N;
	}

protected :
	ffft::FFTReal <float> * _FFT_object;
	float * _In;
	float * _Out;
	int _N;

	void checkPlan(int nbVals)
	{
		if(nbVals != _N){
			_N = nbVals;
			_FFT_object  = new ffft::FFTReal <float> (_N);
			SAFEDELETE_TAB(_In);
			SAFEDELETE_TAB(_Out);
			_In = new float[_N];
			_Out = new float[_N];
		}
	}
};

#endif
