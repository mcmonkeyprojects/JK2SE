// console.c

#include "client.h"
#include "..\strings\con_text.h"
#include "..\qcommon\strip.h"
#include "../qcommon/game_version.h"


int g_console_field_width = 78;

console_t	con;

cvar_t		*con_conspeed;
cvar_t		*con_notifytime;

#define	DEFAULT_CONSOLE_WIDTH	78

vec4_t	console_color = {1.0, 1.0, 1.0, 1.0};


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	// closing a full screen console restarts the demo loop
	if ( cls.state == CA_DISCONNECTED && cls.keyCatchers == KEYCATCH_CONSOLE ) {
		CL_StartDemoLoop();
		return;
	}

	Field_Clear( &kg.g_consoleField );
	kg.g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify ();
	cls.keyCatchers ^= KEYCATCH_CONSOLE;
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void) {	//yell
	chat_playerNum = -1;
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void) {	//team chat
	chat_playerNum = -1;
	chat_team = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f (void) {		//target chat
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f (void) {	//attacker
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	Con_Bottom();		// go to end
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int				l, x, i;
	int			*line;
	fileHandle_t	f;
	char			buffer[1024];

	if (Cmd_Argc() != 2)
	{
		Com_Printf (SP_GetStringText(CON_TEXT_DUMP_USAGE));
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", Cmd_Argv(1) );

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if (!f)
	{
		Com_Printf (S_COLOR_RED"ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for(i=0; i<con.linewidth; i++)
			buffer[i] = (char) (line[i] & 0xff);
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		strcat( buffer, "\n" );
		FS_Write(buffer, strlen(buffer), f);
	}

	FS_FCloseFile( f );
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}

						

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	MAC_STATIC short	tbuf[CON_TEXTSIZE];

//	width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;
	width = (cls.glconfig.vidWidth / SMALLCHAR_WIDTH) - 2;

	if (width == con.linewidth)
		return;


	if (width < 1)			// video hasn't been initialized yet
	{
		con.xadjust = 1;
		con.yadjust = 1;
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)
		{
			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
		}
	}
	else
	{
		// on wide screens, we will center the text
		con.xadjust = 640.0f / cls.glconfig.vidWidth;
		con.yadjust = 480.0f / cls.glconfig.vidHeight;

		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		Com_Memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)

			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;

	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get ("scr_conspeed", "3", 0);

	Field_Clear( &kg.g_consoleField );
	kg.g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &kg.historyEditLines[i] );
		kg.historyEditLines[i].widthInChars = g_console_field_width;
	}

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand ("messagemode4", Con_MessageMode4_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);

	//Initialize values on first print
	con.initialized = qfalse;
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	int		i;

	// mark time for transparent overlay
	if (con.current >= 0)
		con.times[con.current % NUM_CON_TIMES] = cls.realtime;

	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	for(i=0; i<con.linewidth; i++)
		con.text[(con.current%con.totallines)*con.linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( char *txt ) {
	int		y;
	int		c, l;
	int		color;
	int		mcolor;
	bool	blockcol;
	blockcol = Cvar_VariableIntegerValue( "mc_blockrandom" ) == 1;


	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}
	
	if (!con.initialized) {
		con.color[0] = 
		con.color[1] = 
		con.color[2] =
		con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize ();
		con.initialized = qtrue;
	}

	color = ColorIndex(COLOR_WHITE);
	mcolor = 7;

	while ( (c = (unsigned char) *txt) != 0 ) {
		if ( Q_IsColorString( (unsigned char*) txt ) ) {
			mcolor = color;
			color = ColorIndex( *(txt+1) );
		if (blockcol && color == 10)
		{
			color = 9;
		}
		if (color == 'b' - '0')
		{
			color = mcolor;
			color |= 32;
		}
		if (color == 'd' - '0')
		{
			color = mcolor;
			color |= 128;
		}
		if (color == 'r' - '0')
		{
			color = 11;
		}
		if (color == 's' - '0')
		{
			color = mcolor;
			color |= 64;

		}
		if (color == 'k' - '0')
		{
			if (!blockcol)
			{
			color = mcolor;
			color |= 256;
			}
			else
			{
				color = mcolor;
			}
		}
		if (color == 'j' - '0')
		{
			if (!blockcol)
			{
			color = mcolor;
			color |= 512;
			}
			else
			{
				color = mcolor;
			}
		}
		if (color == 'u' - '0')
		{
			color = mcolor;
			color |= 1024;
		}
			/*if (blockcol && color == 10)
			{
				color = 8;
			}*/
			txt += 2;
			continue;
		}
		// count word length
		for (l=0 ; l< con.linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth) ) {
			Con_Linefeed();

		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed ();
			break;
		case '\r':
			con.x = 0;
			break;
		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = (int) ((color << 8) | c);
			con.x++;
			if (con.x >= con.linewidth) {

				Con_Linefeed();
				con.x = 0;
			}
			break;
		}
	}


	// mark time for transparent overlay

	if (con.current >= 0)
		con.times[con.current % NUM_CON_TIMES] = cls.realtime;
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput (void) {
	int		y;

	if ( cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = con.vislines - ( SMALLCHAR_HEIGHT * (re.Language_IsAsian() ? 1.5 : 2) );

	re.SetColor( con.color );

	SCR_DrawSmallChar( (int)(con.xadjust + 1 * SMALLCHAR_WIDTH), y, ']' );

	Field_Draw( &kg.g_consoleField, (int)(con.xadjust + 2 * SMALLCHAR_WIDTH), y,
				SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue );
}




/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
/*int gl_getwidth( void )
{
	return re.gl_mgetwidth();
}
int gl_getheight( void )
{
	return re.gl_mgetheight();
}*/
//extern static int gl_getwidth( void );
//extern static int gl_getheight( void );
void Con_DrawNotify (void)
{
	int		x, v;
	int	*text;
	int		i;
	int		time;
	int		skip;
	int		currentColor;
	bool	bold;
	bool	shadow;
	bool	strike;
	bool	obfu;
	bool	jello;
	bool	underline;

	currentColor = 7;
	bold = false;
	shadow = false;
	strike = false;
	obfu = false;
	jello = false;
	underline = false;
	re.SetColor( g_color_table[currentColor] );

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
			continue;
		}


		if (!cl_conXOffset)
		{
			cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
		}

		// asian language needs to use the new font system to print glyphs...
		//
		// (ignore colours since we're going to print the whole thing as one string)
		//
		if (re.Language_IsAsian())
		{
			static int iFontIndex = re.RegisterFont("ocr_a");	// this seems naughty
			const float fFontScale = 0.75f*con.yadjust;
			const int iPixelHeightToAdvance =   2+(1.3/con.yadjust) * re.Font_HeightPixels(iFontIndex, fFontScale);	// for asian spacing, since we don't want glyphs to touch.

			// concat the text to be printed...
			//
			char sTemp[4096]={0};	// ott
			for (x = 0 ; x < con.linewidth ; x++) 
			{
				if ( ( (text[x]>>8)&7 ) != currentColor ) {
					currentColor = (text[x]>>8)&7;
					strcat(sTemp,va("^%i", (text[x]>>8)&7) );
				}
				strcat(sTemp,va("%c",text[x] & 0xFF));				
			}
			//
			// and print...
			//
			re.Font_DrawString(cl_conXOffset->integer + con.xadjust*(con.xadjust + (1*SMALLCHAR_WIDTH/*aesthetics*/)), con.yadjust*(v), sTemp, g_color_table[currentColor], iFontIndex, -1, fFontScale);

			v +=  iPixelHeightToAdvance;
		}
		else
		{		
			for (x = 0 ; x < con.linewidth ; x++) {
				char let;
				if ( ( text[x] & 0xff ) == ' ' ) {
					continue;
				}
				//if ( ( (text[x]>>8)&7 ) != currentColor ) {
		//		if (((text[x]>>8) & 0xffffff) != currentColor){
					//currentColor = (text[x]>>8)&7;
					currentColor = (text[x]>>8) & 0xffffff;
					if (currentColor & 32)
					{
						bold = true;
						currentColor &= ~32;
					}
					else
					{
						bold = false;
					}
					if (currentColor & 128)
					{
						shadow = true;
						currentColor &= ~128;
					}
					else
					{
						shadow = false;
					}
					if (currentColor & 64)
					{
						strike = true;
						currentColor &= ~64;
					}
					else
					{
						strike = false;
					}
					if (currentColor & 256)
					{
						obfu = true;
						currentColor &= ~256;
					}
					else
					{
						obfu = false;
					}
					if (currentColor & 512)
					{
						jello = true;
						currentColor &= ~512;
					}
					else
					{
						jello = false;
					}
					if (currentColor & 1024)
					{
						underline = true;
						currentColor &= ~1024;
					}
					else
					{
						underline = false;
					}
					re.SetColor( g_color_table[currentColor] );
	//			}
				if (obfu)
				{
					let = irand(33, 126);
				}
				else
				{
					let = text[x] & 0xff;
				}
				if (shadow)
				{
					re.SetColor(g_color_table[0] );
				SCR_DrawSmallChar( (int)(cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH)+2, v+2, let );
					re.SetColor( g_color_table[currentColor] );
				}
				if (currentColor == 10)
				{
					re.SetColor(g_color_table[irand(0, 9)]);
				}
				if (!cl_conXOffset)
				{
					cl_conXOffset = Cvar_Get ("cl_conXOffset", "0", 0);
				}
				if (currentColor == 11)
				{
					re.SetColor(g_color_table[(text[x] & 0xff) % 10]);
				}
				if (bold)
				{
				SCR_DrawSmallChar( (int)(cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH)+1, v, let );
				}
				SCR_DrawSmallChar( (int)(cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH) + (jello?irand(-1,1):0), v + (jello?irand(-1,1):0), let );
				if (strike)
				{
					re.SetColor( g_color_table[7]);
					re.DrawStretchPic( (int)(cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH) * (float)((float)640/(float)gl_getwidth()), (v + SMALLCHAR_HEIGHT/2) * (float)((float)480/(float)gl_getheight()), SMALLCHAR_WIDTH, 1, 0, 0, 0, 0, cls.whiteShader );
					re.SetColor(g_color_table[currentColor]);
				}
				if (underline)
				{
					re.SetColor( g_color_table[7]);
					re.DrawStretchPic( (int)(cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH) * (float)((float)640/(float)gl_getwidth()), (v + SMALLCHAR_HEIGHT) * (float)((float)480/(float)gl_getheight()), SMALLCHAR_WIDTH, 1, 0, 0, 0, 0, cls.whiteShader );
					re.SetColor(g_color_table[currentColor]);
				}
			}

			v += SMALLCHAR_HEIGHT;
		}
	}

	re.SetColor( NULL );

	if (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
		return;
	}

	// draw the chat line
	if ( cls.keyCatchers & KEYCATCH_MESSAGE )
	{
		if (chat_team)
		{
			SCR_DrawBigString (8, v, "say_team:", 1.0f );
			skip = 11;
		}
		else
		{
			SCR_DrawBigString (8, v, "say:", 1.0f );
			skip = 5;
		}

		Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH, v,
			SCREEN_WIDTH - ( skip + 1 ) * BIGCHAR_WIDTH, qtrue );

		v += BIGCHAR_HEIGHT;
	}

}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac ) {
	int				i, x, y;
	int				rows;
	int			*text;
	int				row;
	int				lines;
//	qhandle_t		conShader;
	int				currentColor;
	bool			bold;
	bool			shadow;
	bool			strike;
	bool	obfu;
	bool	jello;
	bool	underline;
	
	strike  = false;
	bold = false;
	shadow = false;
	obfu = false;
	jello = false;
	underline = false;
	lines = (int) (cls.glconfig.vidHeight * frac);
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	// draw the background
	y = (int) (frac * SCREEN_HEIGHT - 2);
	if ( y < 1 ) {
		y = 0;
	}
	else {
		SCR_DrawPic( 0, 0, SCREEN_WIDTH, (float) y, cls.consoleShader );
	}

	const vec4_t color = { 0.409f, 0.609f, 0.847f,  1.0f};
	// draw the bottom bar and version number

	re.SetColor( color );
	re.DrawStretchPic( 0, y, SCREEN_WIDTH, 2, 0, 0, 0, 0, cls.whiteShader );

	i = strlen( Q3_VERSION );

	for (x = 0; x < i; x++) {
		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH, 
			(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)), Q3_VERSION[x] );
	}

	if (update)
	{
		char *updmes = "There is a JK2:SE update!";
	i = strlen( updmes );
	const vec4_t rcolor = { 0.4f, 0.1f, 0.0f,  1.0f};
	re.SetColor( rcolor );
	for (x= 0; x < i; x++)
		{
		SCR_DrawSmallChar( (cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH) + 2, 
			(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2) - SMALLCHAR_HEIGHT) + 2, updmes[x] );
		}
	const vec4_t mcolor = { 1.0f, 0.0f, 0.0f,  1.0f};
	re.SetColor( mcolor );
	for (x= 0; x < i; x++)
		{
		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH, 
			(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2) - SMALLCHAR_HEIGHT), updmes[x] );
		}
	}

	// draw the text
	con.vislines = lines;
	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (con.display != con.current)
	{
	// draw arrows to show the buffer is backscrolled
		re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );
		for (x=0 ; x<con.linewidth ; x+=4)
			SCR_DrawSmallChar( (int) (con.xadjust + (x+1)*SMALLCHAR_WIDTH), y, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}
	
	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}

	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );

	static int iFontIndexForAsian = 0;
	const float fFontScaleForAsian = 0.75f*con.yadjust;
	int iPixelHeightToAdvance = SMALLCHAR_HEIGHT;
	if (re.Language_IsAsian())
	{
		if (!iFontIndexForAsian) 
		{
			iFontIndexForAsian = re.RegisterFont("ocr_a");
		}
		iPixelHeightToAdvance = (1.3/con.yadjust) * re.Font_HeightPixels(iFontIndexForAsian, fFontScaleForAsian);	// for asian spacing, since we don't want glyphs to touch.
	}

	for (i=0 ; i<rows ; i++, y -= iPixelHeightToAdvance, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines) {
			// past scrollback wrap point
			continue;	
		}

		text = con.text + (row % con.totallines)*con.linewidth;

		// asian language needs to use the new font system to print glyphs...
		//
		// (ignore colours since we're going to print the whole thing as one string)
		//
		if (re.Language_IsAsian())
		{
			// concat the text to be printed...
			//
			char sTemp[4096]={0};	// ott
			for (x = 0 ; x < con.linewidth ; x++) 
			{
				if ( ( (text[x]>>8)&7 ) != currentColor ) {
					currentColor = (text[x]>>8)&7;
					strcat(sTemp,va("^%i", (text[x]>>8)&7) );
				}
				strcat(sTemp,va("%c",text[x] & 0xFF));				
			}
			//
			// and print...
			//
			re.Font_DrawString(con.xadjust*(con.xadjust + (1*SMALLCHAR_WIDTH/*(aesthetics)*/)), con.yadjust*(y), sTemp, g_color_table[currentColor], iFontIndexForAsian, -1, fFontScaleForAsian);
		}
		else
		{		
			for (x=0 ; x<con.linewidth ; x++) {
				char let;
				if ( ( text[x] & 0xff ) == ' ' ) {
					continue;
				}

				//if ( ( (text[x]>>8)&7 ) != currentColor ) {
	//			if (((text[x]>>8) & 0xffffff) != currentColor){
					//currentColor = (text[x]>>8)&7;
					currentColor = (text[x]>>8) & 0xffffff;
					if (currentColor & 32)
					{
						bold = true;
						currentColor &= ~32;
					}
					else
					{
						bold = false;
					}
					if (currentColor & 128)
					{
						shadow = true;
						currentColor &= ~128;
					}
					else
					{
						shadow = false;
					}
					if (currentColor & 64)
					{
						strike = true;
						currentColor &= ~64;
					}
					else
					{
						strike = false;
					}
					if (currentColor & 256)
					{
						obfu = true;
						currentColor &= ~256;
					}
					else
					{
						obfu = false;
					}
					if (currentColor & 512)
					{
						jello = true;
						currentColor &= ~512;
					}
					else
					{
						jello = false;
					}
					if (currentColor & 1024)
					{
						underline = true;
						currentColor &= ~1024;
					}
					else
					{
						underline = false;
					}
					re.SetColor( g_color_table[currentColor] );
	//			}
				if (obfu)
				{
					let = irand(33, 126);
				}
				else
				{
					let = text[x] & 0xff;
				}
				if (shadow)
				{
					re.SetColor(g_color_table[0]);
					SCR_DrawSmallChar(  (int) (con.xadjust + (x+1)*SMALLCHAR_WIDTH) + 2, y + 2, let );
					re.SetColor(g_color_table[currentColor] );
				}
				if (currentColor == 10)
				{
					re.SetColor(g_color_table[irand(0, 9)]);
				}
				if (currentColor == 11)
				{
					re.SetColor(g_color_table[(text[x] & 0xff) % 10]);
				}
				if (bold)
				{
			SCR_DrawSmallChar(  (int) (con.xadjust + (x+1)*SMALLCHAR_WIDTH) + 1, y, let );
				}
			SCR_DrawSmallChar(  (int) (con.xadjust + (x+1)*SMALLCHAR_WIDTH) + (jello?irand(-1,1):0), y + (jello?irand(-1,1):0), let );
			if (strike)
				{
					re.SetColor( g_color_table[7]);
					re.DrawStretchPic( ((int)(con.xadjust + (x+1)*SMALLCHAR_WIDTH))*(float)((float)640/(float)gl_getwidth()), (y + SMALLCHAR_HEIGHT/2) * (float)((float)480/(float)gl_getheight()), SMALLCHAR_WIDTH, 1, 0, 0, 0, 0, cls.whiteShader );
					re.SetColor(g_color_table[currentColor]);
				}
				if (underline)
				{
					re.SetColor( g_color_table[7]);
					re.DrawStretchPic( ((int)(con.xadjust + (x+1)*SMALLCHAR_WIDTH))*(float)((float)640/(float)gl_getwidth()), (y + SMALLCHAR_HEIGHT) * (float)((float)480/(float)gl_getheight()), SMALLCHAR_WIDTH, 1, 0, 0, 0, 0, cls.whiteShader );
					re.SetColor(g_color_table[currentColor]);
				}
			}
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();

	re.SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize ();

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
	} else {
		// draw notify lines
		if ( cls.state == CA_ACTIVE ) {
			Con_DrawNotify ();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {
	// decide on the destination height of the console
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
		con.finalFrac = 0.5;		// half screen
	else
		con.finalFrac = 0;				// none visible
	
	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed->value*(float)(cls.realFrametime*0.001);
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed->value*(float)(cls.realFrametime*0.001);
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}

}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &kg.g_consoleField );
	Con_ClearNotify ();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}
