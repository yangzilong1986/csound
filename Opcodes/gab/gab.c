
//  Copyright (C) 2002-2004 Gabriel Maldonado
//
//  The gab library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  The gab library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with the gab library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
//  02111-1307 USA


// Ported to csound5 by: Andres Cabrera
// This file includes the opcodes from newopcodes.c
// TODO: Check if the authors (Peter Neubaeker and Jens Groh) are correct

//printi removed? I can't find the corresponding OENTRY- Left them commented out
//how should exitnow be safely implemented?
//Check the zak opcodes
//changed some comments to c-style
//Check csystem, exitnow
//Check other opcodes commented out in Oentry

#include "csdl.h"
#include "gab.h"
#include <math.h>


#ifdef MAKEDLL
//#define PUBLIC __declspec(dllexport)
#define DIR_SEP '\\'
#else
//#define PUBLIC
#define DIR_SEP '/'
#endif

int krsnsetx(ENVIRON *csound_, KRESONX *p) // Gabriel Maldonado, modifies for arb order 
{
    ENVIRON *csound = p->h.insdshead->csound;
    int scale;
    p->scale = scale = (int) *p->iscl;
    if((p->loop = (int) (*p->ord + .5)) < 1) p->loop = 4; /*default value*/
    if (!*p->istor && (p->aux.auxp == NULL ||
                      (int)(p->loop*2*sizeof(MYFLT)) > p->aux.size))
      auxalloc((long)(p->loop*2*sizeof(MYFLT)), &p->aux);
    p->yt1 = (MYFLT*)p->aux.auxp; p->yt2 = (MYFLT*)p->aux.auxp + p->loop;
    if (scale && scale != 1 && scale != 2) {
      sprintf(errmsg,"illegal reson iscl value, %f",*p->iscl);
      initerror(errmsg);
    }
    p->prvcf = p->prvbw = -100.0f;

    if (!(*p->istor)) {
      int j;
      for (j=0; j< p->loop; j++) p->yt1[j] = p->yt2[j] = 0.0f;
    }
    return OK;
}

int kresonx(ENVIRON *csound_, KRESONX *p) // Gabriel Maldonado, modified 
{
    ENVIRON *csound = p->h.insdshead->csound;
    int	flag = 0, j;
    MYFLT	*ar, *asig;
    MYFLT	c3p1, c3t4, omc3, c2sqr;
    MYFLT *yt1, *yt2, c1,c2,c3; 
	
    if (*p->kcf != p->prvcf) {
		p->prvcf = *p->kcf;
		p->cosf = (MYFLT) cos((double)(*p->kcf * tpidsr * ksmps));
		flag = 1;
    }
    if (*p->kbw != p->prvbw) {
		p->prvbw = *p->kbw;
		p->c3 = (MYFLT) exp((double)(*p->kbw * mtpdsr * ksmps));
		flag = 1;
    }
    if (flag) {
		c3p1 = p->c3 + 1.0f;
		c3t4 = p->c3 * 4.0f;
		omc3 = 1.0f - p->c3;
		p->c2 = c3t4 * p->cosf / c3p1;		// -B, so + below
		c2sqr = p->c2 * p->c2;
		if (p->scale == 1)
			p->c1 = omc3 * (MYFLT)sqrt(1.0 - (double)(c2sqr / c3t4));
		else if (p->scale == 2)
			p->c1 = (MYFLT)sqrt((double)((c3p1*c3p1-c2sqr) * omc3/c3p1));
		else p->c1 = 1.0f;
    }
    c1=p->c1; 
    c2=p->c2;
    c3=p->c3;
    yt1= p->yt1;
    yt2= p->yt2;
    asig = p->asig;
	ar = p->ar;
    for (j=0; j< p->loop; j++) {
		*ar = c1 * *asig + c2 * *yt1 - c3 * *yt2;
		*yt2 = *yt1;
		*yt1 = *ar;
		yt1++;
		yt2++;
		asig= p->ar;
    }
    return OK;
}



//--------------------------------

int metro_set(ENVIRON *csound, METRO *p)
{
    //ENVIRON *csound = p->h.insdshead->csound;
    double	phs = *p->iphs;
    long  longphs;
	
	if (phs >= 0.0) {
		if ((longphs = (long)phs))
			csound->Printf("metro:init phase truncation\n");
		p->curphs = phs - (MYFLT)longphs;
	}
	p->flag=1;
	return OK;
}


int metro(ENVIRON *csound, METRO *p)
{
    //ENVIRON *csound = p->h.insdshead->csound;
    double	phs= p->curphs;
	if(phs == 0.0 && p->flag) {
		*p->sr = FL(1.0);
		p->flag = 0;
	}
	else if ((phs += *p->xcps * onedkr) >= 1.0) {
		*p->sr = FL(1.0);
		phs -= 1.0;
		p->flag = 0;
	}
	else 
		*p->sr = FL(0.0);
	p->curphs = phs;
	return OK;
}



/////////////////////////////////////////////

int fastab_set(ENVIRON *csound,FASTAB *p)
{
	FUNC *ftp;
	if ((ftp = ftnp2find(csound, p->xfn)) == NULL) {
		  initerror("fastab: incorrect table number");
          return NOTOK;
	}
	p->table = ftp->ftable;
	p->xmode = (int) *p->ixmode;
	if (p->xmode)
		p->xbmul = (MYFLT) ftp->flen;
	else 
		p->xbmul = FL(1.0);
	return OK;
 
}

