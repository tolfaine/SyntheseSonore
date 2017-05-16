//Includes application
#include <conio.h>
#include <vector>

#include "external/gl/glew.h"
#include "external/gl/freeglut.h"

#include "engine/sound/sound.h"
#include "engine/sound/granular_sound.h"
#include "engine/sound/continuous_sound.h"
#include "engine/sound/sinus_sound.h"
#include "engine/sound/noise_sound.h"
#include "engine/sound/filter_reverb.h"

#include "engine/sound/filter_lp.h"
#include "engine/sound/basic_fft.h" 

//Nos includes
#include "engine/utils/types_3d.h"
#include "engine/timer.h"
#include "engine/render/renderer.h"

#include "engine/gui/screen.h"
#include "engine/gui/screen_manager.h"
#include "engine/log/log_console.h"
#include "engine/log/log_file.h"

NYRenderer * g_renderer = NULL;
NYTimer * g_timer = NULL;
int g_nb_frames = 0;
float g_elapsed_fps = 0;
int g_main_window_id;
int g_mouse_btn_gui_state = 0;
bool g_fullscreen = false;

//GUI
GUIScreenManager * g_screen_manager = NULL;
GUIBouton * BtnParams = NULL;
GUIBouton * BtnClose = NULL;
GUILabel * LabelFps = NULL;
GUIScreen * g_screen_params = NULL;
GUIScreen * g_screen_jeu = NULL;
GUISlider * g_slider_zoom_rendu_time;
GUISlider * g_slider_zoom_rendu_amplitude;

//Params 
GUISlider * g_slider_freq_sin;
GUISlider * g_slider_freq_noise;
GUISlider * g_slider_reverb;
GUISlider * g_slider_lp;
GUISlider * g_slider_gain_final;

//Params Grains
GUISlider * g_slider_random;
GUISlider * g_slider_pos;
GUISlider * g_slider_size;
GUISlider * g_slider_overlap;
GUISlider * g_slider_crossfade;

//Sons
SoundEngine * g_sound_engine = NULL;
SoundBasic * g_sound_btn;

//Pointeurs utiles sur les son en cours
std::vector<ContinuousSound*> g_sounds_continuous;
ContinuousSound * g_sound_continuous;
ContinuousSound * g_showed_sound = NULL;

//Son utilises
ContinuousSound * g_sound_white;
NoiseSound * g_sound_pink;
SinusSound * g_sound_sinus;
SoundGrain * g_sound_grain;

//Filtres
FilterReverb * g_filter_reverb;
FilterLP * g_filter_lp;

//fft
BasicFFT * g_fft;

//////////////////////////////////////////////////////////////////////////
// GESTION APPLICATION
//////////////////////////////////////////////////////////////////////////
void runApplication(float elapsed)
{
	static float g_eval_elapsed = 0;

	//Calcul des fps
	g_elapsed_fps += elapsed;
	g_nb_frames++;
	if(g_elapsed_fps > 1.0)
	{
		LabelFps->Text = std::string("FPS : ") + toString(g_nb_frames);
		g_elapsed_fps -= 1.0f;
		g_nb_frames = 0;
	}

	//Update son
	g_sound_engine->update(elapsed);
}

