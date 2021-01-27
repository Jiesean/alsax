/* resamplesubs.c - sampling rate conversion subroutines */
// Altered version
#include "resample_usc_ex.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

//#define IBUFFSIZE 368                         /* Input buffer size */
//#define OBUFFSIZE 186                         //(IBUFFSIZE*factor+2.0);

#include "sys_plat.h"
#include "smallfilter.h"
//#include "largefilter.h"    
#include "filterkit_usc.h"

// int IBUFFSIZE = 0;
// int OBUFFSIZE = 0;
// 
// float factor = 0.5;              /* factor = outSampleRate/inSampleRate */
// BOOL interpFilt = 1;
// int nChans = 1;
// 
// UHWORD LpScl = SMALL_FILTER_SCALE;               /* Unity-gain scale factor */
// UHWORD Nwing = SMALL_FILTER_NWING;               /* Filter table size */
// UHWORD Nmult = SMALL_FILTER_NMULT;               /* Filter length for up-conversions */
// HWORD *Imp = SMALL_FILTER_IMP;               /* Filter coefficients */
// HWORD *ImpD= SMALL_FILTER_IMPD;              /* ImpD[n] = Imp[n+1]-Imp[n] */
// 
// UHWORD Xoff;
// UHWORD Nx;
// 
// UWORD Time, Time2;
// UHWORD Xp, Ncreep, Xread;
// 
// HWORD *gX1, *gY1, *gX2, *gY2;

//HWORD X1[IBUFFSIZE], Y1[OBUFFSIZE];
//HWORD X2[IBUFFSIZE], Y2[OBUFFSIZE];



#ifdef DEBUG
static int pof = 0;             /* positive overflow count */
static int nof = 0;             /* negative overflow count */
#endif

static HWORD WordToHword(WORD v, int scl)
{
    HWORD out;
    WORD llsb = (1<<(scl-1));
    v += llsb;          /* round */
    v >>= scl;
    if (v>MAX_HWORD) {
#ifdef DEBUG
        if (pof == 0)
          fprintf(stderr, "*** resample: sound sample overflow\n");
        else if ((pof % 10000) == 0)
          fprintf(stderr, "*** resample: another ten thousand overflows\n");
        pof++;
#endif
        v = MAX_HWORD;
    } else if (v < MIN_HWORD) {
#ifdef DEBUG
        if (nof == 0)
          fprintf(stderr, "*** resample: sound sample (-) overflow\n");
        else if ((nof % 1000) == 0)
          fprintf(stderr, "*** resample: another thousand (-) overflows\n");
        nof++;
#endif
        v = MIN_HWORD;
    }   
    out = (HWORD) v;
    return out;
}

/* Sampling rate up-conversion only subroutine;
 * Slightly faster than down-conversion;
 */
static int SrcUp(HWORD X[], HWORD Y[], float factor, UWORD *Time,
                 UHWORD Nx, UHWORD Nwing, UHWORD LpScl,
                 const HWORD Imp[], const HWORD ImpD[], BOOL Interp)
{
    HWORD *Xp, *Ystart;
    WORD v;
    
    float dt;                  /* Step through input signal */ 
    UWORD dtb;                  /* Fixed-point version of Dt */
    UWORD endTime;              /* When Time reaches EndTime, return to user */
    
    dt = 1.0/factor;            /* Output sampling period */
    dtb = dt*(1<<Np) + 0.5;     /* Fixed-point representation */
    
    Ystart = Y;
    endTime = *Time + (1<<Np)*(WORD)Nx;
    while (*Time < endTime)
    {
        Xp = &X[*Time>>Np];      /* Ptr to current input sample */
        /* Perform left-wing inner product */
        v = FilterUp(Imp, ImpD, Nwing, Interp, Xp, (HWORD)(*Time&Pmask),-1);
        /* Perform right-wing inner product */
        v += FilterUp(Imp, ImpD, Nwing, Interp, Xp+1, 
		      /* previous (triggers warning): (HWORD)((-*Time)&Pmask),1); */
                      (HWORD)((((*Time)^Pmask)+1)&Pmask),1);
        v >>= Nhg;              /* Make guard bits */
        v *= LpScl;             /* Normalize for unity filter gain */
        *Y++ = WordToHword(v,NLpScl);   /* strip guard bits, deposit output */
        *Time += dtb;           /* Move to next sample by time increment */
    }
    return (Y - Ystart);        /* Return the number of output samples */
}


