#include <stdio.h>
#include <string>
#include <fstream>

#define _USE_MATH_DEFINES
#include <math.h>

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

#include "Circle.h"

#include "Integration.h"

typedef enum 
{
	eStop = -1,
	eIdle = 0,
	eStart = 1,
	eSpeedUp,
	eSpeedDown,
	eAngleUp,
	eAngleDown,
} changeType;

double gravity = -9.81; // m/s*s
Vector3d<float> Gravity(0., -9.81, 0.);
double iAngle = M_PI / 3.; // projectile inclination angle in radian
double iSpeed = 15.0;  // initial ball speed
double radius = 0.5;
int circleSections = 10;

const int winWidth = 800;
const int winHeight = 600;
float ratio = (float)winHeight / (float)winWidth;
float WorldWidth = 70.0; // 50 meter wide
float WorldHeight = WorldWidth * ratio; // 
int width = 0, height = 0;

static double clocktime = 0.f;
int stimeSpan = 33; // milliseconds
double stimeInc = (double)stimeSpan * 0.001; // time increment in seconds

Circle3D eulerEBall, eulerSEBall, verletBall, RK4Ball, realBall;

IntegrationHandler integrationModule;

void initPhysics(double rad, double speed, double angle);

std::ofstream ballValues("BallValues.txt");

//////////////////////////////////////////////////////////////////////////////////////////////
int Menu(void)
{
	int r=0,key;
	const double angleInc = M_PI / 180.;
	while(r!=eStart && r!= eStop)
	{
		FsPollDevice();
		key=FsInkey();
		switch(key)
		{
		case FSKEY_S:
			r=eStart;
			break;
		case FSKEY_ESC:
			r=eStop;
			break;
		case FSKEY_UP:
			iSpeed++;
			break;
		case FSKEY_DOWN:
			iSpeed = max(2., iSpeed-1);
			break;
		case FSKEY_LEFT:
			iAngle = max(0., iAngle-angleInc);
			break;
		case FSKEY_RIGHT:
			iAngle = min(90.0, iAngle+angleInc);
			break;
		}

		if (r == eStop)
			return r;
		int wid,hei;
		FsGetWindowSize(wid,hei);


		glViewport(0,0,wid,hei);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-0.5,(GLdouble)wid-0.5,(GLdouble)hei-0.5,-0.5,-1,1);

		glClearColor(0.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor3ub(127,127,127);

		char sSpeed[128];
		sprintf(sSpeed, "Initial ball speed is %f m/s. Use Up/Down keys to change it!\n", iSpeed);
		char sAngle[128];
		sprintf(sAngle, "Initial horizon Angle is %f degrees. Use Left/Right keys to change it!\n", iAngle*180. / M_PI);
		glColor3ub(255,255,255);
		glRasterPos2i(32,32);
		glCallLists(strlen(sSpeed),GL_UNSIGNED_BYTE,sSpeed);
		glRasterPos2i(32,64);
		glCallLists(strlen(sAngle),GL_UNSIGNED_BYTE,sAngle);
		const char *msg1="S.....Start Game";
		const char *msg2="ESC...Exit";
		glRasterPos2i(32,96);
		glCallLists(strlen(msg1),GL_UNSIGNED_BYTE,msg1);
		glRasterPos2i(32,128);
		glCallLists(strlen(msg2),GL_UNSIGNED_BYTE,msg2);

		FsSwapBuffers();
		FsSleep(10);
	}

	initPhysics(radius, iSpeed, iAngle);
	return r;
}

///////////////////////////////////////////////////////////////
void drawAxis()
{
	// We Will Draw Horizontal Lines With A Space Of 1 Meter Between Them.
	glColor3ub(0, 0, 255);										// Draw In Blue
	glBegin(GL_LINES);
	glLineWidth(0.25);

	// Draw The Horizontal Lines
	for (float y = 0.; y <= WorldHeight; y += 1.0f)		// y += 1.0f Stands For 1 Meter Of Space In This Example
	{
		glVertex3f(0, y, 0);
		glVertex3f(WorldWidth, y, 0);
	}
	glEnd(); 
}
///////////////////////////////////////////////////////////////////////////////////////////

