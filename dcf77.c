/*
 * (c) 2003 maZZoo_SPIDER_MONKEY_gmx_DOT_de
 *
 *  License: GPLv2, see http://www.gnu.org/licenses/gpl.txt
 *
 *
 *  transmits DCF77 time information via your monitor/LCD 
 *
 */



  /* play with the values here if you have no success */
#define PWM_WIDTH  8  /* [pixels] seen working for 3, 4, 8 */
#define PWM_LOW    4  /* [pixels] 0 < PWM_LOW <= 0.5*PWM_WIDTH */

//#define PAINT_FULL_PWM


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/xf86vmode.h>

#include <SDL/SDL.h>

#ifndef uint
#  define uint unsigned int
#endif
#ifndef ulong
#  define ulong unsigned long
#endif

void usage (char * n){
  fprintf(stderr, "Usage   : %s [options]\n", n);
  fprintf(stderr, "Options :\n");
  fprintf(stderr, "  -v       verbose\n");
  fprintf(stderr, "  -f       force. don't run fps performance test\n");
  fprintf(stderr, "  -y nn    set year    to nn\n");
  fprintf(stderr, "  -M nn    set month   to nn\n");
  fprintf(stderr, "  -w n     set weekday to n\n");
  fprintf(stderr, "  -d nn    set day     to nn\n");
  fprintf(stderr, "  -h nn    set hour    to nn\n");
  fprintf(stderr, "  -m nn    set minute  to nn\n");
}

int countbits_ul(unsigned long v){
  int ret = 0;
  unsigned int i=0;
  
  for (; i<8*sizeof(unsigned long); i++){
    if (v&1) ret++;
    v>>=1;
  }
  return ret;
}

static inline int parity_min_hour(uint lo, uint hi){
  int ret = 0; /* even parity */
  ret ^= (lo & 1) ? 1:0;
  ret ^= (lo & 2) ? 1:0;
  ret ^= (lo & 4) ? 1:0;
  ret ^= (lo & 8) ? 1:0;
  ret ^= (hi & 1) ? 1:0;
  ret ^= (hi & 2) ? 1:0;
  ret ^= (hi & 4) ? 1:0; /* will be zero in case of hours */

  return ret;
}

static inline int parity_day_thru_year(uint daylo,   uint dayhi, uint weekday,
				uint monthlo, uint monthhi,
				uint yearlo,  uint yearhi ){
  int ret = 0; /* even parity */
  ret ^= (daylo & 1) ? 1:0;
  ret ^= (daylo & 2) ? 1:0;
  ret ^= (daylo & 4) ? 1:0;
  ret ^= (daylo & 8) ? 1:0;
  ret ^= (dayhi & 1) ? 1:0;
  ret ^= (dayhi & 2) ? 1:0;
  
  ret ^= (weekday & 1) ? 1:0;
  ret ^= (weekday & 2) ? 1:0;
  ret ^= (weekday & 4) ? 1:0;
  
  ret ^= (monthlo & 1) ? 1:0;
  ret ^= (monthlo & 2) ? 1:0;
  ret ^= (monthlo & 4) ? 1:0;
  ret ^= (monthlo & 8) ? 1:0;
  ret ^= (monthhi & 1) ? 1:0;

  ret ^= (yearlo & 1) ? 1:0;
  ret ^= (yearlo & 2) ? 1:0;
  ret ^= (yearlo & 4) ? 1:0;
  ret ^= (yearlo & 8) ? 1:0;
  ret ^= (yearhi & 1) ? 1:0;
  
  return ret;
}

