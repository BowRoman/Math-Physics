#include <stdio.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <random>
#include <assert.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifndef MACOSX
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include "fssimplewindow.h"
#include "bitmapfont/ysglfontdata.h"

GLuint theTorus;
static double time = 0;

typedef enum 
{
	eStop = -1,
	eIdle = 0,
	eStart = 1,
	Linear,
	Hermite,
	CatmulRom,
	QuadraticBezier,
	CubicBezier
} changeType;

changeType eInterpolation = Linear;
bool			keys[256];			// Array Used For The Keyboard Routine
float timeRange = 10.;
int WinWidth = 860;
int WinHeight = 720;
float ratio = (float)WinHeight / (float)WinWidth;
float WorldWidth = 50.0; // 50 meter wide
float WorldHeight = WorldWidth * ratio; // 
bool isSpline = false;


struct myVector{
	float x, y, z;
	float color[3];

	myVector(float a, float b) :x(a), y(b), z(0){}
	myVector() : x(0), y(0), z(0){}
	
	myVector(const myVector &p) : x(p.x), y(p.y), z(p.z) {}
	

	void set(float a, float b, float c = 0) { x = a; y = b; z = c; }
	myVector  operator+(const myVector rhs)
	{
		myVector *tmp = new myVector(*this);
		tmp->x += rhs.x;
		tmp->y += rhs.y;
		return *tmp;
	}
	myVector operator-(const myVector rhs)
	{
		myVector *tmp = new myVector(*this);
		tmp->x -= rhs.x;
		tmp->y -= rhs.y;
		return *tmp;
	}
	myVector operator *(float mul)
	{
		myVector *tmp = new myVector(*this);
		tmp->x *= mul; tmp->y *= mul;
		return *tmp;
	}
	myVector & operator=(const myVector rhs)
	{
		if (this == &rhs)
			return *this;
		x = rhs.x;
		y = rhs.y;
		return *this;
	}
	float length() { return sqrt(x*x+y*y + z*z); }
	float dist(myVector &p) { return (*this - p).length(); }

};

void FillTorus(float rc, int numc, float rt, int numt)
{
	int i, j, k;
	double s, t;
	double x, y, z;
	double pi, twopi;

	pi = M_PI;
	twopi = 2 * pi;

	for (i = 0; i < numc; i++) {
		glBegin(GL_QUAD_STRIP);
		for (j = 0; j <= numt; j++) {
			for (k = 1; k >= 0; k--) {
				s = (i + k) % numc + 0.5;
				t = j % numt;

				x = cos(t * twopi / numt) * cos(s * twopi / numc);
				y = sin(t * twopi / numt) * cos(s * twopi / numc);
				z = sin(s * twopi / numc);
				glNormal3f(x, y, z);

				x = (rt + rc * cos(s * twopi / numc)) * cos(t * twopi / numt);
				y = (rt + rc * cos(s * twopi / numc)) * sin(t * twopi / numt);
				z = rc * sin(s * twopi / numc);
				glVertex3f(x, y, z);
			}
		}
		glEnd();
	}
}
void Draw_Axes(void)
{
	static float ORG[3] = { 0, 0, 0 };
	static float XP[3] = { 3, 0, 0 }, YP[3] = { 0,3, 0 }, ZP[3] = { 0, 0, 3 }, 

//	glPushMatrix();

//	glTranslatef(-2.4, -1.5, -5);
//	glRotatef(tip, 1, 0, 0);
//	glRotatef(turn, 0, 1, 0);
//	glScalef(0.25, 0.25, 0.25);

	glLineWidth(3.0);

	glBegin(GL_LINES);
	glColor3b(125, 0, 0); // X axis is red.
	glVertex3fv(ORG);
	glVertex3fv(XP);
	glColor3b(0, 125, 0); // Y axis is green.
	glVertex3fv(ORG);
	glVertex3fv(YP);
	glColor3b(0, 0, 125); // z axis is blue.
	glVertex3fv(ORG);
	glVertex3fv(ZP);
	glEnd();

//	glPopMatrix();
}

