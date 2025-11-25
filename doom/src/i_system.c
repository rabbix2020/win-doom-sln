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
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <time.h>

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"




int	mb_used = 6;


void
I_Tactile
( int	on,
  int	off,
  int	total )
{
  // UNUSED.
  on = off = total = 0;
}

ticcmd_t	emptycmd;
ticcmd_t*	I_BaseTiccmd(void)
{
    return &emptycmd;
}


int  I_GetHeapSize (void)
{
    return mb_used*1024*1024;
}

byte* I_ZoneBase (int*	size)
{
    *size = mb_used*1024*1024;
    return (byte *) malloc (*size);
}



//
// I_GetTime
// returns time in 1/70th second tics
//
int  I_GetTime(void)
{
    struct timespec	tp;
    int			newtics;
    static int		basetime = 0;

    timespec_get(&tp, TIME_UTC);
    if (!basetime)
        basetime = tp.tv_sec;
    newtics = (tp.tv_sec - basetime) * TICRATE + (tp.tv_nsec / 1000) * TICRATE / 1000000;
    return newtics;
}



//
// I_Init
//
void I_Init (void)
{
    I_InitSound();
    //  I_InitGraphics();
}

//
// I_Quit
//
void I_Quit (void)
{
    D_QuitNetGame ();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults ();
    I_ShutdownGraphics();
    exit(0);
}

void I_WaitVBL(int count)
{
    struct timespec	tp;
    timespec_get(&tp, TIME_UTC);
    long cur_time = tp.tv_sec * 1000 + (tp.tv_nsec / 1000000);
    long end_time = cur_time + (count * (1000 / 70));

    while (cur_time < end_time) // this is all need just to update sound when D_DoomLoop can't
    {
        timespec_get(&tp, TIME_UTC);
        cur_time = tp.tv_sec * 1000 + (tp.tv_nsec / 1000000);

        I_UpdateSound();
        I_SubmitSound();
        _sleep(1000 / 70);
    }
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

byte*	I_AllocLow(int length)
{
    byte*	mem;
        
    mem = (byte *)malloc (length);
    memset (mem,0,length);
    return mem;
}


//
// I_Error
//
extern Dboolean demorecording;

void I_Error (char *error, ...)
{
    va_list	argptr;
    char error_msg[256];

    // Message first.
    va_start (argptr,error);
    fprintf (stderr, "Error: ");
    strcpy(error_msg, "Error: ");

    vfprintf (stderr,error,argptr);
    
    char temp_str[248];
    strcpy(temp_str, error);
    _vsnprintf_s(temp_str, sizeof(temp_str) + 1, sizeof(temp_str), error, argptr);
    strcat(error_msg, temp_str);

    fprintf (stderr, "\n");
    va_end (argptr);

    fflush( stderr );
    
    // Shutdown. Here might be other errors.
    if (demorecording)
	G_CheckDemoStatus();

    D_QuitNetGame ();
    I_ShutdownGraphics();

    MessageBoxA(NULL, temp_str, "DOOM", MB_ICONERROR);

    exit(-1);
}