void setTimestring(char t[], uint min, uint hour, uint day, uint weekday, uint month, uint year){
  
  char * pt;
  uint minlow;
  uint minhi;
  uint hourlow;
  uint hourhi;
  uint daylow;
  uint dayhi;
  uint monthlow;
  uint monthhi;
  uint yearlow;
  uint yearhi;

  memset( t, '0', 60*sizeof(char));

  t[18] = '1'; // CET
  t[20] = '1';

  pt = t+21;
  
  minlow = min%10;
  
  *pt += (minlow & 1)>>0; pt++; // Minute Einer
  *pt += (minlow & 2)>>1; pt++;
  *pt += (minlow & 4)>>2; pt++;
  *pt += (minlow & 8)>>3; pt++;
  
  minhi = min - minlow;
  minhi /= 10;
  
  *pt += (minhi & 1)>>0; pt++; // Minute Zehner
  *pt += (minhi & 2)>>1; pt++;
  *pt += (minhi & 4)>>2; pt++;
  
  *pt += parity_min_hour(minlow, minhi); pt++; // P1
  
  hourlow = hour%10;
  
  *pt += (hourlow & 1)>>0; pt++; // Stunde Einer
  *pt += (hourlow & 2)>>1; pt++;
  *pt += (hourlow & 4)>>2; pt++;
  *pt += (hourlow & 8)>>3; pt++;
  
  hourhi = hour - hourlow;
  hourhi /= 10;
  
  *pt += (hourhi & 1)>>0; pt++; // Stunde Zehner
  *pt += (hourhi & 2)>>1; pt++;
  
  *pt += parity_min_hour(hourlow, hourhi); pt++; // P2
  
  daylow = day%10;
  
  *pt += (daylow & 1)>>0; pt++; // Tag Einer
  *pt += (daylow & 2)>>1; pt++;
  *pt += (daylow & 4)>>2; pt++;
  *pt += (daylow & 8)>>3; pt++;
  
  dayhi = day - daylow;
  dayhi /= 10;
  
  *pt += (dayhi & 1)>>0; pt++; // Tag Zehner
  *pt += (dayhi & 2)>>1; pt++;
  
  *pt += (weekday & 1)>>0; pt++; // Wochentag
  *pt += (weekday & 2)>>1; pt++;
  *pt += (weekday & 4)>>2; pt++;
  
  monthlow = month%10;
  
  *pt += (monthlow & 1)>>0; pt++; // Monat Einer
  *pt += (monthlow & 2)>>1; pt++;
  *pt += (monthlow & 4)>>2; pt++;
  *pt += (monthlow & 8)>>3; pt++;
  
  monthhi = month - monthlow;
  monthhi /= 10;
  
  *pt += (monthhi & 1)>>0; pt++; // Monat Zehner
  
  yearlow = year%10;

  *pt += (yearlow & 1)>>0; pt++; // Jahr Einer
  *pt += (yearlow & 2)>>1; pt++;
  *pt += (yearlow & 4)>>2; pt++;
  *pt += (yearlow & 8)>>3; pt++;
  
  yearhi = year - yearlow;
  yearhi /= 10;
  
  *pt += (yearhi & 1)>>0; pt++; // Jahr Einer
  *pt += (yearhi & 2)>>1; pt++;
  *pt += (yearhi & 4)>>2; pt++;
  *pt += (yearhi & 8)>>3; pt++;

  *pt += parity_day_thru_year(daylow, dayhi, weekday, monthlow, monthhi, yearlow, yearhi); pt++; // P3
  
  *pt = 'S'; pt++; // sync-bit
}

