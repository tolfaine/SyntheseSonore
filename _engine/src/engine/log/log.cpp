#include "engine/log/log.h"

bool Log::_HideEngineLog = false;
unsigned long Log::_LastMessageTime =  GetTickCount();
vector<Log*> Log::_Logs;
