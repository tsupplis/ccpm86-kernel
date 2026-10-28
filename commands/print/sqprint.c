#include	<portab.h>
#include	"utildef.h"
#include	"cpmfunc.h"

#define	TAB_DEFAULT	8		/* default tab position		*/

struct	_cmdtail			/* command tail			*/
{
	WORD	gbl_sys;			/* global system option	*/
	WORD	gbl_grp;			/* global group option	*/
	WORD	gbl_tab;			/* global tab option	*/
	WORD	lcl_sys[MAX_FILESPEC];		/* local system option	*/
	WORD	lcl_grp[MAX_FILESPEC];		/* local group option	*/
	WORD	lcl_tab[MAX_FILESPEC];		/* local tab option	*/
	BYTE	filespec[MAX_FILESPEC][FILESPEC_LEN];/* filespec list	*/
};

EXTERN	VOID	cpmerr();
EXTERN	WORD	ok_ver();
EXTERN	VOID	get_tail();
EXTERN	VOID	copy_ucase();
EXTERN	WORD	nextfcb();
EXTERN	WORD	fileopen();
EXTERN	WORD	readsect();
EXTERN	VOID	crlf();
EXTERN	BYTE	firstch();		/* return first non-white char	*/
EXTERN	BYTE	*tlex();		/* tlex returns a pointer	*/
EXTERN	VOID	putfname();
EXTERN	VOID	itoa();

BYTE	*pmt00   =	"Enter Filename: ";
BYTE	*pmt01   =	"Do you want to W(ait) or C(ancel) ? ";
BYTE	*pmt02   =	"Do you want to S(top printing), skip to the";
BYTE	*pmt03   =	"N(ext file), or R(esume printing this file) ? ";

BYTE	*msg00   =	"Printing file: ";
BYTE	*msg01   =	"Press SPACE BAR to stop ";
BYTE	*msg02   =	"Printer is being used by another console.";
BYTE	*msg03   =	"Printing is completed.";
BYTE	*msg04   =	" files printed.";
BYTE	*msg05   =	"Print cancelled.";
BYTE	*err00   =	"No files were specified on the command line.";
BYTE	*err01   =	"File Not Found.";
BYTE	*err03   =	"Invalid Command Option.";
BYTE	*err04   = 	"Requires Concurrent CP/M-86 3.1$";
BYTE	*err05   =	"Could not find file: ";
BYTE	*err06   =	"Invalid file name: ";
BYTE	*err07   =	"Error while opening file: ";
BYTE	*err08   =	"Error while reading file: ";
BYTE	*err09   =	"Error while closing file: ";
BYTE	*err10   =	"A file password is required for file: ";

BYTE	*index();			/* index returns pointer	*/
WORD	ptail();
WORD	set_opt();

