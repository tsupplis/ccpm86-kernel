
/*----------------------------------------------------------------------*\
 |	NAME	:  firstch						|
 |	CREATED	:  15-August-83		LAST MODIFIED:  15-August-83	|
 |	FUNCTION:  Firstch finds and returns the first non-white	|
 |		   character in a string passed to it.			|
 |	INPUT	:  str		--  ptr to string to scan.		|
 |	OUTPUT	:  Returns first non-white character found, or NULL	|
 |		   if none was found.					|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"

BYTE	firstch( str )
BYTE	*str;			/* source to search			*/
{
	BYTE	*sptr;

	sptr = str;
	while( *sptr == ' ' || *sptr == '\t' )
	   sptr++;		/* ignore white space			*/
	return( *sptr );
}
