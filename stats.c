/* File: stats.c
 *
 * Statistics initialization, collection, and output routines for DISPHONETIC.
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: stats.c,v 1.3 1998/09/28 03:30:07 risto Exp risto $
 */

#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "defs.h"
#include "globals.c"


/************ statistics constants *************/

#define WITHINERR 0.15		/* close enough to be counted correct */


/******************** function prototypes ************************/

/* global functions */
#include "prototypes.h"

/* functions local to this file */
static double reszero __P ((double num, int den));


/******************** static variables ************************/

/* statistics */
static int 
  nout[NMODULES],		/* number of times output was counted */
  within[NMODULES],		/* units close enough */
  all[NMODULES][MAXWORDS],	/* # of all words output */
  corr[NMODULES][MAXWORDS];	/* # of words output correctly */
static double deltasum[NMODULES]; /* cumulative error sum */


/*********************  initializations ******************************/

void
init_stats ()
/* initialize the statistics tables */
{
  int modi,			/* module number */
    i;

  /* separate stats for parsing+paraphrasing stories and question answering */
  for (modi = 0; modi < NMODULES; modi++)
    {
      nout[modi] = 0;
      deltasum[modi] = 0.0;
      if (testing)
	{
	  within[modi] = 0;
	  /* there is a different number of phonolical vs. semantic words */
	  for (i = 0;
	       i < ((modi == SINPMOD || modi == SOUTMOD) ? nswords : ((modi == L1LEXINPMOD || modi == L1LEXOUTMOD || modi == L1PHONOLINPMOD || modi == L1PHONOLOUTMOD || modi == L1PHONETICINPMOD || modi == L1PHONETICOUTMOD) ? nl1words : nl2words));
	       i++)
	    corr[modi][i] = all[modi][i] = 0;
	}
    }
}


/*********************  cumulation ******************************/

void
collect_stats (modi)
/* the main routine for accumulating performance statistics */
/* all modules call up this routine after propagation */
     int modi;			/* module number */
{
  int i, nearest;
  double uniterror;		/* error of this unit */
  WORDSTRUCT *words;		/* phonolicon (phonolical or semantic) */
  int nrep;			/* rep dimension (phonolical or semantic) */
  int nwords;			/* number of words (phonolical or semantic) */

  /* first select the right lexicon */
  select_lexicon (modi, &words, &nrep, &nwords);
  
  /* cumulate error for each output unit */
  for (i = 0; i < noutrep[modi]; i++)
    {
      /* calculate the absolute error of this unit */
      uniterror = fabs (tgtrep[modi][i] - outrep[modi][i]);
      /* cumulate error (for average) */
      deltasum[modi] += uniterror;
      if (testing)
	/* cumulate number of "correct" units (close enough to correct) */
	if (uniterror < WITHINERR)
	  within[modi]++;
    }

  if (!testing)
    /* cumulate output count */
    nout[modi]++;
  else
    {
      /* cumulate word-based error counts */
      all[modi][target[modi]]++;	/* cumulate total number of words */
      /* find the representation in the phonolicon closest to the output */
      nearest = find_nearest (outrep[modi], words, nrep, nwords);
      if (nearest == target[modi])
	/* it was the right one; update the correct count for this word */
	corr[modi][target[modi]]++;
    }
}


/*********************  results  ******************************/

void
write_error (fp)
    FILE *fp;
{
  int modi;			/* module number */
  
  for (modi = 0; modi < NMODULES; modi++)
    fprintf (fp, " %f", reszero (deltasum[modi], nout[modi] * noutrep[modi]));
  fprintf (fp, "\n");
}