VOID	main()
{
	BYTE	  buff[DMA_LEN];	/* file input buffer		*/
	BYTE	  tail[DMA_LEN];	/* command tail buffer		*/
	BYTE	  num_buff[6];		/* character buffer for numbers	*/
	BYTE	  response[24];		/* user response buffer		*/
	BYTE	  *dma;			/* ptr to DMA buffer area	*/
	BYTE	  ch;			/* single char. response	*/
	struct _cmdtail cmdtail;	/* command tail structure	*/
	REG WORD  eod;			/* end of data flag		*/
	REG WORD  i;			/* loop control			*/
	WORD	  op_code;		/* option validation code	*/
	WORD	  fsindex;		/* filespec list index		*/
	WORD	  findex;		/* FCB array index		*/
	WORD	  curr_user;		/* current user number		*/
	WORD	  nofile;		/* no files flag		*/
	WORD	  valid_file;		/* filespec validation flag	*/
	WORD	  completed;		/* # files completed processing	*/
	WORD	  attempted;		/* # files attempted processing	*/
	WORD	  col;			/* output column counter	*/
	WORD	  msg_len;		/* prompt message length	*/
	WORD	  open_err;		/* open file error return code	*/
	WORD	  bad_resp;		/* bad user response flag	*/
	UBYTE	  tail_len;		/* length of command tail	*/
	BYTE	  cur_fcb[FCB_LEN]; 	/* current FCB buffer		*/
	BYTE	  wild_fcb[FCB_LEN];	/* filespec FCB buffer		*/

	if( !ok_ver(BDOS_VER,OS_TYPE) )		/* version number check */
	   cpmerr( err04,-1 );

	f_errmode( 0xff );			/* return BDOS errors 	*/

	c_delimit( NULL );			/* set console output	*/
						/* string delimiter	*/

	dma = f_dmaget();			/* get DMA ptr, byte 0	*/
						/* is length, get it	*/
	tail_len = *dma++;			/* then skip over it	*/

	if( tail_len < 1 )			/* tail present ?	*/
	   cpmerr( err00,-1 );
	else
	   copy_ucase( tail,dma,tail_len );

	if( (op_code = ptail( tail,&cmdtail )) != 0 )	/* parse tail	*/
	   cpmerr( err03,-1 );

	if( l_cattach() == 0x00ff )		/* if list device is	*/
	{					/* in use, prompt user.	*/
	   c_writestr( msg02 );			/* the user may W(ait)	*/
	   crlf();				/* or C(ancel)		*/
	   get_tail( pmt01,response );
	   if( firstch(response) == 'W' )
	      l_attach();
	   else
	      cpmerr( msg05,-1 );		/* display cancel msg	*/
	}					/* and exit		*/

	completed = attempted = 0;		/* no files so far	*/
	curr_user = f_usernum( 0xff );		/* get current user #	*/
	fsindex = 0;
	while( cmdtail.filespec[fsindex][0] != NULL )
	{
	   f_usernum( cmdtail.lcl_grp[fsindex] );
	   nofile = 1;

	   cur_fcb[0] = 0xff;	/* init to 0xff on first pass through	*/

	   while( (valid_file = nextfcb(cmdtail.filespec[fsindex],wild_fcb,
		   cur_fcb)) > -1 )
	   {
	      if( is_sysfile( cur_fcb ) == YES
		  && cmdtail.lcl_sys[fsindex] == NO )
		 continue;			/* skip to next file	*/

	      attempted++;			/* attempt to process	*/
	      nofile = 0;			/*    another file	*/
	      cur_fcb[EX_FIELD]  = 0x00;	/* extent set for open	*/
	      cur_fcb[F6_FIELD] |= 0x80;	/* open file read only	*/
						/*    mode		*/
	      if( (open_err = fileopen( cur_fcb )) != 0 )
	      {					/* error on open,print	*/
		 if( open_err != 7 )		/*    message if not	*/
		 {				/*    password error,	*/
		    crlf();			/*    then skip to the	*/
		    cpmerr( err07,0 );		/*    next file		*/
		    putfname( cur_fcb,0 );
		 }
		 continue;
	      }

	      put_header( cur_fcb,&cmdtail );	/* print file header	*/
						/* on the console	*/
	      eod = 0;
	      cur_fcb[CR_FIELD] = 0x00;		/* set CR field = 0	*/
	      col = 0;				/* set output col count	*/

	      while( !eod && (readsect( cur_fcb,buff ) == 0) )
	      {

	         for( i=0; i < SEC_LEN && !eod; i++  ) /* write sector	*/
		 {
		    if( buff[i] == 0x1a )	/* end of file		*/
		    {
		       eod = 1;
		       break;
		    }
		    if( cmdtail.lcl_tab[fsindex] != 0 )	/* expand tabs	*/
		    {
		       switch( buff[i] )
		       {
			   case 0x09	:  do
					   {
			   		       l_write( ' ' );
					       col++;
					   }
			  		   while( col % cmdtail.lcl_tab[fsindex] );
					   break;
			  case 0x0a	:  l_write( buff[i] );
					   col = 0;
					   break;
			  default	:  l_write( buff[i] );
					   col++;
					   break;
		       }
		    }
		    else
		    {
			l_write( buff[i] );
			col++;
		    }

		    if( c_stat() )		/* check to see if the  */
		    {				/* user wants to	*/
		       ch = c_rawio( 0x00fd );	/* intervene		*/

		       if( ch == '\003' )	/* cntrol-c, absolute	*/
		          p_termcpm();		/* halt of print	*/

		       if( ch == SP )		/* user intervenes if	*/
		       {			/* he hits SPACE BAR	*/

			  c_write( CR );	/* clear prompt area	*/
			  msg_len = strlen( msg01 );
			  while( msg_len-- )	/* and display new 	*/
			     c_write( SP );	/* prompt		*/
			  c_write( CR );
			  do
			  {
			      bad_resp = FALSE;	
			      c_writestr( pmt02 );
			      crlf();
			      get_tail( pmt03,response );
			      switch( firstch( response ) )
			      {
			          case 'S'	:  l_write( FF );
					   	   p_termcpm();
					   	   break;
			     	  case 'N'	:  eod = 1;
					  	   --completed;
					   	   break;
			     	  case NULL	:
			     	  case 'R'	:  c_writestr( msg01 );
					   	   break;
			     	  default	:  bad_resp = TRUE;
						   break;
			     }
			  }
			  while( bad_resp == TRUE );
		       }
		    }
		 }
	      }

	      completed++;

	      c_write( CR );			/* erase prompt		*/
	      msg_len = strlen( msg01 );
	      while( msg_len-- )
	         c_write( SP );
	      c_write( CR );

	      l_write( FF );			/* eject a page, next file */
						/* begins on a new page	   */

	      if( (f_close(cur_fcb) & 0x00ff) == 0x00ff )
	      {
		 crlf();
		 cpmerr( err09,0 );
		 putfname( cur_fcb,0 );
	      }

	   }

	   if( nofile && ( valid_file != INVALID_FILE ) )
	   {
	      crlf();
	      cpmerr( err05,0 );
	      putfname( wild_fcb,0 );
	   }

	   fsindex++;
	}

	crlf();				/* display print completion	*/
	c_writestr( msg03 );		/* message			*/
	crlf();
	itoa( completed,num_buff );
	c_writestr( num_buff );
	c_write( '/' );
	itoa( attempted,num_buff );
	c_writestr( num_buff );
	c_writestr( msg04 );
	
	f_usernum( curr_user );		/* reset current user		*/

	f_errmode( 0 );			/* reset BDOS error mode	*/

	l_detach();			/* detach list device		*/
}