/* Sampling rate conversion subroutine */

static int SrcUD(HWORD X[], HWORD Y[], float factor, UWORD *Time,
                 UHWORD Nx, UHWORD Nwing, UHWORD LpScl,
                 const HWORD Imp[], const HWORD ImpD[], BOOL Interp)
{
    HWORD *Xp, *Ystart;
    WORD v;
    
    float dh;                  /* Step through filter impulse response */
    float dt;                  /* Step through input signal */
    UWORD endTime;              /* When Time reaches EndTime, return to user */
    UWORD dhb, dtb;             /* Fixed-point versions of Dh,Dt */
    
    dt = 1.0/factor;            /* Output sampling period */
    dtb = dt*(1<<Np) + 0.5;     /* Fixed-point representation */
    
    dh = MIN(Npc, factor*Npc);  /* Filter sampling period */
    dhb = dh*(1<<Na) + 0.5;     /* Fixed-point representation */
    
    Ystart = Y;
    endTime = *Time + (1<<Np)*(WORD)Nx;
    while (*Time < endTime)
    {
        Xp = &X[*Time>>Np];     /* Ptr to current input sample */
        v = FilterUD(Imp, ImpD, Nwing, Interp, Xp, (HWORD)(*Time&Pmask),
                     -1, dhb);  /* Perform left-wing inner product */
        v += FilterUD(Imp, ImpD, Nwing, Interp, Xp+1, 
		      /* previous (triggers warning): (HWORD)((-*Time)&Pmask), */
                      (HWORD)((((*Time)^Pmask)+1)&Pmask),
                      1, dhb);  /* Perform right-wing inner product */
        v >>= Nhg;              /* Make guard bits */
        v *= LpScl;             /* Normalize for unity filter gain */
        *Y++ = WordToHword(v,NLpScl);   /* strip guard bits, deposit output */
        *Time += dtb;           /* Move to next sample by time increment */
    }
    return (Y - Ystart);        /* Return the number of output samples */
}


static int err_ret(char *s)
{
    //fprintf(stderr,"resample: %s \n\n",s); /* Display error message  */
    return -1;
}

