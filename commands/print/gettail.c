
/*----------------------------------------------------------------------*\
 |	NAME	:  get_tail						|
 |	CREATED	:  29-July-83		LAST MODIFIED:  12-September-83 |
 |	FUNCTION:  Get_tail retrieves the command tail from the user	|
 |		   when it is not explicitly given on the command line.	|
 |	INPUT	:  prompt -- pointer to a user prompt string		|
 |		   tail   -- pointer to a character array for the	|
 |			     tail input					|
 |	OUTPUT	:  tail   -- filled in with input command tail		|
 |		   No return value					|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H
EXTERN	VOID	c_writestr();
EXTERN	VOID	c_readstr();
*/

EXTERN	VOID	crlf();

VOID	get_tail( prompt,tail )
BYTE	*prompt;		/* user prompt				*/
BYTE	*tail;			/* ptr to tail buffer			*/
{
	struct	_cbuf	cbuf;
	BYTE	*tptr;
	WORD	cindex;

	tptr = tail;
	cbuf.max_char = DMA_LEN;
	c_writestr( prompt );	/* promt user				*/
	c_readstr( &cbuf );	/* read user response			*/
	crlf();			/* echo carraige return, line feed	*/

	for( cindex=0; cindex < cbuf.nchar; cindex++ )
	   *tptr++ = toupper( cbuf.buffer[cindex] );
	*tptr = NULL;
}
