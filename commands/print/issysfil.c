
/*----------------------------------------------------------------------*\
 |	NAME	:  is_sysfile						|
 |	CREATED	:  18-August-83		LAST MODIFIED:  18-August-83	|
 |	FUNCTION:  Is_sysfile checks to see if an FCB references a	|
 |		   system type file.					|
 |	INPUT	:  fcb		--  ptr to an FCB to check		|
 |	OUTPUT	:  Return 1 if a system file was  found, 0 if not.	|
\*----------------------------------------------------------------------*/

#include	<portab.h>

WORD	is_sysfile( fcb )
BYTE	fcb[];
{
	if( fcb[10] & ~0x7f )
	   return( YES );
	return( NO );
}
