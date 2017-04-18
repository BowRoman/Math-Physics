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

Vector3d<float> IntegrationHandler::Acceleration(const Vector3d<float> vec1, State state)
{
	const Vector3d<float> k = vec1;
	const float b = 1;

	Vector3d<float> product;
	//product.x = (k.x * state.pos.x) - (b * state.vel.x);
	product.y = (k.y * state.pos.y) - (b * state.vel.y);
	//product.z = (k.x * state.pos.z) - (b * state.vel.z);

	return product;
}

State IntegrationHandler::Derive(State& initial, Vector3d<float> accel, double dTime, const State givenDer)
{
	State state;
	state.pos = initial.pos + givenDer.pos*(float)dTime;
	state.vel = initial.vel + givenDer.vel*(float)dTime;

	State deriv;
	deriv.pos = state.vel;
	deriv.vel = Acceleration(accel,state) /*- state.vel*/;

	return deriv;
}