void
print_stats (epoch)
/* print out the performance statistics in the log output */
    int epoch;			/* current epoch number */
{
  int modi,			/* module number */
    nwords, nrep,		/* number of words and rep components */
    i,
    sum_corr, sum_all,		/* number of correct and all words */
    sum_corrinst, sum_allinst;	/* number of correct and allinstances */

  printf ("\n%s (%d pairs), epoch %d\n", current_inpfile, npairs, epoch);
  printf ("Module        All   Inst  <%.2f    AvgErr\n", WITHINERR);
  /* print all modules separately */
  for (modi = 0; modi < NMODULES; modi++)
    {
      /* first set the dimensions properly (phonolical or semantic) */
      nwords = (modi == SINPMOD || modi == SOUTMOD) ? nswords : ((modi == L1PHONETICINPMOD || modi == L1PHONETICOUTMOD || modi == L1PHONOLINPMOD || modi == L1PHONOLOUTMOD || modi == L1PHONETICINPMOD || modi == L1PHONETICOUTMOD) ? nl1words : nl2words);
      
      if (modi == SINPMOD || modi == SOUTMOD) {
        nrep = nsrep;
      } else if (modi == L1PHONETICINPMOD || modi == L1PHONETICOUTMOD) {
        nrep = nl1lexrep;
      } else if (modi == L1PHONOLINPMOD || modi == L1PHONOLOUTMOD) {
        nrep = nl1phonolrep;
        nwords = nl1words;
      } else if (modi == L1PHONETICINPMOD || modi == L1PHONETICOUTMOD) {
        nrep = nl1phoneticrep;
        nwords = nl1words;
      } else if (modi == L2PHONETICINPMOD || modi == L2PHONETICOUTMOD) {
        nrep = nl2lexrep;
      } else if (modi == L2PHONOLINPMOD || modi == L2PHONOLOUTMOD) {
        nrep = nl2phonolrep;
      } else {
        nrep = nl2phoneticrep;
      }

      sum_corr = sum_all = sum_corrinst = sum_allinst = 0;
      /* sum up all word counts and all correct word counts */
      for (i = 0; i < nwords; i++)
	{
	  sum_corr += corr[modi][i];
	  sum_all += all[modi][i];
	}
      /* sum up all instance word counts */
      for (i = 0; i < ninstances; i++)
	{
	  sum_corrinst += corr[modi][instances[i]];
	  sum_allinst += all[modi][instances[i]];
	}
      printf ("Module %2d: ", modi);
      printf ("%6.1f %6.1f %6.1f %9.4f\n",
	      /* percentage of correct words */
	      reszero (100.0 * sum_corr, sum_all),
	      /* percentage of correct instances */
	      reszero (100.0 * sum_corrinst, sum_allinst),
	      /* percentage of output units close enough */
	      reszero (100.0 * within[modi], sum_all * nrep),
	      /* average error per output unit */
	      reszero (deltasum[modi], sum_all * nrep));
    }
}


