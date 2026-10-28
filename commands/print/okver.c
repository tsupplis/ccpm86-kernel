
/*----------------------------------------------------------------------*\
 |	NAME	:  ok_ver						|
 |	CREATED	:  5-August-83		LAST MODIFIED:  12-September-83 |
 |	FUNCTION:  Ok_ver checks to see that the correct BDOS and OS	|
 |		   are being used.					|
 |	INPUT	:  bdos_ver -- BDOS version number to look for.		|
 |		   os_ver   -- OS version number to look for.		|
 |	OUTPUT	:  Returns 1 for true, 0 for false.			|
\*----------------------------------------------------------------------*/

#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

/* THIS REFERENCE IS IN CPMFUNC.H
EXTERN	WORD	s_bdosver();
*/

WORD	ok_ver( bdos_ver,os_ver )
WORD	bdos_ver;		/* min. BDOS version			*/
WORD	os_ver;			/* min. CP/M version			*/
{
	WORD	ver;		/* S_BDOSVER return value (version #)	*/

	s_bdosver();
	ver = _EXTERR;
	if(((ver & BDOS_MASK) < bdos_ver) || ((ver & OS_MASK) != os_ver))
	   return( NOT_OK );
	return( OK );
}
