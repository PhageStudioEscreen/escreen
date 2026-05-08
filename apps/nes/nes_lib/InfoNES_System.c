/*
 *	InfoNES
 *		SDL ports by mata      03/04/19
 *              Modified by Jay        06/02/25
 *
 * 	Start Date: 2003.04.19
 */

#include "InfoNES_System.h"
#if LV_USE_100ASK_NES != 0

#include "lvgl/lvgl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

#include "InfoNES.h"
#include "InfoNES_pAPU.h"

/* External wave buffers from InfoNES_pAPU.c */
extern WORD wave_buffers[5][735];

/*-------------------------------------------------------------------*/
/*  Sound (ALSA simple)                                              */
/*-------------------------------------------------------------------*/
static snd_pcm_t *pcm = NULL;
static short sound_buffer[4096];
static int sound_buffer_samples = 0;
static int sound_buffer_len = 0;

#define NES_AUDIO_RB_FRAMES 32768U
static int16_t *nes_audio_rb = NULL;
static size_t nes_audio_rb_size = 0;
static size_t nes_audio_rb_in = 0;
static size_t nes_audio_rb_out = 0;
static size_t nes_period_frames = 367;
static pthread_t nes_audio_thread;
static int nes_audio_thread_running = 0;
static pthread_mutex_t nes_audio_lock = PTHREAD_MUTEX_INITIALIZER;

static unsigned long long nes_epipe_count = 0;
static unsigned long long nes_estrpipe_count = 0;
static unsigned long long nes_write_err_count = 0;
static unsigned long long nes_rb_under_count = 0;
static unsigned long long nes_rb_drop_count = 0;
static unsigned long long nes_audio_cb_count = 0;

static size_t nes_audio_rb_used_nolock(void)
{
    return nes_audio_rb_in - nes_audio_rb_out;
}

static size_t nes_audio_rb_pop(int16_t *dst, size_t frames)
{
    size_t got = 0;
    pthread_mutex_lock(&nes_audio_lock);
    size_t used = nes_audio_rb_used_nolock();
    got = used < frames ? used : frames;
    for (size_t i = 0; i < got; i++) {
        dst[i] = nes_audio_rb[(nes_audio_rb_out + i) & (nes_audio_rb_size - 1)];
    }
    nes_audio_rb_out += got;
    pthread_mutex_unlock(&nes_audio_lock);
    return got;
}

static void nes_audio_rb_push_overwrite(const int16_t *src, size_t frames)
{
    pthread_mutex_lock(&nes_audio_lock);

    if (frames > nes_audio_rb_size) {
        size_t skip = frames - nes_audio_rb_size;
        src += skip;
        frames = nes_audio_rb_size;
        nes_rb_drop_count += skip;
    }

    size_t used = nes_audio_rb_used_nolock();
    size_t free_frames = nes_audio_rb_size - used;
    if (frames > free_frames) {
        size_t drop = frames - free_frames;
        nes_audio_rb_out += drop;
        nes_rb_drop_count += drop;
    }

    for (size_t i = 0; i < frames; i++) {
        nes_audio_rb[(nes_audio_rb_in + i) & (nes_audio_rb_size - 1)] = src[i];
    }
    nes_audio_rb_in += frames;

    pthread_mutex_unlock(&nes_audio_lock);
}

static void *nes_audio_thread_fn(void *arg)
{
    (void)arg;
    int16_t *out_buf = malloc(nes_period_frames * sizeof(int16_t));
    if (!out_buf) {
        return NULL;
    }

    while (nes_audio_thread_running) {
        size_t want = nes_period_frames;
        size_t got = nes_audio_rb_pop(out_buf, want);
        if (got < want) {
            memset(out_buf + got, 0, (want - got) * sizeof(int16_t));
            if (got == 0) {
                nes_rb_under_count++;
            }
        }

        size_t written = 0;
        while (written < want && nes_audio_thread_running) {
            snd_pcm_sframes_t ret = snd_pcm_writei(pcm, out_buf + written, want - written);
            if (ret == -EPIPE) {
                nes_epipe_count++;
            } else if (ret == -ESTRPIPE) {
                nes_estrpipe_count++;
            }
            if (ret < 0) {
                int rec = snd_pcm_recover(pcm, (int)ret, 0);
                if (rec < 0) {
                    nes_write_err_count++;
                    usleep(1000);
                    break;
                }
                continue;
            }
            written += (size_t)ret;
        }

        if (written > 0 && snd_pcm_state(pcm) == SND_PCM_STATE_PREPARED) {
            snd_pcm_start(pcm);
        }
    }

    free(out_buf);
    return NULL;
}

