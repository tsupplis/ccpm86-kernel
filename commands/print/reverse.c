
/*----------------------------------------------------------------------*\
 |	NAME	:  reverse						|
 |	CREATED	:  16-August-83		LAST MODIFIED:  16-August-83	|
 |	FUNCTION:  Reverse reverses a string in place.			|
 |	INPUT	:  str		--  ptr to string that will be reversed.|
 |	OUTPUT	:  Reverses string pointed to by str in place.		|
 |		   No return value.					|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"

VOID	reverse( str )
BYTE	str[];			/* string to reverse			*/
{
	WORD	head;		/* head of string index			*/
	WORD	tail;		/* tail of string index			*/
	WORD	cbuff;		/* current char. buffer			*/

	for( head=0,tail=strlen( str )-1; head < tail; head++,tail-- )
	{
	   cbuff     = str[head];
	   str[head] = str[tail];
	   str[tail] = cbuff;
	}
}
