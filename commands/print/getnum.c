
/*----------------------------------------------------------------------*\
 |	NAME	:  get_num						|
 |	CREATED	:  5-August-83		LAST MODIFIED	:  7-October-83	|
 |	FUNCTION:  Get_num retrieves a number appended to the end of	|
 |		   a string pointed to by option.  If none is found the	|
 |		   default value, dflt, is returned.			|
 |	INPUT	:  option -- pointer to command option string		|
 |		   dflt   -- default value of number if none was found	|
 |	OUTPUT	:  Returns number appened to option, or default if not	|
 |		   found.						|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"

WORD	get_num( option,dflt )
BYTE	*option;		/* ptr to current option		*/
WORD	dflt;			/* default number, if not found		*/
{
	WORD	num;		/* number taken from option		*/

	num = 0;
	while( isalpha( *option ) )	/* skip alphabetic ch.		*/
	   option++;
	if( !isdigit( *option ) )	/* no digits found		*/
	   num = dflt;
	else				/* extract number		*/
	   num = atoi( option );
	return( num );
}
