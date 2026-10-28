
/*----------------------------------------------------------------------*\
 |	NAME	:  nextfcb						|
 |	CREATED	:  7-September-83	LAST MODIFIED: 10-February-84	|
 |	FUNCTION:  Nextfcb parses the passed filespec when it is called	|
 |		   for the first time and returns the first matching	|
 |		   FCB for the passed filespec.  On subsequent calls	|
 |		   nextfcb does not parse the filespec.  It will find	|
 |		   and return the next matching FCB.			|
 |	INPUT	:  filespec	-- pointer to a filespec.		|
 |		   curfcb	-- pointer to the current FCB		|
 |				   (set to 0xff the first time)		|
 |	OUTPUT	:  Fills in the current FCB (curfcb)			|
 |		   Returns:						|
 |			-1	-- no files found			|
 |			 0	-- file found, no more following	|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THESE REFERENCES ARE IN CPMFUNC.H
EXTERN	WORD	f_sfirst();
EXTERN	WORD	f_snext();
EXTERN	VOID	f_dmaset();
EXTERN	WORD	f_dmaget();
*/

EXTERN	BYTE	*parse();

WORD	nextfcb( filespec,wild_fcb,curfcb )
BYTE	*filespec;
BYTE	wild_fcb[];
BYTE	curfcb[];
{
	struct	_pfcb	pfcb;
	WORD	dcnt;
	WORD	ret_code;
	WORD	findex;
	BYTE	*save_dma;
	BYTE	dma[DMA_LEN];
	BYTE	tmpfcb[FCB_LEN];

	f_dmaset( dma );		/* set DMA to local area	*/
	if( curfcb[0] == 0xff )		/* first time through ?		*/
	{
	   pfcb.fname = filespec;	/* parse the filespec		*/
	   pfcb.fcbaddr = wild_fcb;
	   if( parse( &pfcb ) == 0xffff )
	      return( -2 );
	   wild_fcb[EX_FIELD] = 0x00;
	   wild_fcb[CR_FIELD] = 0x00;
	   dcnt = f_sfirst( wild_fcb );
	}
	else				/* second, and subsequent calls	*/
	{				/* jump to this point		*/
	   if( (dcnt = f_sfirst( curfcb )) != 0x00ff )
	   {
	      for( findex=0; findex < FCB_LEN; findex++ )
		 curfcb[findex] = wild_fcb[findex];	  /* reset wildcard */
	      dcnt = f_snext();
	   }
	}
	if( dcnt != 0x00ff )
	{
	   dcnt = (dcnt << 5) + 1;			/* dcnt * 32 + 1    */
	   tmpfcb[0] = wild_fcb[0];
	   for( findex=1; findex < FCB_LEN; findex++ )	/* save matched FCB */
	      tmpfcb[findex] = dma[dcnt++];
	   for( findex=16; findex < 24; findex++ )	/* save password    */
	      tmpfcb[findex] = wild_fcb[findex];
	   for( findex=0; findex < FCB_LEN; findex++ )	/* set curfcb       */
	      curfcb[findex] = tmpfcb[findex];
	   ret_code = 0;
	}
	else
	   ret_code = -1;				/* file not found   */

	return( ret_code );
}
