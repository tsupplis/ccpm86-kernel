
/*----------------------------------------------------------------------*\
 |	NAME	:  get_drv						|
 |	CREATED	:  5-August-83		LAST MODIFIED:  12-September-83 |
 |	FUNCTION:  Get_drv extracts the drive code from the FCB passed	|
 |		   to it and converts it to an ASCII character that	|
 |		   that code represents.				|
 |	INPUT	:  ptr to FCB to check.					|
 |	OUTPUT	:  Returns A-P drive value.				|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H
EXTERN	WORD	dir_get();
*/

BYTE	get_drv( fcb )
BYTE	fcb[];			/* ptr to current FCB			*/
{
	BYTE	drv;		/* current drive (A-P)			*/

	if( *fcb == 0 )
	   drv = dir_get() + 'A';
	else
	   drv = fcb[0] + 'A' - 1;
	return( drv );
}
