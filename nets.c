/* File: nets.c
 *
 * The lexical and semantic maps for DISLEX
 *
 * Copyright (C) 1994 Risto Miikkulainen
 *
 *  This software can be copied, modified and distributed freely for
 *  educational and research purposes, provided that this notice is included
 *  in the code, and the author is acknowledged in any materials and reports
 *  that result from its use. It may not be used for commercial purposes
 *  without expressed permission from the author.
 *
 * $Id: nets.c,v 1.15 1994/09/19 05:46:46 risto Exp risto $
 */

#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "defs.h"
#include "globals.c"


/************ lexicon constants *************/

/* list of units with highest activation;
   propagation takes place through these units */
typedef struct LEXPROPUNIT
  {
    int i, j;			/* index of the unit */
    float value;		/* activation value */
  }
LEXPROPUNIT;


/******************** Function prototypes ************************/

/* global functions */
#include "prototypes.h"

/* functions local to this file */
static void present_input __P ((int modi, FMUNIT units[MAXLSNET][MAXLSNET],
				int nnet, WORDSTRUCT words[], int index,
				LEXPROPUNIT prop[], int *nprop, int nc));
static void associate __P ((FMUNIT inputunits[MAXLSNET][MAXLSNET],
			    int modi, FMUNIT units[MAXLSNET][MAXLSNET], 
			    int nnet, WORDSTRUCT words[],
			    int index,  LEXPROPUNIT prop[], int nprop,
			double assoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET]));
static void find_input_image __P ((int modi,
				   FMUNIT inpunits[MAXLSNET][MAXLSNET],
				   int ninpnet, int *besti, int *bestj));
static void compute_propunits __P ((FMUNIT units[MAXLSNET][MAXLSNET],int nnet,
				    LEXPROPUNIT prop[], int *nprop,
				    int lowi, int lowj, int highi, int highj));
static void find_assoc_image __P ((int modi,
				   FMUNIT inpunits[MAXLSNET][MAXLSNET],
				   LEXPROPUNIT prop[], int nprop,
				   FMUNIT outunits[MAXLSNET][MAXLSNET],
				   int noutnet,
				   double assoc[MAXLSNET][MAXLSNET]
				                [MAXLSNET][MAXLSNET],
				   double *abest));
static void compute_assoc_activations __P((FMUNIT outunits[MAXLSNET][MAXLSNET],
					   int noutnet, double abest));
static void assocresponse __P((FMUNIT *unit, int i, int j,
			       FMUNIT inpunits[MAXLSNET][MAXLSNET],
			       double assoc[MAXLSNET][MAXLSNET]
			                   [MAXLSNET][MAXLSNET],
			       LEXPROPUNIT prop[], int nprop));
static void findprop __P((int lowi, int lowj, int highi, int highj,
			  FMUNIT inpunits[MAXLSNET][MAXLSNET], double *best,
			  double *worst, LEXPROPUNIT prop[], int *nprop));
static int propunit __P((int i, int j, LEXPROPUNIT prop[], int nprop,
			 int lowi, int lowj, int highi, int highj));
static void modify_input_weights __P ((int modi,
				       FMUNIT inpunits[MAXLSNET][MAXLSNET],
				       double alpha,
				       LEXPROPUNIT prop[], int nprop));
static void modify_assoc_weights __P ((FMUNIT iunits[MAXLSNET][MAXLSNET],
				       FMUNIT aunits[MAXLSNET][MAXLSNET],
				       LEXPROPUNIT iprop[], int niprop, 
				       LEXPROPUNIT aprop[], int naprop,
				       int nanet,
				       double assoc[MAXLSNET][MAXLSNET]
				       [MAXLSNET][MAXLSNET], 
               double alpha));
static int clip __P((int index, int limit, int (*comparison) (int, int)));
static int ige __P((int i, int j));
static int ile __P((int i, int j));
static bool bool_with_prob __P ((double probability));


/*******************  main wordpair processing loop  **********************/

