#include <stdio.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <random>
#include <assert.h>
#include <map>
#include <iostream>

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
#include <vector>
#include <ctime>
#include <random>
//#include "vector2d.h"
#include "vector3d.h"
#include  "matrices.h"
using namespace std;

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

bool			keys[256];			// Array Used For The Keyboard Routine
float timeRange = 10.;
double gravity = 9.81; // m/s*s
double iAngle = M_PI / 3.; // projectile inclination angle in radian
double iSpeed = 25.0f;
int circleSections = 16;
const double angleInc = M_PI / 180.;

float eyeX = 25.0f, eyeY = 5.0f, eyeZ = 70.0f;

int winWidth = 800;
int winHeight = 600;
const double ratio = (float)winHeight / (float)winWidth;
const double WorldWidth = 120.0; // meter wide
const double WorldDepth = WorldWidth * ratio; // 
const double WorldHeight = 100.;
int width = 0, height = 0;

double radius = 1.;

static double clocktime = 0.f;
int framerate = 30;

bool checkWindowResize();

struct Object3D
{
	Vector3d<double> pos, vel, acc;
	int red, green, blue;

	double mass;
	void set(double x, double y, double z, double m, Vector3d<double> v, int r, int g, int b)
	{
		pos.x = x;
		pos.y = y;
		pos.z = z;
		vel = v;
		mass = m;
		green = r;
		red = g;
		blue = b;
	}

};
Object3D simBall;

void DrawCircle(float cx, float cy, float r, int num_segments) 
{ 
	float theta = 2.f * M_PI / float(num_segments); 
	float c = cosf(theta);//precalculate the sine and cosine
	float s = sinf(theta);
	float t;

	float x = r;//we start at angle = 0 
	float y = 0; 
    
	glBegin(GL_LINE_LOOP); 
	for(int ii = 0; ii < num_segments; ii++) 
	{ 
		glVertex2f(x + cx, y + cy);//output vertex 
        
		//apply the rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	} 
	glEnd(); 
}


///////////////////////////////////////////////////////////////
void initPhysics(double rad, double speed, double angle)
{
	clocktime = 0.f;
	double vx = speed * cos(angle);
	double vy = speed * sin(angle);
	double vz =0.f;
	double initX = rad;
	double inity = rad;
	double initz = WorldDepth/4.f;
	simBall.set(initX, inity, initz, 2, Vector3d<double>(vx, vy, vz), 128, 128, 0);
}

int PollKeys()
{
	FsPollDevice();
	int keyRead = FsInkey();
	keys[keyRead] = true;
	int ret = keyRead;
	switch (keyRead)
	{
	case FSKEY_S:
		ret = eStart;
		break;
	case FSKEY_ESC:
		ret = eStop;
		break;
	case FSKEY_UP:
		eyeY += 0.6;
		break;
	case FSKEY_DOWN:
		eyeY -= 0.6;
		break;
	case FSKEY_LEFT:
		eyeX -= 0.6;
		break;
	case FSKEY_RIGHT:
		eyeX += 0.6;
		break;
	case FSKEY_PAGEDOWN:
		iSpeed = max(iSpeed - 5.0, 0.0);
		break;
	case FSKEY_PAGEUP:
		iSpeed = min(iSpeed + 5.0, 100.);
		break;
	case FSKEY_D:
		iAngle = max(0., iAngle - angleInc);
		break;
	case FSKEY_U:
		iAngle = min(90.0, iAngle + angleInc);
		break;
	case FSKEY_I:
		eyeY = min(eyeY + 1.0, 100.0);
		break;
	case FSKEY_K:
		eyeY = max(eyeY - 1.0, 1.);
		break;
	}
	return ret;
}
//////////////////////////////////////////////////////////////////////////////////////////////
int Menu(void)
{
	int key = eIdle;
	FsGetWindowSize(width, height);
	glDisable(GL_DEPTH_TEST);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.5, (GLdouble)width - 0.5, (GLdouble)height - 0.5, -0.5, -1, 1);

	glClearColor(0.0,0.0,0.0,0.0);
	while (key != eStart && key != eStop)
	{
		key = PollKeys();
		if (key == eStop)
			return key;

		glClear(GL_COLOR_BUFFER_BIT);

		// printing UI message info
		glColor3f(1., 1., 1.);
		char msg[128];

		sprintf_s(msg, "Projectile Angle is %f degrees. Use Left/Right keys to change it!\n", iAngle*180. / M_PI);
		glRasterPos2i(32,64);
		glCallLists(strlen(msg),GL_UNSIGNED_BYTE, msg);

		sprintf_s(msg, "Projectile speed is %f m/s. Use PageUp/PageDown keys to change it!\n", iSpeed);
		glRasterPos2i(32, 96);
		glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		sprintf_s(msg, "Camera height is %f. Use I/K keys to change it!\n", eyeY);
		glRasterPos2i(32, 128);
		glCallLists(strlen(msg), GL_UNSIGNED_BYTE, msg);

		const char *msg1 = "S.....Start Game";
		const char *msg2="ESC...Exit";
		glRasterPos2i(32, 160);
		glCallLists(strlen(msg1),GL_UNSIGNED_BYTE, msg1);
		glRasterPos2i(32,192);
		glCallLists(strlen(msg2),GL_UNSIGNED_BYTE, msg2);

		FsSwapBuffers();
		FsSleep(10);
	}

	initPhysics(radius, iSpeed, iAngle);
	return key;
}