void 
collect_assoc_stats(epoch)
  /* mimics BNT -- only test for 60 word items */
  /* test if correct units in L1 and L2 light up when a sem word is selected */
  int epoch;
{
   int besti, bestj, pair_index, s_i, s_j, i, j, ii, jj,
      l1_lex_i, l1_lex_j, l1_phonol_i, l1_phonol_j, l1_phonetic_i, l1_phonetic_j,
      l2_lex_i, l2_lex_j, l2_phonol_i, l2_phonol_j, l2_phonetic_i, l2_phonetic_j,
      l1_lex_index, l1_phonol_index, l1_phonetic_index,
      l2_lex_index, l2_phonol_index, l2_phonetic_index,
      s_index;
   int l1_correct = 0;
   int l2_correct = 0;
   double best, foo; /* best and worst response found */


   bool verbose = FALSE;

   if (verbose)
    printf("Wrong words (sem -> lex -> phonol -> phonetic):\n");

   /* for each unit with a label in the sem map, find the L1 and L2 units with max response */

   for (s_index = 0; s_index < nswords; s_index++) {
      find_closest_unit(&s_i, &s_j, nsnet, sunits, swords, s_index, nsrep);

      /* find the pair containing the semantic word */
      for (pair_index = 0; pair_index < npairs; pair_index++) {
        if (pairs[pair_index].sindex == s_index) {
          break;
        }
      }
      
      /* add sem activations to l1 lex map */
      for (i = 0; i < nl1net; i++)
        for (j = 0; j < nl1net; j++)
          {
            l1lexunits[i][j].value = sl1lexassoc[s_i][s_j][i][j];
          }
      
      /* add sem activations to l2 lex map */
      for (i = 0; i < nl2net; i++)
        for (j = 0; j < nl2net; j++)
          {
            l2lexunits[i][j].value = sl2lexassoc[s_i][s_j][i][j];
          }

      /* now update l1 lex with activations from l2 lex, keeping track of old value for testing l2 */
      best = (-1);
      foo = (-1);
      for (i = 0; i < nl1net; i++)
        for (j = 0; j < nl1net; j++) {
          l1lexunits[i][j].prevvalue = l1lexunits[i][j].value;
          for (ii = 0; ii < nl2net; ii++)
            for (jj = 0; jj < nl2net; jj++)
          {
            l1lexunits[i][j].value += l2lexunits[ii][jj].value * l2l1lexassoc[ii][jj][i][j];
          }   
          updatebestworst (&best, &foo, &l1_lex_i, &l1_lex_j, &l1lexunits[i][j],
               i, j, fgreater, fsmaller); 
        }  

      l1_lex_index = find_nearest (l1lexunits[l1_lex_i][l1_lex_j].comp, l1lexwords, nl1lexrep, nl1words);

      /* now update l2 lex with activations from l1 lex */
      best = (-1);
      foo = (-1);
      for (i = 0; i < nl2net; i++)
        for (j = 0; j < nl2net; j++) {
          for (ii = 0; ii < nl1net; ii++)
            for (jj = 0; jj < nl1net; jj++)
          {
            l2lexunits[i][j].value += l1lexunits[ii][jj].prevvalue * l1l2lexassoc[ii][jj][i][j];
          }
        updatebestworst (&best, &foo, &l2_lex_i, &l2_lex_j, &l2lexunits[i][j],
             i, j, fgreater, fsmaller);
        }

      l2_lex_index = find_nearest (l2lexunits[l2_lex_i][l2_lex_j].comp, l2lexwords, nl2lexrep, nl2words);



      /* add l1 lex activations to l1 phonol map */
      for (i = 0; i < nl1net; i++)
        for (j = 0; j < nl1net; j++) {
          l1phonolunits[i][j].value = 0;
          for (ii = 0; ii < nl1net; ii++)
            for (jj = 0; jj < nl1net; jj++)
          {
            l1phonolunits[i][j].value += l1lexunits[ii][jj].value * l1lexphonolassoc[ii][jj][i][j];
          }
        }
      
      /* add l2 lex activations to l2 phonol map */
      for (i = 0; i < nl2net; i++)
        for (j = 0; j < nl2net; j++) {
          l2phonolunits[i][j].value = 0;
          for (ii = 0; ii < nl2net; ii++)
            for (jj = 0; jj < nl2net; jj++)
          {
            l2phonolunits[i][j].value += l2lexunits[ii][jj].value * l2lexphonolassoc[ii][jj][i][j];
          }
        }

      /* now update l1 phonol with activations from l2 phonol, keeping track of old value for testing l2 */
      best = (-1);
      foo = (-1);
      for (i = 0; i < nl1net; i++)
        for (j = 0; j < nl1net; j++) {
          l1phonolunits[i][j].prevvalue = l1phonolunits[i][j].value;
          for (ii = 0; ii < nl2net; ii++)
            for (jj = 0; jj < nl2net; jj++)
          {
            l1phonolunits[i][j].value += l2phonolunits[ii][jj].value * l2l1phonolassoc[ii][jj][i][j];
          }   
          updatebestworst (&best, &foo, &l1_phonol_i, &l1_phonol_j, &l1phonolunits[i][j],
               i, j, fgreater, fsmaller); 
        }  

      l1_phonol_index = find_nearest (l1phonolunits[l1_phonol_i][l1_phonol_j].comp, l1phonolwords, nl1phonolrep, nl1words);

      /* now update l2 phonol with activations phonol from l1 */
      best = (-1);
      foo = (-1);
      for (i = 0; i < nl2net; i++)
        for (j = 0; j < nl2net; j++) {
          for (ii = 0; ii < nl1net; ii++)
            for (jj = 0; jj < nl1net; jj++)
          {
            l2phonolunits[i][j].value += l1phonolunits[ii][jj].prevvalue * l1l2phonolassoc[ii][jj][i][j];
          }
        updatebestworst (&best, &foo, &l2_phonol_i, &l2_phonol_j, &l2phonolunits[i][j],
             i, j, fgreater, fsmaller);
        }

      l2_phonol_index = find_nearest (l2phonolunits[l2_phonol_i][l2_phonol_j].comp, l2phonolwords, nl2phonolrep, nl2words);



      /* add l1 phonol activations to l1 phonetic map */
      for (i = 0; i < nl1net; i++)
        for (j = 0; j < nl1net; j++) {
          l1phoneticunits[i][j].value = 0;
          for (ii = 0; ii < nl1net; ii++)
            for (jj = 0; jj < nl1net; jj++)
          {
            l1phoneticunits[i][j].value += l1phonolunits[ii][jj].value * l1phonolphoneticassoc[ii][jj][i][j];
          }
        }
      
      /* add l2 phonol activations to l2 phonetic map */
      for (i = 0; i < nl2net; i++)
        for (j = 0; j < nl2net; j++) {
          l2phoneticunits[i][j].value = 0;
          for (ii = 0; ii < nl2net; ii++)
            for (jj = 0; jj < nl2net; jj++)
          {
            l2phoneticunits[i][j].value += l2phonolunits[ii][jj].value * l2phonolphoneticassoc[ii][jj][i][j];
          }
        }

      /* now update l1 phonetic with activations from l2 phonetic, keeping track of old value for testing l2 */
      best = (-1);
      foo = (-1);
      for (i = 0; i < nl1net; i++)
        for (j = 0; j < nl1net; j++) {
          l1phoneticunits[i][j].prevvalue = l1phoneticunits[i][j].value;
          for (ii = 0; ii < nl2net; ii++)
            for (jj = 0; jj < nl2net; jj++)
          {
            l1phoneticunits[i][j].value += l2phoneticunits[ii][jj].value * l2l1phoneticassoc[ii][jj][i][j];
          }   
          updatebestworst (&best, &foo, &l1_phonetic_i, &l1_phonetic_j, &l1phoneticunits[i][j],
               i, j, fgreater, fsmaller); 
        }  

      l1_phonetic_index = find_nearest (l1phoneticunits[l1_phonetic_i][l1_phonetic_j].comp, l1phoneticwords, nl1phoneticrep, nl1words);

      /* now update l2 phonetic with activations phonetic from l1 */
      best = (-1);
      foo = (-1);
      for (i = 0; i < nl2net; i++)
        for (j = 0; j < nl2net; j++) {
          for (ii = 0; ii < nl1net; ii++)
            for (jj = 0; jj < nl1net; jj++)
          {
            l2phoneticunits[i][j].value += l1phoneticunits[ii][jj].prevvalue * l1l2phoneticassoc[ii][jj][i][j];
          }
        updatebestworst (&best, &foo, &l2_phonetic_i, &l2_phonetic_j, &l2phoneticunits[i][j],
             i, j, fgreater, fsmaller);
        }

      l2_phonetic_index = find_nearest (l2phoneticunits[l2_phonetic_i][l2_phonetic_j].comp, l2phoneticwords, nl2phoneticrep, nl2words);



      if (l1_lex_index == pairs[pair_index].l1lexindex &&
          l1_phonol_index == pairs[pair_index].l1phonolindex &&
          l1_phonetic_index == pairs[pair_index].l1phoneticindex) {
        l1_correct = l1_correct + 1;                 
      } else if (verbose) {
        printf("L1: %s\t%s\t%s\t%s\n", swords[s_index].chars, l1lexwords[l1_lex_index].chars, l1phonolwords[l1_phonol_index].chars, l1phoneticwords[l1_phonetic_index].chars);
      }
      if (l2_lex_index == pairs[pair_index].l2lexindex &&
          l2_phonol_index == pairs[pair_index].l2phonolindex &&
          l2_phonetic_index == pairs[pair_index].l2phoneticindex) {
        l2_correct = l2_correct + 1;                 
      } else if (verbose) {
        printf("L2: %s\t%s\t%s\t%s\n", swords[s_index].chars, l2lexwords[l2_lex_index].chars, l2phonolwords[l2_phonol_index].chars, l2phoneticwords[l2_phonetic_index].chars);
      }
   }
   printf("\nEpoch %d\n", epoch);
   printf("\nL1 correct: %d/%d (%.2f%%)\n", l1_correct, nl1words, 100.0 * (double) ((double)l1_correct/(double)nl1words));
   printf("L2 correct: %d/%d (%.2f%%)\n", l2_correct, nl2words, 100.0 * (double) ((double)l2_correct/(double)nl2words));
}



static double
reszero (num, den)
/* if no data was collected, should print 0 instead of crashing */
     double num;			/* numerator */
     int den;				/* denominator */
{
  if (den == 0)
    return (0.0);
  else
    return (num / (double) den);
}