void
iterate_pairs ()
/* iterate through all pairs; present to maps, propagate, change weights */
{
  /* propagation through these units */
  LEXPROPUNIT l1lexprop[MAXLSNET * MAXLSNET],
    l2lexprop[MAXLSNET * MAXLSNET],
    l1phonolprop[MAXLSNET * MAXLSNET],
    l2phonolprop[MAXLSNET * MAXLSNET],
    l1phoneticprop[MAXLSNET * MAXLSNET],
    l2phoneticprop[MAXLSNET * MAXLSNET],
    sprop[MAXLSNET * MAXLSNET];
  int pairi,				/* word pair counter */
    nl1lexprop, nl1phonolprop, nl1phoneticprop,   /* lex and sem number of prop units */
    nl2lexprop, nl2phonolprop, nl2phoneticprop, 
    nsprop;			

  /* first display the current labels and clean previous activations */
  if (displaying)
    {
      if (l1_running || l1l2_assoc_running || sl1_assoc_running) {
	init_lex_display (L1LEXWINMOD, nl1net, nl1words, l1lexwords, nl1lexrep, l1lexunits);
  init_lex_display (L1PHONOLWINMOD, nl1net, nl1words, l1phonolwords, nl1phonolrep, l1phonolunits);
  init_lex_display (L1PHONETICWINMOD, nl1net, nl1words, l1phoneticwords, nl1phoneticrep, l1phoneticunits);
      }
      if (l2_running || l1l2_assoc_running || sl2_assoc_running) {
  init_lex_display (L2LEXWINMOD, nl2net, nl2words, l2lexwords, nl2lexrep, l2lexunits);
  init_lex_display (L2PHONOLWINMOD, nl2net, nl2words, l2phonolwords, nl2phonolrep, l2phonolunits);
  init_lex_display (L2PHONETICWINMOD, nl2net, nl2words, l2phoneticwords, nl2phoneticrep, l2phoneticunits); 
      }
      if (sem_running || sl1_assoc_running || sl2_assoc_running) {
	init_lex_display (SEMWINMOD, nsnet, nswords, swords, nsrep, sunits);
      }
      /* stop if stepping, check for events */
      wait_and_handle_events ();
    }
  
  /* iterate through all word pairs */
  for (pairi = 0; pairi < npairs; pairi++)
    {
      bool train_l1 = bool_with_prob(l1_exposure);
      bool train_l2 = bool_with_prob(l2_exposure);
      
      /* first propagate from within L1 subcomponents to L2 and semantic */
      if (pairs[shuffletable[pairi]].l1phoneticindex != NONE && 
          pairs[shuffletable[pairi]].l1phonolindex != NONE && 
          pairs[shuffletable[pairi]].l1lexindex != NONE && 
          train_l1)
	{
	  if ((l1_running || l1l2_assoc_running || sl1_assoc_running)) {
	    present_input (L1LEXINPMOD, l1lexunits, nl1net, l1lexwords,
			   pairs[shuffletable[pairi]].l1lexindex,
			   l1lexprop, &nl1lexprop, l1_nc); 
      present_input (L1PHONOLINPMOD, l1phonolunits, nl1net, l1phonolwords,
         pairs[shuffletable[pairi]].l1phonolindex,
         l1phonolprop, &nl1phonolprop, l1_nc); 
      present_input (L1PHONETICINPMOD, l1phoneticunits, nl1net, l1phoneticwords,
         pairs[shuffletable[pairi]].l1phoneticindex,
         l1phoneticprop, &nl1phoneticprop, l1_nc); 
    }

    if (!testing && l1_running) {
      modify_input_weights (L1LEXINPMOD, l1lexunits, l1_alpha, l1lexprop, nl1lexprop);
      modify_input_weights (L1PHONOLINPMOD, l1phonolunits, l1_alpha, l1phonolprop, nl1phonolprop);
      modify_input_weights (L1PHONETICINPMOD, l1phoneticunits, l1_alpha, l1phoneticprop, nl1phoneticprop);
    }

	  if (testing && sl1_assoc_running)
	    associate (l1lexunits, SOUTMOD, sunits, nsnet, swords,
		       pairs[shuffletable[pairi]].sindex,
		       l1lexprop, nl1lexprop, l1lexsassoc);
    if (testing && l1_running) {
      /* lex to phonol */
      associate(l1lexunits, L1PHONOLOUTMOD, l1phonolunits, nl1net, l1phonolwords,
           pairs[shuffletable[pairi]].l1phonolindex,
           l1lexprop, nl1lexprop, l1lexphonolassoc);
      /* phonol to lex */
      associate(l1phonolunits, L1LEXOUTMOD, l1lexunits, nl1net, l1lexwords,
           pairs[shuffletable[pairi]].l1lexindex,
           l1phonolprop, nl1phonolprop, l1phonollexassoc);
      /* phonol to phonetic */
      associate(l1phonolunits, L1PHONETICOUTMOD, l1phoneticunits, nl1net, l1phoneticwords,
           pairs[shuffletable[pairi]].l1phoneticindex,
           l1phonolprop, nl1phonolprop, l1phonolphoneticassoc);
      /* phonetic to phonol */
      associate(l1phoneticunits, L1PHONOLOUTMOD, l1phonolunits, nl1net, l1phonolwords,
           pairs[shuffletable[pairi]].l1phonolindex,
           l1phoneticprop, nl1phoneticprop, l1phoneticphonolassoc);
    }
    if (testing && l1l2_assoc_running) {
      /* lex */
      associate (l1lexunits, L2LEXOUTMOD, l2lexunits, nl2net, l2lexwords,
           pairs[shuffletable[pairi]].l2lexindex,
           l1lexprop, nl1lexprop, l1l2lexassoc);
      /* phonol */
      associate (l1phonolunits, L2PHONOLOUTMOD, l2phonolunits, nl2net, l2phonolwords,
           pairs[shuffletable[pairi]].l2phonolindex,
           l1phonolprop, nl1phonolprop, l1l2phonolassoc);
      /* phonetic */
      associate (l1phoneticunits, L2PHONETICOUTMOD, l2phoneticunits, nl2net, l2phoneticwords,
           pairs[shuffletable[pairi]].l2phoneticindex,
           l1phoneticprop, nl1phoneticprop, l1l2phoneticassoc);
    }
	  if (testing && displaying && (l1_running || l1l2_assoc_running || sl1_assoc_running))
	    {	
	      display_lex (L1LEXINPMOD, l1lexunits, nl1net);
	      display_error (L1LEXINPMOD);
        display_lex (L1PHONOLINPMOD, l1phonolunits, nl1net);
        display_error (L1PHONOLINPMOD);
        display_lex (L1PHONETICINPMOD, l1phoneticunits, nl1net);
        display_error (L1PHONETICINPMOD);

	      if (sl1_assoc_running)
		{
		  display_lex (SOUTMOD, sunits, nsnet);
		  display_error (SOUTMOD);
		}
        if (l1l2_assoc_running)
    {
        display_lex (L2LEXOUTMOD, l2lexunits, nl2net);
        display_error (L2LEXOUTMOD);
        display_lex (L2PHONOLOUTMOD, l2phonolunits, nl2net);
        display_error (L2PHONOLOUTMOD);
        display_lex (L2PHONETICOUTMOD, l2phoneticunits, nl2net);
        display_error (L2PHONETICOUTMOD);

    }
	      wait_and_handle_events ();
	    }
	}

        /* then propagate from L2 subcomponents to L1 and semantic */
      if (pairs[shuffletable[pairi]].l2phoneticindex != NONE && 
          pairs[shuffletable[pairi]].l2phonolindex != NONE && 
          pairs[shuffletable[pairi]].l2lexindex != NONE && 
          train_l2)
  {
    if ((l2_running || l1l2_assoc_running || sl2_assoc_running)) {
      present_input (L2LEXINPMOD, l2lexunits, nl2net, l2lexwords,
         pairs[shuffletable[pairi]].l2lexindex,
         l2lexprop, &nl2lexprop, l2_nc); 
      present_input (L2PHONOLINPMOD, l2phonolunits, nl2net, l2phonolwords,
         pairs[shuffletable[pairi]].l2phonolindex,
         l2phonolprop, &nl2phonolprop, l2_nc); 
      present_input (L2PHONETICINPMOD, l2phoneticunits, nl2net, l2phoneticwords,
         pairs[shuffletable[pairi]].l2phoneticindex,
         l2phoneticprop, &nl2phoneticprop, l2_nc); 
    }

    if (!testing && l2_running) {
      modify_input_weights (L2LEXINPMOD, l2lexunits, l2_alpha, l2lexprop, nl2lexprop);
      modify_input_weights (L2PHONOLINPMOD, l2phonolunits, l2_alpha, l2phonolprop, nl2phonolprop);
      modify_input_weights (L2PHONETICINPMOD, l2phoneticunits, l2_alpha, l2phoneticprop, nl2phoneticprop);
    }

    if (testing && sl2_assoc_running)
      associate (l2lexunits, SOUTMOD, sunits, nsnet, swords,
           pairs[shuffletable[pairi]].sindex,
           l2lexprop, nl2lexprop, l2lexsassoc);
    if (testing && l2_running) {
      /* lex to phonol */
      associate(l2lexunits, L2PHONOLOUTMOD, l2phonolunits, nl2net, l2phonolwords,
           pairs[shuffletable[pairi]].l2phonolindex,
           l2lexprop, nl2lexprop, l2lexphonolassoc);
      /* phonol to lex */
      associate(l2phonolunits, L2LEXOUTMOD, l2lexunits, nl2net, l2lexwords,
           pairs[shuffletable[pairi]].l2lexindex,
           l2phonolprop, nl2phonolprop, l2phonollexassoc);
      /* phonol to phonetic */
      associate(l2phonolunits, L2PHONETICOUTMOD, l2phoneticunits, nl2net, l2phoneticwords,
           pairs[shuffletable[pairi]].l2phoneticindex,
           l2phonolprop, nl2phonolprop, l2phonolphoneticassoc);
      /* phonetic to phonol */
      associate(l2phoneticunits, L2PHONOLOUTMOD, l2phonolunits, nl2net, l2phonolwords,
           pairs[shuffletable[pairi]].l2phonolindex,
           l2phoneticprop, nl2phoneticprop, l2phoneticphonolassoc);
    }
    if (testing && l1l2_assoc_running) {
      /* lex */
      associate (l2lexunits, L1LEXOUTMOD, l1lexunits, nl1net, l1lexwords,
           pairs[shuffletable[pairi]].l1lexindex,
           l2lexprop, nl2lexprop, l2l1lexassoc);
      /* phonol */
      associate (l2phonolunits, L1PHONOLOUTMOD, l1phonolunits, nl1net, l1phonolwords,
           pairs[shuffletable[pairi]].l1phonolindex,
           l2phonolprop, nl2phonolprop, l2l1phonolassoc);
      /* phonetic */
      associate (l2phoneticunits, L1PHONETICOUTMOD, l1phoneticunits, nl1net, l1phoneticwords,
           pairs[shuffletable[pairi]].l1phoneticindex,
           l2phoneticprop, nl2phoneticprop, l2l1phoneticassoc);
    }
    if (testing && displaying && (l2_running || l1l2_assoc_running || sl2_assoc_running))
      { 
        display_lex (L2LEXINPMOD, l2lexunits, nl2net);
        display_error (L2LEXINPMOD);
        display_lex (L2PHONOLINPMOD, l2phonolunits, nl2net);
        display_error (L2PHONOLINPMOD);
        display_lex (L2PHONETICINPMOD, l2phoneticunits, nl2net);
        display_error (L2PHONETICINPMOD);

        if (sl1_assoc_running)
    {
      display_lex (SOUTMOD, sunits, nsnet);
      display_error (SOUTMOD);
    }
        if (l1l2_assoc_running)
    {
        display_lex (L1LEXOUTMOD, l1lexunits, nl1net);
        display_error (L1LEXOUTMOD);
        display_lex (L1PHONOLOUTMOD, l1phonolunits, nl1net);
        display_error (L1PHONOLOUTMOD);
        display_lex (L1PHONETICOUTMOD, l1phoneticunits, nl1net);
        display_error (L1PHONETICOUTMOD);

    }
        wait_and_handle_events ();
      }
  } 
  

      /* then propagate from semantic to L1 and L2 */
      if (pairs[shuffletable[pairi]].sindex != NONE)
	{
	  if (sem_running || sl1_assoc_running || sl2_assoc_running)
	    present_input (SINPMOD, sunits, nsnet, swords,
			   pairs[shuffletable[pairi]].sindex,
			   sprop, &nsprop, sem_nc);
	  if (!testing & sem_running)
	    modify_input_weights (SINPMOD, sunits, sem_alpha, sprop, nsprop);
	  if (testing && sl1_assoc_running)
	    associate (sunits, L1LEXOUTMOD, l1lexunits, nl1net, l1lexwords,
		       pairs[shuffletable[pairi]].l1lexindex,
		       sprop, nsprop, sl1lexassoc);
    if (testing && sl2_assoc_running)
      associate (sunits, L2LEXOUTMOD, l2lexunits, nl2net, l2lexwords,
           pairs[shuffletable[pairi]].l2lexindex,
           sprop, nsprop, sl2lexassoc);
	  if (testing && displaying && (sem_running || sl1_assoc_running || sl2_assoc_running))
	    {
	      display_lex (SINPMOD, sunits, nsnet);
	      display_error (SINPMOD);
	      if (sl1_assoc_running)
		{
		  display_lex (L1LEXOUTMOD, l1lexunits, nl1net);
		  display_error (L1LEXOUTMOD);
		}
        if (sl2_assoc_running)
    {
      display_lex (L2LEXOUTMOD, l2lexunits, nl2net);
      display_error (L2LEXOUTMOD);
    }
	      wait_and_handle_events ();
	    }
	}

  /* finally, update associations */
  /* l1 lex and sem */
  if (!testing && sl1_assoc_running && train_l1 &&
	  pairs[shuffletable[pairi]].l1lexindex != NONE &&
	  pairs[shuffletable[pairi]].sindex != NONE)
	{
	  modify_assoc_weights (l1lexunits, sunits, l1lexprop, nl1lexprop, sprop, nsprop,
				nsnet, l1lexsassoc, sl1_assoc_alpha);
	  modify_assoc_weights (sunits, l1lexunits, sprop, nsprop, l1lexprop, nl1lexprop,
				nl1net, sl1lexassoc, sl1_assoc_alpha);
	  if (displaying)
	    {	
	      display_lex (L1LEXINPMOD, l1lexunits, nl1net);
	      display_error (L1LEXINPMOD);
	      display_lex (SINPMOD, sunits, nsnet);
	      display_error (SINPMOD);
	      wait_and_handle_events ();
	    }
	}

  /* l1 lex and phonol */
  if (!testing && l1_running && train_l1 &&
    pairs[shuffletable[pairi]].l1lexindex != NONE &&
    pairs[shuffletable[pairi]].l1phonolindex != NONE)
  {
    modify_assoc_weights (l1lexunits, l1phonolunits, l1lexprop, nl1lexprop, l1phonolprop, nl1phonolprop,
        nl1net, l1lexphonolassoc, l1_alpha);
    modify_assoc_weights (l1phonolunits, l1lexunits, l1phonolprop, nl1phonolprop, l1lexprop, nl1lexprop,
        nl1net, l1phonollexassoc, l1_alpha);
    if (displaying)
      { 
        display_lex (L1LEXINPMOD, l1lexunits, nl1net);
        display_error (L1LEXINPMOD);
        display_lex (L1PHONOLINPMOD, l1phonolunits, nl1net);
        display_error (L1PHONOLINPMOD);
        wait_and_handle_events ();
      }
  }

  /* l1 phonol and phonetic */
  if (!testing && l1_running && train_l1 &&
    pairs[shuffletable[pairi]].l1phonolindex != NONE &&
    pairs[shuffletable[pairi]].l1phoneticindex != NONE)
  {
    modify_assoc_weights (l1phonolunits, l1phoneticunits, l1phonolprop, nl1phonolprop, l1phoneticprop, nl1phoneticprop,
        nl1net, l1phonolphoneticassoc, l1_alpha);
    modify_assoc_weights (l1phoneticunits, l1phonolunits, l1phoneticprop, nl1phoneticprop, l1phonolprop, nl1phonolprop,
        nl1net, l1phoneticphonolassoc, l1_alpha);
    if (displaying)
      { 
        display_lex (L1PHONOLINPMOD, l1phonolunits, nl1net);
        display_error (L1PHONOLINPMOD);
        display_lex (L1PHONETICINPMOD, l1phonolunits, nl1net);
        display_error (L1PHONETICINPMOD);
        wait_and_handle_events ();
      }
  }

  /* l2 lex and sem */
  if (!testing && sl2_assoc_running && train_l2 &&
    pairs[shuffletable[pairi]].l2lexindex != NONE &&
    pairs[shuffletable[pairi]].sindex != NONE)
  {
    modify_assoc_weights (l2lexunits, sunits, l2lexprop, nl2lexprop, sprop, nsprop,
        nsnet, l2lexsassoc, sl2_assoc_alpha);
    modify_assoc_weights (sunits, l2lexunits, sprop, nsprop, l2lexprop, nl2lexprop,
        nl2net, sl2lexassoc, sl2_assoc_alpha);
    if (displaying)
      { 
        display_lex (L2LEXINPMOD, l2lexunits, nl2net);
        display_error (L2LEXINPMOD);
        display_lex (SINPMOD, sunits, nsnet);
        display_error (SINPMOD);
        wait_and_handle_events ();
      }
  }

  /* l2 lex and phonol */
  if (!testing && l2_running && train_l2 &&
    pairs[shuffletable[pairi]].l2lexindex != NONE &&
    pairs[shuffletable[pairi]].l2phonolindex != NONE)
  {
    modify_assoc_weights (l2lexunits, l2phonolunits, l2lexprop, nl2lexprop, l2phonolprop, nl2phonolprop,
        nl2net, l2lexphonolassoc, l2_alpha);
    modify_assoc_weights (l2phonolunits, l2lexunits, l2phonolprop, nl2phonolprop, l2lexprop, nl2lexprop,
        nl2net, l2phonollexassoc, l2_alpha);
    if (displaying)
      { 
        display_lex (L2LEXINPMOD, l2lexunits, nl2net);
        display_error (L2LEXINPMOD);
        display_lex (L2PHONOLINPMOD, l2phonolunits, nl2net);
        display_error (L2PHONOLINPMOD);
        wait_and_handle_events ();
      }
  }

  /* l2 phonol and phonetic */
  if (!testing && l2_running && train_l2 &&
    pairs[shuffletable[pairi]].l2phonolindex != NONE &&
    pairs[shuffletable[pairi]].l2phoneticindex != NONE)
  {
    modify_assoc_weights (l2phonolunits, l2phoneticunits, l2phonolprop, nl2phonolprop, l2phoneticprop, nl2phoneticprop,
        nl2net, l2phonolphoneticassoc, l2_alpha);
    modify_assoc_weights (l2phoneticunits, l2phonolunits, l2phoneticprop, nl2phoneticprop, l2phonolprop, nl2phonolprop,
        nl2net, l2phoneticphonolassoc, l2_alpha);
    if (displaying)
      { 
        display_lex (L2PHONOLINPMOD, l2phonolunits, nl2net);
        display_error (L2PHONOLINPMOD);
        display_lex (L2PHONETICINPMOD, l2phonolunits, nl2net);
        display_error (L2PHONETICINPMOD);
        wait_and_handle_events ();
      }
  }
  
  /* l1 lex and l2 lex */
      if (!testing && l1l2_assoc_running && train_l1 && train_l2 &&
    pairs[shuffletable[pairi]].l1lexindex != NONE &&
    pairs[shuffletable[pairi]].l2lexindex != NONE)
  {
    modify_assoc_weights (l2lexunits, l1lexunits, l2lexprop, nl2lexprop, l1lexprop, nl1lexprop,
        nl1net, l2l1lexassoc, l1l2_assoc_alpha);
    modify_assoc_weights (l1lexunits, l2lexunits, l1lexprop, nl1lexprop, l2lexprop, nl2lexprop,
        nl2net, l1l2lexassoc, l1l2_assoc_alpha);
    if (displaying)
      { 
        display_lex (L2LEXINPMOD, l2lexunits, nl2net);
        display_error (L2LEXINPMOD);
        display_lex (L1LEXINPMOD, l1lexunits, nl1net);
        display_error (L1LEXINPMOD);
        wait_and_handle_events ();
      }
    }

  /* l1 phonol and l2 phonol */
      if (!testing && l1l2_assoc_running && train_l1 && train_l2 &&
    pairs[shuffletable[pairi]].l1phonolindex != NONE &&
    pairs[shuffletable[pairi]].l2phonolindex != NONE)
  {
    modify_assoc_weights (l2phonolunits, l1phonolunits, l2phonolprop, nl2phonolprop, l1phonolprop, nl1phonolprop,
        nl1net, l2l1phonolassoc, l1l2_assoc_alpha);
    modify_assoc_weights (l1phonolunits, l2phonolunits, l1phonolprop, nl1phonolprop, l2phonolprop, nl2phonolprop,
        nl2net, l1l2phonolassoc, l1l2_assoc_alpha);
    if (displaying)
      { 
        display_lex (L2PHONOLINPMOD, l2phonolunits, nl2net);
        display_error (L2PHONOLINPMOD);
        display_lex (L1PHONOLINPMOD, l1phonolunits, nl1net);
        display_error (L1PHONOLINPMOD);
        wait_and_handle_events ();
      }
    }

  /* l1 phonetic and l2 phonetic */
      if (!testing && l1l2_assoc_running && train_l1 && train_l2 &&
    pairs[shuffletable[pairi]].l1phoneticindex != NONE &&
    pairs[shuffletable[pairi]].l2phoneticindex != NONE)
  {
    modify_assoc_weights (l2phoneticunits, l1phoneticunits, l2phoneticprop, nl2phoneticprop, l1phoneticprop, nl1phoneticprop,
        nl1net, l2l1phoneticassoc, l1l2_assoc_alpha);
    modify_assoc_weights (l1phoneticunits, l2phoneticunits, l1phoneticprop, nl1phoneticprop, l2phoneticprop, nl2phoneticprop,
        nl2net, l1l2phoneticassoc, l1l2_assoc_alpha);
    if (displaying)
      { 
        display_lex (L2PHONETICINPMOD, l2phoneticunits, nl2net);
        display_error (L2PHONETICINPMOD);
        display_lex (L1PHONETICINPMOD, l1phoneticunits, nl1net);
        display_error (L1PHONETICINPMOD);
        wait_and_handle_events ();
      }
    }

  }

}


