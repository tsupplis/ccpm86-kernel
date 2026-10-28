
/*----------------------------------------------------------------------*\
 |	NAME	:  readsect						|
 |	CREATED	:  19-August-83		LAST MODIFIED:  16-August-83	|
 |	FUNCTION:  Readsect reads a sector from the file referenced in	|
 |		   passed FCB.						|
 |	INPUT	:  fcb		--  ptr to FCB for file to read from.	|
 |	OUTPUT	:  Returns 0 if successful, F_READ error code otherwise	|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H
EXTERN	WORD	f_read();
*/

EXTERN	VOID	cpmerr();
EXTERN	VOID	putfname();
EXTERN	VOID	crlf();

EXTERN	BYTE	*err08;

WORD	readsect( fcb,buff )
BYTE	fcb[];
BYTE	*buff;
{
	WORD	ret_code;

	f_dmaset( buff );			/* set input buffer	*/
	f_read( fcb );
	ret_code = _EXTERR;			/* if phy. or ext err	*/
	if( (ret_code & 0x00ff) == 0x00ff )	/*   display message	*/
	{
	   crlf();
	   cpmerr( err08,0 );
	   putfname( fcb,0 );
	   crlf();
	   ret_code = ret_code & 0x00ff;
	}					/* return F_READ error	*/
	return( ret_code );			/* code, 0 = sucecss	*/
}