WORD	ptail( tail,t_struct )
BYTE	*tail;			/* command tail to parse	*/
struct	_cmdtail *t_struct;  	/* struct. to hold parsed tail	*/
{
	BYTE	token[128];	/* cmd. line identifier		*/
	BYTE	*delim;		/* string of delimiters		*/ 
	BYTE	*tptr;		/* tail pointer			*/
	WORD	ret_code;	/* return code			*/
	WORD	op_cnt;		/* option count			*/
	WORD	fsindex;	/* filespec list index		*/
	struct	_oplist	oplist[5];	/* valid options list	*/

	strcpy( oplist[0].opname,OP_TAB );	/* initialize options list */
	oplist[0].max_num = 32767;
	strcpy( oplist[1].opname,OP_SYSTEM );
	oplist[1].max_num = 0;
	strcpy( oplist[2].opname,OP_DIRECTORY );
	oplist[2].max_num = 0;
	strcpy( oplist[3].opname,OP_GROUP );
	oplist[3].max_num = 15;
	oplist[4].opname = NULL;		/* sentinal		   */
	oplist[4].max_num = 0;

	t_struct->gbl_sys = NO;			/* assume no system option */
	t_struct->gbl_grp = f_usernum( 0x00ff );/* assume no system opt    */
	t_struct->gbl_tab = TAB_DEFAULT;	/* default tab position    */
	tptr = tail;
	fsindex = 0;
	ret_code = 0;
	op_cnt = 0;
	delim = ",]";
	while( firstch( tptr ) == '[' && ret_code == 0 )
	{
	   tptr++;	/* skip start symbol	*/
	   op_cnt++;
	   tptr = tlex( tptr,delim,token );
	   ret_code = set_opt( token,t_struct,fsindex,oplist,0 );
	   while( firstch( tptr ) == ',' && ret_code == 0 )
	   {
	      tptr++;
	      op_cnt++;
	      tptr = tlex( tptr,delim,token );
	      ret_code = set_opt( token,t_struct,fsindex,oplist,0 );
	   }
	   if( *(tptr+1) == '[' )	/* skip  '][' condition 	  */
	      tptr++;
	}
	if( op_cnt > 2 )		/* to many options given	  */
	   ret_code = -1;

