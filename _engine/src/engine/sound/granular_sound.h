#ifndef __GRANULAR_SOUND__
#define __GRANULAR_SOUND__

/**
* Synthé granulaire.
* Lui charger un fichier de base ou il pioche les grains
* utiliser setGrainParam pour modifier les params d'extraction et d'ajout des grains
*/

#include "continuous_sound.h"

#define GRAIN_SYNTHE_BUFFER_LENGTH 1.0f //On prend 1s de buffer en memoire

/**
  * Utilise plusieurs buffers :
  * un buffer qui contient le fichier de base
  * un buffer dans lequel on fait la synthèse : on y ajoute les grains les un a la suite des autres
  *   en tenant compte de params de synthèse. C'est un buffer tournant.
  * un buffer temporaire qui permet de créer
  */
class SoundGrain : public ContinuousSound
{
private:

	//Datas du fichier de base
	float _DureeBase; ///< Duree du buffer de base
	void * _BaseBuffer; ///< Buffer qui contient les échantillons du fichier de base
	uint8 * _PtRdFinBaseBuffer; ///< Pointeur sur la fin du buffer de base
	ALenum _Format; ///< Format du fichier de base (enum OpenAL comme AL_FORMAT_MONO16)
	ALsizei _Size; ///< Taille du fichier de base en octets
	ALfloat _Frequency; ///< Frequence d'échantillonage du fichier de base (44100 par exemple)
	int _NbPistes; ///< Nombre de pistes du fichier de base (1 mono, 2 stéréo)
	int _NbBits; ///< Nombre de bits par échantillon (16 ou 8, surtout utile pour affichage)
	int _TailleEchantillon; ///< Taille d'un échantillon en octets (plus utile pour les calculs)

	//Buffer pour faire la synthèse (buffer tournant)
	//Nécessaire car overlap des grains 
	float _DureeBufferSyntheGrain; ///<Duree du buffer de synthèse
	void * _DatasSyntheGrain; ///< Pointeur sur le début du buffer de synthèse
	uint8 * _PtFinBufferSyntheGrain; ///< Pointeur sur la fin du buffer de synthe
	int _TailleBufferSyntheGrain; ///< Taille du buffer de synthèse
	uint8 * _PtRdSyntheGrain; ///< Pointeur de lecture du buffer de synthese
	uint8 * _PtWrSyntheGrain; ///< Pointeur d'ecriture dans le buffer de synthe
	
	//Paramètres de la generation de grains
	float _PosGrainf; ///< Position de prise du grain entre 0 et 1
	float _DureeGrain; ///< Durée du grain en secondes 
	int _TailleGrain; ///< Taille du grain en octets
	int _PosGraini; ///< Position du grain en octets
	int _RandomWidth; //Taille du random quand on chope un grain
	int _SizeOverlap; //Taille d'overlapp entre les grains
	float _CrossFade; //Si on utilise un crossfade linéaire ou equalpower (signal corelé ou pas)

	//Pour la lecture des grains. 
	uint8 * _PtRdGrain; ///< Pointeur de lecture du grain courant
	uint8 * _PtRdFinGrain; ///< Pointeur de fin du grain courant
	int _TailleLastGrain; ///< Taille du grain courant

public :
	void loadBaseFile(char * filename)
	{
		alutLoadMemoryFromFile(filename, &_Format, &_Size, &_Frequency);
		
		string format = "Format : ";
		int formatInt = 0;
		switch (_Format){
		case AL_FORMAT_MONO8:{
			formatInt = 8;
			break;
		}
		case AL_FORMAT_MONO16:{
			formatInt = 16;
			break;
		}
		case AL_FORMAT_STEREO8:{
			formatInt = 8;
			break;
		}
		case AL_FORMAT_STEREO16:{
			formatInt = 16;
			break;
		}
		}
		string header = " ---- File name : " + string(filename) + " -------";

		Log::log(Log::MSG_TYPE::ENGINE_INFO, header.c_str());

		format += to_string(formatInt);

		Log::log(Log::MSG_TYPE::ENGINE_INFO, format.c_str());
		
		string frequence = "Frequence : ";
		frequence += to_string(_Frequency);

		Log::log(Log::MSG_TYPE::ENGINE_INFO, frequence.c_str());
		//On set des params par defaut au cas ou

		string tailleechant = "Taille Echantillon : ";
		int tailleEchantIntOct = 0;
		switch (formatInt){
		case 8:{
			tailleEchantIntOct = 1;
		}
		case 16:{
			tailleEchantIntOct = 2;
		}
		}
		tailleechant += to_string(tailleEchantIntOct);
		tailleechant += " octets";
		Log::log(Log::MSG_TYPE::ENGINE_INFO, tailleechant.c_str());


		string nbEchantant = "Nombres total d'achantillons : ";
		int tailleEchantIntBit = tailleEchantIntOct * 8;
		int nbEchantantInt = (_Size*8) / tailleEchantIntBit;
		nbEchantant += to_string(nbEchantantInt);
		Log::log(Log::MSG_TYPE::ENGINE_INFO, nbEchantant.c_str());

		string dureee = "Duree du fichier : ";
		int nbSecond = nbEchantantInt/_Frequency;
		dureee += to_string(nbSecond);
		dureee += "seconds";
		Log::log(Log::MSG_TYPE::ENGINE_INFO, dureee.c_str());

		setGrainParam(0.5f,0.2f,0.2f,0.05f,0.5f);


		// ((sint16*)buffer)
	}

	void unload()
	{
		SAFEDELETE_TAB(_DatasSyntheGrain);
		free(_BaseBuffer); _BaseBuffer = NULL; //AlutLoad* = malloc;
	}

	void setGrainParam(float posGrain, float dureeGrain, float randomPos, float partOverlap, float crossFade)
	{
		_PosGrainf = posGrain;
		_DureeGrain = dureeGrain;
		_TailleGrain = (int) ( _DureeGrain * _Frequency * _TailleEchantillon );
		_TailleGrain = (_TailleGrain/4)*4; //On s'aligne sur stereo 16 bits
		_PosGraini = (int) (_Size * _PosGrainf);
		_PosGraini = (_PosGraini / 4) * 4;  //On s'aligne sur 16 bits deux voies au cas ou 
		_RandomWidth = (int) ( randomPos * _Frequency *_TailleEchantillon );
		_SizeOverlap = (int ) ( partOverlap * _DureeGrain * _Frequency * _TailleEchantillon );
		_SizeOverlap = (_SizeOverlap/4)*4; //On aligne
		_CrossFade = crossFade;
	}


private :

	/**
	  * Phase de synthèse
	  * Cette méthode va lire le buffer qui contient le fichier de base pour en extraire de grains
	  * qu'elle va copier au fur et à mesure dans le buffer de synthèse
	  * Quand elle a fini la synthèse, elle retourne un échantillon pris dans le buffer de synthèse, 
	  * sous le pointeur de lecture, et avance ce pointeur de lecture.
	  */

	ALuint _Buffer;

	virtual float getNextSample()
	{
		return 0.0f;
	}

	
};

#endif