const float PointSize = 1./3.;
const unsigned maxControlPoints = 15;
struct controlObj {
	myVector vertices[4];
	myVector center;
	int winx, winy; // screen coordinates needed for picking
	unsigned char color[3];
	float width;
	float timeStamp;  // the time at which current point reaches this control point.

	controlObj() {
		set(0, 0, 0);
		color[0] = rand() % 255; color[1] = rand() % 255; color[2] = rand() % 255;
	}
	controlObj(float x, float y, float z){
		set(x, y, z);
		color[0] = rand() % 255; color[1] = rand() % 255; color[2] = rand() % 255;
	}

	void set(float x, float y, float z){
		width = PointSize;
		vertices[0].set(x - width, y - width, z);
		vertices[1].set(x + width, y - width, z);
		vertices[2].set(x + width, y + width, z);
		vertices[3].set(x - width, y + width, z);
		center.set(x, y, z);

		winx = (x*(float)WinWidth) / WorldWidth;
		winy = WinHeight - (int)( (y*(float)WinHeight) / WorldHeight );
	}
	void Render(){
		glColor3ubv(color);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBegin(GL_QUADS);
		glVertex2f(vertices[0].x, vertices[0].y);
		glVertex2f(vertices[1].x, vertices[1].y);
		glVertex2f(vertices[2].x, vertices[2].y);
		glVertex2f(vertices[3].x, vertices[3].y);
		glEnd();
	}

	void RenderToScreen() {
		glColor3ubv(color);
		glPointSize(6);
		glBegin(GL_POINTS);
			glVertex2i(winx, winy);
		glEnd();
	}
	
} contrlPnts[maxControlPoints];

float pieceLength[maxControlPoints-1];
myVector currentPoint; //  this is position of the moving point.
unsigned controlPointCnt = 0;
int width = 0, height = 0;


void lerp(myVector &start, myVector &end, float t, float timespan, myVector &result) // timespan = t_2- t_1
{
	myVector tmp = (start *((timespan - t) / timespan));
	tmp = tmp + (end *(t / timespan));
	result = tmp;
}