	if( firstch( tptr ) == ']' )
	   tptr++;

	if( firstch( tptr ) == NULL )	/* no files specified on command  */
	   cpmerr( err00,-1 );		/* line is an error		  */

	while ( firstch( tptr ) != NULL && ret_code == 0 )
	{
	   delim = "[,	 ";
	   tptr = tlex( tptr,delim,token );
	   t_struct->lcl_sys[fsindex] = t_struct->gbl_sys;
	   t_struct->lcl_grp[fsindex] = t_struct->gbl_grp;
	   t_struct->lcl_tab[fsindex] = t_struct->gbl_tab;
	   strcpy( t_struct->filespec[fsindex],token );
	   op_cnt = 0;
	   while( firstch( tptr ) == '[' && ret_code == 0 )
	   {
	      tptr++;
	      op_cnt++;
	      delim = ",]";
	      tptr = tlex( tptr,delim,token );
	      ret_code = set_opt( token,t_struct,fsindex,oplist,1 );
	      while( firstch( tptr ) == ',' && ret_code == 0 )
	      {
		 tptr++;
		 op_cnt++;
		 tptr = tlex( tptr,delim,token );
		 ret_code = set_opt( token,t_struct,fsindex,oplist,1 );
	      }
	      if( *(tptr+1) == '[' )
		 tptr++;
	   }
	   if( op_cnt > 2 )			/* too many options 	*/
	      ret_code = -1;

	   if( firstch( tptr ) == ']' )
	      tptr++;				/* skip delimiter	*/
	   if( firstch( tptr ) == ',' )
	      tptr++;

	   fsindex++;
	}
	t_struct->filespec[fsindex][0] = NULL;	/* end filespec list	*/
	return( ret_code );
}

WORD	set_opt( token,t_ptr,fsindex,oplist,type )
BYTE	*token;
struct	_cmdtail *t_ptr;
WORD	fsindex;
struct	_oplist  *oplist;
WORD	type;
{
	WORD	ret_code;

	if( (ret_code = valid( token,oplist )) == 0 )
	{
	   switch( (WORD)token[0] )
	   {
	      case 'S'  : if( type == 0 )
			      t_ptr->gbl_sys = YES;
			  else
			      t_ptr->lcl_sys[fsindex] = YES;
			  break;
	      case 'D'  : if( type == 0 )
			      t_ptr->gbl_sys = NO;
			  else
			      t_ptr->lcl_sys[fsindex] = NO;
			  break;
	      case 'G'  : if( type == 0 )
			      t_ptr->gbl_grp = get_num(token,t_ptr->gbl_grp);
			  else
			      t_ptr->lcl_grp[fsindex] = get_num( token,
					t_ptr->lcl_grp[fsindex] );
			  break;
	      case 'T'  : if( type == 0 )
			     t_ptr->gbl_tab = get_num(token,TAB_DEFAULT);
			  else
			     t_ptr->lcl_tab[fsindex] = get_num(token,
					t_ptr->lcl_tab[fsindex] );
			  break;
	      default   : break;
	   }
	}
	return( ret_code );
}

VOID	put_header( cur_fcb,cmdtail )
BYTE	cur_fcb[];
struct	_cmdtail  *cmdtail;
{
	WORD	findex;

	crlf();					/* display user header	*/
	c_writestr( msg00 );			/* on console		*/
	putfname( cur_fcb,0 );
	crlf();
	c_writestr( msg01 );

}
