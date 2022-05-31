/*-------------------------------------------------------------------------
 *
 * createdb
 *
 * Portions Copyright (c) 1996-2022, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/bin/scripts/createdb.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"

#include "common.h"
#include "common/logging.h"
#include "fe_utils/option_utils.h"
#include "fe_utils/string_utils.h"


//GraphicInterface.h
#include <SFML/Graphics.h>	//added
#include <SFML/OpenGL.h>	//added

#define SWITCHBUTTONNUMBER 2
#define TEXTOBJECTNUMBER 15
#define ENTRYFIELDNUMBER 15
#define CHARININENTRYFIELD 28

typedef enum {
	NullArg = -1,
	DBName_Arg,
	tablespace_Arg,
	encoding_Arg,
	locale_Arg,
	lc_collate_Arg,
	lc_ctype_Arg,
	icu_locale_Arg,
	locale_provider_Arg,
	owner_Arg,
	strategy_Arg,
	template_Arg,
	host_Arg,
	port_Arg,
	username_Arg,
	maintenance_Arg,
} ArgEnum;

typedef struct {
	float R;
	float G;
	float B;
} Color_RGB;

typedef struct {
	short CoorX;
	short CoorY;
	short Borders_CoorX_Min;
	short Borders_CoorX_Max;
	short Borders_CoorY_Min;
	short Borders_CoorY_Max;
	Color_RGB Color;
	
	short NumberSymbol;
	sfText *Text;
	char TextStr[CHARININENTRYFIELD];
	sfVector2f position;
} EntryFieldType;

typedef struct {
	short CoorX;
	short CoorY;
	short Borders_CoorX_Min;
	short Borders_CoorX_Max;
	short Borders_CoorY_Min;
	short Borders_CoorY_Max;
	Color_RGB Color;
	
	sfText *Text;
	char TextStr_True[CHARININENTRYFIELD];
	char TextStr_False[CHARININENTRYFIELD];
	sfVector2f position;

	bool Active;
} SwitchButtonType;

typedef struct {
	short AttachmentPoint_CoorX;
	short AttachmentPoint_CoorY;
	sfText *Text;
	char TextStr[100];			//????????????????????
	sfVector2f position;
} TextType;

typedef struct {
	short CoorX;
	short CoorY;
} MouseMoveType;

typedef struct 
{
	short Width;	
	short Height;
	short MaxCoorX;	// where coordinates are from -WindowWidth/2 to WindowWidth/2
	short MaxCoorY;	// where coordinates are from -WindowHeight/2 to WindowHeight/2
	char Name[30];
	sfVideoMode mode;
	sfRenderWindow *Window;
} WindowType;


static void help(const char *progname);
static int MouseAndEntryFieldInteraction(EntryFieldType *EntryField, int EntryFieldNumber, MouseMoveType *MouseMove);
static void FillEntryFieldArray(EntryFieldType *EntryField, int Width, int Height, sfColor *TextColor, sfFont *TextFont, WindowType *MainWindow, int TextSize);
static void FillTextArray (TextType *TextObject, EntryFieldType *EntryField, sfColor *TextColor, sfFont *TextFont, WindowType *MainWindow, int TextSize);
static void DrawWindow(EntryFieldType *EntryField, SwitchButtonType *PasswordArgButton, TextType *TextObject, WindowType *MainWindow, int ReportActive);
static void PutColor(float R, float G, float B, Color_RGB *Color);
static int DrawReportWindow(WindowType *MainWindow, char *Report, sfColor *TextColor, sfFont *TextFont, sfEvent *ReportEvent, TextType *TextReport);
static int Createdb(EntryFieldType *EntryField, TextType *TextReport);
static void FillSwitchButtonArray(SwitchButtonType *PasswordArgButton, int Width, int Height, sfColor *TextColor, sfFont *TextFont, WindowType *MainWindow);


	const char *progname;
	int			optindex;
	int			c;

	const char *dbname = NULL;
	const char *maintenance_db = NULL;
	//char	   *comment = NULL;
	char	   *host = NULL;
	char	   *port = NULL;
	char	   *username = NULL;
	enum trivalue prompt_password = TRI_DEFAULT;
	ConnParams	cparams;
	bool		echo = false;
	char	   *owner = NULL;
	char	   *tablespace = NULL;
	char	   *template = NULL;
	char	   *encoding = NULL;
	char	   *strategy = NULL;
	char	   *lc_collate = NULL;
	char	   *lc_ctype = NULL;
	char	   *locale = NULL;
	char	   *locale_provider = NULL;
	char	   *icu_locale = NULL;

	PQExpBufferData sql;

	PGconn	   *conn;
	PGresult   *result;	

	static struct option long_options[] = {
		{"host", required_argument, NULL, 'h'},
		{"port", required_argument, NULL, 'p'},
		{"username", required_argument, NULL, 'U'},
		{"no-password", no_argument, NULL, 'w'},
		{"password", no_argument, NULL, 'W'},
		{"echo", no_argument, NULL, 'e'},
		{"owner", required_argument, NULL, 'O'},
		{"tablespace", required_argument, NULL, 'D'},
		{"template", required_argument, NULL, 'T'},
		{"encoding", required_argument, NULL, 'E'},
		{"strategy", required_argument, NULL, 'S'},
		{"lc-collate", required_argument, NULL, 1},
		{"lc-ctype", required_argument, NULL, 2},
		{"locale", required_argument, NULL, 'l'},
		{"maintenance-db", required_argument, NULL, 3},
		{"locale-provider", required_argument, NULL, 4},
		{"icu-locale", required_argument, NULL, 5},
		{NULL, 0, NULL, 0}
	};
int
main(int argc, char *argv[])
{
	//Graphics//

	//window declarations
		//Main window
	WindowType MainWindow;

	//SFML declarations
	sfEvent event;
	sfEvent Reportevent;
	sfFont *TextFont;							
	sfColor TextColor = {30, 30, 30, 255};

	//Button declarations
	SwitchButtonType PasswordArgButton[SWITCHBUTTONNUMBER];
	int PasswordArgButton_Width = 150;
	int PasswordArgButton_Height = 25;

	//Entry field declarations
	int EntryField_Width = 550;
	int EntryField_Height = 25;
	EntryFieldType EntryField[ENTRYFIELDNUMBER];

	//Text declarations
	TextType TextObject[TEXTOBJECTNUMBER];
	TextType TextReport[2];
	int TextSize;

	//Mouse declarations
	MouseMoveType MouseMove;

	//utility declarations
	ArgEnum ActiveEntryField = -1;		// change? enum
	ArgEnum LastActiveEntryField = -1;
	//ArgEnum ActiveEntryField_
	//ArgEnum
	ArgEnum InputMode = -1;	
	ArgEnum LastInputMode = -1;
	bool running = 1;	
	bool ReportActive = 0;


	//Fill in window data
		//Main window
	MainWindow.Width = 600;		// Coor x from -250 to 250			// change? uint8
	MainWindow.Height = 900;		// Coor y from -400 to 400			// change? uint8
	MainWindow.MaxCoorX = MainWindow.Width/2;
	MainWindow.MaxCoorY = MainWindow.Height/2;
	strcpy(MainWindow.Name, "Create data base V"PG_VERSION);
	MainWindow.mode.width = MainWindow.Width;
	MainWindow.mode.height = MainWindow.Height;
	MainWindow.mode.bitsPerPixel = 32; 

	TextSize = EntryField_Height;

	//Fill in SFML data 											       //if (!Font) printf("Error!!!\n");
	TextFont = sfFont_createFromFile("/home/o4ina/gitpostgresql/postgres/src/bin/scripts/Mariupol-Regular.ttf");

	//Fill in button data
	PasswordArgButton[0].CoorX = -200;
	PasswordArgButton[0].CoorY = -400;
	PasswordArgButton[0].Active = 0;

	PasswordArgButton[1].CoorX = 200;
	PasswordArgButton[1].CoorY = -400;
	PasswordArgButton[1].Active = 0;
	FillSwitchButtonArray(PasswordArgButton, PasswordArgButton_Width, PasswordArgButton_Height, &TextColor, TextFont, &MainWindow);


	//Fill in Entry Fields data 
	for (int i = 0, StartCoorY = 410; i < ENTRYFIELDNUMBER; ++i)
	{
		EntryField[i].CoorX = 0;
		EntryField[i].CoorY = StartCoorY;

		StartCoorY -= 50;
	}
	FillEntryFieldArray(EntryField, EntryField_Width, EntryField_Height, &TextColor, TextFont, &MainWindow, TextSize);

	//Fill in Text Object data
	FillTextArray(TextObject, EntryField, &TextColor, TextFont, &MainWindow, TextSize);

	//Fill in Mouse data
	MouseMove.CoorX = 0;
	MouseMove.CoorY = 0;


	//create window
	MainWindow.Window = sfRenderWindow_create(MainWindow.mode, MainWindow.Name, sfDefaultStyle, NULL);	//added




	while(running)  //running
	{
		if (1)
		{
			while (sfRenderWindow_pollEvent(MainWindow.Window, &event)) 
			{
				if (event.type == sfEvtMouseButtonPressed) //Mouse button pressed
				{
					if (InputMode == -1) // InputMode off
					{
						InputMode = MouseAndEntryFieldInteraction(EntryField, ENTRYFIELDNUMBER, &MouseMove);
						LastInputMode = InputMode; 
						if (MouseMove.CoorX >= PasswordArgButton[1].Borders_CoorX_Min && 
							MouseMove.CoorX <= PasswordArgButton[1].Borders_CoorX_Max && 
							MouseMove.CoorY >= PasswordArgButton[1].Borders_CoorY_Min &&  
							MouseMove.CoorY <= PasswordArgButton[1].Borders_CoorY_Max) 
						{
							ReportActive = 1;
							DrawWindow(EntryField, PasswordArgButton, TextObject, &MainWindow, ReportActive);
							if (Createdb(EntryField, TextReport))
							{
								if (DrawReportWindow(&MainWindow, "DATABASE CREATE", &TextColor, TextFont, &Reportevent, TextReport))
									ReportActive = 0;
								else
									running = 0;
							}
							else
							{
								if (DrawReportWindow(&MainWindow, "ERROR", &TextColor, TextFont, &Reportevent, TextReport))
									ReportActive = 0;
								else
									running = 0;	
							}
						}
					}
					else // InputMode on
					{
						InputMode = MouseAndEntryFieldInteraction(EntryField, ENTRYFIELDNUMBER, &MouseMove);
						if (InputMode != -1 && LastInputMode != InputMode)
						{
							PutColor(0.85, 0.85, 0.85, &EntryField[LastInputMode].Color);

							PutColor(0.9, 0.9, 0.9, &EntryField[InputMode].Color);
							LastInputMode = InputMode;
							break;
						} 
						else if (InputMode == -1)
						{	
							PutColor(0.85, 0.85, 0.85, &EntryField[LastInputMode].Color);
							break;
						}
					}
					
					if (MouseMove.CoorX >= PasswordArgButton[0].Borders_CoorX_Min && 
						MouseMove.CoorX <= PasswordArgButton[0].Borders_CoorX_Max && 
						MouseMove.CoorY >= PasswordArgButton[0].Borders_CoorY_Min &&  
						MouseMove.CoorY <= PasswordArgButton[0].Borders_CoorY_Max) 
					{
						if (PasswordArgButton[0].Active)
						{
							PasswordArgButton[0].Active = 0;
							prompt_password = TRI_NO;
							PutColor(0.80, 0.50, 0.50, &PasswordArgButton[0].Color);
							sfText_setString(PasswordArgButton[0].Text, PasswordArgButton[0].TextStr_False);
						}
						else 
						{
							PasswordArgButton[0].Active = 1;
							prompt_password = TRI_YES;
							PutColor(0.50, 0.80, 0.50, &PasswordArgButton[0].Color);
							sfText_setString(PasswordArgButton[0].Text, PasswordArgButton[0].TextStr_True);
						}

					}
				}
				else if (event.type == sfEvtMouseMoved) //Mouse moved
				{
					if (InputMode == -1) // InputMode off
					{
						MouseMove.CoorX = event.mouseMove.x - MainWindow.MaxCoorX;
						MouseMove.CoorY = -event.mouseMove.y + MainWindow.MaxCoorY;

						ActiveEntryField = MouseAndEntryFieldInteraction(EntryField, ENTRYFIELDNUMBER, &MouseMove);
						if (ActiveEntryField != LastActiveEntryField)
						{
							PutColor(0.85, 0.85, 0.85, &EntryField[LastActiveEntryField].Color);	
						}
						else
						{
							PutColor(0.9, 0.9, 0.9, &EntryField[ActiveEntryField].Color);
						}
						LastActiveEntryField = ActiveEntryField;
					}
					else // InputMode on
					{
						//LastActiveEntryField = -1;
						MouseMove.CoorX = event.mouseMove.x - MainWindow.MaxCoorX;
						MouseMove.CoorY = -event.mouseMove.y + MainWindow.MaxCoorY;
					}
				}
				else if (event.type == sfEvtTextEntered && InputMode != -1) // InputMode on
				{
					if (event.text.unicode == 8 && EntryField[InputMode].NumberSymbol != 0)
					{
						EntryField[InputMode].TextStr[EntryField[InputMode].NumberSymbol-1] = '\0';
						EntryField[InputMode].NumberSymbol--;
					}
					else if (EntryField[InputMode].NumberSymbol != CHARININENTRYFIELD - 2 && event.text.unicode != 8) 
					{ 
						EntryField[InputMode].TextStr[EntryField[InputMode].NumberSymbol] = event.text.unicode;
						EntryField[InputMode].TextStr[EntryField[InputMode].NumberSymbol + 1] = '\0';
						EntryField[InputMode].NumberSymbol++;
					}
					sfText_setString(EntryField[InputMode].Text, EntryField[InputMode].TextStr);
				} 
				else if (event.type == sfEvtClosed) // closed window
				{
					sfRenderWindow_close(MainWindow.Window);
					running = 0;
				} 
				
			}
		}
		
		//DrawWindow
		DrawWindow(EntryField, PasswordArgButton, TextObject, &MainWindow, ReportActive);      //sfRenderWindow_destroy(MainWindow.Window);
		
	}

}


static int Createdb(EntryFieldType *EntryField, TextType *TextReport)
{
	pg_logging_init("createdb");
	progname = get_progname("createdb");
	set_pglocale_pgservice("createdb", PG_TEXTDOMAIN("pgscripts"));
	
	dbname = NULL;
	tablespace = NULL;
	encoding = NULL;
	locale = NULL;
	lc_ctype = NULL;
	lc_collate = NULL;
	icu_locale = NULL;
	locale_provider = NULL;
	owner = NULL;
	strategy = NULL;
	template = NULL;
	host = NULL;
	port = NULL;
	username = NULL;
	maintenance_db = NULL;

	if (EntryField[DBName_Arg].TextStr[0] != '\0')
		dbname = EntryField[DBName_Arg].TextStr;
	if (EntryField[tablespace_Arg].TextStr[0] != '\0')
		tablespace = EntryField[tablespace_Arg].TextStr;
	if (EntryField[encoding_Arg].TextStr[0] != '\0')
		encoding = EntryField[encoding_Arg].TextStr;
	if (EntryField[locale_Arg].TextStr[0] != '\0')
		locale = EntryField[locale_Arg].TextStr;
	if (EntryField[lc_ctype_Arg].TextStr[0] != '\0')
		lc_ctype = EntryField[lc_ctype_Arg].TextStr;
	if (EntryField[lc_collate_Arg].TextStr[0] != '\0')
		lc_collate = EntryField[lc_collate_Arg].TextStr;
	if (EntryField[icu_locale_Arg].TextStr[0] != '\0')
		icu_locale = EntryField[icu_locale_Arg].TextStr;
	if (EntryField[locale_provider_Arg].TextStr[0] != '\0')
		locale_provider = EntryField[locale_provider_Arg].TextStr;
	if (EntryField[owner_Arg].TextStr[0] != '\0')
		owner = EntryField[owner_Arg].TextStr;
	if (EntryField[strategy_Arg].TextStr[0] != '\0')
		strategy = EntryField[strategy_Arg].TextStr;
	if (EntryField[template_Arg].TextStr[0] != '\0')
		template = EntryField[template_Arg].TextStr;
	if (EntryField[host_Arg].TextStr[0] != '\0')
		host = EntryField[host_Arg].TextStr;
	if (EntryField[port_Arg].TextStr[0] != '\0')
		port = EntryField[port_Arg].TextStr;
	if (EntryField[username_Arg].TextStr[0] != '\0')
		username = EntryField[username_Arg].TextStr;
	if (EntryField[maintenance_Arg].TextStr[0] != '\0')
		maintenance_db = EntryField[maintenance_Arg].TextStr;



	if (locale)
	{
		if (lc_ctype)
		{
			strcpy(TextReport[0].TextStr, "only one of --locale and --lc-ctype can be specified");
			return 0;
		}
		if (lc_collate)
		{
			strcpy(TextReport[0].TextStr, "only one of --locale and --lc-collate can be specified");
			return 0;
		}
		lc_ctype = locale;
		lc_collate = locale;
	}

	if (encoding)
	{
		if (pg_char_to_encoding(encoding) < 0)
		{
			strcpy(TextReport[0].TextStr, "is not a valid encoding name"); //encoding
			return 0;
		}
	}

	if (dbname == NULL)
	{
		if (getenv("PGDATABASE"))
			dbname = getenv("PGDATABASE");
		else if (getenv("PGUSER"))
			dbname = getenv("PGUSER");
		else
			dbname = get_user_name_or_exit(progname);

	}

		/* No point in trying to use postgres db when creating postgres db. */
	if (EntryField[maintenance_Arg].TextStr[0] != '\0' && strcmp(dbname, "postgres") == 0)
		maintenance_db = "template1";

	if (EntryField[maintenance_Arg].TextStr[0] == '\0')
		cparams.dbname = NULL;
	else
		cparams.dbname = maintenance_db;        

	if (EntryField[maintenance_Arg].TextStr[0] == '\0')
		cparams.pghost = NULL;
	else
		cparams.pghost = host;

	if (EntryField[maintenance_Arg].TextStr[0] == '\0')
		cparams.pgport = NULL;
	else
		cparams.pgport = port;

	if (EntryField[maintenance_Arg].TextStr[0] == '\0')
		cparams.pguser = NULL;
	else
		cparams.pguser = username;

	cparams.prompt_password = prompt_password;
	cparams.override_dbname = NULL;



	conn = connectMaintenanceDatabase(&cparams, progname, echo);


	initPQExpBuffer(&sql);

	appendPQExpBuffer(&sql, "CREATE DATABASE %s",
		fmtId(dbname));

	if (EntryField[owner_Arg].TextStr[0] != '\0')
		appendPQExpBuffer(&sql, " OWNER %s", fmtId(owner));
	if (EntryField[tablespace_Arg].TextStr[0] != '\0')
		appendPQExpBuffer(&sql, " TABLESPACE %s", fmtId(tablespace));
	if (EntryField[encoding_Arg].TextStr[0] != '\0')
	{
		appendPQExpBufferStr(&sql, " ENCODING ");
		appendStringLiteralConn(&sql, encoding, conn);
	}
	if (EntryField[strategy_Arg].TextStr[0] != '\0')
		appendPQExpBuffer(&sql, " STRATEGY %s", fmtId(strategy));
	if (EntryField[template_Arg].TextStr[0] != '\0')
		appendPQExpBuffer(&sql, " TEMPLATE %s", fmtId(template));
	if (EntryField[lc_collate_Arg].TextStr[0] != '\0')
	{
		appendPQExpBufferStr(&sql, " LC_COLLATE ");
		appendStringLiteralConn(&sql, lc_collate, conn);
	}
	if (EntryField[lc_ctype_Arg].TextStr[0] != '\0')
	{
		appendPQExpBufferStr(&sql, " LC_CTYPE ");
		appendStringLiteralConn(&sql, lc_ctype, conn);
	}
	if (EntryField[locale_provider_Arg].TextStr[0] != '\0')
		appendPQExpBuffer(&sql, " LOCALE_PROVIDER %s", locale_provider);
	if (EntryField[icu_locale_Arg].TextStr[0] != '\0')
	{
		appendPQExpBufferStr(&sql, " ICU_LOCALE ");
		appendStringLiteralConn(&sql, icu_locale, conn);
	}

	appendPQExpBufferChar(&sql, ';');

	if (echo)
		printf("%s\n", sql.data);
	result = PQexec(conn, sql.data);


	if (PQresultStatus(result) != PGRES_COMMAND_OK)
	{
		strcpy(TextReport[0].TextStr, PQerrorMessage(conn)); //encoding
		pg_log_error("database creation failed: %s", PQerrorMessage(conn));
		PQfinish(conn);
		return 0;
	}
	
	PQclear(result);

	PQfinish(conn);

	return 1;

}

