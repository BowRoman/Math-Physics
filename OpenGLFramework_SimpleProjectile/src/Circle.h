#pragma once
#define _USE_MATH_DEFINES
#include <math.h>
#include "vector3d.h"
#ifndef MACOSX
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif


struct Circle3D
{
	Vector3d<float> cPos, lPos, iPos, cV, iV;
	float radius;
	int red, green, blue;

	float mass;
	void set(double rad, Vector3d<float> pos, Vector3d<float> vel, float m, int r, int g, int b)
	{
		radius = rad;
		cPos = lPos = iPos = pos;
		mass = m;
		cV = iV = vel;
		red = r;
		green = g;
		blue = b;
	}

	// draws a hollow circle centered at (cx, cy) and with radius r, using num_segments triangle segments.
	// It rotates the circle by iAngle as well.
	void DrawCircle(double angle, int num_segments, int r = -1, int g = -1, int b = -1)
	{
		double theta = 2. * M_PI / double(num_segments);
		double c = cos(theta);//precalculate the sine and cosine
		double s = sin(theta);

		double x = radius;//we start at angle = 0 
		double y = 0.;
		double t = x;
		x = cos(angle) * x - sin(angle) * y;
		y = sin(angle) * t + cos(angle) * y;

		r = r < 0 ? red : r;
		g = g < 0 ? green : g;
		b = b < 0 ? blue : b;

		glLineWidth(2);
		glBegin(GL_LINE_LOOP);
		glColor3ub(r, g, b);
		for (int i = 0; i < num_segments; i++)
		{
			glVertex2d(x + cPos.x, y + cPos.y);

			//apply the rotation matrix
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;
		}
		glEnd();
	}

	// draws a solid circle centered at (cx, cy) and with radius r, using num_segments triangle segments.
	// It rotates the circle by iAngle as well.
	void DrawSolidCircle(double angle, int num_segments)
	{
		double theta = 2. * M_PI / double(num_segments);
		double c = cos(theta);//precalculate the sine and cosine
		double s = sin(theta);

		double x = radius;//we start at angle = 0 
		double y = 0;
		double t = x;
		double sx = cos(angle) * x - sin(angle) * y;
		double sy = sin(angle) * t + cos(angle) * y;
		x = sx; y = sy;

		glLineWidth(2);
		glBegin(GL_TRIANGLE_FAN);
		glColor3ub(red, green, blue);
		glVertex2d(cPos.x, cPos.y);
		for (int i = 0; i < num_segments; i++)
		{
			glVertex2d(x + cPos.x, y + cPos.y);//output vertex 
									   //apply the rotation matrix
			t = x;
			x = c * x - s * y;
			y = s * t + c * y;
		}

		glVertex2d(sx + cPos.x, sy + cPos.y);
		glEnd();
	}
};
