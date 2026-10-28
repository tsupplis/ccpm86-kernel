
/*----------------------------------------------------------------------*\
 |	NAME	:  fileopen						|
 |	CREATED	:  19-August-83		LAST MODIFIED:  12-September-83 |
 |	FUNCTION:  Fileopen opens a files specified in the passed FCB,	|
 |		   checking for errors.  If a password error is detected|
 |		   user is prompted, and open is retried once.  After	|
 |		   that BDOS extended or physical error is returned.	|
 |	INPUT	:  fcb		--  ptr to FCB for file to open.	|
 |	OUTPUT	:  Returns BDOS extended or physical error code.	|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THESE REFERENCES ARE IN CPMFUNC.H
EXTERN	WORD	f_dmaget();
EXTERN	VOID	f_dmaset();
EXTERN	WORD	f_open();
EXTERN	VOID	c_writestr();
*/

EXTERN	VOID	cpmerr();

EXTERN	BYTE	*err10;

WORD	fileopen( fcb )
BYTE	fcb[];
{
	WORD	ret_code;

	f_dmaset( &fcb[16] );			/* set DMA to password	*/
	f_open( fcb );
	ret_code = _EXTERR;
	if( (ret_code & 0x00ff) == 0x00ff )	/* logical error	*/
	   if( (ret_code >> 8) == 7 )		/* if password error,	*/
	   {
	      crlf();
	      cpmerr( err10,0 );
	      putfname( fcb,0 );
	   }
	ret_code = ret_code >> 8;		/* error, return extend-*/
	return( ret_code );
}
