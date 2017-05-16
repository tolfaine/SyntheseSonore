#ifndef __CONTINUOUS_SOUND__
#define __CONTINUOUS_SOUND__

/**
* Son continu : genere un buffer tournant de synthèse
*/
#include "sound.h"
#include "filter.h"

#define CARD_BUFFER_LENGTH 0.1f  //On prend 100ms de buffer sur la carte
#define SYNTHE_BUFFER_LENGTH 0.5f  //On prend 500ms de buffer en memoire

class ContinuousSound : public Sound
{
		
public:

	ContinuousSound() : Sound()
	{
		//On set le format en 16bits 44100 mono
		_Format = AL_FORMAT_MONO16;
		_Frequency = 44100;
		_TailleEchantillon = 2;
		_NbPistes = 1;

		_GeneratorActive = true;
		_StateGenerator = ATTACK_SOUND;
		_AttackDuration = 0.01f;
		_ReleaseDuration = 0.1f;
		_SoundDuration = -0.2f; //infini
		_CurrentDuration = 0.0f;
		_GainGenerator = 0.0f;
		_GainFinal = 1.0f;
		
		_PowerSum = 0.0f;
		_PowerSumNbAdd = 0;
		_PowerCalcDuration = 0.1f;
		_PowerTarget = 0.5f;
		_AutoPower = false;

		_DureeBufferAL = CARD_BUFFER_LENGTH; 
		_TailleBufferAL = _DureeBufferAL * _Frequency * _TailleEchantillon * _NbPistes;
		_TailleBufferAL = (_TailleBufferAL/4)*4; //alignement 16bits stereo
		_DatasToLoadAL = new uint8[_TailleBufferAL];
		memset(_DatasToLoadAL,0,_TailleBufferAL); 

		_DureeBuffer = SYNTHE_BUFFER_LENGTH;
		_TailleBuffer = _DureeBuffer * _Frequency * _TailleEchantillon * _NbPistes;
		_TailleBuffer = (_TailleBuffer/4)*4; //alignement 16bits stereo
		_Buffer = new uint8[_TailleBuffer];
		memset(_Buffer,0,_TailleBuffer); 

		_PtFinBuffer = ((uint8*)_Buffer) + _TailleBuffer;
		_PtRdBuffer = ((uint8*)_Buffer);
		_PtWrBuffer = ((uint8*)_Buffer) + _TailleEchantillon;

		_PlayingBufferAL = 0;
		alGenBuffers(3,_BuffersIdAL);
		checkAlError("alGenBuffers,ContinuousSound()");

		_Loop = true;
	}

	~ContinuousSound()
	{
		SAFEDELETE_TAB(_DatasToLoadAL);
		SAFEDELETE_TAB(_Buffer);
	}

	//Enveloppe
	void setAttackDuration(float duration){_AttackDuration = duration;}
	void setReleaseDuration(float duration){_ReleaseDuration = duration;}
	
	//Gain applique a la fin
	void setGainFinal(float gain)
	{
		if(!_AutoPower)
			_GainFinal = gain;
	}
	void setAutoPower(bool autopower){_AutoPower = autopower;}
	float getPower()
	{
		if(_PowerSumNbAdd == 0)
			return 0.0f;
		return _PowerSum / (float)(_PowerSumNbAdd);
	}
	float getPowerMax() { return _PowerMax; }

	//Etat du generateur
	void activateGenerator(bool play) {	_GeneratorActive = play; _CurrentDuration=0.0f; }
	bool isGeneratorActive(void){ return _GeneratorActive; }

	//Recup du buffer de synthèse, notamment pour l'afficher
	void const * getBufferSynthese(int * tailleBuffer, int * tailleEchantillon, int * nbPistes, void ** pt_Read)
	{
		*tailleBuffer = this->_TailleBuffer;
		*tailleEchantillon = this->_TailleEchantillon;
		*nbPistes = this->_NbPistes;
		*pt_Read = _PtRdBuffer;
		return this->_Buffer;
	}

	//Gestion des filtres
	void addFilter(FilterAudio * filter)
	{
		filter->setFrequency(_Frequency);
		_Filters.push_back(filter);
	}
	void removeFilters(void)
	{
		_Filters.clear();		
	}

	//Lecture
	void play(void) 
	{
		//Initialisation du generateur
		init();
		_PowerSum = 0;
		_PowerSumNbAdd = 0;
		_StateGenerator = ATTACK_SOUND;
		_GainGenerator = 0.0f;
		_GainFinal = 1.0;
		_CurrentDuration = 0.0f;

		while(getNbBuffersQueued() < 2)
		{
			fillBaseBuffer();
			_PlayingBufferAL = (_PlayingBufferAL + 1)%3;
			addBufferToQueue(_PlayingBufferAL);
		}

		Sound::play();
	}

	// Mise a jour
	void update(float elapsed)
	{
		Sound::update(elapsed);
		
		if(getNbBuffersProcessed() > 0)
		{
			_PlayingBufferAL = (_PlayingBufferAL + 1)%3;
			removeBufferFromQueue(_PlayingBufferAL);
			fillBaseBuffer();
			addBufferToQueue(_PlayingBufferAL);
		}
	}

protected :