/*********************  lexicon propagation  ******************************/

static void
present_input (modi, units, nnet, words, index, prop, nprop, nc)
/* present a word representation to the input map, propagate or change assoc
   weights, display, collect statistics */
     int modi;				/* module number */
     int nnet;				/* map size */
     int index;				/* input word index */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map units */
     LEXPROPUNIT prop[];		/* propagation through these units */
     WORDSTRUCT words[];		/* lexicon */
     int *nprop;			/* number of propunits */
     int nc;				/* neighborhood radius */
{
  int besti, bestj;			/* indices of the image unit */
  int i;
    
  /* form the input and target */
  for (i = 0; i < ninprep[modi]; i++) {
    inprep[modi][i] = tgtrep[modi][i] = words[index].rep[i];
  }
  target[modi] = index;
  
  /* find the image on the map, display, collect stats */
  find_input_image (modi, units, nnet, &besti, &bestj);
  collect_stats (modi);
  compute_propunits (units, nnet, prop, nprop,
		     clip(besti - nc, 0, ige), clip(bestj - nc, 0, ige),
		     clip(besti+nc, nnet-1, ile), clip(bestj+nc, nnet-1, ile));
}

  
static void
associate (inpunits, modi, units, nnet, words, index, prop, nprop, assoc)
/* present a word representation to the input map, propagate or change assoc
   weights, display, collect statistics */
     int modi;				/* module number */
     int nnet;				/* map size */
     int index;				/* input word index */
     FMUNIT inpunits[MAXLSNET][MAXLSNET],
      units[MAXLSNET][MAXLSNET];	/* input and assoc map units */
     LEXPROPUNIT prop[];		/* propagation through these units */
     WORDSTRUCT words[];		/* lexicon */
     double assoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET]; /* assoc weights */
     int nprop;				/* number of propunits */
{
  /* propagation through these units */
  double abest;				/* activation of the assoc image */
  int i;

  /* form the target for the association */
  for (i = 0; i < ninprep[modi]; i++)
    tgtrep[modi][i] = words[index].rep[i];
  target[modi] = index;
  
  /* propagate activation to the other map, find image, collect stats */
  find_assoc_image (modi, inpunits, prop, nprop, units, nnet, assoc, &abest);
  collect_stats (modi);
  /* we don't need actual activations unless we are displaying them */
  if (displaying)
    compute_assoc_activations (units, nnet, abest);
}