int fastabw (ENVIRON *csound,FASTAB *p)
{
	int	nsmps = ksmps;
	MYFLT *tab = p->table;
	MYFLT *rslt = p->rslt, *ndx = p->xndx;
	if (p->xmode) {
		do *(tab + (long) (*ndx++ * p->xbmul)) = *rslt++;
		while(--nsmps);
	}
	else {
		do *(tab + (long) *ndx++) = *rslt++;
		while(--nsmps);
	}
	return OK;
}

int fastabk(ENVIRON *csound,FASTAB *p) 
{
	if (p->xmode)
		*p->rslt =  *(p->table + (long) (*p->xndx * p->xbmul));
	else
		*p->rslt =  *(p->table + (long) *p->xndx);
	return OK;
}

int fastabkw(ENVIRON *csound,FASTAB *p) 
{
	
	if (p->xmode) 
		*(p->table + (long) (*p->xndx * p->xbmul)) = *p->rslt;		
	else
		*(p->table + (long) *p->xndx) = *p->rslt;
	return OK;
}


int fastabi(ENVIRON *csound,FASTAB *p) 
{
	
	FUNC *ftp;
	//ftp = ftfind(p->xfn);

	if ((ftp = ftnp2find(csound, p->xfn)) == NULL) {
		  initerror("tab_i: incorrect table number");
          return NOTOK;
	}
	if (*p->ixmode)
		*p->rslt =  *(ftp->ftable + (long) (*p->xndx * ftp->flen));
	else
		*p->rslt =  *(ftp->ftable + (long) *p->xndx);
	return OK;
}

int fastabiw(ENVIRON *csound,FASTAB *p) 
{
	FUNC *ftp;
	//ftp = ftfind(p->xfn);
	if ((ftp = ftnp2find(csound, p->xfn)) == NULL) {
		  initerror("tabw_i: incorrect table number");
          return NOTOK;
	}
	if (*p->ixmode)
		*(ftp->ftable + (long) (*p->xndx * ftp->flen)) = *p->rslt;
	else
		*(ftp->ftable + (long) *p->xndx) = *p->rslt;
	return OK;
}


int fastab(ENVIRON *csound,FASTAB *p)
{
	int	nsmps = ksmps;
	MYFLT *tab = p->table;
	MYFLT *rslt = p->rslt, *ndx = p->xndx;
	if (p->xmode) {
		do *rslt++ = *(tab + (long) (*ndx++ * p->xbmul));
		while(--nsmps);
	}
	else {
		do *rslt++ = *(tab + (long) *ndx++ );
		while(--nsmps);
	}
	return OK;
}

static MYFLT *tb0,*tb1,*tb2,*tb3,*tb4,*tb5,*tb6,*tb7,*tb8,*tb9,*tb10,*tb11,*tb12,*tb13,*tb14,*tb15;

#define tabMacro 	FUNC *ftp;\
	if ((ftp = ftnp2find(csound, p->ifn)) == NULL) {\
		  initerror("tab_init: incorrect table number");\
          return NOTOK;\
	}\

int tab0_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb0 = ftp->ftable; return OK;}
int tab1_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb1 = ftp->ftable; return OK;}
int tab2_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb2 = ftp->ftable; return OK;}
int tab3_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb3 = ftp->ftable; return OK;}
int tab4_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb4 = ftp->ftable; return OK;}
int tab5_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb5 = ftp->ftable; return OK;}
int tab6_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb6 = ftp->ftable; return OK;}
int tab7_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb7 = ftp->ftable; return OK;}
int tab8_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb8 = ftp->ftable; return OK;}
int tab9_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb9 = ftp->ftable; return OK;}
int tab10_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb10 = ftp->ftable; return OK;}
int tab11_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb11 = ftp->ftable; return OK;}
int tab12_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb12 = ftp->ftable; return OK;}
int tab13_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb13 = ftp->ftable; return OK;}
int tab14_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb14 = ftp->ftable; return OK;}
int tab15_init(ENVIRON *csound,TB_INIT *p) { tabMacro 	tb15 = ftp->ftable; return OK;}