	//Les filtres, appliqués dans l'ordre d'ajout
	std::vector<FilterAudio*> _Filters;

	//Gestion de l'enveloppe : si debut milieu ou fin
	//On applique l'enveloppe au generateur, c'est à dire a la sortie de la synthèses (pas apres les filtres)
	typedef enum STATE_SOUND_T
	{
		ATTACK_SOUND,
		SUSTAIN_SOUND,
		RELEASE_SOUND
	} STATE_SOUND;
	STATE_SOUND _StateGenerator; ///< Etat du generateur (debut milieu ou fin)
	float _AttackDuration; ///< Duree en secondes de l'attaque
	float _ReleaseDuration; ///< Duree en secondes du fade out
	float _SoundDuration; ///< Duree totale du son (infini si negatif);
	float _CurrentDuration; ///< Duree actuelle de synthese;
	float _GainGenerator; ///< Gain appliqué en fonction de l'état (enveloppe)
	bool _GeneratorActive;  ///< Si le générateur est actif
	
	float _GainFinal; ///< Gain final, appliqué après le filtrage

	//Format du buffer de synthèse
	ALenum _Format; ///< Format du buffer de syntheses (enum OpenAL comme AL_FORMAT_MONO16)
	ALfloat _Frequency; ///< Frequence d'échantillonage du buffer de synthese (44100 par exemple)
	int _NbPistes; ///< Nombre de pistes du fichier de base (1 mono, 2 stéréo)
	int _NbBits; ///< Nombre de bits par échantillon (16 ou 8, surtout utile pour affichage)
	int _TailleEchantillon; ///< Taille d'un échantillon en octets (plus utile pour les calculs)

private:
	//Buffers pour queuing openal
	ALuint _BuffersIdAL[3]; ///<id des buffers
	ALuint _PlayingBufferAL; ///< indice du buffer en cours de lecture par la carte
	float _DureeBufferAL; ///< Duree d'un buffer openal
	int _TailleBufferAL; ///< Taille d'un buffer openal
	void * _DatasToLoadAL; ///< buffer temporaire pour balancer les datas à openal

	//Buffer pour faire la synthèse (buffer tournant)
	float _DureeBuffer; ///< Duree du buffer de synthese en secondes
	int _TailleBuffer; ///< Taille du buffer de synthese en octets
	void * _Buffer; ///< Buffer de synthèse
	uint8 * _PtRdBuffer; ///< Pointeur de lecture du buffer de syntthe vers les buffers openal
	uint8 * _PtWrBuffer; ///< Pointeur d'ecriture dans le buffer de synthe
	uint8 * _PtFinBuffer; ///< Pointeur sur la fin du buffer de synthe
	
	//Ajustement automatique du niveau
	bool _AutoPower; ///< Si on ajuste automatiquement la puissance
	float _PowerTarget; ///< La puissance moyenne qu'on vise
	float _PowerSum; ///< Somme actuelle de la puissance (permet de ne pas parcourir tout le buffer a chaque fois pour faire la somme)
	int _PowerSumNbAdd; ///< Combien on a ajouté d'échantillons à la somme
	float _PowerCalcDuration; ///< Sur combien de temps on clacule la pusisance moyenne
	float _PowerMax; ///< Dernier niveau maximal atteint		

protected:

	/**
	  * Generation d'un sample
	  */
	virtual float getNextSample()
	{
		//du static
		return (float) (randf()*2.0f-1)*0.5f;
	}	

	/**
	  * Initialisation
	  */
	virtual void init()
	{

	}

private :