static void DrawWindow(EntryFieldType *EntryField, SwitchButtonType *PasswordArgButton, TextType *TextObject, WindowType *MainWindow, int ReportActive)
{
	sfRenderWindow_clear(MainWindow->Window, sfTransparent);

	glClearColor(0.95, 0.95, 0.95, 0.0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBegin(GL_QUADS);
		glColor3f(1, 0.95, 0.95);
		glVertex2f(-1,-1);
		glVertex2f(-1,1);
		glColor3f(0.95, 0.95, 0.95);
		glVertex2f(1,1);
		glVertex2f(1,-1);
	glEnd();



	for (int i = 0; i < ENTRYFIELDNUMBER; ++i)
	{
		glBegin(GL_QUADS);
			glColor3f(EntryField[i].Color.R, EntryField[i].Color.G, EntryField[i].Color.B); 
			glVertex2f((float)(EntryField[i].Borders_CoorX_Min) / MainWindow->MaxCoorX, (float)(EntryField[i].Borders_CoorY_Min) / MainWindow->MaxCoorY);      
			glVertex2f((float)(EntryField[i].Borders_CoorX_Min) / MainWindow->MaxCoorX, (float)(EntryField[i].Borders_CoorY_Max) / MainWindow->MaxCoorY); 
			glColor3f(EntryField[i].Color.R + 0.05, EntryField[i].Color.G, EntryField[i].Color.B + 0.05); 
			glVertex2f((float)(EntryField[i].Borders_CoorX_Max) / MainWindow->MaxCoorX, (float)(EntryField[i].Borders_CoorY_Max) / MainWindow->MaxCoorY); 
			glVertex2f((float)(EntryField[i].Borders_CoorX_Max) / MainWindow->MaxCoorX, (float)(EntryField[i].Borders_CoorY_Min) / MainWindow->MaxCoorY); 
		glEnd();

	}

	for (int i = 0; i < SWITCHBUTTONNUMBER; ++i)
	{
		glBegin(GL_QUADS);
			glColor3f(PasswordArgButton[i].Color.R, PasswordArgButton[i].Color.G, PasswordArgButton[i].Color.B); 
			glVertex2f((float)(PasswordArgButton[i].Borders_CoorX_Min) / MainWindow->MaxCoorX, (float)(PasswordArgButton[i].Borders_CoorY_Min) / MainWindow->MaxCoorY);      
			glVertex2f((float)(PasswordArgButton[i].Borders_CoorX_Min) / MainWindow->MaxCoorX, (float)(PasswordArgButton[i].Borders_CoorY_Max) / MainWindow->MaxCoorY); 
			glColor3f(PasswordArgButton[i].Color.R + 0.05, PasswordArgButton[i].Color.G, PasswordArgButton[i].Color.B + 0.05); 
			glVertex2f((float)(PasswordArgButton[i].Borders_CoorX_Max) / MainWindow->MaxCoorX, (float)(PasswordArgButton[i].Borders_CoorY_Max) / MainWindow->MaxCoorY); 
			glVertex2f((float)(PasswordArgButton[i].Borders_CoorX_Max) / MainWindow->MaxCoorX, (float)(PasswordArgButton[i].Borders_CoorY_Min) / MainWindow->MaxCoorY); 
		glEnd();
	}


	sfRenderWindow_pushGLStates(MainWindow->Window); 
	
	for (int i = 0; i < ENTRYFIELDNUMBER; ++i)
	{
		sfRenderWindow_drawText(MainWindow->Window, EntryField[i].Text, NULL);
	}

	for (int i = 0; i < SWITCHBUTTONNUMBER; ++i)
	{
		sfRenderWindow_drawText(MainWindow->Window, PasswordArgButton[i].Text, NULL);
	}

	for (int i = 0; i < TEXTOBJECTNUMBER; ++i)
	{
		sfRenderWindow_drawText(MainWindow->Window, TextObject[i].Text, NULL);
	} 

	sfRenderWindow_popGLStates(MainWindow->Window);

	if (!ReportActive) // ReportActive
	{
		sfRenderWindow_display(MainWindow->Window);
	}

}

// If the mouse is in EntryField then return EntryFieldID else return -1
int MouseAndEntryFieldInteraction(EntryFieldType *EntryField, int EntryFieldNumber, MouseMoveType *MouseMove) {
	for (register int i = 0; i < EntryFieldNumber; ++i) {
		if (MouseMove->CoorX >= EntryField[i].Borders_CoorX_Min && 
			MouseMove->CoorX <= EntryField[i].Borders_CoorX_Max && 
			MouseMove->CoorY >= EntryField[i].Borders_CoorY_Min &&  
			MouseMove->CoorY <= EntryField[i].Borders_CoorY_Max) 
		{
			return i;
		}
	}
	return -1;
}

void FillEntryFieldArray(EntryFieldType *EntryField, int Width, int Height, sfColor *TextColor, sfFont *TextFont, WindowType *MainWindow, int TextSize) 
{
	short Borders_CoorX_Min = -Width/2;
	short Borders_CoorX_Max = Width/2;
	short Borders_CoorY_Min = -Height/2;
	short Borders_CoorY_Max = Height/2;

	for (register int i = 0; i < ENTRYFIELDNUMBER; ++i)
	{
		EntryField[i].Borders_CoorX_Min = Borders_CoorX_Min + EntryField[i].CoorX; 
		EntryField[i].Borders_CoorX_Max = Borders_CoorX_Max + EntryField[i].CoorX;
		EntryField[i].Borders_CoorY_Min = Borders_CoorY_Min + EntryField[i].CoorY;
		EntryField[i].Borders_CoorY_Max = Borders_CoorY_Max + EntryField[i].CoorY;

		EntryField[i].position.x = EntryField[i].Borders_CoorX_Min + MainWindow->MaxCoorX;
		EntryField[i].position.y = -EntryField[i].Borders_CoorY_Max + MainWindow->MaxCoorY - TextSize/4;  //(-EntryField[i].Borders_CoorY_Min - CharacterSize/2) + MainWindow->MaxCoorY - Height/2 /*- CharacterSize/2 /*- Height/4*/;

		PutColor(0.85, 0.85, 0.85, &EntryField[i].Color);

		EntryField[i].NumberSymbol = 0;

		EntryField[i].TextStr[0] = '\0';

		EntryField[i].Text = sfText_create();
		sfText_setPosition(EntryField[i].Text,EntryField[i].position);
		sfText_setFillColor(EntryField[i].Text, *TextColor);
		sfText_setFont(EntryField[i].Text, TextFont);
		sfText_setCharacterSize(EntryField[i].Text, TextSize);
	}
}

void FillTextArray (TextType *TextObject, EntryFieldType *EntryField, sfColor *TextColor, sfFont *TextFont, WindowType *MainWindow, int TextSize)
{
	TextSize = 22;
	for (int i = 0; i < TEXTOBJECTNUMBER; ++i) 
	{
		TextObject[i].position.x = EntryField[i].Borders_CoorX_Min + MainWindow->MaxCoorX;
		TextObject[i].position.y = -EntryField[i].Borders_CoorY_Max + MainWindow->MaxCoorY - TextSize/4 -TextSize;

		TextObject[i].Text = sfText_create();
		sfText_setPosition(TextObject[i].Text, TextObject[i].position);
		sfText_setFillColor(TextObject[i].Text, *TextColor);
		sfText_setFont(TextObject[i].Text, TextFont);
		sfText_setCharacterSize(TextObject[i].Text, TextSize);
	}
	sfText_setString(TextObject[DBName_Arg].Text, "Database name");
	sfText_setString(TextObject[tablespace_Arg].Text, "Default tablespace for the database");
	sfText_setString(TextObject[encoding_Arg].Text, "Encoding for the database");
	sfText_setString(TextObject[locale_Arg].Text, "Locale settings for the database");
	sfText_setString(TextObject[lc_collate_Arg].Text, "LC_COLLATE setting for the database");
	sfText_setString(TextObject[lc_ctype_Arg].Text, "LC_CTYPE setting for the database");
	sfText_setString(TextObject[icu_locale_Arg].Text, "ICU locale setting for the database");
	sfText_setString(TextObject[locale_provider_Arg].Text, "Locale provider for the database's default collation");
	sfText_setString(TextObject[owner_Arg].Text, "Database user to own the new database");
	sfText_setString(TextObject[strategy_Arg].Text, "Database creation strategy wal_log or file_copy");
	sfText_setString(TextObject[template_Arg].Text, "Template database to copy");
	sfText_setString(TextObject[host_Arg].Text, "Database server host or socket directory");
	sfText_setString(TextObject[port_Arg].Text, "Database server port");
	sfText_setString(TextObject[username_Arg].Text, "User name to connect as");
	sfText_setString(TextObject[maintenance_Arg].Text, "Alternate maintenance database");
}

static void FillSwitchButtonArray(SwitchButtonType *PasswordArgButton, int Width, int Height, sfColor *TextColor, sfFont *TextFont, WindowType *MainWindow)
{
	int Borders_CoorX_Min = -Width/2;
	int Borders_CoorX_Max = Width/2;
	int Borders_CoorY_Min = -Height/2;
	int Borders_CoorY_Max = Height/2;

	for (register int i = 0; i < SWITCHBUTTONNUMBER; ++i)
	{
		PasswordArgButton[i].Borders_CoorX_Min = Borders_CoorX_Min + PasswordArgButton[i].CoorX; 
		PasswordArgButton[i].Borders_CoorX_Max = Borders_CoorX_Max + PasswordArgButton[i].CoorX;
		PasswordArgButton[i].Borders_CoorY_Min = Borders_CoorY_Min + PasswordArgButton[i].CoorY;
		PasswordArgButton[i].Borders_CoorY_Max = Borders_CoorY_Max + PasswordArgButton[i].CoorY;

		PasswordArgButton[i].position.x = PasswordArgButton[i].Borders_CoorX_Min + MainWindow->MaxCoorX;
		PasswordArgButton[i].position.y = -PasswordArgButton[i].Borders_CoorY_Max + MainWindow->MaxCoorY - Height/4;//textsize  //(-EntryField[i].Borders_CoorY_Min - CharacterSize/2) + MainWindow->MaxCoorY - Height/2 /*- CharacterSize/2 /*- Height/4*/;

		PasswordArgButton[i].Text = sfText_create();
		sfText_setPosition(PasswordArgButton[i].Text,PasswordArgButton[i].position);
		sfText_setFillColor(PasswordArgButton[i].Text, *TextColor);
		sfText_setFont(PasswordArgButton[i].Text, TextFont);
		sfText_setCharacterSize(PasswordArgButton[i].Text, Height);//textsize
	}

		strcpy(PasswordArgButton[0].TextStr_True, "password");
		strcpy(PasswordArgButton[0].TextStr_False, "no-password");
		PutColor(0.80, 0.50, 0.50, &PasswordArgButton[0].Color);
		sfText_setString(PasswordArgButton[0].Text, PasswordArgButton[0].TextStr_False);

		strcpy(PasswordArgButton[1].TextStr_True, "Create DB");
		strcpy(PasswordArgButton[1].TextStr_False, "Create DB");
		PutColor(0.50, 0.50, 0.50, &PasswordArgButton[1].Color);
		sfText_setString(PasswordArgButton[1].Text, PasswordArgButton[1].TextStr_False);
}

static void PutColor(float R, float G, float B, Color_RGB *Color) 
{
	Color->R = R;
	Color->G = G;
	Color->B = B;
}

static int DrawReportWindow(WindowType *MainWindow, char *Report, sfColor *TextColor, sfFont *TextFont, sfEvent *ReportEvent, TextType *TextReport) 
{
	TextReport[0].Text = sfText_create();
	TextReport[1].Text = sfText_create();

	TextReport[0].position.x = MainWindow->Width/9;
	TextReport[0].position.y = MainWindow->Height/3;
	sfText_setPosition(TextReport[0].Text, TextReport[0].position);
	sfText_setFillColor(TextReport[0].Text, *TextColor);
	sfText_setFont(TextReport[0].Text, TextFont);
	sfText_setCharacterSize(TextReport[0].Text, 15);
	sfText_setString(TextReport[0].Text, TextReport[0].TextStr); //TextReport[0].TextStr

	TextReport[1].position.x = MainWindow->Width/2 - 170;
	TextReport[1].position.y = MainWindow->Width/2 - 50;
	sfText_setPosition(TextReport[1].Text, TextReport[1].position);
	sfText_setFillColor(TextReport[1].Text, *TextColor);
	sfText_setFont(TextReport[1].Text, TextFont);
	sfText_setCharacterSize(TextReport[1].Text, 30);
	sfText_setString(TextReport[1].Text, Report);

	//glClearColor(0.95, 0.95, 0.95, 0.0);

	//sfRenderWindow_clear(ReportWindow->Window, sfTransparent);

	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glBegin(GL_QUADS);
		glColor3f(0.8, 0.8, 0.8); // glColor3f(1, 0.95, 0.95);
			glVertex2f(-500.0/MainWindow->Width,-500.0/MainWindow->Height);
			glVertex2f(-500.0/MainWindow->Width,500.0/MainWindow->Height);
		glColor3f(0.83, 0.83, 0.83);
			glVertex2f(500.0/MainWindow->Width,500.0/MainWindow->Height);
			glVertex2f(500.0/MainWindow->Width,-500.0/MainWindow->Height);
	glEnd();

	sfRenderWindow_pushGLStates(MainWindow->Window); 

	sfRenderWindow_drawText(MainWindow->Window, TextReport[0].Text, NULL);
	sfRenderWindow_drawText(MainWindow->Window, TextReport[1].Text, NULL);

	sfRenderWindow_popGLStates(MainWindow->Window);

	sfRenderWindow_display(MainWindow->Window);

	TextReport[0].TextStr[0] = '\0';

	while (1) 
	{
		while(sfRenderWindow_pollEvent(MainWindow->Window, ReportEvent))
		{

		if (ReportEvent->type == sfEvtMouseButtonPressed)
		{
			return 1;
		}
		else if (ReportEvent->type == sfEvtClosed) // closed window
		{		
			sfRenderWindow_close(MainWindow->Window);
			return 0;
		}

		}
	}





}