const int timeSpan = 33; // milliseconds
const double timeInc = (double)timeSpan * 0.001; // time increment in seconds
const double timeInc2 = timeInc * timeInc;

///////////////////////////////////////////////////////////////////
int Game(void)
{
	FsPassedTime();

	double ballX,ballY,ballVx,ballVy,ballX_1, ballY_1;
	int wid,hei;

	FsGetWindowSize(wid, hei);
	ballX=35.;
	ballY=(double)hei-35.;
	ballVx = iSpeed * cosf(iAngle);
	ballVy = -iSpeed * sinf(iAngle);
	ballX_1 = ballX - ballVx * timeInc;
	ballY_1 = ballY - ballVy * timeInc + 0.5 * gravity * timeInc2;
	double ballX_N, ballY_N;
	while(1)
	{
		int lb,mb,rb,mx,my;
		int passedTime;

		FsPollDevice();
		FsGetMouseState(lb,mb,rb,mx,my);
		FsGetWindowSize(wid,hei);

		//////////// your physics goes here (Verlet integration method) //////////////////////////
		/////////////////////////////////////////////////////////////

		if(ballX<0 && ballVx<0)
		{
			ballX=-ballX;
			ballVx=-ballVx;
			ballX_1 = ballX - ballVx * timeInc;
		}
		if(ballY<0  && ballVy<0)
		{
			ballY=-ballY;
			ballVy=-ballVy;
			ballY_1 = ballY - ballVy * timeInc;
		}
		if(ballX>wid && ballVx>0)
		{
			ballX=wid-(ballX-wid);
			ballVx=-ballVx;
			ballX_1 = ballX - ballVx * timeInc;
		}
		if(ballY>hei)
		{
	//		ballY=(double)hei-35.;
			ballY=(double)hei-(ballY-(double)hei);
			ballVy=-ballVy;
			ballY_1 = ballY - ballVy * timeInc;
		}

		ballX_N = 2. * ballX - ballX_1;
		ballY_N = 2. * ballY - ballY_1 + timeInc2 * gravity;

		glViewport(0,0,wid,hei);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-0.5,(GLdouble)wid-0.5,(GLdouble)hei-0.5,-0.5,-1,1);

		glClearColor(1.0,0.0,0.0,0.0);
		glClear(GL_COLOR_BUFFER_BIT);
		
	//	glColor3ub(255,0,0);
	//	DrawCircle(100, 100, 30, 10);

		glBegin(GL_QUADS);

		glVertex2i(ballX_N-2,ballY_N-2);
		glVertex2i(ballX_N+2,ballY_N-2);
		glVertex2i(ballX_N+2,ballY_N+2);
		glVertex2i(ballX_N-2,ballY_N+2);

		glEnd();

		char str[256];
		sprintf(str,"Velocity:(%f, %f)",ballVx, ballVy);
		glRasterPos2i(32,40);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,str);
		sprintf(str,"Pos:(%f, %f)<--(%f, %f)",ballX, ballY, ballX_1,ballY_1);
		glRasterPos2i(32,25);
		glCallLists(strlen(str),GL_UNSIGNED_BYTE,str);

		FsSwapBuffers();

		ballX_1 = ballX;
		ballY_1 = ballY;
		ballX = ballX_N;
		ballY = ballY_N;

		passedTime=FsPassedTime(); // Making it up to 50fps
		FsSleep(timeSpan-passedTime);
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

int main(void)
{
	int menu;
	FsOpenWindow(32, 32, winWidth, winHeight, 1); 

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


