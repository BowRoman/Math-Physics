#include "Integration.h"

IntegrationHandler::IntegrationHandler()
{
}

IntegrationHandler::~IntegrationHandler()
{
}

void IntegrationHandler::EulerE(Vector3d<float>& pos, Vector3d<float>& curVel, Vector3d<float> accel, double dTime)
{
	pos += curVel * dTime;
	curVel += accel * dTime;
}

void IntegrationHandler::EulerSE(Vector3d<float>& pos, Vector3d<float>& curVel, Vector3d<float> accel, double dTime)
{
	curVel += accel * dTime;
	pos += curVel * dTime;
}

void IntegrationHandler::Verlet(Vector3d<float>& pos, Vector3d<float>& lastPos, Vector3d<float> accel, double dTime)
{
	Vector3d<float> curPos = pos;
	pos = curPos + (curPos - lastPos) + (accel*(float)dTime*(float)dTime);
	lastPos = curPos;
}

void IntegrationHandler::RK4(Vector3d<float>& pos, Vector3d<float>& curVel, Vector3d<float> accel, double dTime)
{
	State state;
	state.pos = pos;
	state.vel = curVel;

	first = Derive(state, accel, 0, State());
	sec = Derive(state, accel, dTime*0.5, first);
	third = Derive(state, accel, dTime*0.5, sec);
	fourth = Derive(state, accel, dTime, third);

	float oneSixth = 1.0f / 6.0f;
	
	Vector3d<float> dxdt = oneSixth * (first.pos + 2.0f*(sec.pos + third.pos) + fourth.pos);

	Vector3d<float> dvdt = oneSixth * (first.vel + 2.0f*(sec.vel + third.vel) + fourth.vel);

	pos += (dxdt*dTime);
	curVel += (dvdt*dTime);
}

Vector3d<float> IntegrationHandler::MultipyVecs(const Vector3d<float> vec1, const Vector3d<float> vec2)
{
	Vector3d<float> product;
	product.x = vec1.x * vec2.x;
	product.y = vec1.y * vec2.y;
	product.z = vec1.z * vec2.z;

	return product;
}

State IntegrationHandler::Derive(State& initial, Vector3d<float> accel, double dTime, const State givenDer)
{
	State state;
	state.pos = initial.pos + givenDer.pos*(float)dTime;
	state.vel = initial.vel + givenDer.vel*(float)dTime;

	State deriv;
	deriv.pos = state.vel;
	deriv.vel = MultipyVecs(accel,state.pos) /*- state.vel*/;

	return deriv;
}
