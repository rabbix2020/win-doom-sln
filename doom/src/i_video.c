// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>

#include <stdarg.h>

#include <signal.h>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"

#include "doomdef.h"

#define POINTER_WARP_COUNTDOWN	1

HWND		X_mainWindow;
HDC			X_mainDC;
MSG			DoomMessage;
BITMAPINFO* bmi;

DEVMODEA X_DevMode = { 0 };

int		X_width;
int		X_height;
Dboolean in_focus = true;

// Fake mouse handling.
// This cannot work properly w/o DGA.
// Needs an invisible mouse cursor at least.
Dboolean		grabMouse;
int		doPointerWarp = POINTER_WARP_COUNTDOWN;

Dboolean fullscreen = false;

void I_SetDevmode(DEVMODEA* Dmode, int flags) {
	if (fullscreen) {
		int err_msg = ChangeDisplaySettingsA(Dmode, flags);
		if (err_msg)
			I_Error("I_SetDevmode: cannot change display settings: %d", err_msg);
	}
}

int xlatekey(WPARAM wparam)
{

	int rc;

	switch (wparam)
	{
	case VK_LEFT:	rc = KEY_LEFTARROW;	break;
	case VK_RIGHT:	rc = KEY_RIGHTARROW;	break;
	case VK_DOWN:	rc = KEY_DOWNARROW;	break;
	case VK_UP:	rc = KEY_UPARROW;	break;
	case VK_ESCAPE:	rc = KEY_ESCAPE;	break;
	case VK_RETURN:	rc = KEY_ENTER;		break;
	case VK_TAB:	rc = KEY_TAB;		break;
	case VK_F1:	rc = KEY_F1;		break;
	case VK_F2:	rc = KEY_F2;		break;
	case VK_F3:	rc = KEY_F3;		break;
	case VK_F4:	rc = KEY_F4;		break;
	case VK_F5:	rc = KEY_F5;		break;
	case VK_F6:	rc = KEY_F6;		break;
	case VK_F7:	rc = KEY_F7;		break;
	case VK_F8:	rc = KEY_F8;		break;
	case VK_F9:	rc = KEY_F9;		break;
	case VK_F10:	rc = KEY_F10;		break;
	case VK_F11:	rc = KEY_F11;		break;
	case VK_F12:	rc = KEY_F12;		break;

	case VK_BACK:
	case VK_DELETE:	rc = KEY_BACKSPACE;	break;

	case VK_PAUSE:	rc = KEY_PAUSE;		break;

	case VK_OEM_PLUS:	rc = KEY_EQUALS;	break;

	case VK_OEM_MINUS:	rc = KEY_MINUS;		break;

	case VK_SHIFT:
		rc = KEY_RSHIFT;
		break;

	case VK_CONTROL:
		rc = KEY_RCTRL;
		break;

	case VK_MENU:
	case VK_LWIN:
	case VK_RWIN:
		rc = KEY_RALT;
		break;

	case VK_OEM_COMMA:
		rc = ',';
		break;

	case VK_OEM_PERIOD:
		rc = '.';
		break;

	default:
		rc = wparam;
		if (rc >= 'A' && rc <= 'Z')
			rc = rc - 'A' + 'a';
		break;
	}

	return rc;

}

void I_ShutdownGraphics(void)
{
	// Destroying window
	free(bmi);
	ReleaseDC(X_mainWindow, X_mainDC);

	ShowCursor(TRUE);
	DestroyWindow(X_mainWindow);
	if (fullscreen) { I_SetDevmode(NULL, 0); }
}



//
// I_StartFrame
//
void I_StartFrame(void)
{
	// er?
}

static int	lastmousex = 0;
static int	lastmousey = 0;
Dboolean		mousemoved = false;

