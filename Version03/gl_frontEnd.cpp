//
//  gl_frontEnd.c
//  GL threads
//
//  C version created by Jean-Yves Hervé on 2017-04-24.
//	Revised C++ version to C++ 2020-11-06

#include <iostream>
#include <cstring>
#include <cstdlib>
//
#include "gl_frontEnd.h"

using namespace std;

//---------------------------------------------------------------------------
//  Private functions' prototypes
//---------------------------------------------------------------------------

void myResize(int w, int h);
void displayTextualInfo(const char* infoStr, int x, int y, bool isLarge);
void myMouse(int b, int s, int x, int y);
void myGridPaneMouse(int b, int s, int x, int y);
void myStatePaneMouse(int b, int s, int x, int y);
void myMenuHandler(int value);
void mySubmenuHandler(int colorIndex);
void* threadFunc(void*);

//---------------------------------------------------------------------------
//  Defined in main.cpp --> don't touch
//---------------------------------------------------------------------------

extern const int MAX_NUM_THREADS;
extern unsigned int rule;
extern unsigned int colorMode;


//---------------------------------------------------------------------------
//  Interface constants
//---------------------------------------------------------------------------

//	I like to setup my meny item indices as enumerated values, but really
//	regular int constants would do the job just fine.

enum MenuItemID {	SEPARATOR = -1,
					//
					QUIT_MENU = 0,
					OTHER_MENU_ITEM
};

const char* MAIN_MENU_ITEM_STR[] = {	"Quit",			//	QUIT_MENU
										"Something"		//	OTHER_MENU_ITEM
};

enum FirstSubmenuItemID {	FIRST_SUBMENU_ITEM = 11,
							SECOND_SUBMENU_ITEM
};

#define SMALL_DISPLAY_FONT    GLUT_BITMAP_HELVETICA_12
#define LARGE_DISPLAY_FONT    GLUT_BITMAP_HELVETICA_18
const int SMALL_FONT_HEIGHT = 12;
const int LARGE_FONT_HEIGHT = 18;
const int TEXT_PADDING = 0;
const float kTextColor[4] = {1.f, 1.f, 1.f, 1.f};

//	Predefine some colors for "age"-based rendering of the cells
GLfloat cellColor[NB_COLORS][4] = {	{0.f, 0.f, 0.f, 1.f},	//	BLACK_COL
									{1.f, 1.f, 1.f, 1.f},	//	WHITE_COL,
									{0.f, 0.f, 1.f, 1.f},	//	BLUE_COL,
									{0.f, 1.f, 0.f, 1.f},	//	GREEN_COL,
									{1.f, 1.f, 0.f, 1.f},	//	YELLOW_COL,
									{1.f, 0.f, 0.f, 1.f}};	//	RED_COL
	

//	Initial position of the window
const int   INIT_WIN_X = 100,
            INIT_WIN_Y = 40;

//	Wow!  g++ on Linux is really dumb!  The code below doesn't even compile there.
//const int	GRID_PANE_WIDTH = 600,
//			GRID_PANE_HEIGHT = GRID_PANE_WIDTH,	//	--> claims GRID_PANE_WIDTH not constant!
//			STATE_PANE_WIDTH = 300,
//			STATE_PANE_HEIGHT = GRID_PANE_HEIGHT,
//			H_PADDING = 5,
//			WINDOW_WIDTH = GRID_PANE_WIDTH + STATE_PANE_WIDTH + H_PADDING,
//			WINDOW_HEIGHT = GRID_PANE_HEIGHT;
//	(sigh!)	This completely negates the point of using constants for this kind of setup.
//	No wonder most Linux apps suck so much, interface-wise.
const int GRID_PANE_WIDTH = 800;
const int GRID_PANE_HEIGHT = 700;
const int STATE_PANE_WIDTH = 300;
const int STATE_PANE_HEIGHT = 700;
const int H_PADDING = 0;
const int WINDOW_WIDTH = 1100;
const int WINDOW_HEIGHT = 700;


//---------------------------------------------------------------------------
//  File-level global variables.  Don't touch
//---------------------------------------------------------------------------

void (*gridDisplayFunc)(void);
void (*stateDisplayFunc)(void);

int	gMainWindow,
	gSubwindow[2];

bool drawGridLines = false;
const bool useLargeTextSize = true;


//---------------------------------------------------------------------------
//	Drawing functions
//---------------------------------------------------------------------------


