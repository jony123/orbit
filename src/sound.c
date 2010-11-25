/*

ORBIT, a freeware space combat simulator
Copyright (C) 1999  Steve Belczyk <steve1@genesis.nred.ma.us>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "orbit.h"

/* Sound file names */
char *sample_names[NSOUNDS] = {
    "sounds/phaser.wav",      /* SOUND_FIRE */
    "sounds/explosion1.wav",  /* SOUND_BOOM */
    "sounds/communicator.wav" /* SOUND_COMM */
};

/*
 *  Lots of stuff for sound
 *
 *  Much of this is based on code from StackUp by Tool@theWaterCooler.com
 */

#ifdef WIN32

#include <dsound.h>

#define USE_DSOUND

/* Windows stuff I don't understand */
HRESULT		g_DX_Result;
HWND	hwnd;
LPDIRECTSOUND	g_lpDS;
int		g_bSoundPresent;
int		m_bSoundPresent;
LPDIRECTSOUNDBUFFER   m_lpDS_Sounds[NSOUNDS];

#define DS_CHECK_ERROR(ErrorMessage) \
    {if(g_DX_Result != DS_OK) {Log (ErrorMessage); return 0;}}
#define DS_CB_ERROR(ErrorMessage) {Log (ErrorMessage); return 0;}

int InitSound (void);
void FinishSound (void);
int g_DS_Init (void);
void g_DS_Finish (void);
int InitSound2 (void);
int FinishSound2 (void);
int DS_Init (void);
int DS_CreateBufferFromWaveFile(char* FileName, DWORD dwBuf);
int DS_CreateSoundBuffer(DWORD dwBuf, DWORD dwBufSize, DWORD dwFreq, DWORD dwBitsPerSample, DWORD dwBlkAlign, BOOL bStereo);
int DS_ReadData(LPDIRECTSOUNDBUFFER lpDSB, FILE* pFile, DWORD dwSize, DWORD dwPos);
int StopAllSounds (void);
int DS_PlaySound(enum sounds nSound, DWORD dwFlags);
void DS_Finish (void);

int InitSound()
{
//  m_pThisApp = AfxGetApp();
	hwnd = GetDesktopWindow();

    g_bSoundPresent = g_DS_Init();
    if(!g_bSoundPresent)
      g_DS_Finish();

  /* Read in the sounds, etc */
  InitSound2();

  return 1;
}

void FinishSound()
{
    g_DS_Finish();
    FinishSound2();
}

int g_DS_Init()
{
	HWND hwnd;

	hwnd = GetActiveWindow();

  g_DX_Result = DirectSoundCreate(NULL, &g_lpDS, NULL);
  DS_CHECK_ERROR("Error - DS - Create - Audio cannot be used");

  g_DX_Result = IDirectSound_SetCooperativeLevel (g_lpDS, GetActiveWindow(), DSSCL_NORMAL);
  DS_CHECK_ERROR("Error - DS - SetCooperativeLevel");

  return 1;
}

void g_DS_Finish()
{
  if(g_lpDS != NULL)
  {
//  g_lpDS->Release();
	IDirectSound_Release (g_lpDS);
    g_lpDS = NULL;
  }
}

int InitSound2()
{
    if((m_bSoundPresent = g_bSoundPresent) == 1)
      m_bSoundPresent = DS_Init();

  return 1;
}

int FinishSound2() 
{
  if(m_bSoundPresent)
    DS_Finish();
  return 1;
}

/*  Playing sounds

      #ifdef USE_DSOUND
        DS_PlaySound(DS_SOUND_SIGNALIZING, NULL);
      #endif
      #ifdef USE_DSOUND
        DS_PlaySound(DS_SOUND_FALLING, NULL);
      #endif
    #ifdef USE_DSOUND
      DS_PlaySound(DS_SOUND_ROTATE, NULL);
    #endif
      #ifdef USE_DSOUND
        DS_PlaySound(DS_SOUND_PLACE, NULL);
      #endif
              #ifdef USE_DSOUND
                DS_PlaySound(DS_SOUND_SPEEDUP, NULL);
              #endif
*/

