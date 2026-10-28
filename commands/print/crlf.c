
/*----------------------------------------------------------------------*\
 |	NAME	:  crlf							|
 |	CREATED	:  20-August-83		LAST MODIFIED:  12-September-83 |
 |	FUNCTION:  Crlf writes a <cr> <lf> sequence to the attatched	|
 |		   console.						|
 |	INPUT	:  No input values.					|
 |	OUTPUT	:  No return value.					|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H
EXTERN	VOID	c_write();
*/

VOID	crlf()
{
	c_write( '\015' );
	c_write( '\012' );
}
