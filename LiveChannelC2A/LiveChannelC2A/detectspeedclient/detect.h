#pragma once
#if defined(MOHO_X86)
void detectthread(union sigval v);
#elif defined(MOHO_WIN32)
VOID CALLBACK detectthread(  //all consumer tasks use only one timer, maybe???
						   HWND hwnd,        // handle to window for timer messages
						   UINT message,     // WM_TIMER message
						   UINT idTimer,     // timer identifier
						   DWORD dwTime)  ;   // current system time
#endif