int DS_Init()
{
  enum sounds i;

  for(i = SOUND_FIRE; i < NSOUNDS; i ++)    // Null out all the sound pointers
    m_lpDS_Sounds[i] = NULL;

  for(i = SOUND_FIRE; i < NSOUNDS; i++)
    if(!DS_CreateBufferFromWaveFile(sample_names[i], i))
      DS_CB_ERROR("Error - DS - Loading .WAV file");

  return 1;
}

int DS_CreateBufferFromWaveFile(char* FileName, DWORD dwBuf)
{
  struct WaveHeader
  {
    BYTE        RIFF[4];          // "RIFF"
    DWORD       dwSize;           // Size of data to follow
    BYTE        WAVE[4];          // "WAVE"
    BYTE        fmt_[4];          // "fmt "
    DWORD       dw16;             // 16
    WORD        wOne_0;           // 1
    WORD        wChnls;           // Number of Channels
    DWORD       dwSRate;          // Sample Rate
    DWORD       BytesPerSec;      // Sample Rate
    WORD        wBlkAlign;        // 1
    WORD        BitsPerSample;    // Sample size
    BYTE        DATA[4];          // "DATA"
    DWORD       dwDSize;          // Number of Samples
  };

  struct WaveHeader wavHdr;
  FILE *pFile;
  DWORD dwSize;
  BOOL bStereo;

  // Open the wave file       
  pFile = fopen(FileName, "rb");
  if(!pFile)
  {
	Log ("DS_CreateBufferFromWaveFile: Can't open %s", FileName);
    return 0;
  }

  // Read in the wave header          
  if (fread(&wavHdr, sizeof(wavHdr), 1, pFile) != 1) 
  {
	Log ("DS_CreateBufferFromWaveFile: Can't read wave header");
    fclose(pFile);
    return 0;
  }

/**
	Log ("Header for WAV file %s", FileName);
	Log ("dwSize: %d", wavHdr.dwSize);
	Log ("dw16: %d", wavHdr.dw16);
	Log ("wOne_0: %d", wavHdr.wOne_0);
	Log ("wChnls: %d", wavHdr.wChnls);
	Log ("dwSRate: %d", wavHdr.dwSRate);
	Log ("BytesPerSec: %d", wavHdr.BytesPerSec);
	Log ("wBlkAlign: %d", wavHdr.wBlkAlign);
	Log ("BitsPerSample: %d", wavHdr.BitsPerSample);
	Log ("dwDSize: %d", wavHdr.dwDSize);
**/

  // Figure out the size of the data region
  dwSize = wavHdr.dwDSize;

  // Is this a stereo or mono file?
  bStereo = wavHdr.wChnls > 1 ? 1 : 0;

  // Create the sound buffer for the wave file
  if(DS_CreateSoundBuffer(dwBuf, dwSize, wavHdr.dwSRate, wavHdr.BitsPerSample, wavHdr.wBlkAlign, bStereo) != TRUE)
  {
	Log ("DS_CreateBufferFromWaveFile: DS_CreateSoundBuffer returned FALSE");
    // Close the file
    fclose(pFile);
    
    return 0;
  }

  // Read the data for the wave file into the sound buffer
  if (!DS_ReadData(m_lpDS_Sounds[dwBuf], pFile, dwSize, sizeof(wavHdr))) 
  {
	Log ("DS_CreateBufferFromWaveFile: DS_ReadData returned FALSE");
    fclose(pFile);
    return 0;
  }
  fclose(pFile);
  return 1;
}