void* UscDspReSampleInit(float factor, int data_len)
{
  int tmp;
  Resample *res = (Resample*)WK_MALLOC(sizeof(Resample));
  if(res == NULL)
    return NULL;
  res->factor = factor;//0.5;              /* factor = outSampleRate/inSampleRate */
  res->interpFilt = 1;
  res->nChans = 1;

  res->LpScl = SMALL_FILTER_SCALE;               /* Unity-gain scale factor */
  res->Nwing = SMALL_FILTER_NWING;               /* Filter table size */
  res->Nmult = SMALL_FILTER_NMULT;               /* Filter length for up-conversions */
  res->Imp = SMALL_FILTER_IMP;               /* Filter coefficients */
  res->ImpD= SMALL_FILTER_IMPD;              /* ImpD[n] = Imp[n+1]-Imp[n] */

  /* Account for increased filter gain when using factors less than 1 */
  if (res->factor < 1)
    res->LpScl = res->LpScl*res->factor + 0.5;

  /* Calc reach of LP filter wing & give some creeping room */
  res->Xoff = ((res->Nmult+1)/2.0) * MAX(1.0,1.0/res->factor) + 10;

  res->IBUFFSIZE = data_len + 2*res->Xoff;//res->IBUFFSIZE = 320 + 2*res->Xoff;
  res->OBUFFSIZE = (res->IBUFFSIZE*res->factor+2.0);

  if (res->IBUFFSIZE < 2*res->Xoff)      /* Check input buffer size */
  {
    WK_FREE(res);
    return NULL;
  }

  res->Nx = res->IBUFFSIZE - 2*res->Xoff;     /* # of samples to process each iteration */

  res->Xp = res->Xoff;                  /* Current "now"-sample pointer for input */
  res->Xread = res->Xoff;               /* Position in input array to read into */
  res->Time = (res->Xoff<<Np);          /* Current-time pointer for converter */  

  res->gX1 = (HWORD*)WK_MALLOC(res->IBUFFSIZE * sizeof(HWORD));
  memset(res->gX1, 0, res->IBUFFSIZE * sizeof(HWORD));
  res->gX2 = NULL;//(HWORD*)WK_MALLOC(IBUFFSIZE * sizeof(HWORD));
  //memset(gX2, 0, IBUFFSIZE * sizeof(HWORD));
  res->gY1 = (HWORD*)WK_MALLOC(res->OBUFFSIZE * sizeof(HWORD));
  memset(res->gY1, 0, res->OBUFFSIZE * sizeof(HWORD));
  res->gY2 = NULL;//(HWORD*)WK_MALLOC(OBUFFSIZE * sizeof(HWORD));
  //memset(gY2, 0, OBUFFSIZE * sizeof(HWORD));
  res->intmp = (short *)malloc(sizeof(short)*data_len);
  tmp = data_len*factor+0.5;
  res->outtmp = (short *)malloc(sizeof(short)*(tmp+10));
  return res;
}

void UscDspReSampleUnInit(void* handle)
{
  Resample *res = (Resample*)handle;
  if(res->gX1)
    WK_FREE(res->gX1);
  if(res->gX2)
    WK_FREE(res->gX1);
  if(res->gY1)
    WK_FREE(res->gY1);
  if(res->gY2)
    WK_FREE(res->gY2);

  free(res->intmp);
  free(res->outtmp);

  WK_FREE(res);
}

void UscDspReSampleReset(void* handle)
{
  Resample *res = (Resample*)handle;
  if(res == NULL)
    return;
  memset(res->gX1, 0, res->IBUFFSIZE * sizeof(HWORD));
  memset(res->gY1, 0, res->OBUFFSIZE * sizeof(HWORD));
}