int tab0(ENVIRON *csound,FASTB *p) { *p->r = tb0[(long) *p->ndx]; return OK;}
int tab1(ENVIRON *csound,FASTB *p) { *p->r = tb1[(long) *p->ndx]; return OK;}
int tab2(ENVIRON *csound,FASTB *p) { *p->r = tb2[(long) *p->ndx]; return OK;}
int tab3(ENVIRON *csound,FASTB *p) { *p->r = tb3[(long) *p->ndx]; return OK;}
int tab4(ENVIRON *csound,FASTB *p) { *p->r = tb4[(long) *p->ndx]; return OK;}
int tab5(ENVIRON *csound,FASTB *p) { *p->r = tb5[(long) *p->ndx]; return OK;}
int tab6(ENVIRON *csound,FASTB *p) { *p->r = tb6[(long) *p->ndx]; return OK;}
int tab7(ENVIRON *csound,FASTB *p) { *p->r = tb7[(long) *p->ndx]; return OK;}
int tab8(ENVIRON *csound,FASTB *p) { *p->r = tb8[(long) *p->ndx]; return OK;}
int tab9(ENVIRON *csound,FASTB *p) { *p->r = tb9[(long) *p->ndx]; return OK;}
int tab10(ENVIRON *csound,FASTB *p) { *p->r = tb10[(long) *p->ndx]; return OK;}
int tab11(ENVIRON *csound,FASTB *p) { *p->r = tb11[(long) *p->ndx]; return OK;}
int tab12(ENVIRON *csound,FASTB *p) { *p->r = tb12[(long) *p->ndx]; return OK;}
int tab13(ENVIRON *csound,FASTB *p) { *p->r = tb13[(long) *p->ndx]; return OK;}
int tab14(ENVIRON *csound,FASTB *p) { *p->r = tb14[(long) *p->ndx]; return OK;}
int tab15(ENVIRON *csound,FASTB *p) { *p->r = tb15[(long) *p->ndx]; return OK;}

//************************************************************ */
// Opcodes from Peter Neub�ker                              */
// ************************************************************ */


// void printi(ENVIRON *csound,PRINTI *p)
// {
//     char    *sarg;
// 
//     if ((*p->ifilcod != sstrcod) || (*p->STRARG == 0)) 
//     {   sprintf(errmsg, "printi parameter was not a \"quoted string\"\n");
//         initerror(errmsg);
//         return NOTOK;
//     }
//     else 
//     {   sarg = p->STRARG;
//         do 
//         { putchar(*sarg);
//         } while (*++sarg != 0);
//         putchar(10);
//         putchar(13);
//     }
//     return OK;
// }


//====================
//opcodes from Jens Groh
//======================*/

int nlalp_set(ENVIRON *csound,NLALP *p) {
   if (!(*p->istor)) {
      p->m0 = 0.;
      p->m1 = 0.;
   }
   return OK;
}


int nlalp(ENVIRON *csound,NLALP *p) {
   int nsmps;
   MYFLT *rp;
   MYFLT *ip;
   double m0;
   double m1;
   double tm0;
   double tm1;
   double klfact;
   double knfact;

   nsmps = ksmps;
   rp = p->aresult;
   ip = p->ainsig;
   klfact = (double)*p->klfact;
   knfact = (double)*p->knfact;
   tm0 = p->m0;
   tm1 = p->m1;
   if (knfact == 0.) { // linear case
      if (klfact == 0.) { // degenerated linear case
         m0 = (double)*ip++ - tm1;
         *rp++ = (MYFLT)(tm0);
         while (--nsmps) {
            *rp++ = (MYFLT)(m0);
            m0 = (double)*ip++;
         }
         tm0 = m0;
         tm1 = 0.;
      } else { // normal linear case
         do {
            m0 = (double)*ip++ - tm1;
            m1 = m0 * klfact;
            *rp++ = (MYFLT)(tm0 + m1);
            tm0 = m0;
            tm1 = m1;
         } while (--nsmps);
      }
   } else { // non-linear case
      if (klfact == 0.) { // simplified non-linear case
         do {
            m0 = (double)*ip++ - tm1;
            m1 = fabs(m0) * knfact;
            *rp++ = (MYFLT)(tm0 + m1);
            tm0 = m0;
            tm1 = m1;
         } while (--nsmps);
      } else { // normal non-linear case
         do {
            m0 = (double)*ip++ - tm1;
            m1 = m0 * klfact + fabs(m0) * knfact;
            *rp++ = (MYFLT)(tm0 + m1);
            tm0 = m0;
            tm1 = m1;
         } while (--nsmps);
      }
   }
   p->m0 = tm0;
   p->m1 = tm1;
   return OK;
}

/* -----------------------------------------------*/

