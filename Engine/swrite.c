/*
    swrite.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "cs.h"                                          /*    SWRITE.C  */
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

static SRTBLK *nxtins(SRTBLK *), *prvins(SRTBLK *);
static char   *pfout(SRTBLK *, char *, int, int);
static char   *nextp(SRTBLK *, char *, int, int);
static char   *prevp(SRTBLK *, char *, int, int);
static char   *ramp(SRTBLK *, char *, int, int);
static char   *expramp(SRTBLK *, char *, int, int);
static char   *randramp(SRTBLK *, char *, int, int);
static char   *pfStr(char *, int, int), *fpnum(char *, int, int);

#define fltout(n) fprintf(SCOREOUT, "%.6f", n)

void swrite(void)
{
    SRTBLK *bp;
    char *p, c, isntAfunc;
    int    lincnt, pcnt=0;

    if ((bp = cenviron.frstbp) == NULL)
      return;
    lincnt = 0;
    if ((c = bp->text[0]) != 'w'
        && c != 's' && c != 'e') {     /* if no warp stmnt but real data,  */
      fprintf(SCOREOUT,"w 0 60\n");    /* create warp-format indicator */
      lincnt++;
    }
 nxtlin:
    lincnt++;                          /* now for each line:           */
    p = bp->text;
    c = *p++;
    putc(c,SCOREOUT);
    isntAfunc = 1;
    switch (c) {
    case 'f':
      isntAfunc = 0;
    case 'q':
    case 'i':
    case 'a':
      putc(*p++,SCOREOUT);
      while ((c = *p++) != SP && c != LF)
        putc(c,SCOREOUT);                       /* put p1       */
      putc(c,SCOREOUT);
      if (c == LF)
        break;
      fltout(bp->p2val);                        /* put p2val,   */
      putc(SP,SCOREOUT);
      fltout(bp->newp2);                        /*   newp2,     */
      while ((c = *p++) != SP && c != LF)
        ;
      putc(c,SCOREOUT);                         /*   and delim  */
      if (c == LF)
        break;
      if (isntAfunc) {
        fltout(bp->p3val);                      /* put p3val,   */
        putc(SP,SCOREOUT);
        fltout(bp->newp3);                      /*   newp3,     */
        while ((c = *p++) != SP && c != LF)
          ;
      }
      else { /*make sure p3s (table length) are ints */
        char temp[256];
        sprintf(temp,"%ld ",(long)bp->p3val);   /* put p3val  */
        fpnum(temp, lincnt, pcnt);
        putc(SP,SCOREOUT);
        sprintf(temp,"%ld ",(long)bp->newp3);   /* put newp3  */
        fpnum(temp, lincnt, pcnt);
        while ((c = *p++) != SP && c != LF)
          ;
      }
      pcnt = 3;
      while (c != LF) {
        pcnt++;
        putc(SP,SCOREOUT);
        p = pfout(bp,p,lincnt,pcnt);            /* now put each pfield  */
        c = *p++;
      }
      putc('\n', SCOREOUT);
      break;
    case 'w':
    case 't':
    case 's':
    case 'e':
      while ((c = *p++) != LF)        /* put entire line      */
        putc(c,SCOREOUT);
      putc(LF,SCOREOUT);
      break;
    default:
      err_printf(Str("swrite: unexpected opcode, section %d line %d\n"),
                 cenviron.sectcnt,lincnt);
      break;
    }
    if ((bp = bp->nxtblk) != NULL)
      goto nxtlin;
}

static char *pfout(SRTBLK *bp, char *p,int lincnt, int pcnt)
{
    switch (*p) {
    case 'n':
      p = nextp(bp,p, lincnt, pcnt);
      break;
    case 'p':
      p = prevp(bp,p, lincnt, pcnt);
      break;
    case '<':
    case '>':
      p = ramp(bp,p, lincnt, pcnt);
      break;
    case '{':
    case '}':
      printf(Str("Deprecated -- use round brackets instead of curly\n"));
    case '(':
    case ')':
      p = expramp(bp,p, lincnt, pcnt);
      break;
    case '~':
      p = randramp(bp, p, lincnt, pcnt);
      break;
    case '"':
      p = pfStr(p, lincnt, pcnt);
      break;
    default:
      p = fpnum(p, lincnt, pcnt);
      break;
    }
    return(p);
}