int DS_CreateSoundBuffer(DWORD dwBuf, DWORD dwBufSize, DWORD dwFreq, DWORD dwBitsPerSample, DWORD dwBlkAlign, BOOL bStereo)
{
  PCMWAVEFORMAT pcmwf;
  DSBUFFERDESC dsbdesc;
  
  // Set up wave format structure.
  memset( &pcmwf, 0, sizeof(PCMWAVEFORMAT) );
  pcmwf.wf.wFormatTag         = WAVE_FORMAT_PCM;      
  pcmwf.wf.nChannels          = bStereo ? 2 : 1;
  pcmwf.wf.nSamplesPerSec     = dwFreq;
  pcmwf.wf.nBlockAlign        = (WORD)dwBlkAlign;
  pcmwf.wf.nAvgBytesPerSec    = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
  pcmwf.wBitsPerSample        = (WORD)dwBitsPerSample;

  // Set up DSBUFFERDESC structure.
  memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));  // Zero it out. 
  dsbdesc.dwSize              = sizeof(DSBUFFERDESC);
  dsbdesc.dwFlags             = DSBCAPS_CTRLDEFAULT;  // Need default controls (pan, volume, frequency).
  dsbdesc.dwBufferBytes       = dwBufSize; 
  dsbdesc.lpwfxFormat         = (LPWAVEFORMATEX)&pcmwf;

//g_DX_Result = g_lpDS->CreateSoundBuffer(&dsbdesc, &m_lpDS_Sounds[dwBuf], NULL);
  g_DX_Result = IDirectSound_CreateSoundBuffer(g_lpDS, &dsbdesc, &m_lpDS_Sounds[dwBuf], NULL);
  DS_CHECK_ERROR("Error - DS - CreateSoundBuffer");

  return 1;
}

int DS_ReadData(LPDIRECTSOUNDBUFFER lpDSB, FILE* pFile, DWORD dwSize, DWORD dwPos) 
{
  LPVOID pData1;
  DWORD  dwData1Size;
  LPVOID pData2;
  DWORD  dwData2Size;
  HRESULT rval;

  // Seek to correct position in file (if necessary)
  if (dwPos != 0xffffffff) 
  {
    if (fseek(pFile, dwPos, SEEK_SET) != 0) 
    {
	  Log ("DS_ReadData: fseek failed");
      return 0;
    }
  }

  // Lock data in buffer for writing
//rval = lpDSB->Lock(0, dwSize, &pData1, &dwData1Size, &pData2, &dwData2Size, DSBLOCK_FROMWRITECURSOR);
  rval = IDirectSoundBuffer_Lock(lpDSB, 0, dwSize, &pData1, &dwData1Size, &pData2, &dwData2Size, DSBLOCK_FROMWRITECURSOR);
//Log ("dwSize = %d, dwData1Size = %d, dwData2Size= %d", dwSize, dwData1Size, dwData2Size);
  if (rval != DS_OK)
  {
	Log ("DS_ReadData: IDirectSoundBuffer_Loc returned %d", rval);
    return 0;
  }

  // Read in first chunk of data
  if (dwData1Size > 0) 
  {
    if (fread(pData1, dwData1Size, 1, pFile) != 1) 
    {
	  Log ("DS_ReadData: fread() of first chunk failed");
      return 0;
    }
  }

  // read in second chunk if necessary
  if (dwData2Size > 0) 
  {
    if (fread(pData2, dwData2Size, 1, pFile) != 1) 
    {
	  Log ("DS_ReadData: fread() of second chunk failed");
      return 0;
    }
  }

  // Unlock data in buffer
//rval = lpDSB->Unlock(pData1, dwData1Size, pData2, dwData2Size);
  rval = IDirectSoundBuffer_Unlock(lpDSB, pData1, dwData1Size, pData2, dwData2Size);
  if (rval != DS_OK)
  {
    Log ("DS_ReadData: IDirectSoundBuffer_Unlock returned %d", rval);
    return 0;
  }

  return 1;
}

