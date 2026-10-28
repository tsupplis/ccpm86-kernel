
/*----------------------------------------------------------------------*\
 |	NAME	  :	utildef.h					|
 |	FUNCTION  :	This is a header file used by the utilities	|
 |			for the Portable Concurrent CP/M 4.0.		|
 |									|
 |	CREATED   :	26-July-83	LAST MODIFIED: 16-September-83	|
 |	AUTHOR	  :	Kim S. Ouye					|
 |									|
 |	      		COPYRIGHT (c) Digital Research 1983		|
 |			    	  all rights reserved			|
\*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*\
 |			Conditional compile flags			|
\*----------------------------------------------------------------------*/

#define	CCPM2		1	/* 1 CCP/M    2.0   0 not CCPM     2.0	*/
#define	CCPM4		0	/* 1 PCCP/M   4.0   0 not CCPM     4.0	*/
#define	CPM68K	 	0	/* 1 CP/M 68K 1.1   0 not CP/M 68K 1.1	*/

#define	CPU_8086	1	/* 1 8086     CPU   0 not 8086     CPU	*/
#define	CPU_68K 	0	/* 1 68K      CPU   0 not 68K      CPU	*/
#define	CPU_286		0	/* 1 80286    CPU   0 not 80286    CPU	*/
#define	CPU_Z8000	0	/* 1 Z8000    CPU   0 not Z8000    CPU	*/

/*----------------------------------------------------------------------*\
 |			Max. & min. length, sizes, values		|
\*----------------------------------------------------------------------*/

#define	FCB_LEN		36	/* FCB buffer length		*/
#define	DMA_LEN		128	/* DMA buffer length		*/
#define	PW_LEN		8	/* file password length		*/
#define	SEC_LEN		128	/* sector length		*/
#define	MAX_MSEC	128	/* max. mutl-sector count	*/
#define	FNAME_LEN	12	/* FCB name field length	*/
#define	FILESPEC_LEN	15	/* max. filespec length		*/
#define	MAX_FILESPEC	16	/* max. list of filespec	*/
#define	PAGE_LEN	23	/* default page length		*/
#define	MAX_USER_NUM	15	/* max. user number		*/
#define	NUM_DIR_COL	4	/* # column DIR display		*/

/*----------------------------------------------------------------------*\
 |			Masks						|
\*----------------------------------------------------------------------*/

#define	BDOS_MASK	0x00ff	/* BDOS version mask		*/
#define	OS_MASK		0xff00	/* CP/M system version mask	*/
#define	RO_MASK		0x0080	/* set F6' (read only) attrib.	*/

/*----------------------------------------------------------------------*\
 |			CP/M specific misc. defines			|
\*----------------------------------------------------------------------*/

#if (CCPM2 & CPU_8086)
#define	BDOS_VER	0x0031	/* BDOS Ver. 3.1		*/
#define	OS_TYPE		0x1400	/* CCP/M on 8086 cpu		*/
#endif
#if (CCPM4 & CPU_68K)
#define	BDOS_VER	0x0040	/* BDOS Ver. 4.0		*/
#define	OS_TYPE		0x2400	/* CCP/M on 68K cpu		*/
#endif
#if (CPM68K)
#define	BDOS_VER	0x0022	/* BDOS Ver. 2.2		*/
#define	OS_TYPE		0x2000	/* CCP/M Ver. 2.0		*/
#endif

#define	WILDCARD	"*.*"	/* CP/M full filespec wildcard	*/
#define	RET_DISP	0x00fe	/* f_errmode, return & display	*/
#define	NO_RET		0x0000	/* f_errmode, no return		*/
#define	FPAR_ERR	0xffff	/* f_parse error return code	*/
#define	CR_FIELD	32	/* CR FCB field index		*/
#define	EX_FIELD	12	/* EX FCB field index		*/
#define	F6_FIELD	6	/* F6' attribute (READ ONLY)	*/

/*----------------------------------------------------------------------*\
 |			Misc. defines					|
\*----------------------------------------------------------------------*/

#define	CR		'\015'	/* carriage return		*/
#define	LF		'\012'	/* line feed			*/
#define	FF		'\014'	/* form feed			*/
#define	SP		'\040'	/* space			*/
#define	HT		'\011'	/* horizontal tab		*/

#define	ERROR		-1	/* error found return code	*/
#define	MATCH		0	/* sting compare match code	*/
#define	YES		1	/* condition met		*/
#define	NO		0	/* condition not met		*/
#define	OK		1	/* boolean truth		*/
#define	NOT_OK		0	/* boolen false			*/
#define	INVALID_FILE	-2	/* invalid filespec		*/

/*----------------------------------------------------------------------*\
 |			Macro definitions				|
\*----------------------------------------------------------------------*/

#define	islower(c)	('a' <= (c) && (c) <= 'z')
#define	isupper(c)	('A' <= (c) && (c) <= 'Z')
#define	isdigit(c)	('0' <= (c) && (c) <= '9')
#define	isalpha(c)	(islower(c) | isupper(c))
#define	tolower(c)	(isupper(c) ? ((c)+0x20):(c))
#define	toupper(c)	(islower(c) ? ((c)-0x20):(c))

/*----------------------------------------------------------------------*\
 |			Valid Command Line Options			|
\*----------------------------------------------------------------------*/

#define	OP_GROUP	"GROUP"		/* User group/number option	*/
#define	OP_PAGE		"PAGE"		/* Page mode/length option	*/
#define	OP_NOPAGE	"NOPAGE"	/* No page mode option		*/
#define	OP_SYSTEM	"SYSTEM"	/* System files option		*/
#define	OP_DIRECTORY	"DIRECTORY"	/* Directory files option	*/
#define	OP_XFCB		"XFCB"		/* XFCB only option		*/
#define	OP_CONFIRM	"CONFIRM"	/* Confirm option		*/
#define	OP_GET		"GET"		/* Get information option	*/
#define	OP_FILE		"FILE"		/* FILE option (not) XFCB	*/
#define	OP_NOCONFIRM	"NOCONFIRM"	/* No confirm option		*/
#define	OP_TAB		"TAB"		/* Expand tabs option		*/

/*----------------------------------------------------------------------*\
 |			Global structure definitions			|
\*----------------------------------------------------------------------*/

struct	_cbuf		/* BDOS c_readstr buffer	*/
{
	BYTE	max_char;		/* max. char. to read	*/
	BYTE	nchar;			/* actual # char. read	*/
	BYTE	buffer[DMA_LEN+2];	/* line buffer		*/
};

struct	_pfcb		/* BDOS F_PARSE file cont. blk	*/
{
	BYTE	*fname;			/* ASCII file spec.	*/
	BYTE	*fcbaddr;		/* FCB address		*/
};

struct	_fcblst		/* expfcb link list entry of FCB's	*/
{
	BYTE	fcb_buff[FCB_LEN];	/* FCB buffer		*/
	struct	_fcblst *next_fcb;	/* ptr to next entry	*/
};

struct	_oplist
{
	BYTE	opname[14];	/* command option name	*/
	WORD	max_num;	/* max. number spec.	*/
};

/*----------------------------------------------------------------------*\
 |			External Variables				|
\*----------------------------------------------------------------------*/

EXTERN	WORD _EXTERR;	/* AX save area; kludge to get all of AX	*/
EXTERN	WORD _SYSERR;	/* CX save area; kludge to get CX error codes	*/