static SRTBLK *nxtins(SRTBLK *bp) /* find nxt note with same p1 */
{
    MYFLT p1;

    p1 = bp->p1val;
    while ((bp = bp->nxtblk) != NULL
           && (bp->p1val != p1 || bp->text[0] != 'i'))
      ;
    return(bp);
}

static SRTBLK *prvins(SRTBLK *bp) /* find prv note with same p1 */
{
    MYFLT p1;

    p1 = bp->p1val;
    while ((bp = bp->prvblk) != NULL
           && (bp->p1val != p1 || bp->text[0] != 'i'))
      ;
    return(bp);
}

static char *nextp(SRTBLK *bp, char *p, int lincnt, int pcnt)
{
    char *q;
    int n;

    q = p;
    p++;                                    /* 1st char     */
    if (*p++ != 'p')                        /* 2nd char     */
      goto error;
    n = 999;
    if (isdigit(*p))
      n = *p++ - '0';
    if (isdigit(*p))                /* n is np subscript no */
      n = 10*n + (*p++ - '0');
    if (*p != SP && *p != LF)
      goto error;
    if ((bp = nxtins(bp)) != NULL   /* for nxtins, same p1  */
        && n <= bp->pcnt) {
      q = bp->text;
      while (n--)
        while (*q++ != SP)          /*   go find the pfield */
          ;
      pfout(bp,q,lincnt,pcnt);      /*   and put it out     */
    }
    else {
    error:
      err_printf(Str("swrite: output, sect%d line%d p%d makes"
                     " illegal reference to "),
                 cenviron.sectcnt,lincnt,pcnt);
      while (q < p)
        err_printf("%c", *q++);
      while (*p != SP && *p != LF)
        err_printf("%c", *p++);
      err_printf(Str("   Zero substituted\n"));
      putc('0',SCOREOUT);
    }
    return(p);
}

static char *prevp(SRTBLK *bp, char *p, int lincnt, int pcnt)
{
    char *q;
    int n;

    q = p;
    p++;                                    /* 1st char     */
    if (*p++ != 'p')                        /* 2nd char     */
      goto error;
    n = 999;
    if (isdigit(*p))
      n = *p++ - '0';
    if (isdigit(*p))                /* n is np subscript no */
      n = 10*n + (*p++ - '0');
    if (*p != SP && *p != LF)
      goto error;
    if ((bp = prvins(bp)) != NULL   /* for prvins, same p1, */
        && n <= bp->pcnt) {
      q = bp->text;
      while (n--)
        while (*q++ != SP)          /*   go find the pfield */
          ;
      pfout(bp,q,lincnt,pcnt);      /*   and put it out     */
    }
    else {
    error:
      err_printf(
          Str("swrite: output, sect%d line%d p%d makes illegal reference to "),
          cenviron.sectcnt,lincnt,pcnt);
      while (q < p)
        err_printf("%c", *q++);
      while (*p != SP && *p != LF)
        err_printf("%c", *p++);
      err_printf(Str("   Zero substituted\n"));
      putc('0',SCOREOUT);
    }
    return(p);
}