#define DEBUG_PRINT(fmt, args...) \
    do { \
        FILE *fp = fopen("/root/nes_debug.log", "a"); \
        if (fp) { \
            fprintf(fp, "[NES] " fmt, ##args); \
            fclose(fp); \
        } \
    } while(0)

/*-------------------------------------------------------------------*/
/*  ROM image file information                                       */
/*-------------------------------------------------------------------*/

char szRomName[ 256 ];
char szSaveName[ 256 ];
int nSRAM_SaveFlag;

/*-------------------------------------------------------------------*/
/*  Constants ( Linux specific )                                     */
/*-------------------------------------------------------------------*/

#define VBOX_SIZE 0
static const char * VERSION = "InfoNES v0.97J RC1";

/*-------------------------------------------------------------------*/
/*  Global Variables ( SDL specific )                                */
/*-------------------------------------------------------------------*/

bool quit = false;

/* for video */

/* For Sound Emulation */
BYTE final_wave[2048];
int waveptr;
int wavflag;
int wavdone;

/* Pad state */
DWORD dwPad1;
DWORD dwPad2;
DWORD dwSystem;
volatile int nes_pause_flag = 0;
volatile int nes_quit_flag = 0;

/*-------------------------------------------------------------------*/
/*  Function prototypes ( SDL specific )                             */
/*-------------------------------------------------------------------*/

int start_application(char *filename);
void poll_event(void);
int LoadSRAM();
int SaveSRAM();


/* Palette data */
#if LV_COLOR_DEPTH == 16
WORD NesPalette[ 64 ] =
{
	0x39ce, 0x1071, 0x0015, 0x2013, 0x440e, 0x5402, 0x5000, 0x3c20,
	0x20a0, 0x0100, 0x0140, 0x00e2, 0x0ceb, 0x0000, 0x0000, 0x0000,
	0x5ef7, 0x01dd, 0x10fd, 0x401e, 0x5c17, 0x700b, 0x6ca0, 0x6521,
	0x45c0, 0x0240, 0x02a0, 0x0247, 0x0211, 0x0000, 0x0000, 0x0000,
	0x7fff, 0x1eff, 0x2e5f, 0x223f, 0x79ff, 0x7dd6, 0x7dcc, 0x7e67,
	0x7ae7, 0x4342, 0x2769, 0x2ff3, 0x03bb, 0x0000, 0x0000, 0x0000,
	0x7fff, 0x579f, 0x635f, 0x6b3f, 0x7f1f, 0x7f1b, 0x7ef6, 0x7f75,
	0x7f94, 0x73f4, 0x57d7, 0x5bf9, 0x4ffe, 0x0000, 0x0000, 0x0000
};
#elif LV_COLOR_DEPTH == 32
WORD NesPalette[ 64 ]=
{
#if 0
	0x616161, 0x000088, 0x1F0D99, 0x371379, 0x561260, 0x5D0010, 0x520E00, 0x3A2308,
	0x21350C, 0x0D410E, 0x174417, 0x003A1F, 0x002F57, 0x000000, 0x000000, 0x000000,
	0xAAAAAA, 0x0D4DC4, 0x4B24DE, 0x6912CF, 0x9014AD, 0x9D1C48, 0x923404, 0x735005,
	0x5D6913, 0x167A11, 0x138008, 0x127649, 0x1C6691, 0x000000, 0x000000, 0x000000,
	0xFCFCFC, 0x639AFC, 0x8A7EFC, 0xB06AFC, 0xDD6FF2, 0xE771AB, 0xE38658, 0xCC9E22,
	0xA8B100, 0x72C100, 0x5ACD4E, 0x34C28E, 0x4FBECE, 0x424242, 0x000000, 0x000000,
	0xFCFCFC, 0xBED4FC, 0xCACAFC, 0xD9C4FC, 0xECC1FC, 0xFAC3E7, 0xF7CEC3, 0xE2CDA7,
	0xDADB9C, 0xC8E39E, 0xBFE5B8, 0xB2EBC8, 0xB7E5EB, 0xACACAC, 0x000000, 0x000000
#else
	0x6A6D6A, 0x001380, 0x1E008A, 0x39007A, 0x550056, 0x5A0018, 0x4F1000, 0x3D1C00,
	0x253200, 0x003D00, 0x004000, 0x003924, 0x002E55, 0x000000, 0x000000, 0x000000,
	0xB9BCB9, 0x1850C7, 0x4B30E3, 0x7322D6, 0x951FA9, 0x9D285C, 0x983700, 0x7F4C00,
	0x5E6400, 0x227700, 0x027E02, 0x007645, 0x006E8A, 0x000000, 0x000000, 0x000000,
	0xFFFFFF, 0x68A6FF, 0x8C9CFF, 0xB586FF, 0xD975FD, 0xE377B9, 0xE58D68, 0xD49D29,
	0xB3AF0C, 0x7BC211, 0x55CA47, 0x46CB81, 0x47C1C5, 0x4A4D4A, 0x000000, 0x000000,
	0xFFFFFF, 0xCCEAFF, 0xDDDEFF, 0xECDAFF, 0xF8D7FE, 0xFCD6F5, 0xFDDBCF, 0xF9E7B5,
	0xF1F0AA, 0xDAFAA9, 0xC9FFBC, 0xC3FBD7, 0xC4F6F6, 0xBEC1BE, 0x000000, 0x000000
#endif
};
#endif

/*===================================================================*/
/*                                                                   */
/*           LoadSRAM() : Load a SRAM                                */
/*                                                                   */
/*===================================================================*/
/* Start application */
int start_application(char *filename)
{
  /* Set a ROM image name */
  strcpy( szRomName, filename );

  /* Load cassette */
  if(InfoNES_Load(szRomName)==0) {
    /* Load SRAM */
    LoadSRAM();

    /* Success */
    return 1;
  }
  /* Failure */
  return 0;
}


/*===================================================================*/
/*                                                                   */
/*           LoadSRAM() : Load a SRAM                                */
/*                                                                   */
/*===================================================================*/
int LoadSRAM()
{
/*
 *  Load a SRAM
 *
 *  Return values
 *     0 : Normally
 *    -1 : SRAM data couldn't be read
 */

  //FILE *fp;
  lv_fs_file_t rom;
  unsigned char pSrcBuf[ SRAM_SIZE ];
  unsigned char chData;
  unsigned char chTag;
  int nRunLen;
  int nDecoded;
  int nDecLen;
  int nIdx;

  // It doesn't need to save it
  nSRAM_SaveFlag = 0;

  // It is finished if the ROM doesn't have SRAM
  if ( !ROM_SRAM )
    return 0;

  // There is necessity to save it
  nSRAM_SaveFlag = 1;

  // The preparation of the SRAM file name
  strcpy( szSaveName, szRomName );
  strcpy( strrchr( szSaveName, '.' ) + 1, "srm" );

  /*-------------------------------------------------------------------*/
  /*  Read a SRAM data                                                 */
  /*-------------------------------------------------------------------*/
  lv_fs_res_t res = lv_fs_open(&rom, szSaveName, LV_FS_MODE_RD);
  /* Error Detection */
  if (res != LV_FS_RES_OK) {
      //fprintf(stderr, "Error: couldn't open file.\n");
      printf("[Open Error]: couldn't open file!\n");
      return -1;
  }

  lv_fs_read(&rom, &pSrcBuf, SRAM_SIZE, NULL);

  lv_fs_close(&rom);

  /*-------------------------------------------------------------------*/
  /*  Extract a SRAM data                                              */
  /*-------------------------------------------------------------------*/

  nDecoded = 0;
  nDecLen = 0;

  chTag = pSrcBuf[ nDecoded++ ];

  while ( nDecLen < 8192 )
  {
    chData = pSrcBuf[ nDecoded++ ];

    if ( chData == chTag )
    {
      chData = pSrcBuf[ nDecoded++ ];
      nRunLen = pSrcBuf[ nDecoded++ ];
      for ( nIdx = 0; nIdx < nRunLen + 1; ++nIdx )
      {
        SRAM[ nDecLen++ ] = chData;
      }
    }
    else
    {
      SRAM[ nDecLen++ ] = chData;
    }
  }

  // Successful
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*           SaveSRAM() : Save a SRAM                                */
/*                                                                   */
/*===================================================================*/
int SaveSRAM()
{
/*
 *  Save a SRAM
 *
 *  Return values
 *     0 : Normally
 *    -1 : SRAM data couldn't be written
 */

  //FILE *fp;
    lv_fs_file_t rom;

  int nUsedTable[ 256 ];
  unsigned char chData;
  unsigned char chPrevData;
  unsigned char chTag;
  int nIdx;
  int nEncoded;
  int nEncLen;
  int nRunLen;
  unsigned char pDstBuf[ SRAM_SIZE ];

  if ( !nSRAM_SaveFlag )
    return 0;  // It doesn't need to save it

  /*-------------------------------------------------------------------*/
  /*  Compress a SRAM data                                             */
  /*-------------------------------------------------------------------*/

  memset( nUsedTable, 0, sizeof nUsedTable );

  for ( nIdx = 0; nIdx < SRAM_SIZE; ++nIdx )
  {
    ++nUsedTable[ SRAM[ nIdx++ ] ];
  }
  for ( nIdx = 1, chTag = 0; nIdx < 256; ++nIdx )
  {
    if ( nUsedTable[ nIdx ] < nUsedTable[ chTag ] )
      chTag = nIdx;
  }

  nEncoded = 0;
  nEncLen = 0;
  nRunLen = 1;

  pDstBuf[ nEncLen++ ] = chTag;

  chPrevData = SRAM[ nEncoded++ ];

  while ( nEncoded < SRAM_SIZE && nEncLen < SRAM_SIZE - 133 )
  {
    chData = SRAM[ nEncoded++ ];

    if ( chPrevData == chData && nRunLen < 256 )
      ++nRunLen;
    else
    {
      if ( nRunLen >= 4 || chPrevData == chTag )
      {
        pDstBuf[ nEncLen++ ] = chTag;
        pDstBuf[ nEncLen++ ] = chPrevData;
        pDstBuf[ nEncLen++ ] = nRunLen - 1;
      }
      else
      {
        for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
          pDstBuf[ nEncLen++ ] = chPrevData;
      }

      chPrevData = chData;
      nRunLen = 1;
    }

  }
  if ( nRunLen >= 4 || chPrevData == chTag )
  {
    pDstBuf[ nEncLen++ ] = chTag;
    pDstBuf[ nEncLen++ ] = chPrevData;
    pDstBuf[ nEncLen++ ] = nRunLen - 1;
  }
  else
  {
    for ( nIdx = 0; nIdx < nRunLen; ++nIdx )
      pDstBuf[ nEncLen++ ] = chPrevData;
  }

  /*-------------------------------------------------------------------*/
  /*  Write a SRAM data                                                */
  /*-------------------------------------------------------------------*/
  // Open SRAM file
  lv_fs_res_t res = lv_fs_open(&rom, szSaveName, LV_FS_MODE_WR);

  /* Error Detection */
  if (res != LV_FS_RES_OK) {
      //fprintf(stderr, "Save Error: couldn't open file.\n");
      printf("[Save Error]: couldn't open file!\n");
      return -1;
  }

  // Write SRAM data
  lv_fs_write(&rom, pDstBuf, nEncLen, NULL);

  // Close SRAM file
  lv_fs_close(&rom);

  // Successful
  return 0;
}

/*===================================================================*/
/*                                                                   */
/*                  InfoNES_Menu() : Menu screen                     */
/*                                                                   */
/*===================================================================*/
int InfoNES_Menu() {
  int prev_mute = APU_Mute;
  APU_Mute = 1;
  
  // Clear audio buffers to prevent noise
  InfoNES_MemorySet( (void *)wave_buffers[0], 0, 1470 );
  InfoNES_MemorySet( (void *)wave_buffers[1], 0, 1470 );
  InfoNES_MemorySet( (void *)wave_buffers[2], 0, 1470 );
  InfoNES_MemorySet( (void *)wave_buffers[3], 0, 1470 );
  InfoNES_MemorySet( (void *)wave_buffers[4], 0, 1470 );
  
  //lv_100ask_nes_menu();
  
  APU_Mute = prev_mute;
  return 0;
}

/* Read ROM image file */
int InfoNES_ReadRom( const char *pszFileName ) {
  //FILE *fp;
  lv_fs_file_t rom;
  uint32_t rn;
  long file_size;

  /* Open ROM file */
  lv_fs_res_t res = lv_fs_open(&rom, pszFileName, LV_FS_MODE_RD);
  /* Error Detection */
  if (res != LV_FS_RES_OK) {
      //fprintf(stderr, "Error: couldn't open file.\n");
      printf("[Open Error]: couldn't open file!\n");
      return 8;
  }
  /* Getting filesize */
	lv_fs_seek(&rom, 0L, LV_FS_SEEK_END);
	lv_fs_tell(&rom, &file_size);


	if (file_size < 0x4010) {
		//fprintf(stderr, "Error: input file is too small.\n");
    printf("[Check Error]: input file is too small!\n");

		lv_fs_close(&rom);
        return 8;
	}

  lv_fs_close(&rom);
  lv_fs_open(&rom, pszFileName, LV_FS_MODE_RD | LV_FS_MODE_WR);


  lv_fs_read(&rom, &NesHeader, sizeof(NesHeader), &rn);
  if ( memcmp( NesHeader.byID, "NES\x1a", 4 ) != 0 )
  {
      /* not .nes file */
      lv_fs_close(&rom);
      printf("[Check Error]: Not .nes file!\n");
      return -1;
  }

  /* Clear SRAM */
  memset( SRAM, 0, SRAM_SIZE );

  /* If trainer presents Read Triner at 0x7000-0x71ff */
  if(NesHeader.byInfo1 & 4){
    lv_fs_read(&rom, &SRAM[ 0x1000 ], 512, NULL);
  }


  /* Allocate Memory for ROM Image */
#if LV_MEM_CUSTOM == 0
  if(ROM == NULL)
    ROM = (BYTE *)lv_malloc( NesHeader.byRomSize * 0x4000 );
  else
    ROM = (BYTE *)lv_realloc(ROM, NesHeader.byRomSize * 0x4000 );
#else
  if(ROM == NULL)
    ROM = (BYTE *)malloc( NesHeader.byRomSize * 0x4000 );
  else
    ROM = (BYTE *)realloc(ROM, NesHeader.byRomSize * 0x4000 );
#endif

  printf("NesHeader.byRomSize * 0x4000:%d\n", NesHeader.byRomSize * 0x4000);

  /* Read ROM Image */
  lv_fs_read(&rom, ROM, 0x4000 * NesHeader.byRomSize, NULL);

  if ( NesHeader.byVRomSize > 0 )
  {
      /* Allocate Memory for VROM Image */
#if LV_MEM_CUSTOM == 0
      if(ROM == NULL)
        VROM = (BYTE *)lv_malloc( NesHeader.byVRomSize * 0x2000 );
      else
        VROM = (BYTE *)lv_realloc(VROM, NesHeader.byVRomSize * 0x2000 );
#else
      if(ROM == NULL)
        VROM = (BYTE *)malloc( NesHeader.byVRomSize * 0x2000 );
      else
        VROM = (BYTE *)realloc(VROM, NesHeader.byVRomSize * 0x2000 );
#endif

      printf("NesHeader.byVRomSize * 0x2000: %d\n", NesHeader.byVRomSize * 0x2000);

      /* Read VROM Image */
      lv_fs_read(&rom, VROM, 0x2000*NesHeader.byVRomSize, NULL);
  }

  /* File close */
  lv_fs_close(&rom);
  printf("Read file successful!!!\n");

  /* Successful */
  return 0;
}

/* Release a memory for ROM */
void InfoNES_ReleaseRom(){
  if(ROM) {
#if LV_MEM_CUSTOM == 0
    lv_free(ROM);
#else
    free(ROM);
#endif

  ROM=NULL;
  }

  if(VROM){
#if LV_MEM_CUSTOM == 0
    lv_free(VROM);
#else
    free(VROM);
#endif

    VROM=NULL;
  }
}

/* Transfer the contents of work frame on the screen */
void InfoNES_LoadFrame(){
  //lv_100ask_nes_flush();
}

/* Get a joypad state */
void InfoNES_PadState( DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem )
{
  //poll_event();
  *pdwPad1 = dwPad1;
  *pdwPad2 = dwPad2;
  *pdwSystem = dwSystem;
}

static const int joy_commit_range = 3276;

#if 0
void poll_event(void)
{
  SDL_Event	e;
  static signed char	x_joy=0, y_joy=0;

  while(SDL_PollEvent(&e))
  {
    switch(e.type)
    {
    case SDL_QUIT:
      dwSystem|=PAD_SYS_QUIT;
      quit=1;
      break;

    case SDL_KEYDOWN:
      switch(e.key.keysym.sym)
      {
      case SDLK_RETURN:
	if( !(e.key.keysym.mod & KMOD_ALT)) break;
	SDL_WM_ToggleFullScreen(screen);
	break;

      case SDLK_ESCAPE:
	dwSystem|=PAD_SYS_QUIT;
	quit=1;
	break;

      case SDLK_RIGHT:   dwPad1 |= (1<<7);break;
      case SDLK_LEFT:    dwPad1 |= (1<<6);break;
      case SDLK_DOWN:    dwPad1 |= (1<<5);break;
      case SDLK_UP:      dwPad1 |= (1<<4);break;
      case SDLK_s:	 dwPad1 |= (1<<3);break;    /* Start */
      case SDLK_a:	 dwPad1 |= (1<<2);break;    /* Select */
      case SDLK_z:	 dwPad1 |= (1<<1);break;    /* 'B' */
      case SDLK_x:	 dwPad1 |= (1<<0);break;    /* 'A' */
      case SDLK_m:
	/* Toggle of sound mute */
	APU_Mute = ( APU_Mute ? 0 : 1 );break;
      case SDLK_c:
	/* Toggle up and down clipping */
	PPU_UpDown_Clip = ( PPU_UpDown_Clip ? 0 : 1 ); break;
      } /* keydown */
      break;

    case SDL_KEYUP:
      switch(e.key.keysym.sym)
      {
      case SDLK_RIGHT:   dwPad1 &=~(1<<7);break;
      case SDLK_LEFT:    dwPad1 &=~(1<<6);break;
      case SDLK_DOWN:	 dwPad1 &=~(1<<5);break;
      case SDLK_UP:	 dwPad1 &=~(1<<4);break;
      case SDLK_s:	 dwPad1 &=~(1<<3);break;   /* Start */
      case SDLK_a:	 dwPad1 &=~(1<<2);break;   /* Select */
      case SDLK_z:	 dwPad1 &=~(1<<1);break;   /* 'B' */
      case SDLK_x:	 dwPad1 &=~(1<<0);break;   /* 'A' */
      } /* keyup */
      break;

    case SDL_JOYAXISMOTION:
      switch(e.jaxis.axis){
      case 0:	/* X axis */
	if(e.jaxis.value >  joy_commit_range){
	  if(x_joy > 0) break;
	  if(x_joy < 0) dwPad1 &=~(1<<6);
	  dwPad1 |= (1<<7); x_joy=+1; break; }
	if(e.jaxis.value < -joy_commit_range){
	  if(x_joy < 0) break;
	  if(x_joy > 0) dwPad1 &=~(1<<7);
	  dwPad1 |= (1<<6); x_joy=-1; break; }
	if     (x_joy < 0) dwPad1 &=~(1<<6);
	else if(x_joy > 0) dwPad1 &=~(1<<7);
	x_joy= 0; break;
      case 1: /* Y asis */
	if(e.jaxis.value >  joy_commit_range){
	  if(y_joy > 0) break;
	  if(y_joy < 0) dwPad1 &=~(1<<4);
	  dwPad1 |= (1<<5); y_joy=+1; break; }
	if(e.jaxis.value < -joy_commit_range){
	  if(y_joy < 0) break;
	  if(y_joy > 0) dwPad1 &=~(1<<5);
	  dwPad1 |= (1<<4); y_joy=-1; break; }
	if      (y_joy < 0) dwPad1 &=~(1<<4);
	else if (y_joy > 0) dwPad1 &=~(1<<5);
	y_joy= 0; break;
      } /* joysxismotion */

    case SDL_JOYBUTTONUP:
      switch(e.jbutton.button){
      case 2: dwPad1 &=~(1<<0);break; /* A */
      case 1: dwPad1 &=~(1<<1);break; /* B */
      case 8: dwPad1 &=~(1<<2);break; /* select */
      case 9: dwPad1 &=~(1<<3);break; /* start */
      } break;
    case SDL_JOYBUTTONDOWN:
      switch(e.jbutton.button){
      case 2: dwPad1 |= (1<<0);break; /* A */
      case 1: dwPad1 |= (1<<1);break; /* B */
      case 8: dwPad1 |= (1<<2);break; /* select */
      case 9: dwPad1 |= (1<<3);break; /* start */
      } break;
    }
  }
}
#endif // 0


/* memcpy */
void *InfoNES_MemoryCopy( void *dest, const void *src, int count ){
  memcpy( dest, src, count );
  return dest;
}

/* memset */
void *InfoNES_MemorySet( void *dest, int c, int count ){
  memset( dest, c, count);
  return dest;
}

/* Print debug message */
void InfoNES_DebugPrint( char *pszMsg ) {
  //fprintf(stderr,"%s\n", pszMsg);
  printf("[Error]: %s\n", pszMsg);
}

/* Wait */
void InfoNES_Wait(){}

/* Sound Initialize */
void InfoNES_SoundInit( void ){}

/* Sound Open */
int InfoNES_SoundOpen( int samples_per_sync, int rate ){
    int rc;
    const char *dev = getenv("PGS_SND_CARD");
    if (!dev) dev = "default";

    memset(sound_buffer, 0, sizeof(sound_buffer));

    if ((rc = snd_pcm_open(&pcm, dev, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        DEBUG_PRINT("ALSA: cannot open device %s: %s\n", dev, snd_strerror(rc));
        return -1;
    }

    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm, params);
    snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, params, 1);

    unsigned int req_rate = (unsigned int)rate;
    snd_pcm_hw_params_set_rate_near(pcm, params, &req_rate, NULL);

    snd_pcm_uframes_t period = (snd_pcm_uframes_t)samples_per_sync;
    snd_pcm_hw_params_set_period_size_near(pcm, params, &period, NULL);
    snd_pcm_uframes_t buffer = period * 8;
    snd_pcm_hw_params_set_buffer_size_near(pcm, params, &buffer);

    if ((rc = snd_pcm_hw_params(pcm, params)) < 0) {
        DEBUG_PRINT("ALSA: cannot set params: %s\n", snd_strerror(rc));
        snd_pcm_close(pcm);
        pcm = NULL;
        return -1;
    }

    snd_pcm_hw_params_get_period_size(params, &period, NULL);
    snd_pcm_hw_params_get_buffer_size(params, &buffer);
    snd_pcm_hw_params_get_rate(params, &req_rate, NULL);
    nes_period_frames = (size_t)period;
    DEBUG_PRINT("ALSA: actual rate=%u period=%lu, buffer=%lu\n", req_rate, period, buffer);

    snd_pcm_sw_params_t *sw;
    snd_pcm_sw_params_alloca(&sw);
    snd_pcm_sw_params_current(pcm, sw);
    snd_pcm_sw_params_set_avail_min(pcm, sw, period);
    snd_pcm_sw_params_set_start_threshold(pcm, sw, period);
    snd_pcm_sw_params(pcm, sw);

    if ((rc = snd_pcm_prepare(pcm)) < 0) {
        DEBUG_PRINT("ALSA: prepare failed: %s\n", snd_strerror(rc));
        snd_pcm_close(pcm);
        pcm = NULL;
        return -1;
    }

    int16_t *silence = calloc((size_t)buffer, sizeof(int16_t));
    if (silence) {
        snd_pcm_uframes_t remain = buffer;
        while (remain > 0) {
            snd_pcm_sframes_t w = snd_pcm_writei(pcm, silence + (buffer - remain), remain);
            if (w == -EPIPE || w == -ESTRPIPE) {
                snd_pcm_recover(pcm, (int)w, 0);
                continue;
            }
            if (w < 0) break;
            remain -= (snd_pcm_uframes_t)w;
        }
        free(silence);
    }

    nes_audio_rb_size = NES_AUDIO_RB_FRAMES;
    nes_audio_rb = malloc(nes_audio_rb_size * sizeof(int16_t));
    if (!nes_audio_rb) {
        snd_pcm_close(pcm);
        pcm = NULL;
        return -1;
    }
    nes_audio_rb_in = 0;
    nes_audio_rb_out = 0;
    nes_epipe_count = 0;
    nes_estrpipe_count = 0;
    nes_write_err_count = 0;
    nes_rb_under_count = 0;
    nes_rb_drop_count = 0;
    nes_audio_cb_count = 0;

    nes_audio_thread_running = 1;
    if (pthread_create(&nes_audio_thread, NULL, nes_audio_thread_fn, NULL) != 0) {
        nes_audio_thread_running = 0;
        free(nes_audio_rb);
        nes_audio_rb = NULL;
        snd_pcm_close(pcm);
        pcm = NULL;
        return -1;
    }

    sound_buffer_samples = samples_per_sync;
    DEBUG_PRINT("ALSA: sound opened, rate=%d, samples=%d\n", (int)req_rate, samples_per_sync);
    return 1;
}

/* Sound Close */
void InfoNES_SoundClose( void ){
    if (nes_audio_thread_running) {
        nes_audio_thread_running = 0;
        if (pcm) {
            snd_pcm_drop(pcm);
        }
        pthread_join(nes_audio_thread, NULL);
    }

    if (pcm) {
        snd_pcm_close(pcm);
        pcm = NULL;
    }

    if (nes_audio_rb) {
        free(nes_audio_rb);
        nes_audio_rb = NULL;
        nes_audio_rb_size = 0;
        nes_audio_rb_in = 0;
        nes_audio_rb_out = 0;
    }
}

/* For High-pass filter (DC offset removal) */
static int hp_filter_state = 0;
static int hp_filter_prev_input = 0;

#define HP_ALPHA 0.9995f
#define VOLUME   100

/* Sound Output 5 Waves - 2 Pulse, 1 Triangle, 1 Noise. 1 DPCM */
void InfoNES_SoundOutput(int samples, WORD *wave1, WORD *wave2, WORD *wave3, WORD *wave4, WORD *wave5){
    if (!pcm || !nes_audio_rb) return;

    int i;
    for (i = 0; i < samples; i++) {
        int mixed = (wave1[i] + wave2[i] + wave3[i] + wave4[i] + wave5[i]) / 5;
        int sample = (mixed - 128);

        int output = (int)(HP_ALPHA * (hp_filter_state + sample - hp_filter_prev_input));
        hp_filter_prev_input = sample;
        hp_filter_state = output;

        sound_buffer[i] = output * VOLUME;
    }

    nes_audio_rb_push_overwrite(sound_buffer, (size_t)samples);

    nes_audio_cb_count++;
    if ((nes_audio_cb_count % 1200) == 0) {
        size_t used;
        pthread_mutex_lock(&nes_audio_lock);
        used = nes_audio_rb_used_nolock();
        pthread_mutex_unlock(&nes_audio_lock);
        DEBUG_PRINT("audio-stats cb=%llu epipe=%llu estr=%llu err=%llu rb_used=%zu rb_drop=%llu rb_under=%llu\n",
                    nes_audio_cb_count, nes_epipe_count, nes_estrpipe_count, nes_write_err_count,
                    used, nes_rb_drop_count, nes_rb_under_count);
    }
}

int InfoNES_IsPaused(void)
{
    return nes_pause_flag != 0;
}

void InfoNES_SetPause(int pause)
{
    nes_pause_flag = pause ? 1 : 0;
    APU_Mute = nes_pause_flag ? 1 : 0;
    if (nes_pause_flag) {
        if (pcm) {
            snd_pcm_drop(pcm);
        }
        if (nes_audio_rb) {
            pthread_mutex_lock(&nes_audio_lock);
            nes_audio_rb_in = 0;
            nes_audio_rb_out = 0;
            pthread_mutex_unlock(&nes_audio_lock);
        }
    } else {
        if (pcm) {
            snd_pcm_prepare(pcm);
        }
    }
}

int InfoNES_ShouldQuit(void)
{
    return nes_quit_flag != 0;
}

void InfoNES_RequestQuit(int quit)
{
    nes_quit_flag = quit ? 1 : 0;
}

/* Print system message */
void InfoNES_MessageBox(char *pszMsg, ...){}

#endif  /*LV_USE_100ASK_NES*/

