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

HWND		X_mainWindow;
HDC			X_mainDC;
MSG			DoomMessage;
BITMAPINFO*  bmi;

// Fake mouse handling.
// This cannot work properly w/o DGA.
// Needs an invisible mouse cursor at least.
Dboolean		grabMouse;

//
//  Translates the key currently in X_event
//

int win32_TranslatePress(WPARAM wparam) {
	switch (wparam)
	{
	case MK_LBUTTON:
		return 1;
	case MK_MBUTTON:
		return 2;
	case MK_RBUTTON:
		return 4;
	default:
		return 0;
		break;
	}
}

int xlatekey(WPARAM wparam)
{

    int rc;

    switch(wparam)
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
	DestroyWindow(X_mainWindow);
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?
}

static int	lastmousex = 0;
static int	lastmousey = 0;
Dboolean		mousemoved = false;

//
// I_StartTic
//
void I_StartTic (void)
{

    if (!X_mainWindow)
	return;

	GetMessageA(&DoomMessage, NULL, 0, 0);
	TranslateMessage(&DoomMessage);
	DispatchMessageA(&DoomMessage);

	if (grabMouse) {
		RECT temp_rect;
		GetWindowRect(X_mainWindow, &temp_rect);
		SetCursorPos((temp_rect.left + temp_rect.right) / 2, (temp_rect.top + temp_rect.bottom) / 2);
	}
	mousemoved = false;
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
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

	for (i=0 ; i<tics*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
	for ( ; i<20*2 ; i+=2)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    
    }

	SetDIBitsToDevice(
		X_mainDC,
		0,         // Destination x coordinate
		0,         // Destination y coordinate
		SCREENWIDTH,
		SCREENHEIGHT,
		0,         // Source x coordinate in the bitmap data
		0,         // Source y coordinate in the bitmap data
		0,         // First scan line
		SCREENHEIGHT,    // Number of scan lines
		screens[0],
		bmi,
		DIB_RGB_COLORS
	);

}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}


//
// Palette stuff.
//

void UploadNewPalette(byte *palette)
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
	    for (i=0 ; i<256 ; i++)
	    {
		c = gammatable[usegamma][*palette++];
		bmi->bmiColors[i].rgbRed = (c<<8) + c;
		c = gammatable[usegamma][*palette++];
		bmi->bmiColors[i].rgbGreen = (c<<8) + c;
		c = gammatable[usegamma][*palette++];
		bmi->bmiColors[i].rgbBlue = (c<<8) + c;
		bmi->bmiColors[i].rgbReserved = 0;
	    }
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    UploadNewPalette(palette);
}


LRESULT CALLBACK DoomWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	POINT xmotion;
	event_t event;

	if (msg == WM_LBUTTONUP || msg == WM_MBUTTONUP || msg == WM_RBUTTONUP) {
		event.type = ev_mouse;
		event.data1 = 0;
		event.data2 = event.data3 = 0;
		D_PostEvent(&event);
		return;
	}

	switch (msg)
	{
	case WM_CLOSE:
		I_Quit();
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
		break;
	case WM_SYSKEYUP:
		event.type = ev_keyup;
		event.data1 = xlatekey(wparam);
		D_PostEvent(&event);
		// fprintf(stderr, "ku");
		break;


	// Button up check
	case WM_LBUTTONDOWN:
		event.type = ev_mouse;
		event.data1 = 1;
		event.data2 = event.data3 = 0;
		D_PostEvent(&event);
		break;
	case WM_MBUTTONDOWN:
		event.type = ev_mouse;
		event.data1 = 2;
		event.data2 = event.data3 = 0;
		D_PostEvent(&event);
		break;
	case WM_RBUTTONDOWN:
		event.type = ev_mouse;
		event.data1 = 4;
		event.data2 = event.data3 = 0;
		D_PostEvent(&event);
		break;

	// mouse move
	case WM_MOUSEMOVE:
		GetCursorPos(&xmotion);
		ScreenToClient(hwnd, &xmotion);
		event.type = ev_mouse;
		event.data1 = win32_TranslatePress(wparam);
		event.data2 = (xmotion.x - lastmousex) << 2;
		event.data3 = (lastmousey - xmotion.y) << 2;

		if (event.data2 || event.data3)
		{
			lastmousex = xmotion.x;
			lastmousey = xmotion.y;

			if (xmotion.x != SCREENWIDTH / 2 &&
				xmotion.y != SCREENHEIGHT / 2)
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
	case WM_PAINT:
		// For some annoying reason, the window freezes all game
		// when DefWindowProc handles WM_PAINT
		break;
	default:
		return DefWindowProcA(hwnd, msg, wparam, lparam);
	}
}

void I_InitGraphics(void)
{

    char*		displayname;
    char*		d;
    int			n;
    int			pnum;
    int			x=0;
    int			y=0;
    
    // warning: char format, different type arg
    char		xsign=' ';
    char		ysign=' ';
    
    int			oktodraw;
    unsigned long	attribmask;
    int			valuemask;
    static int		firsttime=1;

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
    if ( (pnum=M_CheckParm("-geom")) ) // suggest parentheses around assignment
    {
	// warning: char format, different type arg 3,5
	n = sscanf(myargv[pnum+1], "%c%d%c%d", &xsign, &x, &ysign, &y);
	
	if (n==2)
	    x = y = 0;
	else if (n==6)
	{
	    if (xsign == '-')
		x = -x;
	    if (ysign == '-')
		y = -y;
	}
	else
	    I_Error("bad -geom parameter");
    }

	X_mainWindow = CreateWindowA(
		"DoomWindow", "DOOM", 
		WS_SYSMENU & ~WS_CAPTION,
		x, y, SCREENWIDTH+15, SCREENHEIGHT+39,
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