int StopAllSounds()
{
  int i;

  // Make sure we have a valid sound buffer
  for (i = 0; i < NSOUNDS; i++)
  {
    if(m_lpDS_Sounds[i])
    {
      DWORD dwStatus;
//    g_DX_Result = m_lpDS_Sounds[i]->GetStatus(&dwStatus);
      g_DX_Result = IDirectSoundBuffer_GetStatus(m_lpDS_Sounds[i], &dwStatus);
      DS_CHECK_ERROR("Error - DS - GetStatus");

      if ((dwStatus & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING)
      {  
//      g_DX_Result = m_lpDS_Sounds[i]->Stop();     // Play the sound
        g_DX_Result = IDirectSoundBuffer_Stop(m_lpDS_Sounds[i]);     // Play the sound
        DS_CHECK_ERROR("Error - DS - Stop");
      }
    }
  }
  return 1;
}

int PlayAudio (enum sounds nSound)
{
  DS_PlaySound(nSound, 0);
}

int DS_PlaySound(int nSound, DWORD dwFlags)
{
  if(!m_bSoundPresent)
    return 1;

  if(m_lpDS_Sounds[nSound])  // Make sure we have a valid sound buffer
  {
    DWORD dwStatus;
//  g_DX_Result = m_lpDS_Sounds[nSound]->GetStatus(&dwStatus);
    g_DX_Result = IDirectSoundBuffer_GetStatus(m_lpDS_Sounds[nSound], &dwStatus);
    DS_CHECK_ERROR("Error - DS - GetStatus");

    if((dwStatus & DSBSTATUS_PLAYING) != DSBSTATUS_PLAYING)
    {
//    g_DX_Result = m_lpDS_Sounds[nSound]->Play(0, 0, dwFlags);    // Play the sound
      g_DX_Result = IDirectSoundBuffer_Play(m_lpDS_Sounds[nSound], 0, 0, dwFlags);    // Play the sound
      DS_CHECK_ERROR("Error - DS - Play");
    }
  }

  return 1;
}

void DS_Finish()
{
  int i;

  if(g_lpDS != NULL)
  {
    for(i = 0; i < NSOUNDS; i ++)
    {
      if(m_lpDS_Sounds[i])
      {       
//      m_lpDS_Sounds[i]->Release();
        IDirectSound_Release (m_lpDS_Sounds[i]);
        m_lpDS_Sounds[i] = NULL;
      }
    }
  }
}

#elif defined ESD /* ifdef WIN32 */

/*
 *  Enlightened Sound Daemon
 */
#include "esd.h"

char *host = "localhost";
char *name = "Orbit";
int samples[NSOUNDS];
int esd_fd = -1;

int InitSound (void)
{
    int error = 0;
    int i;

    /* Open sound daemon */
    esd_fd = esd_open_sound(host);
    /* Check for success */
    if(esd_fd < 0)
        return 0;

    for(i=0; i<NSOUNDS; i++)
    {
        samples[i] = esd_file_cache(esd_fd, name, sample_names[i]);
        if(samples[i] < 0)
        {
            error = 1;
            break;
        }
    }
    /* Check for errors */
    if(error == 0)
        return 1;

    /* Error, close daemon */
    esd_close(esd_fd);
    return 0;
}

int PlayAudio(enum sounds nSound)
{
    if(esd_fd < 0)
        return 0;
    /* Play precached sample */
    esd_sample_play(esd_fd, samples[nSound]);
    return 1;
}

void FinishSound (void)
{
    int i;

    if(esd_fd < 0)
        return;
    /* Uncache samples */
    for(i = 0; i < NSOUNDS; i++)
    {
        /*esd_sample_kill(esd_fd, samples[i]);*/
        esd_sample_free(esd_fd, samples[i]);
    }
    /* Close daemon */
    esd_close(esd_fd);
    esd_fd = -1;
    return;
}
#else /* if defined ESD */

/* Dummy functions for non existing functions */
int InitSound(void)
{ return 0; }
int PlayAudio(enum sounds nSound)
{ return 0; }
void FinishSound(void)
{}

#endif