unsigned int UscDspReSamplePush(void* handle, const short *in1, const short *in2, short *out1, short *out2)
{
  Resample *res = (Resample*)handle;
  int IBUFFSIZE = res->IBUFFSIZE;
  int OBUFFSIZE = res->OBUFFSIZE;

  float factor = res->factor;              /* factor = outSampleRate/inSampleRate */
  BOOL interpFilt = res->interpFilt;
  int nChans = res->nChans;

  UHWORD LpScl = res->LpScl;               /* Unity-gain scale factor */
  UHWORD Nwing = res->Nwing;               /* Filter table size */
  //UHWORD Nmult = res->Nmult;               /* Filter length for up-conversions */
  const HWORD *Imp = res->Imp;               /* Filter coefficients */
  const HWORD *ImpD= res->ImpD;              /* ImpD[n] = Imp[n+1]-Imp[n] */

  UHWORD Xoff = res->Xoff;
  UHWORD Nx = res->Nx;

  UWORD Time = res->Time, Time2 = res->Time2;
  UHWORD Xp = res->Xp, Ncreep = res->Ncreep, Xread = res->Xread;
  HWORD *X1 = res->gX1, *Y1 = res->gY1, *X2 = res->gX2, *Y2 = res->gY2;

  UHWORD Nout;
  int i;
  
  memcpy(X1 + 2*Xoff, in1, sizeof(HWORD) * Nx);

  /* Resample stuff in input buffer */
  Time2 = Time;
  if (factor >= 1) {      /* SrcUp() is faster if we can use it */
    Nout=SrcUp(X1,Y1,factor,&Time,Nx,Nwing,LpScl,Imp,ImpD,interpFilt);
    if (nChans==2)
      Nout=SrcUp(X2,Y2,factor,&Time2,Nx,Nwing,LpScl,Imp,ImpD,
      interpFilt);
  }
  else {
    Nout=SrcUD(X1,Y1,factor,&Time,Nx,Nwing,LpScl,Imp,ImpD,interpFilt);
    if (nChans==2)
      Nout=SrcUD(X2,Y2,factor,&Time2,Nx,Nwing,LpScl,Imp,ImpD,
      interpFilt);
  }

  Time -= (Nx<<Np);       /* Move converter Nx samples back in time */
  Xp += Nx;               /* Advance by number of samples processed */
  Ncreep = (Time>>Np) - Xoff; /* Calc time accumulation in Time */
  if (Ncreep) {
    Time -= (Ncreep<<Np);    /* Remove time accumulation */
    Xp += Ncreep;            /* and add it to read pointer */
  }
  for (i=0; i<IBUFFSIZE-Xp+Xoff; i++) { /* Copy part of input signal */
    X1[i] = X1[i+Xp-Xoff]; /* that must be re-used */
    if (nChans==2)
      X2[i] = X2[i+Xp-Xoff]; /* that must be re-used */
  }
  
  Xread = i;              /* Pos in input buff to read new data into */
  Xp = Xoff;  

  if (Nout > OBUFFSIZE) /* Check to see if output buff overflowed */
    return err_ret("Output array overflow");

  if (nChans==1) {
    for (i = 0; i < Nout; i++)
      out1[i] = Y1[i];
  } else {
    for (i = 0; i < Nout; i++) {
      out1[i] = Y1[i];
      out2[i] = Y2[i];
    }
  }
  //fwrite(Y1, sizeof(short), Nout, outfd);
  //printf(".");  fflush(stdout);

  return Nout;
}

void downsample(void *handle,short *In,int In_len, short *Out)
{
	int i,ch,count_out;
	Resample *dowsam1 = (Resample *)handle;

	count_out = UscDspReSamplePush(dowsam1,In,0,dowsam1->outtmp,0);
	memcpy(Out,dowsam1->outtmp,sizeof(short)*count_out);
}


/*void downsample(void *handle1,void *handle2,void *handle3,void *handle4, short *In,int In_len, short *Out)
{
	int i,ch,count_out;
	Resample *dowsam1 = (Resample *)handle1;
	Resample *dowsam2 = (Resample *)handle2;
	Resample *dowsam3 = (Resample *)handle3;
	Resample *dowsam4 = (Resample *)handle4;
	//5,3,1,7
	for(i=0; i<In_len; i++)
	{
		dowsam1->intmp[i] = In[8*i+4];
	}
	count_out = UscDspReSamplePush(dowsam1,dowsam1->intmp,0,dowsam1->outtmp,0);
	for(i=0; i<count_out; i++)
	{
		Out[4*i+0] = dowsam1->outtmp[i];
	}

	for(i=0; i<In_len; i++)
	{
		dowsam2->intmp[i] = In[8*i+2];
	}
	count_out = UscDspReSamplePush(dowsam2,dowsam2->intmp,0,dowsam2->outtmp,0);
	for(i=0; i<count_out; i++)
	{
		Out[4*i+1] = dowsam2->outtmp[i];
	}

	for(i=0; i<In_len; i++)
	{
		dowsam3->intmp[i] = In[8*i+0];
	}
	count_out = UscDspReSamplePush(dowsam3,dowsam3->intmp,0,dowsam3->outtmp,0);
	for(i=0; i<count_out; i++)
	{
		Out[4*i+2] = dowsam3->outtmp[i];
	}

	for(i=0; i<In_len; i++)
	{
		dowsam4->intmp[i] = In[8*i+6];
	}
	count_out = UscDspReSamplePush(dowsam4,dowsam4->intmp,0,dowsam4->outtmp,0);
	for(i=0; i<count_out; i++)
	{
		Out[4*i+3] = dowsam4->outtmp[i];
	}
}*/
