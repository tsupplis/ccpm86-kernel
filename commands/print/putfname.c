
/*----------------------------------------------------------------------*\
 |	NAME	:  putfname						|
 |	CREATED	:  26-August-83		LAST MODIFIED:  12-September-83 |
 |	FUNCTION:  Putfname writes out a drive and filename to the	|
 |		   console from the FCB passed to it.  The high bits	|
 |		   (attributes) are masked off first.  As an option,	|
 |		   a space may be printed inseated of the drive char.	|
 |	INPUT	:  fcb	--  ptr to modified FCB with filename to print,	|
 |			    where the drive code has been translated	|
 |			    into the appropriate drive char.		|
 |		   mode --  print drive char. or space flag		|
 |	OUTPUT	:  No return value.					|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H
EXTERN	VOID	c_write();
*/

VOID	putfname( fcb,mode )
BYTE	fcb[];
WORD	mode;
{
	WORD	findex;
	BYTE	drive;

	if( mode == 0 )			/* If mode = 0 then translate	*/
	{				/* the drive code into the	*/
	   drive = get_drv( fcb );	/* appropriate drive char.	*/
	   c_write( drive );		/* If mode != 0 then print a	*/
	}				/* space for the drive char.	*/
	else
	   c_write( ' ' );
	c_write( ':' );
	c_write( ' ' );
	for( findex=1; findex < 12; findex++ )
	{
	   if( findex == 9 )
	      c_write( ' ' );
	   c_write( (fcb[findex] & ~0x80) );
	}
}