static char *ramp(SRTBLK *bp, char *p, int lincnt, int pcnt) 
  /* NB np's may reference a ramp but ramps must terminate in valid nums */
{
    char *q;
    char   *psav;
    SRTBLK *prvbp, *nxtbp;
    MYFLT pval, qval, rval, p2span;
extern  MYFLT stof(char*);
    int pnum, n;

    psav = ++p;
    if (*psav != SP && *psav != LF)
      goto error1;
    pnum = 0;
    q = bp->text;
    while (q < p)
      if (*q++ == SP)
        pnum++;
    prvbp = bp;
 backup:
    if ((prvbp = prvins(prvbp)) != NULL) {
      p = prvbp->text;
      n = pnum;
      while (n--)
        while (*p++ != SP)
          ;
      if (*p == '>' || *p == '<')
        goto backup;
    }
    else goto error2;
    nxtbp = bp;
 forwrd:
    if ((nxtbp = nxtins(nxtbp)) != NULL) {
      q = nxtbp->text;
      n = pnum;
      while (n--)
        while (*q++ != SP)
          ;
      if (*q == '>' || *q == '<')
        goto forwrd;
    }
    else goto error2;
    pval = stof(p);           /* the error msgs generated by stof     */
    qval = stof(q);                                 /* are misleading */
    if ((p2span = nxtbp->newp2 - prvbp->newp2) <= 0)
      goto error2;
    rval = (qval - pval) * (bp->newp2 - prvbp->newp2) / p2span + pval;
    fltout(rval);
    return(psav);

 error1:
    err_printf(
        Str("swrite: output, sect%d line%d p%d has illegal ramp symbol\n"),
        cenviron.sectcnt,lincnt,pcnt);
    goto put0;
 error2:
    err_printf(Str("swrite: output, sect%d line%d p%d ramp has illegal"
                   " forward or backward ref\n"),
               cenviron.sectcnt,lincnt,pcnt);
 put0:
    putc('0',SCOREOUT);
    return(psav);
}

static char *expramp(SRTBLK *bp, char *p, int lincnt, int pcnt)
  /* NB np's may reference a ramp but ramps must terminate in valid nums */
{
    char *q;
    char   *psav;
    SRTBLK *prvbp, *nxtbp;
    MYFLT pval, qval, rval;
    double p2span;
    extern  MYFLT stof(char*);
    int pnum, n;

    psav = ++p;
    if (*psav != SP && *psav != LF)
      goto error1;
    pnum = 0;
    q = bp->text;
    while (q < p)
      if (*q++ == SP)
        pnum++;
    prvbp = bp;
 backup:
    if ((prvbp = prvins(prvbp)) != NULL) {
      p = prvbp->text;
      n = pnum;
      while (n--)
        while (*p++ != SP)
          ;
      if (*p == '}' || *p == '{' || *p == '(' || *p == ')')
        goto backup;
    }
    else goto error2;
    nxtbp = bp;
 forwrd:
    if ((nxtbp = nxtins(nxtbp)) != NULL) {
      q = nxtbp->text;
      n = pnum;
      while (n--)
        while (*q++ != SP)
          ;
      if (*q == '}' || *q == '{' || *q == '(' || *q == ')')
        goto forwrd;
    }
    else goto error2;
    pval = stof(p);         /* the error msgs generated by stof     */
    qval = stof(q);                                 /* are misleading */
    p2span = (double)(nxtbp->newp2 - prvbp->newp2);
/*  printf("pval=%f qval=%f span = %f\n", pval, qval, p2span); */
    rval = pval * (MYFLT)pow((double)(qval/pval),
                             (double)(bp->newp2 - prvbp->newp2) / p2span);
/*  printf("rval=%f bp->newp2=%f prvbp->newp2-%f\n",
           rval, bp->newp2, prvbp->newp2); */
    fltout(rval);
    return(psav);

 error1:
    err_printf(Str("swrite: output, sect%d line%d p%d has illegal"
                   " expramp symbol\n"),
               cenviron.sectcnt,lincnt,pcnt);
    goto put0;
 error2:
    err_printf(Str("swrite: output, sect%d line%d p%d expramp has illegal "
                   "forward or backward ref\n"),
               cenviron.sectcnt,lincnt,pcnt);
 put0:
    putc('0',SCOREOUT);
    return(psav);
}

