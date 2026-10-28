
/*----------------------------------------------------------------------*\
 |	NAME	:  itoa							|
 |	CREATED	:  16-August-83		LAST MODIFIED:  16-August-83	|
 |	FUNCTION:  Itoa converts a WORD (integer) into an ASCII string	|
 |		   that represents that number.				|
 |	INPUT	:  number	--  number to conver.			|
 |		   str		--  ptr to string that will hold the	|
 |				    converted number.			|
 |	OUTPUT	:  Fills in the string pointed to by str with the ASCII	|
 |		   representation of number.				|
 |		   No return value.					|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"


EXTERN	VOID	reverse();

VOID	itoa( number,str )
WORD	number;			/* number to convert to string		*/
BYTE	str[];			/* converted number			*/
{
	WORD	sindex;		/* string index				*/
	WORD	sign;		/* number sign				*/

	if( (sign = number) < 0 )
	   number = -number;
	sindex = 0;
	do
	   str[sindex++] = (number % 10) + '0';
	while( (number /= 10) > 0 );
	if( sign < 0 )
	   str[sindex++] = '-';
	str[sindex] = NULL;
	reverse( str );
}
