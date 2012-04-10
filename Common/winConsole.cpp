//-----------------------------------------------------------------------------
// Torque Game Engine Advanced
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "winConsole.h"
#include "Lock.h"
#include <assert.h>

static WinConsole* s_pWinConsole = NULL;
WinConsole* WinConsole::GetInstance()
{
	assert(s_pWinConsole != NULL);
	return s_pWinConsole;
}

WinConsole::WinConsole(const char* title, ExecFunc execFunc)
:m_execFunc(execFunc), iCmdIndex(0), inpos(0), completeStart(0), command(false)
{
	InitializeCriticalSection(&m_criticalSection);

	for (int iIndex = 0; iIndex < MAX_CMDS; iIndex++)
		rgCmds[iIndex][0] = '\0';
	curTabComplete[0] = '\0';

	AllocConsole();
	SetConsoleTitleA(title);
	stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	stdIn  = GetStdHandle(STD_INPUT_HANDLE);
	stdErr = GetStdHandle(STD_ERROR_HANDLE);

	_printf("%s", CONSOLE_PROMPT);
	s_pWinConsole = this;

	GetConsoleScreenBufferInfo(stdOut, &csbi);
}

WinConsole::~WinConsole()
{
	if (s_pWinConsole == this)
		s_pWinConsole = NULL;
	DeleteCriticalSection(&m_criticalSection);
}

void WinConsole::_printf(const char *s, ...)
{
	// Get the line into a buffer.
	static const int BufSize = 4096;
	static char buffer[4096];
	DWORD bytes;
	va_list args;
	va_start(args, s);
	vsnprintf_s(buffer, BufSize, _TRUNCATE, s, args);
	// Replace tabs with carats, like the "real" console does.
	char* pos = buffer;
	while (*pos) {
		if (*pos == '\t') {
			*pos = '^';
		}
		pos++;
	}
	// Print it.
	WriteFile(stdOut, buffer, strlen(buffer), &bytes, NULL);
	FlushFileBuffers(stdOut);
}