void DrawPixel(SDL_Surface *screen, int x, int y,
	       Uint8 R,
	       Uint8 G,
	       Uint8 B)
{
  Uint32 color = SDL_MapRGB(screen->format, R, G, B);
  
  switch (screen->format->BytesPerPixel) {
  case 1: { /* vermutlich 8 Bit */
    Uint8 *bufp;
    
    bufp = (Uint8 *)screen->pixels + y*screen->pitch + x;
    *bufp = color;
  }
    break;
    
  case 2: { /* vermutlich 15 Bit oder 16 Bit */
    Uint16 *bufp;
    
    bufp = (Uint16 *)screen->pixels + y*screen->pitch/2 + x;
    *bufp = color;
  }
    break;
    
  case 3: { /* langsamer 24-Bit-Modus, selten verwendet */
    Uint8 *bufp;
    
    bufp = (Uint8 *)screen->pixels + y*screen->pitch + x * 3;
    if(SDL_BYTEORDER == SDL_LIL_ENDIAN) {
      bufp[0] = color;
      bufp[1] = color >> 8;
      bufp[2] = color >> 16;
    } else {
      bufp[2] = color;
      bufp[1] = color >> 8;
      bufp[0] = color >> 16;
    }
  }
    break;
    
  case 4: { /* vermutlich 32 Bit */
    Uint32 *bufp;
    
    bufp = (Uint32 *)screen->pixels + y*screen->pitch/4 + x;
    *bufp = color;
  }
    break;
  }
}

void paintDCFscreen(SDL_Surface * s, XF86VidModeModeLine * ML, int greyflag, int lambda){
  
  int    FIXMEgrey = 255;      /* still testing ... */
  int    pc        = 0;        /* pixelcounter */

  int j, k;

  if ( SDL_MUSTLOCK(s) )
    SDL_LockSurface(s);


  for (j=0; j<ML->vtotal; j++){
    for (k=0; k<ML->htotal; k++){
      
      if ((j<ML->vdisplay)&&(k<ML->hdisplay)){/* SDL Parachute */

	if (pc<lambda){ /* PWM-grey for grey (s25), white for s100 */
	
	  if (greyflag){
	    if((pc%PWM_WIDTH) >= PWM_LOW)
	      DrawPixel(s , k, j, FIXMEgrey, FIXMEgrey, FIXMEgrey);
	    else
	      DrawPixel(s , k, j, 0,0,0);
	  }else
	    DrawPixel(s, k, j, 255, 255, 255); // FIXME use sinewave
	
	  
	}else{          /* 1-PWM-grey for grey (s25), black for s100 */
	  
	  if ((j<ML->vdisplay)&&(k<ML->hdisplay)){/* SDL Parachute */
	    if (greyflag){
	      if((pc%PWM_WIDTH) < PWM_LOW){
#ifdef PAINT_FULL_PWM
		DrawPixel(s , k, j, FIXMEgrey, FIXMEgrey, FIXMEgrey);
#endif
		;
	      }else
		DrawPixel(s , k, j, 0, 0, 0);
	    }else
	      DrawPixel(s, k, j, 0, 0, 0);
	  }
	  
	  
	}
      }
      
      pc++;
      if (pc>(lambda<<1))
	pc = 0;

    }
  }

  if ( SDL_MUSTLOCK(s) )
    SDL_UnlockSurface(s);
}

void initMethod1Screens(SDL_Surface * s25, SDL_Surface * s100, XF86VidModeModeLine * ML, int lambda){
  paintDCFscreen(s25,  ML, 1, lambda);
  paintDCFscreen(s100, ML, 0, lambda);
}

static inline void sleepon(struct timeval * tlast,
			   long wait_us){

  static struct timeval tnow;
  static struct timeval twait;

  gettimeofday(&tnow, NULL);

  twait.tv_sec  = tnow.tv_sec  - tlast->tv_sec;
  twait.tv_usec = wait_us - tnow.tv_usec + tlast->tv_usec;

  while (twait.tv_usec < 0){
    twait.tv_usec += 1000000L;
    twait.tv_sec--;
  }

  while (twait.tv_usec > 1000000L){
    twait.tv_usec -= 1000000L;
    twait.tv_sec--;
  }

  select(0, 0, 0, 0, &twait);

}