int UI_Process()
{
	FsPollDevice();
	int key = FsInkey();
	keys[key] = true;
	int ret = key;
	switch (key)
	{
	case FSKEY_S:
		ret = eStart;
		break;
	case FSKEY_ESC:
		ret = eStop;
		break;
	case FSKEY_L:
		eInterpolation = Linear;
		break;
	case FSKEY_H:
		eInterpolation = Hermite;
		break;
	case FSKEY_B:
		eInterpolation = CubicBezier;
		break;
	case FSKEY_Q:
		eInterpolation = QuadraticBezier;
		break;
	case FSKEY_C:
		eInterpolation = CatmulRom;
		break;
	}
	return ret;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int Menu()
{
	int r=0;
	
	while(r!=eStart && r!= eStop)
	{
		r = UI_Process();

		if(r== eStop)
			return eStop;
		int wid,hei;
		FsGetWindowSize(wid,hei);

		glViewport(0,0,wid,hei);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-0.5,(GLdouble)wid-0.5,(GLdouble)hei-0.5,-0.5,-1,1);

		glClearColor(0.5,0.5,0.5,1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3ub(127,127,127);

		char sSpeed[200];
		sprintf(sSpeed, "Starting point (%f, %f), End point (%f, %f)\n", 
			contrlPnts[0].center.x, contrlPnts[0].center.y, contrlPnts[maxControlPoints - 1].center.x, contrlPnts[maxControlPoints - 1].center.y);
		char *message = "Use L, H, B, Q, C to cycle among interpolation schemes.\n";
		char interpolstyle[180];
		sprintf(interpolstyle, "interpolation method is %s %s!\n",( eInterpolation==Linear ? "Linear" : 
														         eInterpolation==CubicBezier ? "Cubic Bezier" :
																 eInterpolation == QuadraticBezier ? "Quadratic Bezier" :
																 eInterpolation == Hermite ? "Hermite" : "CatMulRom"), 
																 isSpline ? "spline" : "");
																 
		glColor3ub(255, 255, 255);
		glRasterPos2i(32,32);
		glCallLists(strlen(sSpeed),GL_UNSIGNED_BYTE,sSpeed);
		glRasterPos2i(32, 64);
		glCallLists(strlen(message), GL_UNSIGNED_BYTE, message);
		glRasterPos2i(32, 96);
		glCallLists(strlen(interpolstyle), GL_UNSIGNED_BYTE, interpolstyle);
		
		const char *msg1 = "S.....Start Game";
		const char *msg2="ESC...Exit";
		glRasterPos2i(32,128);
		glCallLists(strlen(msg1),GL_UNSIGNED_BYTE,msg1);
		glRasterPos2i(32,160);
		glCallLists(strlen(msg2),GL_UNSIGNED_BYTE,msg2);

		FsSwapBuffers();
		FsSleep(10);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	FsSwapBuffers();

	return r;
}
void clearScreen()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();									// Reset The Current Modelview Matrix
	glColor3ub(0, 0, 0);
	glBegin(GL_QUADS);
	{
		glVertex2f(0, 0);
		glVertex2f(WorldWidth, 0);
		glVertex2f(WorldWidth, WorldHeight);
		glVertex2f(0, WorldHeight);
		glEnd();
	}
	FsSwapBuffers();
}
///////////////////////////////////////////////////////////////
void updatePath(double timeInc)
{
	int pieceIndex = 0;
	while ((contrlPnts[pieceIndex].timeStamp < time) || fabs(contrlPnts[pieceIndex].timeStamp - time) < 0.0001)
		pieceIndex++;
	
	assert(pieceIndex > 0);
	myVector startPnt = contrlPnts[pieceIndex-1].center;
	myVector endPnt = contrlPnts[pieceIndex].center;
	
	float pieceTime = time - contrlPnts[pieceIndex-1].timeStamp;
	float pieceTimeSpan = contrlPnts[pieceIndex].timeStamp - contrlPnts[pieceIndex-1].timeStamp;
	lerp(startPnt, endPnt, pieceTime, pieceTimeSpan, currentPoint);
	time += timeInc;
}

void initLighting()
{
	static float lmodel_ambient[] = { 0.5, 0.0, 0.0, 0.0 };
	static float lmodel_twoside[] = { GL_FALSE };
	static float lmodel_local[] = { GL_FALSE };
	static float light0_ambient[] = { 0.0, 0.1, 0.5, 1.0 };
	static float light0_diffuse[] = { 0.0, 0.5, 1.0, 0.0 };
	static float light0_position[] = { 10.0, 10.0, 10.0, 0 };
	static float light0_specular[] = { 1.0, 1.0, 1.0, 0.0 };
	static float bevel_mat_ambient[] = { 0.0, 0.5, 0.0, 1.0 };
	static float bevel_mat_shininess[] = { 40.0 };
	static float bevel_mat_specular[] = { 1.0, 1.0, 1.0, 0.0 };
	static float bevel_mat_diffuse[] = { 0.0, 0.0, 1.0, 0.0 };


	glClearColor(0.5, 0.5, 0.5, 1.0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glEnable(GL_LIGHT0);

	glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, lmodel_local);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	//glEnable(GL_LIGHTING);

	glMaterialfv(GL_FRONT, GL_AMBIENT, bevel_mat_ambient);
	glMaterialfv(GL_FRONT, GL_SHININESS, bevel_mat_shininess);
	glMaterialfv(GL_FRONT, GL_SPECULAR, bevel_mat_specular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, bevel_mat_diffuse);

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);

}
void init()
{
	controlPointCnt = 0;
	contrlPnts[controlPointCnt++].set(PointSize+1., WorldHeight / 3.0, 0.);
	contrlPnts[controlPointCnt++].set(WorldWidth - PointSize - 1.0, WorldHeight / 2.0, 0);
	contrlPnts[0].timeStamp = 0.f;
	contrlPnts[1].timeStamp = timeRange;
		
	pieceLength[0] = contrlPnts[1].center.dist(contrlPnts[0].center);

	theTorus = glGenLists(1);
	glNewList(theTorus, GL_COMPILE);
	FillTorus(0.3, 8, 1.0, 25);
	glEndList();

	// init lighting
	initLighting();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
void reset(bool clear)
{
	time = 0.0;

	currentPoint = contrlPnts[0].center;

	glClearDepth(2.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	/// clear the window:
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0., (GLdouble)WorldWidth, 0., (GLdouble)WorldHeight, -10, 10);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix

	if (clear)
	{
		clearScreen();
	}
}

void renderControlPoints(GLenum mode)
{
	if (mode == GL_RENDER){
		for (int i=0; i < controlPointCnt; i++) {
			contrlPnts[i].Render();
		}
	}
	else if(mode == GL_SELECT){
		int i = 0;
		for (; i < controlPointCnt; i++) {
			glLoadName(i);
			contrlPnts[i].RenderToScreen();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
float rotAngle = 0.0f;
void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();									// Reset The Current Modelview Matrix

	///////////// draw the end or control points /////////////
	renderControlPoints(GL_RENDER);

	//////////////////////////// draw the path
	glPointSize(2);
	switch(eInterpolation){
	case Linear : 
		glColor3ub(28, 10, 255);
		glBegin(GL_POINTS);
		glVertex2f(currentPoint.x, currentPoint.y);
		glEnd();
		break;
	default:
		printf("!No proper interpolation selected!!\n");
	}
	
	glPushMatrix();
	glEnable(GL_LIGHTING);
	glTranslatef(currentPoint.x, currentPoint.y, currentPoint.z);
	glRotatef(rotAngle++, 1, 0, 0);
	glCallList(theTorus);
	glDisable(GL_LIGHTING);
	Draw_Axes();
	glPopMatrix();


	FsSwapBuffers();
}

#define MAXSELECT 100
#define MAXFEED 300
GLuint selectBuf[MAXSELECT];
GLfloat feedBuf[MAXFEED];
int vp[4];

void updateControlPoint(int mx, int my, float xWorld, float yWorld)
{
	glSelectBuffer(MAXSELECT, selectBuf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(~0);

	glPushMatrix();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPickMatrix(mx, my, 30, 30, vp);
	gluOrtho2D(0, WinWidth, 0, WinHeight);
	glMatrixMode(GL_MODELVIEW);

	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	renderControlPoints(GL_SELECT);
	
	glPopMatrix();
	int hits = glRenderMode(GL_RENDER);
	if (hits > 0)
	{
		int hitIdx = selectBuf[(hits - 1) * 4 + 3];
		if (hitIdx != -1)
		{
			contrlPnts[hitIdx].set(xWorld, yWorld, 0.);
		}
	}
}

void recalculateTimeStamps()
{
	// first calculate piecelength for each piece, i.e. distance between consecutive control pnts
	float totalLength = 0.f;
	for (int i = 1; i< controlPointCnt; i++)
	{
		pieceLength[i-1] = contrlPnts[i].center.dist(contrlPnts[i-1].center);
		totalLength += pieceLength[i-1];
	}

	// now compute timestamp for each control point
	for(int i=1; i<controlPointCnt; i++)
	{
		contrlPnts[i].timeStamp = contrlPnts[i-1].timeStamp + 
									(pieceLength[i-1]/totalLength) * timeRange;
	}
}

///////////////////////////////////////////////////////////////////
int Game(void)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glDepthFunc(GL_LEQUAL);


	DWORD passedTime = 0;
	FsPassedTime(true);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//////////// initial setting up the scene ////////////////////////////////////////
	reset(true);
	int timeSpan = 33; // milliseconds
	double timeInc = (double)timeSpan * 0.001; // time increment in seconds

	FsGetWindowSize(WinWidth, WinHeight);

	int lb,mb,rb,mx,my;

	glViewport(0, 0, WinWidth, WinHeight);
	glGetIntegerv(GL_VIEWPORT, vp);
	////////////////////// main simulation loop //////////////////////////
	while (1)
	{
		int key = UI_Process();
		if (key == eStop)
			break;
		int mouseEventType = FsGetMouseEvent(lb, mb, rb, mx, my);
		float xWorld = ((float)mx / (float)WinWidth) * WorldWidth;
		float yWorld = ((float)(WinHeight - my) / (float)WinHeight) * WorldHeight;
		if (rb == 1 && mouseEventType == FSMOUSEEVENT_RBUTTONDOWN && (controlPointCnt != maxControlPoints-1)) // right mouse button clicked. So create a new control point here:
		{
			contrlPnts[controlPointCnt] = contrlPnts[controlPointCnt - 1];
			contrlPnts[controlPointCnt-1].set(xWorld, yWorld, 0);
			controlPointCnt++;
			recalculateTimeStamps();
			reset(true);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		else if (lb == 1 && mouseEventType == FSMOUSEEVENT_MOVE) // start dragging operation on a control point
		{
			updateControlPoint(mx, my, xWorld, yWorld);
			reset(false);
		}

		// specify if it is a spline or not!
		if (eInterpolation == Linear)
			isSpline = false;
		else if (eInterpolation == CubicBezier && controlPointCnt > 4)
			isSpline = true;
		else if (controlPointCnt > 3)
			isSpline = true;

		timeInc = (double)(passedTime) * 0.001;

		/////////// update path /////////////////
		updatePath(timeInc);
		if (time >= timeRange)
		{
			reset(false);
		}

		renderScene();

		////// update time lapse /////////////////
		passedTime = FsPassedTime(); // Making it up to 50fps
		int timediff = timeSpan-passedTime;
	//	printf("\ntimeInc=%f, passedTime=%d, timediff=%d", timeInc, passedTime, timediff);
		while(timediff >= timeSpan/3)
		{
			FsSleep(5);
			passedTime=FsPassedTime(); // Making it up to 50fps
			timediff = timeSpan-passedTime;
	//		printf("--passedTime=%d, timediff=%d", passedTime, timediff);
		}
		passedTime=FsPassedTime(true); // Making it up to 50fps
	}
	return 0;
}

/////////////////////////////////////////////////////////////////
void GameOver(int score)
{
	int r=0;

	FsPollDevice();
	while(FsInkey()!=0)
	{
		FsPollDevice();
	}

	while(FsInkey()==0)
	{
		FsPollDevice();

		int wid,hei;
		FsGetWindowSize(wid,hei);

		glViewport(0,0,wid,hei);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0,(float)wid-1,(float)hei-1,0,-1,1);

		glClearColor(0.5,0.5,0.5,0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		const char *msg1="Game Over";
		char msg2[256];
		glColor3ub(255,255,255);
		glRasterPos2i(32,32);
		glCallLists(strlen(msg1),GL_UNSIGNED_BYTE,msg1);

		sprintf(msg2,"Your score is %d",score);

		glRasterPos2i(32,48);
		glCallLists(strlen(msg2),GL_UNSIGNED_BYTE,msg2);

		FsSwapBuffers();
		FsSleep(10);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
int main(void)
{
	int menu;
	FsOpenWindow(32, 32, WinWidth, WinHeight, 1); // 800x600 pixels, useDoubleBuffer=1

	int listBase;
	listBase=glGenLists(256);
	YsGlUseFontBitmap8x12(listBase);
	glListBase(listBase);
	
	init();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDepthFunc(GL_ALWAYS);

	while(1)
	{
		reset(false);
		menu = Menu();
		if(menu==1)
		{
			int score;
			score=Game();
			GameOver(score);
		}
		else if(menu==eStop)
		{
			break;
		}
	}

	return 0;
}