static void
find_input_image (modi, units, nnet, besti, bestj)
/* present a word representation to the input map,
   find the image unit, set up output vector */
     int modi;				/* module number */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map units */
     int nnet;				/* map size */
     int *besti, *bestj;		/* indices of the image unit */
{
  int i, j;				/* indices to the map */
  double best,				/* activation of the image unit */
    foo;				/* not used */

  /* compute responses of the units to the external input
     and find max and min responding units */
  best = LARGEFLOAT;
  foo = (-1.0);
  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      {
	/* response proportional to the distance of weight and input vectors */
	distanceresponse (&units[i][j], inprep[modi], ninprep[modi],
			  distance, NULL);
	/* find best and worst and best indices */
	updatebestworst (&best, &foo, besti, bestj, &units[i][j],
			 i, j, fsmaller, fgreater);
      }

  /* set up the output representation */
  for (i = 0; i < noutrep[modi]; i++)
    outrep[modi][i] = units[*besti][*bestj].comp[i];
}


static void
compute_propunits (units, nnet, prop, nprop, lowi, lowj, highi, highj)
/* define the propagation neighborhood and set up activations */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map units */
     int nnet;				/* map size */
     LEXPROPUNIT prop[];	        /* propagation through these units */
     int *nprop;			/* number of propunits */
     int lowi, lowj, highi, highj;	/* neighborhood boundaries */
{
  int i, j;
  double best, worst;			/* act. of max and min unit */

  /* determine the propagation neighborhood */
  findprop (lowi, lowj, highi, highj, units, &best, &worst, prop, nprop);

  /* we only need the actual activations if we show them or propagate */
  if (displaying || l1l2_assoc_running || sl1_assoc_running || sl2_assoc_running)
    /* calculate the actual activation values */
    for (i = 0; i < nnet; i++)
      for (j = 0; j < nnet; j++)
	/* only the propagation units should be active */
	if (propunit (i, j, prop, *nprop, lowi, lowj, highi, highj))
	  if (worst > best)
	    /* scale the values between max and min activation
	       in the entire nc neighborhood */ 
	    units[i][j].value = 1.0 - (units[i][j].value - best)
	                              / (worst - best);
	  else
	    units[i][j].value = 1.0;
	else
	  /* make everything else 0 */
	  units[i][j].value = 0.0;
}