void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0., WorldWidth, 0., (GLdouble)WorldHeight);

	////////////////////////// Drawing The Coordinate Plane Starts Here.
	//drawAxis();
	
	/////////////////////////draw the hallow 2d disc /////////////

	//.DrawCircle(0, circleSections);
	eulerSEBall.DrawCircle(0, circleSections);
	eulerEBall.DrawCircle(0, circleSections);
	verletBall.DrawCircle(0, circleSections);
	RK4Ball.DrawCircle(0, circleSections);
	realBall.DrawCircle(0, circleSections);

	///////////// draw the overlay HUD /////////////////////
	glColor3ub(127, 127, 127);
	char str[256];
	//sprintf(str, "simBall1: pos(%f, %f), velocity(%f, %f)", simBall1.cx, simBall1.cy, simBall1.vx, simBall1.vy);
	//glRasterPos2i(32, height-32);
	//glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
//	sprintf(str, "real: pos(%f, %f), velocity(%f, %f)", realBall.cPos.x, realBall.cPos.y, realBall.cV.x, realBall.cV.y);
//	glRasterPos2i(32, height - 32);
//	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
	//printf("%s\n", str);
	FsSwapBuffers();
}
/////////////////////check edge collision ////////////////////////////////////////
void CheckEdgeBoundaries(Circle3D &ball)
{
	if (ball.cPos.x<0. && ball.cV.x <0.)
	{
		ball.cPos.x = -ball.cPos.x;
		ball.cV.x = -ball.cV.x;
	}
	if (ball.cPos.y<0 && ball.cV.y<0)
	{
		ball.cPos.y = -ball.cPos.y;
		ball.cV.y = -ball.cV.y;
	}
	if (ball.cPos.x>WorldWidth && ball.cV.x > 0.f)
	{
		ball.cPos.x = WorldWidth - (ball.cPos.x - WorldWidth);
		ball.cV.x = -ball.cV.x;
	}
	if (ball.cPos.y > WorldHeight && ball.cV.y > 0.f)
	{
		ball.cPos.y = WorldHeight - (ball.cPos.y - WorldHeight);
		ball.cV.y = -ball.cV.y;
	}
}

/////////////////////////////////////////////////////////////////////
void updateEulerEPhysics(Circle3D &ball, double timeInc)
{
	//////////// your physics goes here //////////////////////////
	// we use a coordinate system in which x goes from left to right of the screen and y goes from top to bottom of the screen
	// we have 1 forces here: 1) gravity which is in positive y direction. 
	//////////////Explicit Euler Integration:///////////////////////
	integrationModule.EulerE(ball.cPos, ball.cV, Gravity, timeInc);
	ballValues << "EulerE pos: " << ball.cPos.x << ", " << ball.cPos.y << "\n";

	CheckEdgeBoundaries(ball);
}
/////////////////////////////////////////////////////////////////////
void updateEulerSEPhysics(Circle3D &ball, double timeInc)
{
	integrationModule.EulerSE(ball.cPos, ball.cV, Gravity, timeInc);
	ballValues << "EulerSE pos: " << ball.cPos.x << ", " << ball.cPos.y << "\n";

	CheckEdgeBoundaries(ball);
}
/////////////////////////////////////////////////////////////////////
void updateVerletPhysics(Circle3D &ball, double timeInc, int i)
{
	// More exact formula for variable time step is the following:
	//  x_2 = x_1 + (x_1 - x_0)(dt_1/dt_0) + accel * dt_1 * dt_1
	integrationModule.Verlet(ball.cPos, ball.lPos, ball.cV, Gravity, timeInc);
	ballValues << "Verlet pos: " << ball.cPos.x << ", " << ball.cPos.y << "\n";
	CheckEdgeBoundaries(ball);
}

/////////////////////////////////////////////////////////////////////////
void updatePrecisePhysics(Circle3D &ball, double timeInc)
{
	printf("update: timeInc=%f\n", timeInc);
	double ax = 0;
	ball.cPos = ball.cPos + ball.cV * timeInc + Gravity * (timeInc * timeInc * 0.5);
	ball.cV = ball.cV + Gravity * timeInc;
	ballValues << "Precise pos: " << ball.cPos.x << ", " << ball.cPos.y << "\n";
	CheckEdgeBoundaries(ball);
}

/////////////////////////////////////////////////////////////////////

