/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <unistd.h>
#include "back.h"

extern int buffnum;
char	outbuff[BUFSIZ];

static char	plred[] = "Player is red, computer is white.";
static char	plwhite[] = "Player is white, computer is red.";
static char	nocomp[] = "(No computer play.)";

char  *descr[] = {
	"Usage:  backgammon [-] [n r w b pr pw pb t3a]\n",
	"\t-\tgets this list\n\tn\tdon't ask for rules or instructions",
	"\tr\tplayer is red (implies n)\n\tw\tplayer is white (implies n)",
	"\tb\ttwo players, red and white (implies n)",
	"\tpr\tprint the board before red's turn",
	"\tpw\tprint the board before white's turn",
	"\tpb\tprint the board before both player's turn",
	"\tterm\tterminal is a term",
	"\tsfile\trecover saved game from file",
	0
};

void
errexit (char *s)
{
	write (2,"\n",1);
	perror (s);
	getout (0);
}

void
strset (char *s1, char *s2)
{
	while ( (*s1++ = *s2++) != '\0');
}

int
addbuf (int c)
{
	buffnum++;
	if (buffnum == BUFSIZ)  {
		if (write(1, outbuff, BUFSIZ) != BUFSIZ)
			errexit ("addbuf (write):");
		buffnum = 0;
	}
	outbuff[buffnum] = c;
	return 0;
}

void
buflush ()
{
	if (buffnum < 0)
		return;
	buffnum++;
	if (write (1,outbuff,buffnum) != buffnum)
		errexit ("buflush (write):");
	buffnum = -1;
}

int
readc ()
{
	char	c;

	if (tflag)  {
		cline();
		newpos();
	}
	buflush();
	if (read(0,&c,1) != 1)
		errexit ("readc");
	if (c == '\177')
		getout (0);
	if (c == '\033' || c == '\015')
		return ('\n');
	if (cflag)
		return (c);
	if (c == '\014')
		return ('R');
	if (c >= 'a' && c <= 'z')
		return (c & 0137);
	return (c);
}

void
writec (int c)
{
	if (tflag)
		fancyc (c);
	else
		addbuf (c);
}

void
writel (const char *l)
{
#ifdef DEBUG
	register const char	*s;

	if (trace == NULL)
		trace = fopen ("bgtrace","w");

	fprintf (trace,"writel: \"");
	for (s = l; *s; s++) {
		if (*s < ' ' || *s == '\177')
			fprintf (trace,"^%c",(*s)^0100);
		else
			putc (*s,trace);
	}
	fprintf (trace,"\"\n");
	fflush (trace);
#endif

	while (*l)
		writec (*l++);
}

void
proll ()
{
	if (d0)
		swap;
	if (cturn == 1)
		writel ("Red's roll:  ");
	else
		writel ("White's roll:  ");
	writec (D0+'0');
	writec ('\040');
	writec (D1+'0');
	if (tflag)
		cline();
}

void
wrint (int n)
{
	register int	i, j, t;

	for (i = 4; i > 0; i--)  {
		t = 1;
		for (j = 0; j<i; j++)
			t *= 10;
		if (n > t-1)
			writec ((n/t)%10+'0');
	}
	writec (n%10+'0');
}

void
gwrite()
{
	register int	r = 0, c = 0;

	if (tflag)  {
		r = curr;
		c = curc;
		curmove (16,0);
	}

	if (gvalue > 1)  {
		writel ("Game value:  ");
		wrint (gvalue);
		writel (".  ");
		if (dlast == -1)
			writel (color[0]);
		else
			writel (color[1]);
		writel (" doubled last.");
	} else  {
		switch (pnum)  {
		case -1:			    /* player is red */
			writel (plred);
			break;
		case 0:				    /* player is both colors */
			writel (nocomp);
			break;
		case 1:				    /* player is white */
			writel (plwhite);
		}
	}

	if (rscore || wscore)  {
		writel ("  ");
		wrscore();
	}

	if (tflag)  {
		cline();
		curmove (r, c);
	}
}

int
quit ()
{
	if (tflag)  {
		curmove (20,0);
		clend();
	} else
		writec ('\n');
	writel ("Are you sure you want to quit?");
	if (yorn (0))  {
		if (rfl)  {
			writel ("Would you like to save this game?");
			if (yorn(0))
				save(0);
		}
		cturn = 0;
		return (1);
	}
	return (0);
}