//
// I_StartTic
//
void I_StartTic(void)
{

	if (!X_mainWindow)
		return;

	while (PeekMessageA(&DoomMessage, X_mainWindow, 0, 0, PM_REMOVE)) {
		TranslateMessage(&DoomMessage);
		DispatchMessageA(&DoomMessage);
	}

	if (grabMouse && !menuactive && !demoplayback && in_focus) {

		if (!--doPointerWarp)
		{
			RECT temp_rect;
			GetClientRect(X_mainWindow, &temp_rect);
			ClientToScreen(X_mainWindow, &temp_rect);

			SetCursorPos(temp_rect.left+(X_width >> 1), temp_rect.top+(X_height >> 1));

			lastmousex = X_width >> 1;
			lastmousey = X_height >> 1;

			doPointerWarp = POINTER_WARP_COUNTDOWN;
		}
	}
	mousemoved = false;
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit(void)
{
	// what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate(void)
{

	static int	lasttic;
	int		tics;
	int		i;
	// UNUSED static unsigned char *bigscreen=0;

	// draws little dots on the bottom of the screen
	if (devparm)
	{

		i = I_GetTime();
		tics = i - lasttic;
		lasttic = i;
		if (tics > 20) tics = 20;

		for (i = 0; i < tics * 2; i += 2)
			screens[0][(SCREENHEIGHT - 1) * SCREENWIDTH + i] = 0xff;
		for (; i < 20 * 2; i += 2)
			screens[0][(SCREENHEIGHT - 1) * SCREENWIDTH + i] = 0x0;

	}

	StretchDIBits(
		X_mainDC,
		0,
		0,
		X_width,
		X_height,
		0,
		0,
		SCREENWIDTH,
		SCREENHEIGHT,
		screens[0],
		bmi,
		DIB_RGB_COLORS,
		SRCCOPY
	);

}


//
// I_ReadScreen
//
void I_ReadScreen(byte* scr)
{
	memcpy(scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}


//
// Palette stuff.
//

void UploadNewPalette(byte* palette)
{

	register int	i;
	register int	c;
	static Dboolean	firstcall = true;

	// initialize the colormap
	if (firstcall)
	{
		firstcall = false;
		bmi = malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi->bmiHeader.biWidth = SCREENWIDTH;
		bmi->bmiHeader.biHeight = -SCREENHEIGHT;
		bmi->bmiHeader.biPlanes = 1;
		bmi->bmiHeader.biBitCount = 8;
		bmi->bmiHeader.biCompression = BI_RGB;
		bmi->bmiHeader.biSizeImage = 0;
	}

	// set the Windows colormap entries
	for (i = 0; i < 256; i++)
	{
		c = gammatable[usegamma][*palette++];
		bmi->bmiColors[i].rgbRed = (c << 8) + c;
		c = gammatable[usegamma][*palette++];
		bmi->bmiColors[i].rgbGreen = (c << 8) + c;
		c = gammatable[usegamma][*palette++];
		bmi->bmiColors[i].rgbBlue = (c << 8) + c;
		bmi->bmiColors[i].rgbReserved = 0;
	}
}

//
// I_SetPalette
//
void I_SetPalette(byte* palette)
{
	UploadNewPalette(palette);
}


LRESULT CALLBACK DoomWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	POINT xmotion;
	event_t event;

	int buttons = 0;
	
	switch (msg)
	{
	case WM_CLOSE:
		I_Quit();
		break;

	case WM_KILLFOCUS:
		I_SetDevmode(NULL, 0);
		if (fullscreen) { ShowWindow(hwnd, SW_MINIMIZE); }
		in_focus = false;
		break;

	case WM_ACTIVATE:
		I_SetDevmode(&X_DevMode, CDS_FULLSCREEN);
		in_focus = true;
		break;

		// key presses
	case WM_KEYDOWN:
		event.type = ev_keydown;
		event.data1 = xlatekey(wparam);
		D_PostEvent(&event);
		// fprintf(stderr, "k");
		break;
	case WM_KEYUP:
		event.type = ev_keyup;
		event.data1 = xlatekey(wparam);
		D_PostEvent(&event);
		// fprintf(stderr, "ku");
		break;

	case WM_SYSKEYDOWN:
		event.type = ev_keydown;
		event.data1 = xlatekey(wparam);
		D_PostEvent(&event);
		// fprintf(stderr, "k");
		return DefWindowProcA(hwnd, msg, wparam, lparam); // for alt+f4 handling
	case WM_SYSKEYUP:
		event.type = ev_keyup;
		event.data1 = xlatekey(wparam);
		D_PostEvent(&event);
		// fprintf(stderr, "ku");
		return DefWindowProcA(hwnd, msg, wparam, lparam); // for alt+f4 handling


	// Button press check
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN: {

		if (wparam & MK_LBUTTON)
			buttons |= 1;

		if (wparam & MK_RBUTTON)
			buttons |= 2;

		if (wparam & MK_MBUTTON)
			buttons |= 4;

		event.type = ev_mouse;
		event.data1 = buttons;
		event.data2 = event.data3 = 0;
		D_PostEvent(&event);
		break;
	}

		// mouse move
	case WM_MOUSEMOVE: {

		if (wparam & MK_LBUTTON)
			buttons |= 1;

		if (wparam & MK_RBUTTON)
			buttons |= 2;

		if (wparam & MK_MBUTTON)
			buttons |= 4;

		GetCursorPos(&xmotion);
		ScreenToClient(hwnd, &xmotion);
		event.type = ev_mouse;
		event.data1 = buttons;
		event.data2 = (xmotion.x - lastmousex) << 2;
		event.data3 = (lastmousey - xmotion.y) << 2;

		if (event.data2 || event.data3)
		{
			lastmousex = xmotion.x;
			lastmousey = xmotion.y;

			if (xmotion.x != X_width / 2 &&
				xmotion.y != X_height / 2)
			{
				D_PostEvent(&event);
				// fprintf(stderr, "m");
				mousemoved = false;
			}
			else
			{
				mousemoved = true;
			}
		}
		break;
	}
	default:
		return DefWindowProcA(hwnd, msg, wparam, lparam);
	}
}

void I_InitGraphics(void)
{

	char* displayname;
	char* d;
	int			n;
	int			pnum;
	int			x = 0;
	int			y = 0;

	// warning: char format, different type arg
	char		xsign = ' ';
	char		ysign = ' ';

	int			oktodraw;
	unsigned long	attribmask;
	int			valuemask;
	static int		firsttime = 1;

	if (!firsttime)
		return;
	firsttime = 0;

	WNDCLASSA wc = { 0 };
	wc.lpfnWndProc = DoomWndProc;
	wc.hInstance = global_hInstance;
	wc.lpszClassName = "DoomWindow";

	RegisterClassA(&wc);


	signal(SIGINT, (void (*)(int)) I_Quit);

	// check if the user wants to grab the mouse (quite unnice)
	grabMouse = !!M_CheckParm("-grabmouse");

	// check for command-line geometry
	if ((pnum = M_CheckParm("-geom"))) // suggest parentheses around assignment
	{
		// warning: char format, different type arg 3,5
		n = sscanf(myargv[pnum + 1], "%c%d%c%d", &xsign, &x, &ysign, &y);

		if (n == 2)
			x = y = 0;
		else if (n == 6)
		{
			if (xsign == '-')
				x = -x;
			if (ysign == '-')
				y = -y;
		}
		else
			I_Error("bad -geom parameter");
	}

	// check if the user wants to full screen (quite unnice)
	fullscreen = !!M_CheckParm("-fullscreen");

	int window_settings = fullscreen ? WS_POPUP | WS_VISIBLE : WS_SYSMENU & ~WS_CAPTION;

	if (fullscreen) {
		grabMouse = true;

		X_width = SCREENWIDTH * 4;
		X_height = SCREENHEIGHT * 4;
		x = 0;
		y = 0;

		X_DevMode.dmSize = sizeof(X_DevMode);
		X_DevMode.dmPelsWidth = X_width;
		X_DevMode.dmPelsHeight = X_height;
		X_DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

	}
	else {
		X_width = SCREENWIDTH;
		X_height = SCREENHEIGHT;
	}

	X_mainWindow = CreateWindowA(
		"DoomWindow",
		"DOOM",
		window_settings,
		x, y, X_width + 15 * !fullscreen, X_height + 39 * !fullscreen,
		NULL, NULL,
		global_hInstance, NULL);

	if (!X_mainWindow)
	{
		I_Error("Could not create window");
	}
	else
	{
		X_mainDC = GetDC(X_mainWindow);
		ShowWindow(X_mainWindow, global_nCmdShow);
		UpdateWindow(X_mainWindow);
		if (grabMouse) { SetCapture(X_mainWindow); }
	}

	ShowCursor(FALSE);
}