void WinConsole::process()
{
	DWORD numEvents;
	GetNumberOfConsoleInputEvents(stdIn, &numEvents);
	if (numEvents)
	{
		INPUT_RECORD rec[20];
		char outbuf[512];
		int outpos = 0;

		ReadConsoleInput(stdIn, rec, 20, &numEvents);
		DWORD i;
		CLock lock(&m_criticalSection);
		for (i = 0; i < numEvents; i++)
		{
			if (rec[i].EventType == KEY_EVENT)
			{
				KEY_EVENT_RECORD* ke = &(rec[i].Event.KeyEvent);
				if (ke->bKeyDown)
				{
					switch (ke->uChar.AsciiChar)
					{
						// If no ASCII char, check if it's a handled virtual key
					case 0:
						switch (ke->wVirtualKeyCode)
						{
							// UP ARROW
						case 0x26:
							{
								// Go to the previous command in the cyclic array
								inbuf[inpos] = '\0';
								if (strcmp(inbuf, rgCmds[completeStart]) || inbuf[0] == '\0')
									completeStart = (iCmdIndex - 1 + MAX_CMDS) % MAX_CMDS;
								else
									completeStart = (completeStart - 1 + MAX_CMDS) % MAX_CMDS;

								int i = completeStart;
								do 
								{
									if (rgCmds[i][0] != '\0' && strcmp(inbuf, rgCmds[i]))
									{
										completeStart = i;
										break;
									}
									i = (i - 1 + MAX_CMDS) % MAX_CMDS;
								} while (i != completeStart);

								// If this command isn't empty ...
								if (rgCmds[completeStart][0] != '\0')
								{
									// Obliterate current displayed text
									for (i = outpos = 0; i < inpos; i++)
									{
										outbuf[outpos++] = '\b';
										outbuf[outpos++] = ' ';
										outbuf[outpos++] = '\b';
									}

									// Copy command into command and display buffers
									for (inpos = 0; inpos < (int)strlen(rgCmds[completeStart]); inpos++, outpos++)
									{
										outbuf[outpos] = rgCmds[completeStart][inpos];
										inbuf [inpos ] = rgCmds[completeStart][inpos];
									}
								}
							}
							break;

							// DOWN ARROW
						case 0x28: 
							{
								// Go to the next command in the command array, if
								// it isn't empty
								inbuf[inpos] = '\0';
								if (strcmp(inbuf, rgCmds[completeStart]) || inbuf[0] == '\0')
									completeStart = iCmdIndex;
								else
									completeStart = (completeStart + 1) % MAX_CMDS;

								int i = completeStart;
								do 
								{
									if (rgCmds[i][0] != '\0' && strcmp(inbuf, rgCmds[i]))
									{
										completeStart = i;
										break;
									}
									i = (i + 1) % MAX_CMDS;
								} while (i != completeStart);

								if (rgCmds[completeStart][0] != '\0')
								{
									// Obliterate current displayed text
									for (i = outpos = 0; i < inpos; i ++)
									{
										outbuf[outpos++] = '\b';
										outbuf[outpos++] = ' ';
										outbuf[outpos++] = '\b';
									}

									// Copy command into command and display buffers
									for (inpos = 0; inpos < (int)strlen(rgCmds[completeStart]); inpos++, outpos++)
									{
										outbuf[outpos] = rgCmds[completeStart][inpos];
										inbuf [inpos ] = rgCmds[completeStart][inpos];
									}
								}
							}
							break;

							// LEFT ARROW
						case 0x25:
							break;

							// RIGHT ARROW
						case 0x27:
							break;

						default:
							break;
						}
						break;
					case '\b':
						if (inpos > 0)
						{
							outbuf[outpos++] = '\b';
							outbuf[outpos++] = ' ';
							outbuf[outpos++] = '\b';
							inpos--;
						}
						break;
					case '\t':
						{
							int change = 1;
							if (ke->dwControlKeyState & SHIFT_PRESSED)
								change = -1;
							inbuf[inpos] = '\0';
							int i;
							if (strcmp(inbuf, rgCmds[completeStart]) || inbuf[0] == '\0')
							{
								for (i = 0; i < inpos; i++)
									curTabComplete[i] = inbuf[i];
								curTabComplete[i] = '\0';
								if (ke->dwControlKeyState & SHIFT_PRESSED)
									completeStart = (iCmdIndex - 1 + MAX_CMDS) % MAX_CMDS;
								else
									completeStart = iCmdIndex;
							}
							else
							{
								completeStart = (completeStart + change + MAX_CMDS) % MAX_CMDS;
							}
							// Modify the input buffer with the completion.
							int completeLen = (int)strlen(curTabComplete);
							i = completeStart;
							bool found = false;
							do 
							{
								if (rgCmds[i][0] != '\0' && strcmp(inbuf, rgCmds[i]))
								{
									int j;
									for (j = 0; j < completeLen && curTabComplete[j] == rgCmds[i][j]; j++);
									if (j == completeLen)
									{
										found = true;
										break;
									}
								}
								i = (i + change + MAX_CMDS) % MAX_CMDS;
							} while (i != completeStart);
							if (found)
							{
								completeStart = i;
								// Erase the current line.
								for (i = 0; i < inpos; i++) {
									outbuf[outpos++] = '\b';
									outbuf[outpos++] = ' ';
									outbuf[outpos++] = '\b';
								}

								// Copy command into command and display buffers
								for (inpos = 0; inpos < (int)strlen(rgCmds[completeStart]); inpos++, outpos++)
								{
									outbuf[outpos] = rgCmds[completeStart][inpos];
									inbuf [inpos ] = rgCmds[completeStart][inpos];
								}
							}
						}
						break;
					case '\n':
					case '\r':
						outbuf[outpos++] = '\r';
						outbuf[outpos++] = '\n';

						inbuf[inpos] = 0;
						outbuf[outpos] = 0;
						_printf("%s", outbuf);

						// Pass the line along to the console for execution.
						command = true;
						m_execFunc(inbuf);
						command = false;

						// Put the new command into the array
						strcpy_s(rgCmds[iCmdIndex], sizeof(rgCmds[0]), inbuf);

						// If we've gone off the end of our array, wrap
						// back to the beginning
						iCmdIndex = (iCmdIndex + 1) % MAX_CMDS;

						_printf("%s", CONSOLE_PROMPT);
						inpos = outpos = 0;
						break;
					default:
						inbuf[inpos++] = ke->uChar.AsciiChar;
						outbuf[outpos++] = ke->uChar.AsciiChar;
						break;
					}
				}
			}
		}
		if (outpos)
		{
			outbuf[outpos] = 0;
			_printf("%s", outbuf);
		}
	}
}

void WinConsole::printf(const char* fmt, ...)
{
	va_list argptr;
	va_start(argptr, fmt);
	char buffer[4096] = "";
	vsnprintf_s(PTAIL(buffer), _TRUNCATE, fmt, argptr);
	va_end(argptr);
	char outbuf[512];
	int outpos = 0;
	for (int i = 0; i < inpos+CONSOLE_PROMPT_SIZE; i++) 
	{
		outbuf[outpos++] = '\b';
		outbuf[outpos++] = ' ';
		outbuf[outpos++] = '\b';
	}
	outbuf[outpos] = 0;
	CLock lock(&m_criticalSection);
	_printf("%s%s%s\n", outbuf, OUTPUT_PROMPT, buffer);

	if (!command)
	{
		CONSOLE_SCREEN_BUFFER_INFO tcsbi;
		GetConsoleScreenBufferInfo(stdOut, &tcsbi);
		SetConsoleTextAttribute(stdOut, csbi.wAttributes);
		inbuf[inpos] = '\0';
		_printf("%s%s", CONSOLE_PROMPT, inbuf);
		SetConsoleTextAttribute(stdOut, tcsbi.wAttributes);
	}
}

ConsoleColor::ConsoleColor(WORD color)
{
	SetConsoleTextAttribute(CONSOLE->stdOut, color);
}

ConsoleColor::~ConsoleColor()
{
	SetConsoleTextAttribute(CONSOLE->stdOut, CONSOLE->csbi.wAttributes);
}