int
yorn (int special)                              /* special response */
{
	register int	c;
	register int	i;

	i = 1;
	while ((c = readc()) != 'Y' && c != 'N')  {
		if (special && c == special)
			return (2);
		if (i)  {
			if (special)  {
				writel ("  (Y, N, or ");
				writec (special);
				writec (')');
			} else
				writel ("  (Y or N)");
			i = 0;
		} else
			writec ('\007');
	}
	if (c == 'Y')
		writel ("  Yes.\n");
	else
		writel ("  No.\n");
	if (tflag)
		buflush();
	return (c == 'Y');
}

void
wrhit (int i)
{
	writel ("Blot hit on ");
	wrint (i);
	writec ('.');
	writec ('\n');
}

void
nexturn ()
{
	register int	c;

	cturn = -cturn;
	c = cturn/abs(cturn);
	home = bar;
	bar = 25-bar;
	offptr += c;
	offopp -= c;
	inptr += c;
	inopp -= c;
	Colorptr += c;
	colorptr += c;
}

void
getarg (char ***arg)
{
	register char	**s;

	/* process arguments here.  dashes are ignored, nbrw are ignored
	   if the game is being recovered */

	s = *arg;
	while (s[0][0] == '-') {
		switch (s[0][1])  {

		/* don't ask if rules or instructions needed */
		case 'n':
			if (rflag)
				break;
			aflag = 0;
			args[acnt++] = 'n';
			break;

		/* player is both read and white */
		case 'b':
			if (rflag)
				break;
			pnum = 0;
			aflag = 0;
			args[acnt++] = 'b';
			break;

		/* player is red */
		case 'r':
			if (rflag)
				break;
			pnum = -1;
			aflag = 0;
			args[acnt++] = 'r';
			break;

		/* player is white */
		case 'w':
			if (rflag)
				break;
			pnum = 1;
			aflag = 0;
			args[acnt++] = 'w';
			break;

		/* print board after move according to following character */
		case 'p':
			if (s[0][2] != 'r' && s[0][2] != 'w' && s[0][2] != 'b')
				break;
			args[acnt++] = 'p';
			args[acnt++] = s[0][2];
			if (s[0][2] == 'r')
				bflag = 1;
			if (s[0][2] == 'w')
				bflag = -1;
			if (s[0][2] == 'b')
				bflag = 0;
			break;

		case 't':
			if (s[0][2] == '\0') {	/* get terminal caps */
				s++;
				tflag = getcaps (*s);
			} else
				tflag = getcaps (&s[0][2]);
			break;

		case 's':
			s++;
			/* recover file */
			recover (s[0]);
			break;
		}
		s++;
	}
	if (s[0] != 0)
		recover(s[0]);
}

void
init ()
{
	register int	i;
	for (i = 0; i < 26;)
		board[i++] = 0;
	board[1] = 2;
	board[6] = board[13] = -5;
	board[8] = -3;
	board[12] = board[19] = 5;
	board[17] = 3;
	board[24] = -2;
	off[0] = off[1] = -15;
	in[0] = in[1] = 5;
	gvalue = 1;
	dlast = 0;
}

void
wrscore ()
{
	writel ("Score:  ");
	writel (color[1]);
	writec (' ');
	wrint (rscore);
	writel (", ");
	writel (color[0]);
	writec (' ');
	wrint (wscore);
}

void
fixtty (int mode)
{
	if (tflag)
		newpos();
	buflush();
	tty.sg_flags = mode;
	ioctl (0, TIOCSETP, &tty);
}

void
getout (int sig)
{
	/* go to bottom of screen */
	if (tflag)  {
		curmove (23,0);
		cline();
	} else
		writec ('\n');

	/* fix terminal status */
	fixtty (old);
	exit(-1);
}

void
roll ()
{
	register char	c;
	register int	row = 0;
	register int	col = 0;

	if (iroll)  {
		if (tflag)  {
			row = curr;
			col = curc;
			curmove (17,0);
		} else
			writec ('\n');
		writel ("ROLL: ");
		c = readc();
		if (c != '\n')  {
			while (c < '1' || c > '6')
				c = readc();
			D0 = c-'0';
			writec (' ');
			writec (c);
			c = readc();
			while (c < '1' || c > '6')
				c = readc();
			D1 = c-'0';
			writec (' ');
			writec (c);
			if (tflag)  {
				curmove (17,0);
				cline();
				curmove (row,col);
			} else
				writec ('\n');
			return;
		}
		if (tflag)  {
			curmove (17,0);
			cline();
			curmove (row,col);
		} else
			writec ('\n');
	}
	D0 = rnum(6)+1;
	D1 = rnum(6)+1;
	d0 = 0;
}
