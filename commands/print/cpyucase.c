
/*----------------------------------------------------------------------*\
 |	NAME	:  copy_ucase						|
 |	CREATED	:  24-August-83		LAST MODIFIED:  6-October-83	|
 |	FUNCTION:  Copy_ucase copies n characters from the source 	|
 |		   destination string, upper casing each char first.	|
 |		   NULL characters are replaced with a space if it	|
 |		   is not the last character on the line.		|
 |	INPUT	:  src	--  ptr to source string			|
 |		   dst	--  ptr to destination string			|
 |		   max	--  number of characters to copy		|
 |	OUTPUT	:  No return value.					|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H 
EXTERN	VOID	c_write();
*/

VOID	copy_ucase( dst,src,max_len )
BYTE	dst[];
BYTE	*src;
WORD	max_len;
{
	BYTE	ch;
	WORD	dindex;

	for( dindex=0; dindex < max_len; dindex++ )
	{
	   ch = *src++;
	   if( ch == NULL )
	      dst[dindex] = ' ';
	   else	
	      dst[dindex] = toupper( ch );
	}
	dst[dindex] = NULL;
}