void render2D(void)
{

	if(g_showed_sound != NULL)
	{	
		//On recup le buffer de synthèse et son format
		int tailleBuffer = 0;
		int tailleEchantillon = 0;
		int nbPistes = 0;
		void * pt_read_card;
		void const * buffer = g_showed_sound->getBufferSynthese(&tailleBuffer, &tailleEchantillon, &nbPistes,&pt_read_card);


		//Rendu du buffer de synthèse, que pour du 16bits mono
		if(tailleEchantillon == 2 && nbPistes == 1)
		{
			glColor3f(1.0f,0,0);
			float zoomx = g_slider_zoom_rendu_time->Value * 30.0f + 1.0F;
			float zoomy = g_slider_zoom_rendu_amplitude->Value * 30.0f + 1.0F;
			sint16 * ptRead = (sint16*)(buffer);
			int nbEch = tailleBuffer / tailleEchantillon;
			nbEch = nbEch/2 * 2;
			float yMax = g_renderer->_ScreenHeight;
			float yBase = g_renderer->_ScreenHeight / 2;
			float xMax = g_renderer->_ScreenWidth;
			float deltaX = (xMax / nbEch) * zoomx;
			glBegin(GL_LINE_STRIP);
			for(int i=0;i<nbEch;i++)
			{
				float value = ((*ptRead) * zoomy) / (65535.0f/2.0f);
				glVertex3f(i*deltaX,yBase - (yBase*value),1.0f);
				ptRead++;
			}
			glEnd();

			float power = g_showed_sound->getPower();
			glColor3f(0.0f,1.0f,0);
			glBegin(GL_LINES);
			glVertex3f(0,yBase - (yBase*power),1.0f);
			glVertex3f(xMax,yBase - (yBase*power),1.0f);
			glEnd();

			float powerMax = g_showed_sound->getPowerMax();
			glColor3f(0.0f,0.0f,1.0f);
			glBegin(GL_LINES);
			glVertex3f(0,yBase - (yBase*powerMax),1.0f);
			glVertex3f(xMax,yBase - (yBase*powerMax),1.0f);
			glEnd();

			//FFT
			sint16* s16_pt_read_card = (sint16*)pt_read_card;
			sint16* s16_buffer = (sint16*)buffer;
			int N = g_fft->getNb();
			if((s16_pt_read_card-s16_buffer) < (tailleBuffer / tailleEchantillon)-N)	
				g_fft->updateInputSint16(s16_pt_read_card,N);

			//if(*s16_pt_read_card == 0 && *(s16_pt_read_card+1) == 0)
				//Log::log(Log::ENGINE_ERROR,"Bad sample for fft");

			float const * vals_fft = g_fft->getValues();

			glColor3f(1.0f,0.0f,1.0f);
			int width = 1;
			glBegin(GL_QUADS);
			for(int j=0;j<N/2;j+=2)
			{
				float r = vals_fft[j];
				float i = vals_fft[j+1];
				float mag = sqrt((r * r) + (i * i));
				glVertex3f((j/2)*width + xMax - N/4*width - 10,yMax,1.0f);
				glVertex3f((j/2+1)*width + xMax - N/4*width - 10,yMax,1.0f);
				glVertex3f((j/2+1)*width + xMax - N/4*width - 10,yMax - mag * 10,1.0f);
				glVertex3f((j/2)*width + xMax - N/4*width - 10,yMax - mag * 10,1.0f);
			}
			glEnd();
		}
	}
	
	g_screen_manager->render();
}

void renderObjectsDepthOnly(void)
{

}

void renderObjects(void)
{
	
}

void renderFunction(void)
{
	float elapsed = g_timer->getElapsedSeconds(true);
	runApplication(elapsed);
	g_renderer->render(elapsed);
}

void resizeFunction(int width, int height)
{
	glViewport(0, 0, width, height);
	g_renderer->resize(width,height);
}

//////////////////////////////////////////////////////////////////////////
// GESTION CLAVIER SOURIS
//////////////////////////////////////////////////////////////////////////

void specialDownFunction(int key, int p1, int p2)
{
	if(key == GLUT_KEY_F5)
	{
	
	}
}

void specialUpFunction(int key, int p1, int p2)
{

}

void keyboardDownFunction(unsigned char key, int p1, int p2)
{
	if(key == VK_ESCAPE)
	{
		glutDestroyWindow(g_main_window_id);	
		exit(0);
	}

	if(key == 'f')
	{
		if(!g_fullscreen){
			glutFullScreen();
			g_fullscreen = true;
		} else if(g_fullscreen){
			glutLeaveGameMode();
			glutLeaveFullScreen();
			glutReshapeWindow(g_renderer->_ScreenWidth, g_renderer->_ScreenWidth);
			glutPositionWindow(0,0);
			g_fullscreen = false;
		}
	}
}

void keyboardUpFunction(unsigned char key, int p1, int p2)
{
	
}

void mouseWheelFunction(int wheel, int dir, int x, int y)
{
	
}

void mouseFunction(int button, int state, int x, int y)
{
	//GUI
	g_mouse_btn_gui_state = 0;
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
		g_mouse_btn_gui_state |= GUI_MLBUTTON;
	
	bool mouseTraite = false;
	mouseTraite = g_screen_manager->mouseCallback(x,y,g_mouse_btn_gui_state,0,0);

	if(mouseTraite && state == GLUT_DOWN)
		g_sound_btn->play();
	if(mouseTraite)
	{
		g_sound_grain->setGrainParam((float) g_slider_pos->Value,(float) g_slider_size->Value,(float) g_slider_random->Value, (float) g_slider_overlap->Value, (float)g_slider_crossfade->Value);
		g_sound_sinus->setFreq(g_slider_freq_sin->Value,5.0f);
		g_sound_pink->setFreq(g_slider_freq_noise->Value);
		g_filter_reverb->setReverb(g_slider_reverb->Value);
		g_filter_lp->setAlpha(g_slider_lp->Value);
		if(g_sound_continuous)
			g_sound_continuous->setGainFinal(g_slider_gain_final->Value);
		
	}
}