static char *randramp(SRTBLK *bp, char *p, int lincnt, int pcnt)
  /* NB np's may reference a ramp but ramps must terminate in valid nums */
{
    char *q;
    char   *psav;
    SRTBLK *prvbp, *nxtbp;
    MYFLT pval, qval, rval;
    extern  MYFLT stof(char*);
    int pnum, n;

    psav = ++p;
    if (*psav != SP && *psav != LF)
      goto error1;
    pnum = 0;
    q = bp->text;
    while (q < p)
      if (*q++ == SP)
        pnum++;
    prvbp = bp;
 backup:
    if ((prvbp = prvins(prvbp)) != NULL) {
      p = prvbp->text;
      n = pnum;
      while (n--)
        while (*p++ != SP)
          ;
      if (*p == '~')
        goto backup;
    }
    else goto error2;
    nxtbp = bp;
 forwrd:
    if ((nxtbp = nxtins(nxtbp)) != NULL) {
      q = nxtbp->text;
      n = pnum;
      while (n--)
        while (*q++ != SP)
          ;
      if (*q == '~')
        goto forwrd;
    }
    else goto error2;
    pval = stof(p);           /* the error msgs generated by stof     */
    qval = stof(q);                                 /* are misleading */
    rval = (MYFLT)rand()/((MYFLT)RAND_MAX);
    rval = rval*(qval-pval) + pval;
    fltout(rval);
    return(psav);

 error1:
    err_printf(Str("swrite: output, sect%d line%d p%d has illegal"
                   " expramp symbol\n"),
               cenviron.sectcnt,lincnt,pcnt);
    goto put0;
 error2:
    err_printf(Str("swrite: output, sect%d line%d p%d expramp has illegal "
                   "forward or backward ref\n"),
               cenviron.sectcnt,lincnt,pcnt);
 put0:
    putc('0',SCOREOUT);
    return(psav);
}

static char *pfStr(char *p, int lincnt, int pcnt) /* moves quoted ascii string */
                              /* to SCOREOUT file  with no internal format chk */
{
    char *q = p;
    putc(*p++,SCOREOUT);
    while (*p != '"')
      putc(*p++,SCOREOUT);
    putc(*p++,SCOREOUT);
    if (*p != SP && *p != LF) {
      err_printf(Str("swrite: output, sect%d line%d p%d has illegally"
                     " terminated string   "),
                 cenviron.sectcnt,lincnt,pcnt);
      while (q < p)
        err_printf("%c", *q++);
      while (*p != SP && *p != LF)
        err_printf("%c", *p++);
      err_printf("\n");
    }
    return(p);
}

static char *fpnum(char *p, int lincnt, int pcnt) /* moves ascii string */
                              /* to SCOREOUT file with fpnum format chk */
{
    char *q;
    int dcnt;

    q = p;
    if (*p == '+')
      p++;
    if (*p == '-')
      putc(*p++,SCOREOUT);
    dcnt = 0;
    while (isdigit(*p)) {
      putc(*p++,SCOREOUT);
      dcnt++;
    }
    if (*p == '.')
      putc(*p++,SCOREOUT);
    while (isdigit(*p)) {
      putc(*p++,SCOREOUT);
      dcnt++;
    }
    if (*p == 'E' || *p == 'e') { /* Allow exponential notation */
      putc(*p++,SCOREOUT);
      dcnt++;
      if (*p == '+' || *p == '-') {
        putc(*p++,SCOREOUT);
        dcnt++;
      }
      while (isdigit(*p)) {
        putc(*p++,SCOREOUT);
        dcnt++;
      }
    }
    if ((*p != SP && *p != LF) || !dcnt) {
      err_printf(Str("swrite: output, sect%d line%d p%d has illegal number  "),
                 cenviron.sectcnt,lincnt,pcnt);
      while (q < p)
        err_printf("%c", *q++);
      while (*p != SP && *p != LF)
        err_printf("%c", *p++);
      err_printf(Str("    String truncated\n"));
      if (!dcnt)
        putc('0',SCOREOUT);
    }
    return(p);
}