static void
find_assoc_image (modi, inpunits, prop, nprop, outunits, noutnet, assoc, abest)
/* propagate through the neighborhood to the output map,
   find max output unit, setup output vector */
     int modi;				/* module number */
     FMUNIT inpunits[MAXLSNET][MAXLSNET]; /* input map */
     FMUNIT outunits[MAXLSNET][MAXLSNET]; /* output map */
     int noutnet;			/* output map size */
     double assoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET]; /* assoc weights */
     LEXPROPUNIT prop[];	        /* propagation through these units */
     int nprop;				/* number of propunits */
     double *abest;			/* best activation on output map */
{
  int i, j;				/* output map indices */
  double foo = (-1.0);			/* worst activation (not used) */
  int associ, assocj;			/* coordinates of associative image */

  /* propagate activation from the input map to the output map
     and find max and min responding units */
  *abest = (-1.0);
  for (i = 0; i < noutnet; i++)
    for (j = 0; j < noutnet; j++)
      {
	/* propagate activation from input to output map */
	assocresponse (&outunits[i][j], i, j, inpunits, assoc, prop, nprop);
	/* find best and worst and best indices */
	updatebestworst (abest, &foo, &associ, &assocj, &outunits[i][j],
			 i, j, fgreater, fsmaller);
      }
  
  /* setup the output representation */
  for (i = 0; i < noutrep[modi]; i++)
    outrep[modi][i] = outunits[associ][assocj].comp[i];
}