	/**
	  * Remplissage du buffer de synthèse, jusqu'au pointeur de lecture
	  */
	void fillBaseBuffer()
	{
		//On ajoute des echantillons generes jusqu'au buffer de lecture
		while(_PtWrBuffer != _PtRdBuffer)
		{
			//On recup l'echantillon du generateur
			float ech = getNextSample();
							
			//On gere l'attaque et release simples
			if(!_GeneratorActive)
				_StateGenerator = RELEASE_SOUND;
			else
				if(_GainGenerator >= 1.0f)
					_StateGenerator = SUSTAIN_SOUND;
				else
					_StateGenerator = ATTACK_SOUND;

			if(_SoundDuration > 0 && _CurrentDuration >= _SoundDuration)
				_StateGenerator = RELEASE_SOUND;

			if(_StateGenerator == ATTACK_SOUND)
				_GainGenerator += 1.0f/(_AttackDuration * _Frequency);
			if(_StateGenerator == RELEASE_SOUND)
				_GainGenerator -= 1.0f/(_ReleaseDuration * _Frequency);
			
			if(_GainGenerator < 0.0f)
				_GainGenerator = 0.0f;
			if(_GainGenerator > 1.0f)
				_GainGenerator = 1.0f;

			ech *= _GainGenerator;

			//On filtre si choisi
			for(int i=0;i<_Filters.size();i++)
			{
				ech = _Filters[i]->filter(ech);
			}

			//Gain final
			ech *= _GainFinal;

			//Calcul de la puissance
			//Attention, si on ajoute l'ech direct et qu'il sature, quand on va le retirer, il sera plus petit (coupe par le max du sint16)
			//et le _Power va rester bloqué en haut
			sint16 poweradd = ech * (65536/2.0f); 
			_PowerSum += abs((float)poweradd /(65536/2.0f));
			if(_PowerSumNbAdd > 0)
			{
				sint16* ptr = ((sint16*)(_PtWrBuffer)) - _PowerSumNbAdd;
				if(ptr < _Buffer)
					ptr += (_TailleBuffer/2);

				_PowerSum -= abs(((float)*(ptr))/(65536/2.0f)); 
			}

			if(_PowerSumNbAdd < (int)(_Frequency*_PowerCalcDuration))
				_PowerSumNbAdd++;

			if(abs(ech) > _PowerMax)
				_PowerMax = abs(ech);

			//Auto Power
			if(_AutoPower && _PowerSumNbAdd > 100)
			{
				float elapsed = 1.0f/_Frequency;

				//Limite violente pour eviter explosier
				if(_PowerMax > 0.8f)
				{
					_GainFinal = _GainFinal / (_PowerMax / 0.8f);
					_PowerMax = 0.8f;
				}
				else
				{
					//Si sous seuil bas
					if(getPower() < _PowerTarget - 0.1f)
						_GainFinal += 1.0 * elapsed;

					//Si sur seuil haut
					if(getPower() > _PowerTarget + 0.1f)
						_GainFinal -= 1.0 * elapsed;
				}
				
				_PowerMax -= elapsed * 0.01;
				_PowerMax = max(_PowerMax,0);
				_PowerMax = min(_PowerMax,1.0f);

				if(_GainFinal < 0)
					_GainFinal = 0;
				if(_GainFinal > 100)
					_GainFinal = 100;
			}
			
			 
			//On ecrit l'echantillon
			*((sint16*)_PtWrBuffer) = ech * (65536/2.0f);
			
			//On avance
			_PtWrBuffer += _TailleEchantillon;

			//On avance dans le temps
			_CurrentDuration += 1/_Frequency;

			//On fait le tour du buffer tournant
			if(_PtWrBuffer >= _PtFinBuffer)
				_PtWrBuffer = (uint8*) _Buffer;
		}
	}	


	/**
	  * Phase de copie
	  * Cette méthode rempli un buffer temporaire en mémoire CPU avec une partie du buffer de synthèse.
	  * elle copie ensuite ces données vers un buffer opanal et l'ajoute à la suite de a liste de buffers.
	  * on utilise 3 buffers openal pour gérer la file d'attente.
	  */
	void addBufferToQueue(int numBuffer)
	{
		//On copie les datas du buffer tournant vers le buffer de chargement
		uint8* ptWrite = (uint8*)_DatasToLoadAL;
		for(int i=0;i<_TailleBufferAL;i++)
		{
			//On charge
			*ptWrite = *(_PtRdBuffer);

			//En meme temps on efface ce qui a ete charge
			if(_NbBits == 8)
				*(_PtRdBuffer) = 128;
			else
				*(_PtRdBuffer) = 0;

			//On passe a l'octet suivant
			_PtRdBuffer++;
			if(_PtRdBuffer >= _PtFinBuffer)
				_PtRdBuffer = (uint8*)_Buffer;
			ptWrite++;
		}

		//On envoie les datas a openal
		alBufferData(_BuffersIdAL[numBuffer],_Format,_DatasToLoadAL,_TailleBufferAL,(ALsizei)_Frequency);
		checkAlError("alBufferData,addBufferToQueue");
		alSourceQueueBuffers(_Source,1,&(_BuffersIdAL[numBuffer]));
		checkAlError("alSourceQueueBuffers,addBufferToQueue");
	}

	/**
	  * retire un buffer de la liste des buffers openal
	  */
	void removeBufferFromQueue(int numBuffer)
	{
		alSourceUnqueueBuffers(_Source,1,&(_BuffersIdAL[numBuffer]));
		checkAlError("alSourceUnqueueBuffers,removeBufferFromQueue");
	}

	/**
	  * Récupère le nombre de buffers deja parcourus par openal
	  */
	int getNbBuffersProcessed(void)
	{
		ALint  value;
		alGetSourcei(_Source,AL_BUFFERS_PROCESSED,&value);
		checkAlError("alGetSourcei,getNbBuffersProcessed");
		return value;
	}

	/**
	  * Récupère le nombre de buffers dans la liste de buffers en attente
	  */
	int getNbBuffersQueued(void)
	{
		ALint  value;
		alGetSourcei(_Source,AL_BUFFERS_QUEUED,&value);
		checkAlError("alGetSourcei,getNbBuffersQueued");
		return value;
	}
};

#endif
