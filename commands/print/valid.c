
/*----------------------------------------------------------------------*\
 |	NAME	:  valid		LAST MODIFIED:  16-October-83	|
 |	FUNCTION:  Valid compares the "opt" string against a list of	|
 |		   valid option names in "oplist" and all abbreviations |
 |		   thereof (i.e P PA PAG PAGE matches "PAGE").		|
 |		   Limits options to unique first letters.		|
 |	INPUT	:  opt    -- ptr to string to validate as an option	|
 |		   oplist -- ptr to struct array containing the valid	|
 |			     options					|
 |	OUTPUT	:  Returns 0 if valid, 1 or 2 if invalid (maybe changed)|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"

WORD	valid( opt,oplist )
BYTE	*opt;			/* option to validate			*/
struct	_oplist oplist[];	/* list of valid options		*/
{
	WORD	num;		/* number on option,i.e. PAGE10 	*/
	WORD	found;		/* option matched entry			*/
	WORD	op_err;		/* option error return			*/
	WORD	op_size;	/* length of option name		*/
	WORD	save_bi;	/* save area for buffer index		*/
	WORD	bi,i,j,k;	/* various loop control vars.		*/
	BYTE	op_tbl[10][11];	/* valid options table			*/
	BYTE	buff[128];	/* option buffer			*/

	strcpy( buff,opt ); 	/* save option that was passed		*/
	op_err = 0;		/* assume valid option			*/
	found = 0;		/* assume option not found yet		*/

				/* stip off number if found		*/
	for( bi=0; !(isdigit( buff[bi] )) && buff[bi]; bi++ )
	   ;
	buff[bi] = NULL;

	i = 0;
	while( *oplist[i].opname && !found )	/* search until end of	*/
	{					/* the option table or	*/
	   op_size = strlen( oplist[i].opname ); /* build abbrev. table	*/
	   for( k=0; k < op_size; k++ )
	   {
	      for( j=0; j <= k; j++ )
		 op_tbl[k][j] = oplist[i].opname[j];
	      op_tbl[k][j] = NULL;
	   }
   	   op_tbl[k][0] = NULL;			/* table sentinel	*/
	   k = 0;
	   while( *op_tbl[k] && !found )	/* scan for option	*/
	   {
	      if( strcmp( buff,op_tbl[k] ) == 0 )
		 found = 1;
	      else
		 k++;
	   }
	   i++;
	}
	if( found )				/* if found check for	*/
	{					/*    number		*/
	   if( oplist[--i].max_num > 0 )
	   {
	      if( (num = get_num( opt,0 )) > oplist[i].max_num )
		 op_err = 1;			/* number > max_num	*/
	   }
	}
	else
	   op_err = 2;
	
	return( op_err );
}