static void
compute_assoc_activations (outunits, noutnet, abest)
/* calculate the actual activation values in the output map */
     FMUNIT outunits[MAXLSNET][MAXLSNET]; /* output map */
     int noutnet;			/* output map size */
     double abest;			/* best activation on output map */
{
  int i, j;
  
  /* scale the activation in the output map within 0 and 1 */
  for (i = 0; i < noutnet; i++)
    for (j = 0; j < noutnet; j++)
      {
	outunits[i][j].value = outunits[i][j].value / abest;
	/* avoid unnecessary display update */
	if (outunits[i][j].value < 0.1)
	  outunits[i][j].value = 0.0;
      }
}


static void
assocresponse (unit, i, j, inpunits, assoc, prop, nprop)
/* propagate activation from the input map to this output map unit */
     FMUNIT *unit;                          /* unit on the map */
     int i, j;
     FMUNIT inpunits[MAXLSNET][MAXLSNET];   /* input map */
     double assoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET];
                                            /* associative weights */
     LEXPROPUNIT prop[];	            /* propage through these units */
     int nprop;				    /* number of propunits */
{
  int p;

  /* save previous value to avoid redisplay if it does not change */
  unit->prevvalue = unit->value;

  /* propagate activity in the input map through the associative weights
     to the output unit */
  unit->value = 0.0;
  for (p = 0; p < nprop; p++)
    unit->value += inpunits[prop[p].i][prop[p].j].value
                   * assoc[prop[p].i][prop[p].j][i][j];
}


/****************** propagation neighborhood  */

static void
findprop (lowi, lowj, highi, highj, units, best, worst, prop, nprop)
/* find the units in the neighborhood that should be included 
   in the propagation and place them in the global array prop */
     int lowi, lowj, highi, highj;	/* boundaries of the neighborhood */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* the map */
     double *best, *worst;		/* best and worst in the neighborhd */
     LEXPROPUNIT prop[];	        /* propagation through these units */
     int *nprop;			/* number of propunits */
{
  int i, j,				/* map indices */
  p = 0, k;				/* indices to activation list */

  prop[0].value = LARGEFLOAT;		/* initialize the list for sorting */
  *best = LARGEFLOAT;			/* initialize the best value */
  *worst = (-1.0);			/* initialize the weakest value */

  for (i = lowi; i <= highi; i++)	/* look at all units in the  */
    for (j = lowj; j <= highj; j++)	/* neighborhood */
      {
	if (!npropunits_given)
	  /* we don't need to order them; just insert in the list */
	  {
	    prop[p].i = i;
	    prop[p].j = j;
	    p++;
	  }
	else
	  /* see if the activation of the current value is within
	     the npropunits highest activations found so far
	     (note: low value means high activation) */
	  for (p = 0; p < npropunits; p++)
	    if (units[i][j].value < prop[p].value)
	      {
		/* found the right place; move the rest of the list down */
		for (k = npropunits - 1; k > p; k--)
		  {
		    prop[k].value = prop[k - 1].value;
		    prop[k].i = prop[k - 1].i;
		    prop[k].j = prop[k - 1].j;
		  }
		/* insert the current value in the list */
		prop[p].value = units[i][j].value;
		prop[p].i = i;
		prop[p].j = j;
		break;
	      }
  	/* update the best and worst activation in the neighborhood */
	if (units[i][j].value < *best)
	  *best = units[i][j].value;
	if (units[i][j].value > *worst)
	  *worst = units[i][j].value;
      }
  /* set up the number of units in the neighborhood for propagation */
  *nprop = (highi - lowi + 1) * (highj - lowj + 1);
  if (npropunits_given && npropunits < *nprop)
    *nprop = npropunits;
}


static int
propunit (i, j, prop, nprop, lowi, lowj, highi, highj)
/* check if unit i, j is in the list of units included in the propagation */
     int  i, j;				/* unit indices */
     LEXPROPUNIT prop[];	        /* propagation through these units */
     int lowi, lowj, highi, highj;	/* boundaries of the neighborhood */
     int nprop;				/* number of propunits */
{
  int p;
  
  if (!npropunits_given)
    /* a quick check is possible based on the indices */
    if (i >= lowi && j >= lowj && i <= highi && j <= highj)
      return (TRUE);
    else
      return (FALSE);
  else
    for (p = 0; p < nprop; p++)
      if (prop[p].i == i && prop[p].j == j)
	return(TRUE);
  return (FALSE);
}


/*********************	weight adaptation ************************/

static void
modify_input_weights (modi, units, alpha, prop, nprop)
/* adapt the input weights of units in the neighborhood of image unit */
  int modi;			/* module number */
  FMUNIT units[MAXLSNET][MAXLSNET]; /* input units */
  LEXPROPUNIT prop[];	        /* weight change in these units */
  double alpha;
  int nprop;			/* number of propunits */
{
  int p, k;

  for (p = 0; p < nprop; p++) {
    for (k = 0; k < ninprep[modi]; k++) {
      units[prop[p].i][prop[p].j].comp[k] +=
	alpha * (inprep[modi][k] - units[prop[p].i][prop[p].j].comp[k]);
    }
  }
}


static void
modify_assoc_weights (iunits, aunits, iprop, niprop, aprop, naprop, nanet,
		      assoc, alpha)
/* adapt the associative connections */
   FMUNIT iunits[MAXLSNET][MAXLSNET],	/* input units */
     aunits[MAXLSNET][MAXLSNET];	/* assoc units */
   int niprop, naprop,		      	/* number of active units */
     nanet;				/* output net size */
   LEXPROPUNIT iprop[], aprop[];	/* active input and assoc units */
   double assoc[MAXLSNET][MAXLSNET][MAXLSNET][MAXLSNET];
   double alpha;
{
  int i, a, ii, jj;
  float sum;

  /* modify the associative connections through Hebbian learning  */
  for (i = 0; i < niprop; i++) {
    for (a = 0; a < naprop; a++) {
      assoc[iprop[i].i][iprop[i].j][aprop[a].i][aprop[a].j] += 
	alpha * iunits[iprop[i].i][iprop[i].j].value *
      	              aunits[aprop[a].i][aprop[a].j].value;
      } 
    }

  /* normalize the associative output connections of a unit */
  for (i = 0; i < niprop; i++)
    {
      sum = 0.0;
      for (ii = 0; ii < nanet; ii++)
	for ( jj = 0; jj < nanet; jj++)
	  sum += assoc[iprop[i].i][iprop[i].j][ii][jj] *
	         assoc[iprop[i].i][iprop[i].j][ii][jj];
      sum = sqrt (sum);
      for (ii = 0; ii < nanet; ii++)
	for ( jj = 0; jj < nanet; jj++)
	   assoc[iprop[i].i][iprop[i].j][ii][jj] /= sum;
    }	  
}


/********************* feature map search routines  ********************/