int adsynt2_set(ENVIRON *csound,ADSYNT2 *p)
{
    FUNC    *ftp;
    MYFLT fmaxlen = (MYFLT)MAXLEN;
    int     count;
    long    *lphs;
	MYFLT	*pAmp;
    p->inerr = 0;

    if ((ftp = ftnp2find(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
    }
    else {
      p->inerr = 1;
      //initerror(Str(X_173,"adsynt: wavetable not found!"));
      initerror("adsynt: wavetable not found!");
      return NOTOK;
    }

    count = (int)*p->icnt;
    if (count < 1)
      count = 1;
    p->count = count;

    if ((ftp = ftnp2find(csound, p->ifn)) != NULL) {
      p->freqtp = ftp;
    }
    else {
      p->inerr = 1;
      //initerror(Str(X_309,"adsynt: freqtable not found!"));
      initerror("adsynt: freqtable not found!");
      return NOTOK;
    }
    if (ftp->flen < count) {
      p->inerr = 1;
//       initerror(Str(X_1424,"adsynt: partial count is greater than freqtable size!"));
      initerror("adsynt: partial count is greater than freqtable size!");
      return NOTOK;
    }

    if ((ftp = ftnp2find(csound, p->ifn)) != NULL) {
      p->amptp = ftp;
    }
    else {
      p->inerr = 1;
//       initerror(Str(X_1473, "adsynt: amptable not found!"));
      initerror("adsynt: amptable not found!");
      return NOTOK;
    }
    if (ftp->flen < count) {
      p->inerr = 1;
//       initerror(Str(X_1474,"adsynt: partial count is greater than amptable size!"));
      initerror("adsynt: partial count is greater than amptable size!");
      return NOTOK;
    }

    if (p->lphs.auxp==NULL || p->lphs.size < (long)(sizeof(long)+sizeof(MYFLT))*count)
      auxalloc((sizeof(long)+sizeof(MYFLT))*count, &p->lphs);

    lphs = (long*)p->lphs.auxp;
    if (*p->iphs > 1) {
      do
        *lphs++ = ((long)((MYFLT)((double)rand()/(double)RAND_MAX)
                          * fmaxlen)) & PHMASK;
      while (--count);
    }
    else if (*p->iphs >= 0){
      do
        *lphs++ = ((long)(*p->iphs * fmaxlen)) & PHMASK;
      while (--count);
    }
	pAmp = p->previousAmp = (MYFLT *) lphs + sizeof(MYFLT)*count;
	count = (int)*p->icnt;
	do 
		*pAmp++ = FL(0.);
    while (--count);
    return OK;
}

int adsynt2(ENVIRON *csound,ADSYNT2 *p)
{
    FUNC    *ftp, *freqtp, *amptp;
    MYFLT   *ar, *ar0, *ftbl, *freqtbl, *amptbl, *prevAmp;
    MYFLT   amp0, amp, cps0, cps, ampIncr,amp2;
    long    phs, inc, lobits;
    long    *lphs;
    int     nsmps, count;

    if (p->inerr) {
      //initerror(Str(X_1475,"adsynt: not initialized"));
      initerror("adsynt: not initialized");
      return NOTOK;
    }
    ftp = p->ftp;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    freqtp = p->freqtp;
    freqtbl = freqtp->ftable;
    amptp = p->amptp;
    amptbl = amptp->ftable;
    lphs = (long*)p->lphs.auxp;
	prevAmp = p->previousAmp;

    cps0 = *p->kcps;
    amp0 = *p->kamp;
    count = p->count;

    ar0 = p->sr;
    ar = ar0;
    nsmps = ksmps;
    do
      *ar++ = FL(0.0);
    while (--nsmps);

    do {
      ar = ar0;
      nsmps = ksmps;
      amp2 = *prevAmp;
	  amp = *amptbl++ * amp0;
      cps = *freqtbl++ * cps0;
      inc = (long) (cps * sicvt);
      phs = *lphs;
	  ampIncr = (amp - *prevAmp) / ensmps;
      do {
        *ar++ += *(ftbl + (phs >> lobits)) * amp2;
        phs += inc;
        phs &= PHMASK;
		amp2 += ampIncr;
      }
      while (--nsmps);
	  *prevAmp++ = amp;
      *lphs++ = phs;
    }
    while (--count);
    return OK;
}


int exitnow(ENVIRON *csound,EXITNOW *p) 
{ // gab c3
	//fcloseall(); 
#undef exit
	exit(0);
	return OK;
}

// extern MYFLT *zkstart;
//extern MYFLT *zastart;
//-----zak opcodes
// int zread(ENVIRON *csound,ZKR *p)
// {
// 	*p->rslt = zkstart[(long) *p->ndx];
// 	return OK;
// }

/*void a_k_set(INDIFF *p)
{
	p->prev = FL(0.0);
}*/

//===================================================================


int split_trig_set(ENVIRON *csound,SPLIT_TRIG *p)
{

	/* syntax of each table element:
   numtics_elem1, 
   tic1_out1, tic1_out2, ... , tic1_outN,
   tic2_out1, tic2_out2, ... , tic2_outN,
   tic3_out1, tic3_out2, ... , tic3_outN,
   .....
   ticN_out1, ticN_out2, ... , ticN_outN,
   
   numtics_elem2, 
   tic1_out1, tic1_out2, ... , tic1_outN,
   tic2_out1, tic2_out2, ... , tic2_outN,
   tic3_out1, tic3_out2, ... , tic3_outN,
   .....
   ticN_out1, ticN_out2, ... , ticN_outN,

	*/	

    FUNC *ftp;
    if ((ftp = ftnp2find(csound, p->ifn)) == NULL) {
      //initerror(Str(X_1535,"splitrig: incorrect table number"));
      initerror("splitrig: incorrect table number");
      return NOTOK;
    }
    p->table = ftp->ftable;
	p->numouts =  p->INOCOUNT-4; 
	p->currtic = 0;
    return OK;
}


int split_trig(ENVIRON *csound,SPLIT_TRIG *p)
{
	int	j;
	int numouts =  p->numouts;
	MYFLT **outargs = p->outargs;
	
	if (*p->trig) {
		int ndx = (int) *p->ndx * (numouts * (int) *p->maxtics + 1);
		int	numtics =  (int) p->table[ndx];
		MYFLT *table = &(p->table[ndx+1]);
		int kndx = (int) *p->ndx;
		int currtic;

		if (kndx != p->old_ndx) {
			p->currtic = 0;
			p->old_ndx = kndx;
		}
		currtic = p->currtic;
		
		for (j = 0; j < numouts; j++) 
			*outargs[j] = table[j +  currtic * numouts ]; 
		
		p->currtic = (currtic +1) % numtics;
		
	}

	else {
		for(j =0; j< numouts; j++) 
			*outargs[j] = FL(0.0);
	}
	return OK;
}


//--------------------------

int timeseq_set(ENVIRON *csound,TIMEDSEQ *p)
{
	FUNC *ftp;
	MYFLT *table;
	int j;
	if ((ftp = ftnp2find(csound, p->ifn)) == NULL)
	{
		initerror("timeseq: incorrect table number");
		return NOTOK;
	}
	table = p->table = ftp->ftable;
	p->numParm = p->INOCOUNT-2; // ?
	for (j = 0; j < ftp->flen; j+= p->numParm) {
		if (table[j] < 0) { 
			p->endSeq = table[j+1];
			p->endIndex = j/p->numParm;
			break;
		}
	}
	p->initFlag = 1;
	return OK;
}


int timeseq(ENVIRON *csound,TIMEDSEQ *p)
{
	MYFLT *table = p->table, minDist = onedkr;
	MYFLT phs = *p->kphs, endseq = p->endSeq;
	int  j,k, numParm = p->numParm, endIndex = p->endIndex;
	while (phs > endseq) 
		phs -=endseq;
	while (phs < 0 ) 
		phs +=endseq;
	
	if (p->initFlag) {
	prev:
		for (j=0,k=endIndex; j < endIndex; j++, k--) {
			if (table[j*numParm + 1] > phs ) {
				p->nextActime = table[j*numParm + 1]; 
				p->nextIndex = j;
				p->prevActime = table[(j-1)*numParm + 1]; 
				p->prevIndex = j-1;
				break;
			}
			if (table[k*numParm + 1] < phs ) {
				p->nextActime = table[(k+1)*numParm + 1]; 
				p->nextIndex = k+1;
				p->prevActime = table[k*numParm + 1]; 
				p->prevIndex = k;
				break;
			}
		}
		if (phs == p->prevActime&& p->prevIndex != -1 )  {
			*p->ktrig = 1;
			for (j=0; j < numParm; j++) {
				*p->args[j]=table[p->prevIndex*numParm + j];
			}
		}
		else if (phs == p->nextActime && p->nextIndex != -1 )  {
			*p->ktrig = 1;
			for (j=0; j < numParm; j++) {
				*p->args[j]=table[p->nextIndex*numParm + j];
			}
		}
		//p->oldPhs = phs;
		p->initFlag=0;
	}
	else {
		if (phs > p->nextActime || phs < p->prevActime) {			
			for (j=0; j < numParm; j++) {
				*p->args[j]=table[p->nextIndex*numParm + j];
			}
			if (table[p->nextIndex*numParm] != -1) // if it is not end locator
				//*p->ktrig = 1;
				*p->ktrig = table[p->nextIndex*numParm + 3];
			if (phs > p->nextActime) {
				if (p->prevIndex > p->nextIndex && p->oldPhs < phs) { // there is a phase jump
					*p->ktrig = 0;
					goto fine;
				} 
				if (fabs(phs-p->nextActime) > minDist) 
					goto prev;

				p->prevActime = table[p->nextIndex*numParm + 1];
				p->prevIndex = p->nextIndex;
				p->nextIndex = (p->nextIndex + 1) % endIndex;
				p->nextActime = table[p->nextIndex*numParm + 1];
			}
			else {
				if (fabs(phs-p->nextActime) > minDist) 
					goto prev;

				p->nextActime = table[p->prevIndex*numParm + 1]; //p->nextActime+1;
				p->nextIndex = p->prevIndex;
				p->prevIndex = (p->prevIndex - 1);
				if (p->prevIndex < 0) {
					p->prevIndex += p->endIndex;
				}
				p->prevActime = table[p->prevIndex*numParm + 1]; //p->nextActime+1;
			}
		}
		else 
			*p->ktrig = 0;
		fine:
		p->oldPhs = phs;
	}
	return OK;
	
}

// extern "C" void timeseq_set(TIMEDSEQ *p)
// {
// 	FUNC *ftp;
// 	MYFLT *table;
// 	if ((ftp = ftfind(p->ifn)) != NULL) {
//       ftp = ftp;
//     }
// 	else return;
// 	table = p->table = ftp->ftable;
// 	p->numParm = p->INOCOUNT-3; // ?
// 	for (int j = 0; j < ftp->flen; j+= p->numParm) {
// 		if (table[j] < 0) { // if insnum  < 0
// 			p->endSeq = table[j+1];
// 			p->endIndex = j/p->numParm;
// 			break;
// 		}
// 	}
// 	p->prevActime = p->nextActime = 0;
// 	p->lastIndex=-1;
// 	p->oldPhs =0;
// 	p->nextIndex=0;
// 
// }
// 
// 
// extern "C" void timeseq(TIMEDSEQ *p)
// {
// 	MYFLT minDist = *p->iMinDist, *table = p->table;
// 	MYFLT phs = *p->kphs;
// 	int  j, index  = p->currIndex, numParm = p->numParm, endIndex = p->endIndex;
// 
// 	if (p->lastIndex > p->nextIndex) {
// 		if (p->oldPhs < phs) {
// 			*p->ktrig = 0;
// 			goto fine;
// 		}
// 	}
// 	if (phs > p->nextActime) {
// 			*p->ktrig = 1;
// 			for (j=0; j < p->numParm; j++) {
// 				*p->args[j]=table[p->nextIndex * p->numParm+j];
// 			}
// 			p->lastIndex = p->nextIndex;
// 			p->nextIndex = (p->nextIndex + 1) % p->endIndex;
// 			p->nextActime = table[p->nextIndex * p->numParm+1]; //p->nextActime+1;
// 		}
// 	else *p->ktrig = 0;
// fine:
// 	p->oldPhs = phs;
// }




// typedef struct	{
// 	OPDS	h;
// 	MYFLT	*ktrig, *kphs, *iMinDist, *ifn, *args[PMAX];
// 	MYFLT endSeq, *table, oldPhs;
// 	int numParm, currIndex, endIndex, prevIndex, nextIndex, lastIndex;
// 	MYFLT prevActime, nextActime, lastActime;
// 
// } TIMEDSEQ;



//---===---===---===---===---===---===---===---===---===---===---===---===

//extern int maxfnum;
//extern FUNC	**flist;


int tabrec_set(ENVIRON *csound,TABREC *p) 
{
    //FUNC *ftp;
    //if ((ftp = ftfind(p->ifn)) == NULL) {
    //  initerror(Str(X_1535,"tabrec: incorrect table number"));
    //  return;
    //}
    //p->table = ftp->ftable;
	//p->tablen = ftp->flen;
	p->recording = 0;
	p->currtic = 0;
	p->ndx = 0;
	p->numins = p->INOCOUNT-4;
	return OK;
}

int tabrec_k(ENVIRON *csound,TABREC *p) 
{
	if (*p->ktrig_start) {
		if (*p->kfn != p->old_fn) {
			FUNC *ftp;
			MYFLT *fno;
			if ((*fno = (int)*p->kfn) <= 0 || *fno > csound->maxfnum || (ftp = ftnp2find(csound,fno)) == NULL) {
				//sprintf(errmsg, Str(X_315,"Invalid ftable no. %f"),*p->kfn);
				sprintf(errmsg, "Invalid ftable no. %f",*p->kfn);
				perferror(errmsg);
				return NOTOK;
			}
			else {
				p->tablen = ftp->flen;
				p->table = &ftp->ftable[1];
				p->currtic = 0;
				p->ndx = 0;
				ftp->ftable[0] = *p->numtics;
			}
		}
		
		p->recording = 1;
		p->ndx = 0;
		p->currtic = 0;
	}
	if (*p->ktrig_stop) {
		
		if (p->currtic >= *p->numtics) {
			p->recording = 0;
			return;
		}
		p->currtic++;
	}
	if (p->recording) {
		int j, curr_frame = p->ndx * p->numins;
		
		MYFLT *table = p->table;
		MYFLT **inargs = p->inargs;
		if (curr_frame + p->numins < p->tablen) { /* record only if table is not full */
			for (j = 0; j < p->numins; j++)
				table[curr_frame + j] = *inargs[j];
		}
		(p->ndx)++;
	}
	return OK;
}
/*-------------------------*/
int tabplay_set(ENVIRON *csound,TABPLAY *p) 
{
 //   FUNC *ftp;
   // if ((ftp = ftfind(p->ifn)) == NULL) {
   //   initerror(Str(X_1535,"tabplay: incorrect table number"));
   //   return;
   // }
  //  p->table = ftp->ftable;
//	p->tablen = ftp->flen;
	p->playing = 0;
	p->currtic = 0;
	p->ndx = 0;
	p->numouts = p->INOCOUNT-3;
	return OK;
}

int tabplay_k(ENVIRON *csound,TABPLAY *p) 
{
	if (*p->ktrig) {
		if (*p->kfn != p->old_fn) {
			FUNC *ftp;
			MYFLT *fno;
// 			if ((fno = (int)*p->kfn) <= 0 || fno > csound->maxfnum || (ftp = flist[fno]) == NULL) {
			if ((*fno = (int)*p->kfn) <= 0 || *fno > csound->maxfnum || (ftp = ftnp2find(csound,fno)) == NULL) {
				//sprintf(errmsg, Str(X_315,"Invalid ftable no. %f"),*p->kfn);
				sprintf(errmsg, "Invalid ftable no. %f",*p->kfn);
				perferror(errmsg);
				return NOTOK;
			}
			else {
				p->tablen = ftp->flen;
				p->table = &ftp->ftable[1];
				p->currtic = 0;
				p->ndx = 0;
				ftp->ftable[0] = *p->numtics;
				p->old_fn = *p->kfn;
			}
		}

		p->playing = 1;
		if (p->currtic == 0)
			p->ndx = 0;
		if (p->currtic >= *p->numtics) {
			p->playing = 0;
			return;
		}
		p->currtic++;
		p->currtic %= (long) *p->numtics;

	}
	if (p->playing) {
		int j, curr_frame = p->ndx * p->numouts;
		MYFLT *table = p->table;
		MYFLT **outargs = p->outargs;
		if (curr_frame + p->numouts < p->tablen) { /* play only if ndx is inside table */
			//for (j = p->ndx* p->numouts; j < end; j++)
			//	*outargs[j] = table[j];

			for (j = 0; j < p->numouts; j++)
				*outargs[j] = table[curr_frame+j];
		}
		(p->ndx)++;
	}
	return OK;
}

int isChanged_set(ENVIRON *csound,ISCHANGED *p) 
{
	p->numargs = p->INOCOUNT;
	return OK;
}

int isChanged(ENVIRON *csound,ISCHANGED *p) 
{
	MYFLT **inargs = p->inargs;	
	MYFLT *old_inargs = p->old_inargs;
	int numargs = p->numargs, ktrig = 0, j;
	
	for (j =0; j< numargs; j++) {
		if (*inargs[j] != old_inargs[j]) {
			ktrig = 1;
			break;
		}
	}

	if (ktrig) {
		for (j =0; j< numargs; j++) {
			old_inargs[j] = *inargs[j];
		}
	}
	*p->ktrig = (MYFLT) ktrig;
	return OK;
}

// #include "oload.h" /* for strset */
// 
// int CSsystem(ENVIRON *csound,CSSYSTEM *p) 
// {
// 	long filno;
// 	char comLine[512];
// 	int ret;
// 
// 	if (*p->commandLine == sstrcod) { /* if char string name given */
//       extern EVTBLK *currevent;
//       if (p->STRARG == NULL) strcpy(comLine,unquote(currevent->strarg));
//       else strcpy(comLine,unquote(p->STRARG));    /* unquote it,  else use */
//     }
//     else if ((filno=(long)*p->commandLine) <= strsmax && strsets != NULL &&
// 	     strsets[filno])
//       strcpy(comLine, strsets[filno]);
//     else sprintf(comLine,"soundin.%ld",filno);  /* soundin.filno */
// 	ret = system(NULL);
// 	if (ret)
// 		ret = system( comLine );
//     return OK;
// }

// -------------------------

int partial_maximum_set(ENVIRON *csound,P_MAXIMUM *p) 
{
	p->max = 0;
	p->counter = 0;
	return OK;
}

int partial_maximum(ENVIRON *csound,P_MAXIMUM *p) 
{

	int	n = ksmps, flag = (int) *p->imaxflag;
	MYFLT *a = p->asig;
	MYFLT max = p->max;
	switch(flag) {
		case 0: /* absolute maximum */
			do 	{
				MYFLT temp;
				if ((temp= (MYFLT) fabs(*a++)) > max) max = temp;
			} while (--n);
			if (max > p->max) p->max = max;
			break;
		case 1: // actual maximum */
				do 	{
					if (*a > max) max = *a;
					++a;
				} while (--n);
				if (max > p->max) p->max = max;
				break;
		case 2: // actual minimum */
				do 	{
					if (*a < max) max = *a;
					++a;
				} while (--n);
				if (max < p->max) p->max = max;
				break;
		case 3: { // average */
				MYFLT temp=0;
				do 	temp += *a++;
				while (--n);
				++(p->counter);
				p->max += temp;
				}
				break;
		default:
			perferror("max_k: invalid imaxflag value");
			return NOTOK;
	}
	if (*p->ktrig) {
		if (flag == 3) { 
			*p->kout = p->max / p->counter;
			p->counter = 0;
		}
		else *p->kout = p->max;
		p->max = 0;
	}
	return OK;
}

// From fractals.c
// mandelbrot set scanner 
int mandel_set(ENVIRON *csound,MANDEL *p)
{
	p->oldx=-99999; //probably unused values 
	p->oldy=-99999;
	p->oldCount = -1;
	return OK;
}

int mandel(ENVIRON *csound,MANDEL *p)
{
	MYFLT px=*p->kx, py=*p->ky;
	if (*p->ktrig && (px != p->oldx || py != p->oldy)) {
		int maxIter = (int) *p->kmaxIter, j;
		MYFLT x=0.f, y=0.f, newx, newy;
		for (j=0; j<maxIter; j++) {
			newx = x*x - y*y + px;
			newy = 2*x*y + py;
			x=newx;
			y=newy;
			if (x*x+y*y >= 4) break;
		}
		p->oldx = px;
		p->oldy = py;
		if (p->oldCount != j) *p->koutrig = 1.f;
		else *p->koutrig = 0.f;
		*p->kr = (MYFLT) (p->oldCount = j);
	}
	else {
		*p->kr = (MYFLT) p->oldCount;
		*p->koutrig = 0.f;
	}
	return OK;
}


#define S sizeof
    
static OENTRY localops[] = { 
        {"resonxk", S(KRESONX),     3,      "k",    "kkkooo", (SUBR)krsnsetx, (SUBR)kresonx, NULL  },
	{ "metro", S(METRO),	  3,      "k",   "ko",	(SUBR)metro_set, (SUBR)metro, NULL},

	{ "tab_i",S(FASTAB),  1,   "i",    "iio", (SUBR)fastabi               },
	{ "tab",S(FASTAB),  7,   "s",    "xio", (SUBR)fastab_set, (SUBR)fastabk,   (SUBR)fastab },
	{ "tabw_i",S(FASTAB),  1,   "",    "iiio", (SUBR)fastabiw               },
	{ "tabw",S(FASTAB),  7,   "",    "xxio", (SUBR)fastab_set, (SUBR)fastabkw,   (SUBR)fastabw },
	
	{ "tb0_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab0_init},
	{ "tb1_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab1_init},
	{ "tb2_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab2_init},
	{ "tb3_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab3_init},
	{ "tb4_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab4_init},
	{ "tb5_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab5_init},
	{ "tb6_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab6_init},
	{ "tb7_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab7_init},
	{ "tb8_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab8_init},
	{ "tb9_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab9_init},
	{ "tb10_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab10_init},
	{ "tb11_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab11_init},
	{ "tb12_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab12_init},
	{ "tb13_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab13_init},
	{ "tb14_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab14_init},
	{ "tb15_init", S(TB_INIT),	1,     "",	"i",	(SUBR)tab15_init},
	
	{ "tb0_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab0,	NULL},
	{ "tb1_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab1,	NULL},
	{ "tb2_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab2,	NULL},
	{ "tb3_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab3,	NULL},
	{ "tb4_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab4,	NULL},
	{ "tb5_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab5,	NULL},
	{ "tb6_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab6,	NULL},
	{ "tb7_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab7,	NULL},
	{ "tb8_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab8,	NULL},
	{ "tb9_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab9,	NULL},
	{ "tb10_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab10,	NULL},
	{ "tb11_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab11,	NULL},
	{ "tb12_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab12,	NULL},
	{ "tb13_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab13,	NULL},
	{ "tb14_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab14,	NULL},
	{ "tb15_k",	  S(FASTB),		2,     "k",	"k",	NULL,   (SUBR)tab15,	NULL},
	//tbx_t (t-rate version removed here)
	{ "tb0_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab0	},
	{ "tb1_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab1	},
	{ "tb2_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab2	},
	{ "tb3_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab3	},
	{ "tb4_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab4	},
	{ "tb5_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab5	},
	{ "tb6_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab6	},
	{ "tb7_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab7	},
	{ "tb8_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab8	},
	{ "tb9_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab9	},
	{ "tb10_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab10	},
	{ "tb11_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab11	},
	{ "tb12_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab12	},
	{ "tb13_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab13	},
	{ "tb14_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab14	},
	{ "tb15_i",	  S(FASTB),		1,     "i",	"i",	(SUBR)tab15	},
	
	{ "nlalp", S(NLALP),		5,     "a",    "akkoo", (SUBR)nlalp_set, NULL, (SUBR)nlalp     },
	
	{ "adsynt2",S(ADSYNT2),		5,     "a",  "kkiiiio", (SUBR)adsynt2_set,  NULL,  (SUBR)adsynt2  },
	
	{ "exitnow",S(EXITNOW),		1,     "",  "", (SUBR)exitnow  },
	
// 	{ "zr_i",  S(ZKR),	1,     "i",	"i",	(SUBR)zread,	 NULL, 	NULL},
// 	{ "zr_k",  S(ZKR),	2,     "k",	"k",	NULL,   (SUBR)zread,	NULL},
// 	{ "zr_a",  S(ZAR),	5,     "a",	"a",	(SUBR)zaset,   NULL, 	(SUBR)zar},
// 	
	//{ "k_i",	   S(ASSIGN),	1,     "k",	"i",	(SUBR)assign},
// 	{ "k_t",	   S(ASSIGN),	2,     "k",	"t",	NULL, (SUBR)assign},
// 	{ "a_k",	  S(INDIFF),	5,	"a",	"k",	(SUBR)a_k_set,NULL,	(SUBR)interp	},
	{ "splitrig",	  S(SPLIT_TRIG),	3,     "",	"kkiiz",	(SUBR)split_trig_set, (SUBR)split_trig },
	
	{ "timedseq",S(TIMEDSEQ),		3,     "k",  "kiz", (SUBR)timeseq_set,(SUBR)timeseq  },
	
	{ "tabrec",	S(TABREC),		3,     "",	"kkkkz",(SUBR)tabrec_set, (SUBR)tabrec_k },
	{ "tabplay",	S(TABPLAY),		3,     "",	"kkkz", (SUBR)tabplay_set, (SUBR)tabplay_k },
	
	{ "changed",    S(ISCHANGED),		3,     "k",	"z",		(SUBR)isChanged_set, (SUBR)isChanged },
	
	//{ "ftlen_k",S(EVAL),	2,	"k",	"k", NULL,	(SUBR)ftlen	},
	{ "max_k",  S(P_MAXIMUM),	5,	"k",	"aki", 	(SUBR)partial_maximum_set,(SUBR)NULL,partial_maximum	},
	{ "mandel",S(MANDEL),     3,      "kk",    "kkkk",   mandel_set,   mandel      },

    };
   
LINKAGE
//
//Called by Csound to obtain the size of
// the table of OENTRY structures defined in this shared library.
//
// PUBLIC int opcode_size()
// {
// return sizeof(gabOentry);
// }

// *
// * Called by Csound to obtain a pointer to
// * the table of OENTRY structures defined in this shared library.

// PUBLIC OENTRY *opcode_init(ENVIRON *csound)
// {
// return gabOentry;
// }
