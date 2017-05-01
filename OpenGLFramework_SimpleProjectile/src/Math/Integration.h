#pragma once
#include "src\vector3d.h"

struct State
{
	Vector3d<float> pos;
	Vector3d<float> vel;
};

class IntegrationHandler
{
public:
	IntegrationHandler();
	~IntegrationHandler();

	void EulerE(Vector3d<float>& pos, Vector3d<float>& curVel, Vector3d<float> accel, double dTime);
	void EulerSE(Vector3d<float>& pos, Vector3d<float>& curVel, Vector3d<float> accel, double dTime);
	void Verlet(Vector3d<float>& pos, Vector3d<float>& lastPos, Vector3d<float>& curVel, Vector3d<float> accel, double dTime);
	void RK4(Vector3d<float>& pos, Vector3d<float>& curVel, Vector3d<float> accel, double dTime);

private:
	State first, sec, third, fourth;

	Vector3d<float> Acceleration(const Vector3d<float> vec1, State state);
	State Derive(State& initial, Vector3d<float> accel, double dTime, const State givenDer);
};