void
distanceresponse (unit, inpvector, ninpvector, responsefun, indices)
/* compute the response of the feature map unit to the input vector
   proportional to distance (distance function given as parameter) */
     FMUNIT *unit;                      /* unit on the map */
     double inpvector[];			/* input vector values */
     int ninpvector;			/* dimension of the inputvector */
     int indices[];			/* include these components */
     /* the function determining the unit response */
     double (*responsefun) __P((int *, double *, double *, int));
{
  /* save previous value to avoid redisplay if it does not change */
  unit->prevvalue = unit->value;
  /* calculate the response of this unit to the external input */
  unit->value = (*responsefun) (indices, inpvector, unit->comp, ninpvector);
}


void
updatebestworst (best, worst, besti, bestj, unit, i, j, better, worse)
/* check if this unit is the best or the worst so far, and if so,
   update the best indices */
     double *best, *worst;		/* activity of max and min resp unit */
     int *besti, *bestj;		/* indices of the max resp unit */
     FMUNIT *unit;                      /* unit on the map */
     int i, j;				/* indices of the unit */
     /* these functions are either < or > depending on how the activity
        is represented (as distance or actual activation) */
     int (*better) __P((double, double)),
     (*worse) __P((double, double));
{
  /* check if this unit's response is best so far encountered */
  /* if the current unit is better, store it; depending on whether
     we are looking for the highest or lowest activity, better
     is either < or > */
  if ((*better) (unit->value, *best))
    {
      *besti = i;
      *bestj = j;
      *best = unit->value;
    }
  /* otherwise check if it is the worst response so far */
  if ((*worse) (unit->value, *worst))
    *worst = unit->value;
}


static int
clip (index, limit, comparison)
/* if index is beyond limit, reduce it to the limit */
     int index, limit;
     int (*comparison) __P((int n, int m)); /* <= if upper limit, */
                                            /* >= if lower limit */
{
  if ((*comparison) (index, limit))
    return (index);
  else
    return (limit);
}


static int
ige (num1, num2)
/* integer greater or equal (for limit comparison) */
     int num1, num2;
{
  return (num1 >= num2);
}


static int
ile (num1, num2)
/* integer less or equal (for limit comparison) */
     int num1, num2;
{
  return (num1 <= num2);
}


/*********************  initializations ******************************/

void
iterate_weights (dofun, fp, par1, par2)
/* read the weights from the file */
     void (*dofun) __P((FILE *, double *, double, double)); /* read function */
     FILE *fp;
     double par1, par2;			/* distribution parameters */
{
  int i, j, k, ii, jj;

  /* L1 map input weights */
  for (i = 0; i < nl1net; i++)
    for (j = 0; j < nl1net; j++)
      for (k = 0; k < nl1lexrep; k++)
	(*dofun) (fp, &l1lexunits[i][j].comp[k], par1, par2);

  for (i = 0; i < nl1net; i++)
    for (j = 0; j < nl1net; j++)
      for (k = 0; k < nl1phonolrep; k++)
  (*dofun) (fp, &l1phonolunits[i][j].comp[k], par1, par2);

  for (i = 0; i < nl1net; i++)
    for (j = 0; j < nl1net; j++)
      for (k = 0; k < nl1phoneticrep; k++)
  (*dofun) (fp, &l1phoneticunits[i][j].comp[k], par1, par2);

  /* L2 map input weights */
  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (k = 0; k < nl2lexrep; k++)
  (*dofun) (fp, &l2lexunits[i][j].comp[k], par1, par2);

  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (k = 0; k < nl2phonolrep; k++)
  (*dofun) (fp, &l2phonolunits[i][j].comp[k], par1, par2);

  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (k = 0; k < nl2phoneticrep; k++)
  (*dofun) (fp, &l2phoneticunits[i][j].comp[k], par1, par2);

  /* semantic map input weights */
  for (i = 0; i < nsnet; i++)
    for (j = 0; j < nsnet; j++)
      for (k = 0; k < nsrep; k++)
	(*dofun) (fp, &sunits[i][j].comp[k], par1, par2);

  /* L1 L2 lex and Sem associative connections */
  for (i = 0; i < nl1net; i++)
    for (j = 0; j < nl1net; j++)
      for (ii = 0; ii < nsnet; ii++)
	for (jj = 0; jj < nsnet; jj++)
	  {
	    (*dofun) (fp, &l1lexsassoc[i][j][ii][jj], par1, par2);
	    (*dofun) (fp, &sl1lexassoc[ii][jj][i][j], par1, par2);
	  }

  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (ii = 0; ii < nsnet; ii++)
  for (jj = 0; jj < nsnet; jj++)
    {
      (*dofun) (fp, &l2lexsassoc[i][j][ii][jj], par1, par2);
      (*dofun) (fp, &sl2lexassoc[ii][jj][i][j], par1, par2);
    }

  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (ii = 0; ii < nl1net; ii++)
  for (jj = 0; jj < nl1net; jj++)
    {
      (*dofun) (fp, &l2l1lexassoc[i][j][ii][jj], par1, par2);
      (*dofun) (fp, &l1l2lexassoc[ii][jj][i][j], par1, par2);
    }

  /* L1 L2 phonol and lex associative connections */
  for (i = 0; i < nl1net; i++)
    for (j = 0; j < nl1net; j++)
      for (ii = 0; ii < nl1net; ii++)
  for (jj = 0; jj < nl1net; jj++)
    {
      (*dofun) (fp, &l1lexphonolassoc[i][j][ii][jj], par1, par2);
      (*dofun) (fp, &l1phonollexassoc[ii][jj][i][j], par1, par2);
    }

  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (ii = 0; ii < nl2net; ii++)
  for (jj = 0; jj < nl2net; jj++)
    {
      (*dofun) (fp, &l2lexphonolassoc[i][j][ii][jj], par1, par2);
      (*dofun) (fp, &l2phonollexassoc[ii][jj][i][j], par1, par2);
    }

  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (ii = 0; ii < nl1net; ii++)
  for (jj = 0; jj < nl1net; jj++)
    {
      (*dofun) (fp, &l2l1phonolassoc[i][j][ii][jj], par1, par2);
      (*dofun) (fp, &l1l2phonolassoc[ii][jj][i][j], par1, par2);
    }

  /* L1 L2 phonetic and phonol associative connections */
  for (i = 0; i < nl1net; i++)
    for (j = 0; j < nl1net; j++)
      for (ii = 0; ii < nl1net; ii++)
  for (jj = 0; jj < nl1net; jj++)
    {
      (*dofun) (fp, &l1phonolphoneticassoc[i][j][ii][jj], par1, par2);
      (*dofun) (fp, &l1phoneticphonolassoc[ii][jj][i][j], par1, par2);
    }

  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (ii = 0; ii < nl2net; ii++)
  for (jj = 0; jj < nl2net; jj++)
    {
      (*dofun) (fp, &l2phonolphoneticassoc[i][j][ii][jj], par1, par2);
      (*dofun) (fp, &l2phoneticphonolassoc[ii][jj][i][j], par1, par2);
    }

  for (i = 0; i < nl2net; i++)
    for (j = 0; j < nl2net; j++)
      for (ii = 0; ii < nl1net; ii++)
  for (jj = 0; jj < nl1net; jj++)
    {
      (*dofun) (fp, &l2l1phoneticassoc[i][j][ii][jj], par1, par2);
      (*dofun) (fp, &l1l2phoneticassoc[ii][jj][i][j], par1, par2);
    }

}


