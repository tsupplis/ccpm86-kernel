
/*----------------------------------------------------------------------*\
 |	NAME	:  cpmerr						|
 |	CREATED	:  10-August-83		LAST MODIFIED:  12-September-83 |
 |	FUNCTION:  Cpmerr prints CP/M error messages, specifically	|
 |		   BDOS error messages.					|
 |	INPUT	:  term_msg	--  ptr to an error message to print.	|
 |		   err_mode	--  return or exit after printing.	|
 |	OUTPUT	:  No return value.					|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H
EXTERN	VOID	c_writestr();
*/

VOID	cpmerr( term_msg,err_mode )
BYTE	*term_msg;			/* termination message		*/
WORD	err_mode;			/* exit/return mode		*/
{
	c_writestr( term_msg );
	if( err_mode < 0 )
	   p_termcpm();
}
