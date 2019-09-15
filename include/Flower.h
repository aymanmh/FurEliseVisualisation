#pragma once
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/cairo/Cairo.h"
#include "opensimplexnois.h"

using namespace ci;
using namespace std;

const int screenWidth_g = 1280;
const int screenHeight_g = 720;

class Petal
{
public:
	Petal(float petalAngle, float radius_m, float petalOutsideRadius_m, float petalInsideRadius_m, Color color);
	~Petal();
	void draw(cairo::Context& ctx);
	void update(OpenSimplexNoise& simplexNoise_m, vec2& orgLoc);
	bool isDestroyed_m = false;
private:
	float petalAngle_m, angle1_m, angle2_m;
	vec2 outsideCircleCenter_m, insideCircleCenter_m;
	vec2 direction_m = vec2(0, 0);
	float radius_m, petalOutsideRadius_m, petalInsideRadius_m;
	Color color_m;
	float xoffest = 0.001f;
	//float xdis_m = 0.f;
};

class Flower
{
public:
	Flower(int x, float radius, int numPetals, float hue);
	~Flower();

	void draw(cairo::Context & ctx);

	void destroy();

public:
	std::vector<unique_ptr<Petal>> petals_m;
	double orginalXPos_m;
	double ypos_m = screenHeight_g - 229;
	//float xoffest = 0.00001f;
	float xoffest_m = 0.1f;
	bool isVisible_m = true;
	OpenSimplexNoise simplexNoise_m;
	float displacment_m = 0;
	ColorA		color_m;
	float hue_m;
	float saturation_m = 1.f;
	float alpha_m = 1.f;
	float disappearanceRate_m;
	bool isDestroyed_m = false;
	int velocity_m = 1;
	static Rand rand_m;

};