int main(int argc, char ** argv){

  const int       fC = 77500;  /* carrier-frequency */ 

  char *display_name = getenv("DISPLAY");
  Display *D;
  
  int              S;          /* Screen */

  XF86VidModeModeLine ML;      /* currently set modeline */

  int              XFdotclock; /* in kHz */
  long             dotclock;   /* in Hz  */

  int              depth;      /* bits per pixel */

  XVisualInfo *XVI;
  XVisualInfo XVItmpl;
  int nXVI;
  int nR, nG, nB;

  double lambdaS;              /* pixels on screen for one fC-cycle */
  int lambda;                  /* pixels for white */

  /*  for DCF77 we need only two different screens (+ 1 target screen)
   *  one with 100% amplitude (black-white), and
   *  one with 25% amplitude (black-25%grey)
   */
  SDL_Surface * screen, * screen100, * screen25;

  char timestring[61];         /* time information for one minute */
                               /* sequence of '1' and '0' */

  char *p;
  SDL_Event event;

  int opt_force   = 0;
  int opt_verbose = 0;
  uint currmin = 23;           /* adjust the default time here */
  uint hour    = 23;
  uint day     = 1;
  uint weekday = 1;
  uint month   = 12;
  uint year    = 4;            /* 0-99 */

  struct timeval tv_last;
  struct timeval tv_dbg;

  char c;
  int  i;
  long  fps_time_us;
  float fps;

  while ( (c = getopt(argc, argv, "vfy:M:w:d:h:m:")) != EOF ){
    switch(c){
    case 'v':
      opt_verbose = 1;
      break;
    case 'f':
      opt_force = 1;
      break;
    case 'y':
      year = atoi(optarg);
      break;
    case 'M':
      month = atoi(optarg);
      break;
    case 'w':
      weekday = atoi(optarg);
      break;
    case 'd':
      day = atoi(optarg);
      break;
    case 'h':
      hour = atoi(optarg);
      break;
    case 'm':
      currmin = atoi(optarg);
      break;
    default:
      usage(argv[0]);
      exit(1);
    }
  }

  
  D = XOpenDisplay(display_name);

  if (!D)
    D = XOpenDisplay(NULL); /* opens default display, same-same as above I guess */

  if (!D) {
    fprintf(stderr, "Cannot connect to X server\n");
    exit (-1);
  }
  
  S = DefaultScreen(D);
  
  XF86VidModeGetModeLine(D, S, &XFdotclock, &ML);
  
  dotclock = 1000 * XFdotclock; /* kHz -> Hz */

  depth = DefaultDepth(D, S);

  XVItmpl.screen = S;
  XVI = XGetVisualInfo(D, VisualNoMask, &XVItmpl, &nXVI);

  nR = countbits_ul(XVI->red_mask);
  nG = countbits_ul(XVI->green_mask);
  nB = countbits_ul(XVI->blue_mask);

  lambdaS = (double) dotclock / fC; /* pixels on screen for one fC-cycle */
  lambda = (int) (lambdaS/2.0);     /* pixels for white */

  if (SDL_Init(SDL_INIT_VIDEO)){
    fprintf(stderr, "Could not init SDL : %s\n", SDL_GetError());
    exit (1);
  }
  atexit(SDL_Quit);
  
  

  screen = SDL_SetVideoMode(ML.hdisplay, ML.vdisplay, depth, SDL_FULLSCREEN);
  
  if (!screen){
    fprintf(stderr, "Could not set SDL video mode %dx%dx%d: %s\n",
	   ML.hdisplay, ML.vdisplay, depth, SDL_GetError());
    exit (1);
  }
  
  SDL_WM_SetCaption("broadcasting DCF77 time via screen",
		    "DCF77");



  screen25  = SDL_CreateRGBSurface(screen->flags, screen->w, screen->h,
				   depth,
				   screen->format->Rmask,
				   screen->format->Gmask,
				   screen->format->Bmask,
				   screen->format->Amask);
  if (!screen25){
    fprintf(stderr, "Could not set SDL video mode %dx%dx%d: %s\n",
	   ML.hdisplay, ML.vdisplay, depth, SDL_GetError());
    exit (1);
  }


  screen100 = SDL_CreateRGBSurface(screen->flags, screen->w, screen->h,
				   depth,
				   screen->format->Rmask,
				   screen->format->Gmask,
				   screen->format->Bmask,
				   screen->format->Amask);
    if (!screen100){
    fprintf(stderr, "Could not set SDL video mode %dx%dx%d: %s\n",
	   ML.hdisplay, ML.vdisplay, depth, SDL_GetError());
    exit (1);
  }





  if (opt_verbose){
    printf("dotclock : %ld Hz\n", dotclock);
    printf("display  : %dx%d\n", ML.hdisplay  , ML.vdisplay );
    printf("total    : %dx%d\n", ML.htotal    , ML.vtotal   );
    printf("hsync    : %d-%d\n", ML.hsyncstart, ML.hsyncend );
    printf("vsync    : %d-%d\n", ML.vsyncstart, ML.vsyncend );

/* Mode flags */
#define V_PHSYNC        0x001 
#define V_NHSYNC        0x002
#define V_PVSYNC        0x004
#define V_NVSYNC        0x008
#define V_INTERLACE     0x010
#define V_DBLSCAN       0x020
#define V_CSYNC         0x040
#define V_PCSYNC        0x080
#define V_NCSYNC        0x100

    /* Print out the flags (if any) */
    printf("flags    :");
    if (ML.flags & V_PHSYNC)    printf(" +hsync");
    if (ML.flags & V_NHSYNC)    printf(" -hsync");
    if (ML.flags & V_PVSYNC)    printf(" +vsync");
    if (ML.flags & V_NVSYNC)    printf(" -vsync");
    if (ML.flags & V_INTERLACE) printf(" interlace");
    if (ML.flags & V_CSYNC)     printf(" composite");
    if (ML.flags & V_PCSYNC)    printf(" +csync");
    if (ML.flags & V_NCSYNC)    printf(" -csync");
    if (ML.flags & V_DBLSCAN)   printf(" doublescan");
    printf("\n");

    printf("depth    : %d bits per pixel\n", depth);

    printf("R-mask   : 0x%8.8lx\n", XVI->red_mask);
    printf("G-mask   : 0x%8.8lx\n", XVI->green_mask);
    printf("B-mask   : 0x%8.8lx\n", XVI->blue_mask);

    if ( (XVI->red_mask   > XVI->green_mask) &&
	 (XVI->green_mask > XVI->blue_mask ) ){
      printf("format   : | R | G | B |\n");
      printf("           |%2d |%2d |%2d |\n", nR, nG, nB);
    }else{
      if ( (XVI->blue_mask  > XVI->green_mask) &&
	   (XVI->green_mask > XVI->red_mask  ) ){
	printf("format   : | B | G | R |\n");
	printf("           |%2d |%2d |%2d |\n", nB, nG, nR);
      }else{
	printf("UNKNOWN COLOR FORMAT! (probably you are running 256 colors which is currently not supported)\n");
	exit (1);
      }
    }

    printf("lambdaS  : %f pixels per %d Hz cycle\n", lambdaS, fC);
    printf("pitch    : %d\n", screen->pitch);
  }


  initMethod1Screens(screen25, screen100, &ML, lambda);
  

  if (!opt_force){
    for (i=0;i<25;i++){ /* measuring max fps */
      if (i==5) gettimeofday(&tv_dbg, NULL);
      SDL_BlitSurface(screen100, NULL, screen, NULL);
      SDL_UpdateRect (screen   , 0, 0, ML.hdisplay-1, ML.vdisplay-1);
      SDL_BlitSurface(screen25 , NULL, screen, NULL);
      SDL_UpdateRect (screen   , 0, 0, ML.hdisplay-1, ML.vdisplay-1);
    }
    gettimeofday(&tv_last, NULL);
    fps_time_us  = (tv_last.tv_sec - tv_dbg.tv_sec)*1E6;
    fps_time_us -=  tv_dbg.tv_usec;
    fps_time_us +=  tv_last.tv_usec;

    fps = 40.0*1E6/fps_time_us;
    if (opt_verbose)
      printf("fps      : %f\n", fps);
    if (fps<20.0){
      printf("ERROR: your screen doesn't perform good enough (%f fps). We need 20 fps.\n", fps);
      printf("       try reducing/changing the color depth of your screen.\n");
      printf("       You can override this test with the force-flag -f .\n");
      exit(1);
    }
  }
  
  
  timestring[60]='\0';
  
  printf("            1         2         3         4         5\n");
  printf("  012345678901234567890123456789012345678901234567890123456789\n");
  
  gettimeofday(&tv_last, NULL);

  while (1){
    
    setTimestring( timestring, currmin, hour, day, weekday, month, year);

    gettimeofday(&tv_dbg, NULL);

    printf("  %s - %2.2d:%2.2dh - %lds %ldus\n", timestring, hour, currmin,
	   tv_dbg.tv_sec, tv_dbg.tv_usec);

    p = timestring;

    while (*p){
      
//#define DBG_CARRIER 1//00000

#define METHOD_1
      
      if (*p == 'S'){ /* sync bit, second 59 */
	
	gettimeofday(&tv_last, NULL);
	sleepon(&tv_last, 500000L);
      }

      if (*p == '1'){


      /* part one of the second */
	

	gettimeofday(&tv_last, NULL);
#ifdef METHOD_1
	SDL_BlitSurface(screen100, NULL, screen, NULL);
#endif
#ifdef METHOD_2
	paintDCFscreen(screen, &ML, 0, lambda);
#endif
      	SDL_UpdateRect (screen   , 0, 0, ML.hdisplay-1, ML.vdisplay-1);
	
	
#ifndef DBG_CARRIER
	sleepon(&tv_last, 250000L);
#else
	usleep(DBG_CARRIER);
#endif
      }

      if (*p == '0'){

	gettimeofday(&tv_last, NULL);
#ifdef METHOD_1
	SDL_BlitSurface(screen100, NULL, screen, NULL);
#endif
#ifdef METHOD_2
	paintDCFscreen(screen, &ML, 0, lambda);
#endif
      	SDL_UpdateRect (screen   , 0, 0, ML.hdisplay-1, ML.vdisplay-1);


#ifndef DBG_CARRIER
	sleepon(&tv_last, 150000L);
#else
	usleep(DBG_CARRIER);
#endif
      }



      /* part two of the second */



      gettimeofday(&tv_last, NULL);
#ifdef METHOD_1
      SDL_BlitSurface(screen25, NULL, screen, NULL);
#endif
#ifdef METHOD_2
      paintDCFscreen(screen, &ML, 1, lambda);
#endif
      SDL_UpdateRect (screen   , 0, 0, ML.hdisplay-1, ML.vdisplay-1);

      if (*p == 'S'){ /* sync bit, second 59 */
	sleepon(&tv_last, 500000L);
      }

      if (*p == '1'){

#ifndef DBG_CARRIER
	sleepon(&tv_last, 750000);
#else
	usleep(DBG_CARRIER);
#endif
      }

      if (*p == '0'){

#ifndef DBG_CARRIER
	sleepon(&tv_last, 850000L);
#else
	usleep(DBG_CARRIER);
#endif
      }
      

      p++; /* next second */
      
      while(SDL_PollEvent(&event))
	if ( (event.type == SDL_MOUSEBUTTONDOWN) ||
	     (event.type == SDL_KEYDOWN        ) ||
	     (event.type == SDL_QUIT           ) ){
	  exit(0);
	}
    }

    currmin++; /* next minute */
    if (currmin==60){currmin=0;hour++;}
    if (hour   ==24){hour=0;   day++;}
    if (day    ==32){day=1;}           // FIXME there's more ...

  }
  
  return 0;
}