//	This is the function that does the actual grid drawing
void drawGrid(unsigned int** grid, unsigned short numRows, unsigned short numCols)
{
	const float	DH = (1.f * GRID_PANE_WIDTH) / numCols,
				DV = (1.f * GRID_PANE_HEIGHT) / numRows;
	
	//	Display the grid as a series of quad strips
	for (unsigned short i=0; i<numRows; i++)
	{
		glBegin(GL_QUAD_STRIP);
			for (unsigned short j=0; j<numCols; j++)
			{
				
				glColor4fv(cellColor[grid[i][j]]);

				glVertex2f(j*DH, i*DV);
				glVertex2f(j*DH, (i+1)*DV);
				glVertex2f((j+1)*DH, i*DV);
				glVertex2f((j+1)*DH, (i+1)*DV);
			}
		glEnd();
	}

	if (drawGridLines)
	{
		//	Then draw a grid of lines on top of the squares
		glColor4f(0.5f, 0.5f, 0.5f, 1.f);
		glBegin(GL_LINES);
			//	Horizontal
			for (unsigned short i=0; i<= numRows; i++)
			{
				glVertex2f(0, i*DV);
				glVertex2f(GRID_PANE_WIDTH, i*DV);
			}
			//	Vertical
			for (unsigned short j=0; j<= numCols; j++)
			{
				glVertex2f(j*DH, 0);
				glVertex2f(j*DH, GRID_PANE_HEIGHT);
			}
		glEnd();
	}
}


void displayTextualInfo(const char* infoStr, int xPos, int yPos, bool isLarge)
{
	//-----------------------------------------------
    //  0.  get current material properties
    //-----------------------------------------------
    float oldAmb[4], oldDif[4], oldSpec[4], oldShiny;
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, oldAmb);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, oldDif);
    glGetMaterialfv(GL_FRONT, GL_SPECULAR, oldSpec);
    glGetMaterialfv(GL_FRONT, GL_SHININESS, &oldShiny);

    glPushMatrix();

    //-----------------------------------------------
    //  1.  Build the string to display <-- parameter
    //-----------------------------------------------
    unsigned int infoLn = (unsigned int) strlen(infoStr);

    //-----------------------------------------------
    //  2.  Determine the string's length (in pixels)
    //-----------------------------------------------
    int textWidth = 0;
    for (unsigned short k=0; k<infoLn; k++)
	{
		if (isLarge)
			textWidth += glutBitmapWidth(LARGE_DISPLAY_FONT, infoStr[k]);
		else
			textWidth += glutBitmapWidth(SMALL_DISPLAY_FONT, infoStr[k]);
		
    }
	//  add a few pixels of padding
    textWidth += 2*TEXT_PADDING;
	
    //-----------------------------------------------
    //  4.  Draw the string
    //-----------------------------------------------    
    glColor4fv(kTextColor);
    int x = xPos;
    for (unsigned short k=0; k<infoLn; k++)
    {
        glRasterPos2i(x, yPos);
		if (isLarge)
		{
			glutBitmapCharacter(LARGE_DISPLAY_FONT, infoStr[k]);
			x += glutBitmapWidth(LARGE_DISPLAY_FONT, infoStr[k]);
		}
		else
		{
			glutBitmapCharacter(SMALL_DISPLAY_FONT, infoStr[k]);
			x += glutBitmapWidth(SMALL_DISPLAY_FONT, infoStr[k]);
		}
	}

    //-----------------------------------------------
    //  5.  Restore old material properties
    //-----------------------------------------------
	glMaterialfv(GL_FRONT, GL_AMBIENT, oldAmb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, oldDif);
	glMaterialfv(GL_FRONT, GL_SPECULAR, oldSpec);
	glMaterialf(GL_FRONT, GL_SHININESS, oldShiny);  
    
    //-----------------------------------------------
    //  6.  Restore reference frame
    //-----------------------------------------------
    glPopMatrix();
}



void drawState(unsigned short numLiveThreads)
{
	const int H_PAD = STATE_PANE_WIDTH / 16;
	const int TOP_LEVEL_TXT_Y = 4*STATE_PANE_HEIGHT / 5;
	const int TEXT_ROW_HEIGHT = useLargeTextSize ? 	2*LARGE_FONT_HEIGHT :
													2*SMALL_FONT_HEIGHT;
	//	Build, then display text info
	char infoStr[256];

	//	update/replace/remove text display
	
	//	Row 0 of text
	//	display info about number of live threads
	sprintf(infoStr, "Live Threads: %d", numLiveThreads);
	displayTextualInfo(infoStr, H_PAD, TOP_LEVEL_TXT_Y + 0*TEXT_ROW_HEIGHT, useLargeTextSize);
	
	//	Row 1 of text
	//	display info about number of live threads
	sprintf(infoStr, "CSC412 - Cellular Automaton");
	displayTextualInfo(infoStr, H_PAD, TOP_LEVEL_TXT_Y - 1*TEXT_ROW_HEIGHT, useLargeTextSize);

	//	Row 2 of text
	sprintf(infoStr, "Something interesting or useful");
	displayTextualInfo(infoStr, H_PAD, TOP_LEVEL_TXT_Y - 2*TEXT_ROW_HEIGHT, useLargeTextSize);

}


