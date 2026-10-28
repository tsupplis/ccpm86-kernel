
/*----------------------------------------------------------------------*\
 |	NAME	:  tlex							|
 |	CREATED	:  29-July-83		LAST MODIFIED:  9-August-83	|
 |	FUNCTION:  Tlex retrieves a token from the source line.  Tokens |
 |		   are identified by their delimiters only.  Tokens are |
 |		   thus defined as all characters found until the	|
 |		   delimiter is found.  Leading white space is ignored. |
 |	INPUT	:  line  -- ptr to source line containing tokens	|
 |		   delim -- ptr to string containing delimiter(s)	|
 |		   token -- ptr to token buffer area loaded by tlex	|
 |	OUTPUT	:  token -- loaded with a token if found		|
 |		   Returns ptr to the delimiter	that was found.		|
 |		   If no token found then returns ptr to a null.	|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"

BYTE	*tlex( line,delim,token )
BYTE	*line;			/* ptr to null terminated source line	*/
BYTE	*delim;			/* ptr to delimiter(s) (null terminated)*/
BYTE	*token;			/* ptr to token buffer (output)		*/
{
	BYTE	*del_ptr;	/* index ptr. into delimiter string	*/
	WORD	found;		/* flag set when finding a delimiter	*/

	found = 0;
	while( *line == ' ' || *line == '\t' )	/* ignore white space 	*/
	   line++;
	while( *line && !found )
	{
	   for( del_ptr=delim; *del_ptr; del_ptr++ )
	   {
	      if( *line == *del_ptr )
	      {
		 found = 1;
		 break;
	      }
	   }
	   if( !found )
	      *token++ = *line++;
	}
	*token = NULL;		/* delimit token string			*/
	return( line );		/* return null if eoln or not found	*/
}