void mouseMoveFunction(int x, int y, bool pressed)
{
	bool mouseTraite = false;

	mouseTraite = g_screen_manager->mouseCallback(x,y,g_mouse_btn_gui_state,0,0);
	if(pressed && mouseTraite)
	{
		g_sound_grain->setGrainParam((float) g_slider_pos->Value, (float) g_slider_size->Value, (float) g_slider_random->Value, (float) g_slider_overlap->Value, (float)g_slider_crossfade->Value);
		g_sound_sinus->setFreq(g_slider_freq_sin->Value,5.0f);
		g_sound_pink->setFreq(g_slider_freq_noise->Value);
		g_filter_reverb->setReverb(g_slider_reverb->Value);
		g_filter_lp->setAlpha(g_slider_lp->Value);
		if(g_sound_continuous)
			g_sound_continuous->setGainFinal(g_slider_gain_final->Value);
	}
}

void mouseMoveActiveFunction(int x, int y)
{
	mouseMoveFunction(x,y,true);
}
void mouseMovePassiveFunction(int x, int y)
{
	mouseMoveFunction(x,y,false);
}

void joystickFunction(unsigned int buttonmask,int x, int y, int z)
{
	_cprintf("%x %d %d %d\n",buttonmask,x,y,z);
}

void clickBtnParams (GUIBouton * bouton)
{
	g_screen_manager->setActiveScreen(g_screen_params);
}

void clickBtnCloseParam (GUIBouton * bouton)
{
	g_screen_manager->setActiveScreen(g_screen_jeu);
}


//UI SON
void resetContSounds()
{
	g_filter_reverb->init();
	g_filter_lp->init();

	for(int i=0;i<g_sounds_continuous.size();i++)
	{
		g_sounds_continuous[i]->stop();
		g_sounds_continuous[i]->removeFilters();
	}
}

void clickWhiteNoise(GUIBouton * bouton)
{
	resetContSounds();
	g_sound_continuous = g_sound_white;
	g_showed_sound = g_sound_white;
	if(g_sound_white->isPlaying())
	{
		g_sound_white->stop();
	}
	else
	{
		g_sound_white->play();
	}

}

void clickPinkNoise(GUIBouton * bouton)
{
	resetContSounds();
	g_sound_continuous = g_sound_pink;
	g_showed_sound = g_sound_pink;
	if(g_sound_pink->isPlaying())
	{
		g_sound_pink->stop();
	}
	else
	{
		g_sound_pink->play();
	}

}


void clickSinus(GUIBouton * bouton)
{
	resetContSounds();
	g_sound_continuous = g_sound_sinus;
	g_showed_sound = g_sound_sinus;
	if(g_sound_sinus->isPlaying())
	{
		g_sound_sinus->stop();
	}
	else
	{
		g_sound_sinus->play();
	}
}

void clickGrain (GUIBouton * bouton)
{
	resetContSounds();
	g_sound_continuous = g_sound_grain;
	g_showed_sound = g_sound_grain;
	if(g_sound_grain->isPlaying())
	{
		g_sound_grain->stop();
	}
	else
	{
		g_sound_grain->play();
	}
	
}

void clickGenerator(GUIBouton * bouton)
{
	if(g_sound_continuous)
	{
		if(g_sound_continuous->isGeneratorActive())
			g_sound_continuous->activateGenerator(false);
		else
			g_sound_continuous->activateGenerator(true);
	}
}

void clickReverb(GUIBouton * bouton)
{
	if(g_sound_continuous)
	{
		g_filter_reverb->init();
		g_sound_continuous->addFilter(g_filter_reverb);
			
	}
}

void clickLowPass(GUIBouton * bouton)
{
	if(g_sound_continuous)
	{
		g_filter_lp->init();
		g_sound_continuous->addFilter(g_filter_lp);

	}
}

/**
  * POINT D'ENTREE PRINCIPAL
  **/