//	This callback function is called when the window is resized
//	(generally by the user of the application).
//	It is also called when the window is created, why I placed there the
//	code to set up the virtual camera for this application.
//
void myResize(int w, int h)
{
	if ((w != WINDOW_WIDTH) || (h != WINDOW_HEIGHT))
	{
		glutReshapeWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
	}
	else
	{
		glutPostRedisplay();
	}
}


void myDisplayFunc(void)
{
    glutSetWindow(gMainWindow);

    glMatrixMode(GL_MODELVIEW);
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();

	gridDisplayFunc();
	stateDisplayFunc();
	
    glutSetWindow(gMainWindow);	
}

//	This function is called when a mouse event occurs just in the tiny
//	space between the two subwindows.
//
void myMouse(int button, int state, int x, int y)
{
	(void) button; (void) state; (void) x; (void) y;
	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}

//	This function is called when a mouse event occurs in the grid pane
//
void myGridPaneMouse(int button, int state, int x, int y)
{
	(void) x; (void) y;
	
	switch (button)
	{
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN)
			{
				//	do something
			}
			else if (state == GLUT_UP)
			{
				//	exit(0);
			}
			break;
			
		default:
			break;
	}

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}

//	This function is called when a mouse event occurs in the state pane
void myStatePaneMouse(int button, int state, int x, int y)
{
	(void) x; (void) y;
	
	switch (button)
	{
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN)
			{
				//	do something
			}
			else if (state == GLUT_UP)
			{
				//	exit(0);
			}
			break;
			
		default:
			break;
	}

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}


void myMenuHandler(int choice)
{
	switch (choice)
	{
		//	Exit/Quit
		case QUIT_MENU:
			exit(0);
			break;
		
		//	Do something
		case OTHER_MENU_ITEM:
			break;
		
		default:	//	this should not happen
			break;
	
	}

    glutPostRedisplay();
}

void toggleDrawGridLines(void)
{
	drawGridLines = !drawGridLines;
}


void initializeFrontEnd(int argc, const char** argv, void (*gridDisplayCB)(void),
						void (*stateDisplayCB)(void))
{
	//	Initialize glut and create a new window
	glutInit(&argc, const_cast<char**>(argv));
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);


	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(INIT_WIN_X, INIT_WIN_Y);
	gMainWindow = glutCreateWindow("Programming Assignment 06 -- Cellular Automaton -- CSC 412 - Fall 2020");
	glClearColor(0.2f, 0.2f, 0.2f, 1.f);
	
	//	set up the callbacks for the main window
	glutDisplayFunc(myDisplayFunc);
	glutReshapeFunc(myResize);
	glutMouseFunc(myMouse);
	glutTimerFunc(20, myTimerFunc, 0);

	gridDisplayFunc = gridDisplayCB;
	stateDisplayFunc = stateDisplayCB;
	
	//	create the two panes as glut subwindows
	gSubwindow[GRID_PANE] = glutCreateSubWindow(gMainWindow,
												0, 0,	//	origin
												GRID_PANE_WIDTH, GRID_PANE_HEIGHT);
    glViewport(0, 0, GRID_PANE_WIDTH, GRID_PANE_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrtho(0.0f, GRID_PANE_WIDTH, 0.0f, GRID_PANE_HEIGHT, -1, 1);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glutKeyboardFunc(myKeyboardFunc);
	glutMouseFunc(myGridPaneMouse);
	glutDisplayFunc(gridDisplayCB);
	
	
	glutSetWindow(gMainWindow);
	gSubwindow[STATE_PANE] = glutCreateSubWindow(gMainWindow,
												GRID_PANE_WIDTH + H_PADDING, 0,	//	origin
												STATE_PANE_WIDTH, STATE_PANE_HEIGHT);
    glViewport(0, 0, STATE_PANE_WIDTH, STATE_PANE_WIDTH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrtho(0.0f, STATE_PANE_WIDTH, 0.0f, STATE_PANE_HEIGHT, -1, 1);
	glClearColor(0.f, 0.f, 0.f, 1.f);
	glutKeyboardFunc(myKeyboardFunc);
	glutMouseFunc(myGridPaneMouse);
	glutDisplayFunc(stateDisplayCB);
}