void
normalize_all_assocweights ()
/* run through all output weights and normalize */
/* convenient to do both directions at once */
{
  double l1value, l2value, l1lexsvalue, sl1lexvalue, l2lexsvalue, sl2lexvalue, l1l2value, l2l1value;
  int i, j, ii, jj;

  /* l1 lex to sem */
  l1lexsvalue = 1.0 / sqrt ((double) (nsnet * nsnet));
  sl1lexvalue = 1.0 / sqrt ((double) (nl1net * nl1net));
  for(i = 0; i < nl1net; i++)
    for(j = 0; j < nl1net; j++)
      for(ii = 0; ii < nsnet; ii++)
  for(jj = 0; jj < nsnet; jj++)
    {
      l1lexsassoc[i][j][ii][jj] = l1lexsvalue;
      sl1lexassoc[ii][jj][i][j] = sl1lexvalue;
    }

  /* l1 lex to phonol */
  l1value = 1.0 / sqrt ((double) (nl1net * nl1net));
  for(i = 0; i < nl1net; i++)
    for(j = 0; j < nl1net; j++)
      for(ii = 0; ii < nl1net; ii++)
  for(jj = 0; jj < nl1net; jj++)
    {
      l1lexphonolassoc[i][j][ii][jj] = l1value;
      l1phonollexassoc[ii][jj][i][j] = l1value;
    }

  /* l1 phonol to phonetic */
  for(i = 0; i < nl1net; i++)
    for(j = 0; j < nl1net; j++)
      for(ii = 0; ii < nl1net; ii++)
  for(jj = 0; jj < nl1net; jj++)
    {
      l1phonolphoneticassoc[i][j][ii][jj] = l1value;
      l1phoneticphonolassoc[ii][jj][i][j] = l1value;
    }

  /* l2 lex to sem */
  l2lexsvalue = 1.0 / sqrt ((double) (nsnet * nsnet));
  sl2lexvalue = 1.0 / sqrt ((double) (nl2net * nl2net));
  for(i = 0; i < nl2net; i++)
    for(j = 0; j < nl2net; j++)
      for(ii = 0; ii < nsnet; ii++)
  for(jj = 0; jj < nsnet; jj++)
    {
      l2lexsassoc[i][j][ii][jj] = l2lexsvalue;
      sl2lexassoc[ii][jj][i][j] = sl2lexvalue;
    }

  /* l2 lex to phonol */
  l2value = 1.0 / sqrt ((double) (nl2net * nl2net));
  for(i = 0; i < nl2net; i++)
    for(j = 0; j < nl2net; j++)
      for(ii = 0; ii < nl2net; ii++)
  for(jj = 0; jj < nl2net; jj++)
    {
      l2lexphonolassoc[i][j][ii][jj] = l2value;
      l2phonollexassoc[ii][jj][i][j] = l2value;
    }

  /* l2 phonol to phonetic */
  for(i = 0; i < nl2net; i++)
    for(j = 0; j < nl2net; j++)
      for(ii = 0; ii < nl2net; ii++)
  for(jj = 0; jj < nl2net; jj++)
    {
      l2phonolphoneticassoc[i][j][ii][jj] = l2value;
      l2phoneticphonolassoc[ii][jj][i][j] = l2value;
    }


  l2l1value = 1.0 / sqrt ((double) (nl1net * nl1net));
  l1l2value = 1.0 / sqrt ((double) (nl2net * nl2net));
  /* l1 lex to l2 lex */  
  for(i = 0; i < nl2net; i++)
    for(j = 0; j < nl2net; j++)
      for(ii = 0; ii < nl1net; ii++)
  for(jj = 0; jj < nl1net; jj++)
    {
      l2l1lexassoc[i][j][ii][jj] = l2l1value;
      l1l2lexassoc[ii][jj][i][j] = l1l2value;
    }

  /* l1 phonol to l2 phonol */  
  for(i = 0; i < nl2net; i++)
    for(j = 0; j < nl2net; j++)
      for(ii = 0; ii < nl1net; ii++)
  for(jj = 0; jj < nl1net; jj++)
    {
      l2l1phonolassoc[i][j][ii][jj] = l2l1value;
      l1l2phonolassoc[ii][jj][i][j] = l1l2value;
    }

  /* l1 phonetic to l2 phonetic */  
  for(i = 0; i < nl2net; i++)
    for(j = 0; j < nl2net; j++)
      for(ii = 0; ii < nl1net; ii++)
  for(jj = 0; jj < nl1net; jj++)
    {
      l2l1phoneticassoc[i][j][ii][jj] = l2l1value;
      l1l2phoneticassoc[ii][jj][i][j] = l1l2value;
    }
}



void
clear_values (units, nnet)
/* clear map activations */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
     int nnet;				/* map size */
{
  int i, j;

  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      units[i][j].value = 0.0;
}


void
clear_prevvalues (units, nnet)
/* reset the previous activations to NOPREV, so that the whole map
   will be displayed (because prevvalues are different to current values) */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
     int nnet;				/* map size */
{
  int i, j;

  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      units[i][j].prevvalue = NOPREV;
}


void
clear_labels (units, nnet)
/* clear labels */
     FMUNIT units[MAXLSNET][MAXLSNET];	/* map */
     int nnet;				/* map size */
{
  int i, j;
  for (i = 0; i < nnet; i++)
    for (j = 0; j < nnet; j++)
      {
	units[i][j].labelcount = 1;	/* reserve 0 for nearestlabel */
	sprintf(units[i][j].labels[0], "%s", ""); /* none to begin with */
      }
}

bool
bool_with_prob (probability)
    /* returns a boolean with indicated probability of returning 1 */
    /* reference: http://stackoverflow.com/questions/3771551/how-to-generate-a-boolean-with-p-probability-using-c-rand-function */
    double probability;
{
  return ((double)((double)rand()/(double)RAND_MAX) <= probability);
}

