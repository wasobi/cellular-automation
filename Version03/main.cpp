//
//  main.c
//  Cellular Automaton
//
//  Created by Jean-Yves Hervé on 2018-04-09.
//	C++ version 2020-11-06

 /*-------------------------------------------------------------------------+
 |	A graphic front end for a grid+state simulation.						|
 |																			|
 |	This application simply creates a glut window with a pane to display	|
 |	a colored grid and the other to display some state information.			|
 |	Sets up callback functions to handle menu, mouse and keyboard events.	|
 |	Normally, you shouldn't have to touch anything in this code, unless		|
 |	you want to change some of the things displayed, add menus, etc.		|
 |	Only mess with this after everything else works and making a backup		|
 |	copy of your project.  OpenGL & glut are tricky and it's really easy	|
 |	to break everything with a single line of code.							|
 |																			|
 |	Current keyboard controls:												|
 |																			|
 |		- 'ESC' --> exit the application									|
 |		- space bar --> resets the grid										|
 |																			|
 |		- 'c' --> toggle color mode on/off									|
 |		- 'b' --> toggles color mode off/on									|
 |		- 'l' --> toggles on/off grid line rendering						|
 |																			|
 |		- '+' --> increase simulation speed									|
 |		- '-' --> reduce simulation speed									|
 |																			|
 |		- '1' --> apply Rule 1 (Conway's classical Game of Life: B3/S23)	|
 |		- '2' --> apply Rule 2 (Coral: B3/S45678)							|
 |		- '3' --> apply Rule 3 (Amoeba: B357/S1358)							|
 |		- '4' --> apply Rule 4 (Maze: B3/S12345)							|
 |																			|
 +-------------------------------------------------------------------------*/

#include <iostream>
#include <ctime>
#include <unistd.h>
#include <sstream>
#include <random>
#include <pthread.h>
#include "gl_frontEnd.h"

using namespace std;

#if 0
//==================================================================================
#pragma mark -
#pragma mark Custom data types
//==================================================================================
#endif

typedef struct ThreadInfo
{
	pthread_t id;
	int index;
} ThreadInfo;

#define RUN_DEBUG 1
#define RUN_NORMAL 2


#define VERSION RUN_NORMAL

#if 0
//==================================================================================
#pragma mark -
#pragma mark Function prototypes
//==================================================================================
#endif

void displayGridPane(void);
void displayStatePane(void);
void initializeApplication(void);
void initializeRandGenerator (void);
void startSimulation (void);
void* generateThreadsFunc(void* arg);
void lockCells (int i, int j);
void unlockCells (int i, int j);
void* computationThreadFunc(void*);
void joinThreads (void);
void freeGrid (void);
void cleanupAndquit(void);
unsigned int cellNewState(unsigned int i, unsigned int j);


//==================================================================================
//	Precompiler #define to let us specify how things should be handled at the
//	border of the frame
//==================================================================================

#if 0
//==================================================================================
#pragma mark -
#pragma mark Simulation mode:  behavior at edges of frame
//==================================================================================
#endif

#define FRAME_DEAD		0	//	cell borders are kept dead
#define FRAME_RANDOM	1	//	new random values are generated at each generation
#define FRAME_CLIPPED	2	//	same rule as elsewhere, with clipping to stay within bounds
#define FRAME_WRAP		3	//	same rule as elsewhere, with wrapping around at edges

//	Pick one value for FRAME_BEHAVIOR
#define FRAME_BEHAVIOR	FRAME_DEAD

#if 0
//==================================================================================
#pragma mark -
#pragma mark Application-level global variables
//==================================================================================
#endif

//	Don't touch
extern int gMainWindow, gSubwindow[2];

unsigned int* mainGrid;
unsigned int** mainGrid2D;
pthread_mutex_t** lockGrid;

//	Piece of advice, whenever you do a grid-based (e.g. image processing),
//	implementastion, you should always try to run your code with a
//	non-square grid to spot accidental row-col inversion bugs.
//	When this is possible, of course (e.g. makes no sense for a chess program).
unsigned short numRows, numCols;

