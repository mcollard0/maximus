/*
 * Maximus Version 3.02
 * Copyright 1989, 2002 by Lanius Corporation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#pragma off(unreferenced)
static char rcs_id[]="$Id: disp_dat.c,v 1.2 2003/06/04 23:12:08 wesgarland Exp $";
#pragma on(unreferenced)

/*# name=.BBS-file display routines (ctrl-F data codes)
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <io.h>
#include <fcntl.h>
#include "prog.h"
#include "mm.h"
#include "display.h"

/* void Parse_Datacode(FILE *bbsfile,int *ck_abort,int *allanswers,int type,int *skipcr) */

word DisplayDatacode(DSTK *d)
{
  FILE *quotefile;
  byte *p;

  time_t longtime;
  struct tm *localt;

  sword x;

  switch (DispSlowGetChar(d))
  {
    case 1:   /* Quote */
#ifndef UNIX
      sprintf(d->scratch, "%s.BBS", PRM(quote));
#else
      sprintf(d->scratch, "%s.bbs", PRM(quote));
#endif

      if ((quotefile=shfopen(d->scratch, fopen_read, O_RDONLY))==NULL)
        return 0;

      /* Blank-line-separated quotes.  Build a list of absolute start
       * offsets for each complete quote, then pick one at random (or
       * advance bstats.quote_pos sequentially as a fallback seed).
       * Never print from mid-quote — that produced fragments such as
       * "that the competition already has the order."
       */
      {
        long starts[2048];
        int nstarts=0;
        long pos=0L;
        int at_start=TRUE;

        fseek(quotefile, 0L, SEEK_SET);
        while (nstarts < (int)(sizeof starts / sizeof starts[0]) &&
               fgets(d->scratch, PATHLEN, quotefile) != NULL)
        {
          long after=ftell(quotefile);
          Trim_Line(d->scratch);

          if (! *d->scratch)
          {
            at_start=TRUE;          /* next non-empty line begins a quote */
          }
          else
          {
            if (at_start)
            {
              starts[nstarts++]=pos; /* pos is start of this non-empty line */
              at_start=FALSE;
            }
          }
          pos=after;
        }

        if (nstarts <= 0)
        {
          fclose(quotefile);
          break;
        }

        /* Prefer rotating index stored in shared bstats; mix with rand so
         * concurrent nodes still diverge.
         */
        {
          unsigned idx;
          long qpos;
          byte *p;

          /* Sequential rotation via shared bbstats.bbs quote_pos index */
          idx=(unsigned)(bstats.quote_pos % (dword)nstarts);
          qpos=starts[idx];

          fseek(quotefile, qpos, SEEK_SET);
          while ((p=fgets(d->scratch, PATHLEN, quotefile)) != NULL)
          {
            Trim_Line(d->scratch);
            if (! *d->scratch)
              break;
            Printf("%s\n", d->scratch);
          }

          /* Store next index (not byte offset) so sequential rotation works
           * even when quotes have different lengths.
           */
          bstats.quote_pos=(dword)((idx + 1U) % (unsigned)nstarts);
          d->skipcr=TRUE;
        }
      }

      fclose(quotefile);
      break;

case 2:   /* User's name */
      Puts(usrname);
      break;

    case 3:   /* User's city/state */
      Puts(usr.city);
      break;

    case 4:   /* Current date (string) */
      longtime=time(NULL);
      localt=localtime(&longtime);

      strftime(d->scratch, 15, "%d %b %y", localt);
      Puts(d->scratch);
      break;

    case 5:   /* Total number of usercalls (ordinal) */
      x=usr.times+((d->type & DISPLAY_PCALL) ? 1 : 0);

      Printf("%d%s", x, Ordinal(x));
      break;

    case 6:   /* User's first name */
      Puts(firstname);
      break;

    case 7:   /* Dramatic pause */
      Mdm_flush_ck_tic(4000, (int)d->ck_abort, TRUE);

      vbuf_flush();

      longtime=timerset(50);

      while (! timeup(longtime))
      {
        Mdm_check();
        Giveaway_Slice();
      }

      break;

    case 11: /* Total minutes on-line in past 24hrs, including current call */
      Printf(pu, usr.time+timeonline());
      break;

    case 12:  /* Length of current call, in minutes */
      Printf(pd, timeonline());
      break;

    case 14:  /* Disconnect user */
      if (d->ck_abort)
        Mdm_flush_ck();
      else Mdm_flush();

      mdm_hangup();
      break;

    case 15:  /* Num of minutes remaining for call */
      Printf(pd, timeleft());
      break;

    case 16:  /* Date/time user must be off system (with C/R!) */
      strftime(d->scratch, 26, asctime_format, 
               localtime((time_t *)(&timeoff)));
      Puts(d->scratch);
      break;

    case 17:  /* Total number of system calls (ordinal) */
      Printf("%lu%s",
             bstats.num_callers,
             Ordinal(bstats.num_callers));
      break;

    case 18:  /* Net downloads (dl-ul) */
      Printf(pl, usr.downtoday-ultoday);
      break;

    case 20:  /* Current Time */
      strftime(d->scratch, 15, "%H:%M:%S",
               localtime(((longtime=time(NULL)),&longtime)));

      Puts(d->scratch);
      break;

    case 21:  /* AllAnswers Required */
      d->allanswers=TRUE;
      break;

    case 22:  /* AllAnswers Optional */
      d->allanswers=FALSE;
      break;

    case 23:  /* Total user ULs */
      Printf(plu, usr.up);
      break;

    case 24:  /* Total user DLs */
      Printf(plu, usr.down);
      break;

    case 25:  /* UL:DL ratio */
      Printf("%lu:%lu",
             (long)(usr.up==0 ? 0L : 1L),
             (long)(usr.down/(usr.up==0L ? 1L : usr.up)));
      break;
  }
  
  return 0;
}