void updateRK4Physics(Circle3D &ball, double timeInc)
{
	integrationModule.RK4(ball.cPos, ball.cV, Gravity, timeInc);
	ballValues << "RK4 pos: " << ball.cPos.x << ", " << ball.cPos.y << "\n";
	CheckEdgeBoundaries(ball);
}

///////////////////////////////////////////////////////////////
void initPhysics(double rad, double speed, double angle)
{
	clocktime = 0.f;
	double vx = speed * cos(angle);
	double vy = speed * sin(angle);
	Vector3d<float> ipos(rad, rad, 0);
	Vector3d<float> iv(vx, vy, 0);
	eulerEBall.set(rad, ipos, iv, 2, 230, 0, 0);
	eulerSEBall.set(rad, ipos, iv, 2, 128, 128, 0);
	verletBall.set(rad, ipos, iv, 2, 0, 200, 0);
	RK4Ball.set(rad, ipos, iv, 2, 0, 0, 200);
	realBall.set(rad, ipos, iv, 2, 128, 128, 128);

	//set prev position for verlet
	// be careful to use exactly correct formula to compute prev position
	// based on initial state. For projectile we use exact position formula:
	//Vector3d<float> prevPos = ipos + iv * (-stimeInc+0.003) +Gravity * (0.5 * (-stimeInc+0.003)*(-stimeInc+0.003));
}

///////////////////////////////////////////////////////////////////
int Game(void)
{
	DWORD passedTime = 0;
	FsPassedTime(true);

	//////////// initial setting up the scene ////////////////////////////////////////
	FsGetWindowSize(width, height);

	int lb,mb,rb,mx,my;
	glViewport(0, 0, width, height);

	double timeInc = stimeInc;
	////////////////////// main simulation loop //////////////////////////
	while (1)
	{
		FsPollDevice();
		FsGetMouseState(lb,mb,rb,mx,my);
		int key=FsInkey();
		if(key == FSKEY_ESC)
			break;
		timeInc = (double)(passedTime) * 0.001;
		clocktime += timeInc;

		/////////// update physics /////////////////
	/*	updatePrecisePhysics(realBall, timeInc);
		updateEulerEPhysics(eulerEBall, timeInc);
		updateEulerSEPhysics(eulerSEBall, timeInc);
		updateVerletPhysics(verletBall, timeInc);
		updateRK4Physics(RK4Ball, timeInc);
		*/
		/////////////////////////////////////////

		//// pseudo code update to fix time increment problems:
		float actualTimeInc = timeInc;
		const float maxPossible_dt = 0.002f;
		int numOfIteration = stimeInc / maxPossible_dt + 1;
		//if(numOfIteration != 0)
		actualTimeInc = stimeInc / numOfIteration; 

		for(int i=0; i<numOfIteration; i++)
		{
			/////////// update physics /////////////////
			updatePrecisePhysics(realBall, actualTimeInc);
			updateEulerEPhysics(eulerEBall, actualTimeInc);
			updateEulerSEPhysics(eulerSEBall, actualTimeInc);
			updateVerletPhysics(verletBall, actualTimeInc, i);
			updateRK4Physics(RK4Ball, actualTimeInc);
			/////////////////////////////////////////
		}
		//	UpdatePhysics(actualTimeInc);

		// now do rendering:
		renderScene();

		////// update time lapse /////////////////
		passedTime = FsPassedTime(); // Making it up to 50fps
		int timediff = stimeSpan-passedTime;
	//	printf("\ntimeInc=%f, passedTime=%d, timediff=%d", timeInc, passedTime, timediff);
		while(timediff > 5)
		{
			FsSleep(5);
			passedTime=FsPassedTime(); // Making it up to 50fps
			timediff = stimeSpan-passedTime;
	//		printf("--passedTime=%d, timediff=%d", passedTime, timediff);
		}
		passedTime=FsPassedTime(true); // Making it up to 50fps
	
	//	printf("--passedTime=%d, timediff=%d", passedTime, timediff);
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

		glClearColor(0.0,0.0,0.0,0.0);
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
	FsOpenWindow(32, 32, winWidth, winHeight, 1); // 800x600 pixels, useDoubleBuffer=1

	int listBase;
	listBase=glGenLists(256);
	YsGlUseFontBitmap8x12(listBase);
	glListBase(listBase);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDepthFunc(GL_ALWAYS);

	while(1)
	{
		menu=Menu();
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


