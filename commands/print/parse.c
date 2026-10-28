
/*----------------------------------------------------------------------*\
 |	NAME	:  parse						|
 |	CREATED	:  29-July-83		LAST MODIFIED:  16-September-83 |
 |	FUNCTION:  Parse calls f_parse to parse a filespec.  It is	|
 |		   responsible for handling error conditions returned	|
 |		   by f_parse.						|
 |	INPUT	:  pfcb_ptr	--  ptr to a parse FCB structure, which	|
 |				    contains address for a filespec	|
 |				    and an FCB.				|
 |	OUTPUT	:  Fills in FCB pointed to by pfcb.fcbaddr.		|
 |		   Returns pointer to filespec delimiter.		|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H
EXTERN	WORD	f_parse();
*/

EXTERN	VOID	cpmerr();
EXTERN	VOID	crlf();
EXTERN	VOID	putfname();

EXTERN	BYTE	*err06;
EXTERN	BYTE	*err10;

BYTE	*parse( pfcb_ptr )
struct _pfcb *pfcb_ptr;		/* ptr to a parse FCB			*/
{
	WORD	ret_code;	/* BDOS call return code		*/
	WORD	err_code;	/* BDOS call error code			*/

	f_parse( pfcb_ptr );
	ret_code = _EXTERR;
	err_code = _SYSERR;
	if( ret_code == FPAR_ERR )
	{
	   crlf();
	   switch( err_code )
	   {
	      case 23	: 				/* bad drive	*/
	      case 24	: 				/* bad name	*/
	      case 25	: cpmerr( err06,0 );		/* bad type	*/
			  putfname( pfcb_ptr->fcbaddr,0 );
			  crlf();
			  break;
	      case 38	: cpmerr( err10,0 );		/* bad password	*/
			  putfname( pfcb_ptr->fcbaddr,0 );
			  crlf();
			  break;
	      default   : break;
	   }
	}
	return( ret_code );	/* return ptr to delimiter	*/
}