int main(int argc, char* argv[])
{ 
	int screen_width = 800;
	int screen_height = 600;

	Log::addLog(new LogConsole());

	glutInit(&argc, argv); 
	glutInitContextVersion(3,0);
	glutSetOption(
		GLUT_ACTION_ON_WINDOW_CLOSE,
		GLUT_ACTION_GLUTMAINLOOP_RETURNS
		);

	glutInitWindowSize(screen_width,screen_height);
	glutInitWindowPosition (0, 0);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE );

	glEnable(GL_MULTISAMPLE);

	bool gameMode = true;
	for(int i=0;i<argc;i++)
	{
		if(argv[i][0] == 'w')
		{
			Log::log(Log::ENGINE_INFO,"arg w asking window mode");
			gameMode = false;
		}
	}

	/*if(gameMode)
	{
		//glutGameModeGet	(	GLenum info );
		int width = glutGet(GLUT_SCREEN_WIDTH);
		int height = glutGet(GLUT_SCREEN_HEIGHT);
		//int color = glutGet();
		
		char gameModeStr[200];
		sprintf(gameModeStr,"%dx%d:32@60",width,height);
		glutGameModeString(gameModeStr);
		g_main_window_id = glutEnterGameMode();
	}
	else*/
	{
		g_main_window_id = glutCreateWindow("Synthese Granulaire");
		glutReshapeWindow(screen_width,screen_height);
	}

	if(g_main_window_id < 1) 
	{
		Log::log(Log::ENGINE_ERROR,"Unable to create main window");
		exit(EXIT_FAILURE);
	}

	GLenum glewInitResult = glewInit();

	if (glewInitResult != GLEW_OK)
	{
		Log::log(Log::ENGINE_ERROR,("Unable to init glew "+toString(glewGetErrorString(glewInitResult))).c_str());
		exit(EXIT_FAILURE);
	}
	
	glutDisplayFunc(renderFunction);
	glutReshapeFunc(resizeFunction);
	glutKeyboardFunc(keyboardDownFunction);
	glutKeyboardUpFunc(keyboardUpFunction);
	glutSpecialFunc(specialDownFunction);
	glutSpecialUpFunc(specialUpFunction);
	glutMouseFunc(mouseFunction);
	glutMotionFunc(mouseMoveActiveFunction);
	glutPassiveMotionFunc(mouseMovePassiveFunction);
	//glutJoystickFunc(joystickFunction,10);
	glutIgnoreKeyRepeat(1);

	//Initialisation du renderer
	g_renderer = NYRenderer::getInstance();
	g_renderer->setRenderObjectFun(renderObjects);
	g_renderer->setRenderObjectDepthOnlyFun(renderObjectsDepthOnly);
	g_renderer->setRender2DFun(render2D);
	g_renderer->initialise();

	//On applique la config du renderer
	glViewport(0, 0, g_renderer->_ScreenWidth, g_renderer->_ScreenHeight);
	g_renderer->resize(g_renderer->_ScreenWidth,g_renderer->_ScreenHeight);

	
	
	//Ecran de jeu
	uint16 x = 10;
	uint16 y = 10;
	g_screen_jeu = new GUIScreen(); 

	g_screen_manager = new GUIScreenManager();
		
	//Bouton pour afficher les params
	BtnParams = new GUIBouton();
	BtnParams->Titre = std::string("Params");
	BtnParams->X = x;
	BtnParams->setOnClick(clickBtnParams);
	g_screen_jeu->addElement(BtnParams);

	y += BtnParams->Height + 1;

	LabelFps = new GUILabel();
	LabelFps->Text = "FPS";
	LabelFps->X = x;
	LabelFps->Y = y;
	LabelFps->Visible = true;
	g_screen_jeu->addElement(LabelFps);

	y += LabelFps->Height + 1;

	GUILabel * label = new GUILabel();
	label->Text = "Zoom x";
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_jeu->addElement(label);

	x += label->Width;
	
	g_slider_zoom_rendu_time  = new GUISlider();
	g_slider_zoom_rendu_time->setPos(x,y);
	g_slider_zoom_rendu_time->setMaxMin(1.0,0.0);
	g_slider_zoom_rendu_time->Visible = true;
	g_screen_jeu->addElement(g_slider_zoom_rendu_time);

	y += g_slider_zoom_rendu_time->Height + 1;
	x = 10;

	label = new GUILabel();
	label->Text = "Zoom y";
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_jeu->addElement(label);

	x += label->Width;

	g_slider_zoom_rendu_amplitude  = new GUISlider();
	g_slider_zoom_rendu_amplitude->setPos(x,y);
	g_slider_zoom_rendu_amplitude->setMaxMin(1.0,0.0);
	g_slider_zoom_rendu_amplitude->Visible = true;
	g_screen_jeu->addElement(g_slider_zoom_rendu_amplitude);

	y += g_slider_zoom_rendu_amplitude->Height + 1;
	x = 10;

	//Ecran de parametrage
	x = 10;
	y = 10;
	g_screen_params = new GUIScreen();

	GUIBouton * btn = NULL;

	btn = new GUIBouton();
	btn->Titre = std::string("Close");
	btn->X = x;
	btn->setOnClick(clickBtnCloseParam);
	g_screen_params->addElement(btn);

	y += btn->Height + 1;
	y+=10;
	x+=10;

	btn = new GUIBouton();
	btn->Titre = std::string("White Noise");
	btn->X = x;
	btn->Y = y;
	btn->setOnClick(clickWhiteNoise);
	g_screen_params->addElement(btn);

	x += btn->Width + 10;

	btn = new GUIBouton();
	btn->Titre = std::string("Pink Noise");
	btn->X = x;
	btn->Y = y;
	btn->setOnClick(clickPinkNoise);
	g_screen_params->addElement(btn);

	x += btn->Width + 10;

	btn = new GUIBouton();
	btn->Titre = std::string("Sin");
	btn->X = x;
	btn->Y = y;
	btn->setOnClick(clickSinus);
	g_screen_params->addElement(btn);

	x += btn->Width + 10;

	btn = new GUIBouton();
	btn->Titre = std::string("Grain");
	btn->X = x;
	btn->Y = y;
	btn->setOnClick(clickGrain);
	g_screen_params->addElement(btn);

	x = 20;
	y += btn->Height + 10;

	btn = new GUIBouton();
	btn->Titre = std::string("Generator");
	btn->X = x;
	btn->Y = y;
	btn->setOnClick(clickGenerator);
	g_screen_params->addElement(btn);

	y += btn->Height + 10;

	btn = new GUIBouton();
	btn->Titre = std::string("Add Reverb");
	btn->X = x;
	btn->Y = y;
	btn->setOnClick(clickReverb);
	g_screen_params->addElement(btn);

	x += btn->Width + 10;

	btn = new GUIBouton();
	btn->Titre = std::string("Add LowPass");
	btn->X = x;
	btn->Y = y;
	btn->setOnClick(clickLowPass);
	g_screen_params->addElement(btn);

	x = 20;
	y += btn->Height + 10;

	//Largeur des prochains labels
	int widthLab = 270;

	//FREQ SINUS 
	label = new GUILabel();
	label->Text = "Freq Sin (10,4000):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_freq_sin = new GUISlider();
	g_slider_freq_sin->setPos(x,y);
	g_slider_freq_sin->setMaxMin(4000,10);
	g_slider_freq_sin->setValue(440);
	g_slider_freq_sin->Visible = true;
	g_screen_params->addElement(g_slider_freq_sin);

	x = 20;
	y += label->Height + 10;

	//FREQ NOISE
	label = new GUILabel();
	label->Text = "Freq Noise (10,4000):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_freq_noise = new GUISlider();
	g_slider_freq_noise->setPos(x,y);
	g_slider_freq_noise->setMaxMin(4000,10);
	g_slider_freq_noise->setValue(440);
	g_slider_freq_noise->Visible = true;
	g_screen_params->addElement(g_slider_freq_noise);

	x = 20;
	y += label->Height + 10;

	y+=10;

	//POS GRAIN
	label = new GUILabel();
	label->Text = "Pos grain (0,1):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_pos = new GUISlider();
	g_slider_pos->setPos(x,y);
	g_slider_pos->setMaxMin(1,0);
	g_slider_pos->Visible = true;
	g_screen_params->addElement(g_slider_pos);

	x = 20;
	y += label->Height + 10;

	//DELTA POS RND
	label = new GUILabel();
	label->Text = "Delta Pos Rnd Size (0,1):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_random = new GUISlider();
	g_slider_random->setPos(x,y);
	g_slider_random->setMaxMin(1.0,0);
	g_slider_random->Visible = true;
	g_screen_params->addElement(g_slider_random);

	x = 20;
	y += label->Height + 10;

	//SIZE GRAIN
	label = new GUILabel();
	label->Text = "Size Grain (0,1):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_size = new GUISlider();
	g_slider_size->setPos(x,y);
	g_slider_size->setMaxMin(1.0,0.01);
	g_slider_size->Visible = true;
	g_screen_params->addElement(g_slider_size);

	x = 20;
	y += label->Height + 10;

	//OVERLAP GRAIN
	label = new GUILabel();
	label->Text = "Overlap Grain (0,1):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_overlap = new GUISlider();
	g_slider_overlap->setPos(x,y);
	g_slider_overlap->setMaxMin(0.95,0.0);
	g_slider_overlap->Visible = true;
	g_screen_params->addElement(g_slider_overlap);

	x = 20;
	y += label->Height + 10;

	// CROSS FADE GRAINS
	label = new GUILabel();
	label->Text = "Lin/EqPow CrossFade Grain (0,1):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_crossfade = new GUISlider();
	g_slider_crossfade->setPos(x,y);
	g_slider_crossfade->setMaxMin(1.0,0);
	g_slider_crossfade->Visible = true;
	g_screen_params->addElement(g_slider_crossfade);

	x = 20;
	y += label->Height + 10;

	y+=10;

	//GAIN FINAL
	label = new GUILabel();
	label->Text = "Reverb (0,1):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_reverb = new GUISlider();
	g_slider_reverb->setPos(x,y);
	g_slider_reverb->setMaxMin(1,0);
	g_slider_reverb->setValue(1.0);
	g_slider_reverb->Visible = true;
	g_screen_params->addElement(g_slider_reverb);

	x = 20;
	y += label->Height + 10;

	//GAIN FINAL
	label = new GUILabel();
	label->Text = "Low Pass (0,1):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_lp = new GUISlider();
	g_slider_lp->setPos(x,y);
	g_slider_lp->setMaxMin(1,0);
	g_slider_lp->setValue(1.0);
	g_slider_lp->Visible = true;
	g_screen_params->addElement(g_slider_lp);

	x = 20;
	y += label->Height + 10;

	y+=10;

	//GAIN FINAL
	label = new GUILabel();
	label->Text = "Gain Final (0,3):";
	label->Width = widthLab;
	label->X = x;
	label->Y = y;
	label->Visible = true;
	g_screen_params->addElement(label);

	x += label->Width;

	g_slider_gain_final = new GUISlider();
	g_slider_gain_final->setPos(x,y);
	g_slider_gain_final->setMaxMin(10,0);
	g_slider_gain_final->setValue(1.0);
	g_slider_gain_final->Visible = true;
	g_screen_params->addElement(g_slider_gain_final);

	x = 20;
	y += label->Height + 10;

	//Ecran a rendre
	g_screen_manager->setActiveScreen(g_screen_jeu);

	//On charge les sons
	g_sound_engine = SoundEngine::getInstance();

	g_sound_btn =  new SoundBasic(); 
	g_sound_btn->load("sound/snd_btn_1.wav");
	g_sound_engine->addSound(g_sound_btn);

	
	g_sound_continuous = NULL;
	
	g_sound_white = new ContinuousSound();
	g_sound_engine->addSound(g_sound_white);
	g_sounds_continuous.push_back(g_sound_white);
	
	g_sound_pink = new NoiseSound();
	g_sound_engine->addSound(g_sound_pink);	
	g_sounds_continuous.push_back(g_sound_pink);
	
	g_sound_sinus = new SinusSound();
	g_sound_engine->addSound(g_sound_sinus);	
	g_sounds_continuous.push_back(g_sound_sinus);
	
	g_sound_grain = new SoundGrain();
	g_sound_grain->loadBaseFile("sound/white.wav");
	g_sound_grain->_Loop = true;
	g_sound_grain->setGrainParam(0.5f,0.05f,0.05f,0.7f,0.5f);
	g_sound_engine->addSound(g_sound_grain);
	g_sounds_continuous.push_back(g_sound_grain);

	g_filter_reverb = new FilterReverb();
	g_filter_lp = new FilterLP();
	
	/*g_slider_pos->setValue(0.5);
	g_slider_size->setValue(0.05);
	g_slider_random->setValue(0.05);
	g_slider_overlap->setValue(0.7);
	g_slider_crossfade->setValue(0.5);*/

	g_slider_pos->setValue(0.5);
	g_slider_size->setValue(0.5);
	g_slider_random->setValue(0.0);
	g_slider_overlap->setValue(0.5);
	g_slider_crossfade->setValue(0.5);
	
	//FFT
	g_fft = new BasicFFT();
	g_fft->init(2048);
	
	//Init Timer
	g_timer = new NYTimer();
	
	//On start
	g_timer->start();



	glutMainLoop(); 

	return 0;
}