//	the number of live computation threads (that haven't terminated yet)
unsigned short numLiveThreads = 0;
unsigned short numThreads;

unsigned int rule = GAME_OF_LIFE_RULE;

unsigned int colorMode = 0;

bool runSimulation = true;
bool quitGlut = false;
ThreadInfo* info;
unsigned int sleepTime = 300;

//	Random generator:  Ffor a uniform distribution
random_device randGen;
default_random_engine engine(randGen());
uniform_int_distribution<unsigned int> randRow;
uniform_int_distribution<unsigned int> randCol;
uniform_int_distribution<unsigned int> randState;

#if 0
//==================================================================================
#pragma mark -
#pragma mark Computation functions
//==================================================================================
#endif


//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function
//------------------------------------------------------------------------
int main(int argc, const char* argv[])
{
    cout << "-----------------------------------------" << endl;
    cout << "|\tprog06--multi-threading \t|" << endl;
    cout << "|\tauthor: Sierra Obi\t\t|" << endl;
    cout << "-----------------------------------------" << endl;
    cout << "running..." << endl;

	if (argc == 4)
	{
		numRows = atoi(argv[1]);
		numCols = atoi(argv[2]);
		numThreads = atoi(argv[3]);
		if (numRows > 5 && numCols > 5 && numThreads >= 0 && numThreads <= numRows)
		{
			// This takes care of initializing glut and the GUI.
			//	You shouldn’t have to touch this
			initializeFrontEnd(argc, argv, displayGridPane, displayStatePane);
			initializeRandGenerator();
			//	Now we can do application-level initialization
			startSimulation();
		}
		else
		{
			cout << "\t[ERROR] Input must meet the following conditions:\n\t  -> number of threads must be less than the number of rows\n\t  -> all arguements must be non-negative\n\t  -> the number of rows/columns must be greater than 5" << endl;
					exit(1);
		}
	}
	else
	{
		cout << "Usage: " << argv[0] << " <number-of-rows> <number-of-cols> <number-of-threads" << endl;
		exit(1);
	}
	//	This will never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}

void initializeRandGenerator (void)
{
	randRow = uniform_int_distribution<unsigned int>(1, numRows-2);
	randCol = uniform_int_distribution<unsigned int>(1, numCols-2);
	randState = uniform_int_distribution<unsigned int>(0, 1);
}

void initializeApplication(void)
{
    mainGrid = new unsigned int[numRows*numCols];
    mainGrid2D = new unsigned int*[numRows];
	mainGrid2D[0] = mainGrid;
    for (unsigned int i = 1; i < numRows; i++)
    {
        mainGrid2D[i] = mainGrid2D[i-1] + numCols;
    }
	lockGrid = new pthread_mutex_t*[numRows];
	for (int i = 0; i < numRows; i++)
	{
		lockGrid[i] = new pthread_mutex_t[numCols];
		for (int j = 0; j < numCols; j++)
		{
			pthread_mutex_init(&(lockGrid[i][j]),nullptr);
		}
	}
	//	seed the pseudo-random generator
	// srand((unsigned int) time(NULL));
	resetGrid();
}

void startSimulation (void)
{
	initializeApplication();
	//	Create the main simulation thread
	pthread_t renderingThread;
	pthread_create(&renderingThread, NULL,   generateThreadsFunc, NULL);
	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
}

//---------------------------------------------------------------------
//	You will need to implement/modify the two functions below
//---------------------------------------------------------------------

void* generateThreadsFunc(void* arg)
{
	(void) arg;
	info = new ThreadInfo [numThreads];
	for (int k = 0; k <numThreads; k++)
	{
        info[k].index = k;
	}
    for (int k = 0; k <numThreads; k++)
	{
        pthread_create(&(info[k].id),nullptr,computationThreadFunc,info+k);
		numLiveThreads++;
	}
	return NULL;
}

void lockCells (int i, int j)
{
	pthread_mutex_lock(&(lockGrid[i-1][j-1]));
	pthread_mutex_lock(&(lockGrid[i-1][j]));
	pthread_mutex_lock(&(lockGrid[i-1][j+1]));
	pthread_mutex_lock(&(lockGrid[i][j-1]));
	pthread_mutex_lock(&(lockGrid[i][j]));
	pthread_mutex_lock(&(lockGrid[i][j+1]));
	pthread_mutex_lock(&(lockGrid[i+1][j-1]));
	pthread_mutex_lock(&(lockGrid[i+1][j]));
	pthread_mutex_lock(&(lockGrid[i+1][j+1]));
}


void unlockCells (int i, int j)
{
	pthread_mutex_unlock(&(lockGrid[i-1][j-1]));
	pthread_mutex_unlock(&(lockGrid[i-1][j]));
	pthread_mutex_unlock(&(lockGrid[i-1][j+1]));
	pthread_mutex_unlock(&(lockGrid[i][j-1]));
	pthread_mutex_unlock(&(lockGrid[i][j]));
	pthread_mutex_unlock(&(lockGrid[i][j+1]));
	pthread_mutex_unlock(&(lockGrid[i+1][j-1]));
	pthread_mutex_unlock(&(lockGrid[i+1][j]));
	pthread_mutex_unlock(&(lockGrid[i+1][j+1]));
}


void* computationThreadFunc(void*)
{
	// TEST
	#if VERSION == RUN_DEBUG
		{
			stringstream sstr;
			sstr << "\t++-- Threads created -> " << numLiveThreads << "\n";
			cout << sstr.str() << flush;
		}
	#endif
	// END TEST
    while(runSimulation)
    {
		unsigned int i = randRow(engine);
		unsigned int j = randCol(engine);
		lockCells(i,j);
		unsigned int newState = cellNewState(i, j);
		//	In black and white mode, only alive/dead matters
		//	Dead is dead in any mode
		if (colorMode == 0 || newState == 0)
		{
			mainGrid2D[i][j] = newState;
		}
		//	in color mode, color reflext the "age" of a live cell
		else
		{
			//	Any cell that has not yet reached the "very old cell"
			//	stage simply got one generation older
			if (mainGrid2D[i][j] < NB_COLORS-1)
				mainGrid2D[i][j] = mainGrid2D[i][j] + 1;
			//	An old cell remains old until it dies
			else
				mainGrid2D[i][j] = mainGrid2D[i][j];
		}
		unlockCells(i,j);
		usleep(sleepTime);
    }
	return NULL;
}

//	This is the function that determines how a cell update its state
//	based on that of its neighbors.
//	1. As repeated in the comments below, this is a "didactic" implementation,
//	rather than one optimized for speed.  In particular, I refer explicitly
//	to the S/B elements of the "rule" in place.
//	2. This implentation allows for different behaviors along the edges of the
//	grid (no change, keep border dead, clipping, wrap around). All these
//	variants are used for simulations in research applications.
unsigned int cellNewState(unsigned int i, unsigned int j)
{
	//	First count the number of neighbors that are alive
	//----------------------------------------------------
	//	Again, this implementation makes no pretense at being the most efficient.
	//	I am just trying to keep things modular and somewhat readable
	int count = 0;

	//	Away from the border, we simply count how many among the cell's
	//	eight neighbors are alive (cell state > 0)
	if (i>0 && i<numRows-1 && j>0 && j<numCols-1)
	{
		//	remember that in C, (x == val) is either 1 or 0
		count = (mainGrid2D[i-1][j-1] != 0) +
				(mainGrid2D[i-1][j] != 0) +
				(mainGrid2D[i-1][j+1] != 0)  +
				(mainGrid2D[i][j-1] != 0)  +
				(mainGrid2D[i][j+1] != 0)  +
				(mainGrid2D[i+1][j-1] != 0)  +
				(mainGrid2D[i+1][j] != 0)  +
				(mainGrid2D[i+1][j+1] != 0);
	}
	//	on the border of the frame...
	else
	{
		#if FRAME_BEHAVIOR == FRAME_DEAD

			//	Hack to force death of a cell
			count = -1;

		#elif FRAME_BEHAVIOR == FRAME_RANDOM

			count = randRow(engine) % 9;

		#elif FRAME_BEHAVIOR == FRAME_CLIPPED

			if (i>0)
			{
				if (j>0 && mainGrid2D[i-1][j-1] != 0)
					count++;
				if (mainGrid2D[i-1][j] != 0)
					count++;
				if (j<numCols-1 && mainGrid2D[i-1][j+1] != 0)
					count++;
			}

			if (j>0 && mainGrid2D[i][j-1] != 0)
				count++;
			if (j<numCols-1 && mainGrid2D[i][j+1] != 0)
				count++;

			if (i<numRows-1)
			{
				if (j>0 && mainGrid2D[i+1][j-1] != 0)
					count++;
				if (mainGrid2D[i+1][j] != 0)
					count++;
				if (j<numCols-1 && mainGrid2D[i+1][j+1] != 0)
					count++;
			}


		#elif FRAME_BEHAVIOR == FRAME_WRAPPED

			unsigned int 	iM1 = (i+numRows-1)%numRows,
							iP1 = (i+1)%numRows,
							jM1 = (j+numCols-1)%numCols,
							jP1 = (j+1)%numCols;
			count = mainGrid2D[iM1][jM1] != 0 +
					mainGrid2D[iM1][j] != 0 +
					mainGrid2D[iM1][jP1] != 0  +
					mainGrid2D[i][jM1] != 0  +
					mainGrid2D[i][jP1] != 0  +
					mainGrid2D[iP1][jM1] != 0  +
					mainGrid2D[iP1][j] != 0  +
					mainGrid2D[iP1][jP1] != 0 ;

		#else
			#error undefined frame behavior
		#endif

	}	//	end of else case (on border)

	//	Next apply the cellular automaton rule
	//----------------------------------------------------
	//	by default, the grid square is going to be empty/dead
	unsigned int newState = 0;

	//	unless....

	switch (rule)
	{
		//	Rule 1 (Conway's classical Game of Life: B3/S23)
		case GAME_OF_LIFE_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (mainGrid2D[i][j] != 0)
			{
				if (count == 3 || count == 2)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;

		//	Rule 2 (Coral Growth: B3/S45678)
		case CORAL_GROWTH_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (mainGrid2D[i][j] != 0)
			{
				if (count > 3)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;

		//	Rule 3 (Amoeba: B357/S1358)
		case AMOEBA_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (mainGrid2D[i][j] != 0)
			{
				if (count == 1 || count == 3 || count == 5 || count == 8)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3 || count == 5 || count == 7)
					newState = 1;
			}
			break;

		//	Rule 4 (Maze: B3/S12345)							|
		case MAZE_RULE:

			//	if the cell is currently occupied by a live cell, look at "Stay alive rule"
			if (mainGrid2D[i][j] != 0)
			{
				if (count >= 1 && count <= 5)
					newState = 1;
			}
			//	if the grid square is currently empty, look at the "Birth of a new cell" rule
			else
			{
				if (count == 3)
					newState = 1;
			}
			break;

			break;

		default:
			cout << "Invalid rule number" << endl;
			exit(5);
	}

	return newState;
}
void freeGrid (void)
{
	delete []info;
	delete []mainGrid2D;
	delete []mainGrid;
	for (unsigned int i = 0; i < numRows; i++)
	{
		for (unsigned int j = 0; j < numCols; j++)
		{
			pthread_mutex_destroy(&(lockGrid[i][j]));
		}
		delete []lockGrid[i];
	}
	delete []lockGrid;
}
void joinThreads (void)
{
	int count = 0;
	for (int k = 0; k < numThreads; k++)
	{
        pthread_join(info[k].id,nullptr);
		numLiveThreads--;
		count ++;
	}
	#if VERSION == RUN_DEBUG
		stringstream sstr;
		// notice that the threads will continue to run forever
		// join the threads once they have finished processing
		sstr << "  simulation terminated" << endl;
		sstr << "  " << count << " threads joined\n";
		cout << sstr.str() << flush;
	#endif
}
void cleanupAndquit(void)
{
	runSimulation = false;
	joinThreads();
	freeGrid();
	cout << "end." << endl;
	exit(0);
}



#if 0
#pragma mark -
#pragma mark GUI functions
#endif

//==================================================================================
//	GUI functions
//	These are the functions that tie the simulation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================


void displayGridPane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[GRID_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render the grid.
	// THIS IS A READER
	//---------------------------------------------------------
	drawGrid(mainGrid2D, numRows, numCols);

	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();

	glutSetWindow(gMainWindow);
}

void displayStatePane(void)
{
	//	This is OpenGL/glut magic.  Don't touch
	glutSetWindow(gSubwindow[STATE_PANE]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//	You may want to add stuff to display
	//---------------------------------------------------------
	drawState(numLiveThreads);

	//	This is OpenGL/glut magic.  Don't touch
	glutSwapBuffers();
	glutSetWindow(gMainWindow);
}


//	This callback function is called when a keyboard event occurs
//  can adjust sleep time
void myKeyboardFunc(unsigned char c, int x, int y)
{
	(void) x; (void) y;

	switch (c)
	{
		//	'ESC' --> exit the application
		case 27:
			quitGlut = true;
			cleanupAndquit();
			break;

		//	spacebar --> resets the grid
		case ' ':
			resetGrid();
			break;

		//	'+' --> increase simulation speed
		case '+':
			break;

		//	'-' --> reduce simulation speed
		case '-':
			break;

		//	'1' --> apply Rule 1 (Game of Life: B23/S3)
		case '1':
			rule = GAME_OF_LIFE_RULE;
			break;

		//	'2' --> apply Rule 2 (Coral: B3_S45678)
		case '2':
			rule = CORAL_GROWTH_RULE;
			break;

		//	'3' --> apply Rule 3 (Amoeba: B357/S1358)
		case '3':
			rule = AMOEBA_RULE;
			break;

		//	'4' --> apply Rule 4 (Maze: B3/S12345)
		case '4':
			rule = MAZE_RULE;
			break;

		//	'c' --> toggles on/off color mode
		//	'b' --> toggles off/on color mode
		case 'c':
		case 'b':
			colorMode = !colorMode;
			break;

		//	'l' --> toggles on/off grid line rendering
		case 'l':
			toggleDrawGridLines();
			break;
			//	',' --> slowdown
			case ',':
				sleepTime = 11*sleepTime/10;
				break;
			//	'.' --> speedup
			case '.':
				if (sleepTime > 10000)
					sleepTime = 9*sleepTime/10;

		default:
			break;
	}

	glutSetWindow(gMainWindow);
	glutPostRedisplay();
}

void myTimerFunc(int value)
{
	//	value not used.  Warning suppression
	(void) value;
	//	This is not the way it should be done, but it seems that Apple is
	//	not happy with having marked glut as deprecated.  They are doing
	//	things to make it break
    //glutPostRedisplay();
    myDisplayFunc();
	if (!quitGlut)
	{
	//	And finally I perform the rendering
		glutTimerFunc(15, myTimerFunc, 0);
	}
}

//---------------------------------------------------------------------
//	You shouldn't need to touch the functions below
//---------------------------------------------------------------------

void resetGrid(void)
{
	for (unsigned int i = 0; i < numRows; i++)
	{
		for (unsigned int j = 0; j < numCols; j++)
		{
			mainGrid2D[i][j] = 	randState(engine);
		}
	}
}
