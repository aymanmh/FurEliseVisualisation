#include "Flower.h"


Petal::Petal(float petalAngle, float radius, float petalOutsideRadius, float petalInsideRadius, Color color) :
	petalAngle_m(petalAngle), radius_m(radius), petalOutsideRadius_m(petalOutsideRadius), petalInsideRadius_m(petalInsideRadius), color_m(color)
{
	angle1_m = petalAngle + M_PI / 2 + M_PI;
	angle2_m = petalAngle + M_PI / 2;

	//find what direction the petal is pointing to, so when its detached, it moves in that direction.
	direction_m.x = cos((angle1_m + angle2_m) / 2.f);
	direction_m.y = sin((angle1_m + angle2_m) / 2.f);
}

Petal::~Petal()
{
}

void Petal::update(OpenSimplexNoise& simplexNoise, vec2 & orgLoc)
{
	if (isDestroyed_m)
	{
		//float x = simplexNoise.Evaluate(radius_m, xoffest);
		//float xm = lmap<float>(x, -1.f, 1.f, 0, getWindowWidth());
		////float ym = lmap<float>(y, -1.f, 1.f, 0, getWindowHeight());
		//if (xdis_m == 0)
		//{
		//	xdis_m = orgLoc.x - xm;
		//	//	ydis = orgpos.y - ym;
		//}
		////console() << x <<"---" << y << endl;
		//outsideCircleCenter_m.x = xm + xdis_m;
		//direction_m += vec2(x, x);
		outsideCircleCenter_m -= direction_m;
		insideCircleCenter_m -= direction_m;

		//outsideCircleCenter_m.y += x;
		//insideCircleCenter_m.y += x;


		//xoffest += 0.0009f;
	}
	else
	{
		outsideCircleCenter_m = orgLoc + vec2(1, 0) * (float)cos(petalAngle_m) * radius_m + vec2(0, 1) * (float)sin(petalAngle_m) * radius_m;
		insideCircleCenter_m = orgLoc + vec2(1, 0) * (float)cos(petalAngle_m) * petalInsideRadius_m + vec2(0, 1) * (float)sin(petalAngle_m) * petalInsideRadius_m;
	}
}

void Petal::draw(cairo::Context &ctx)
{
	ctx.newSubPath();

	ctx.arc(outsideCircleCenter_m, petalOutsideRadius_m, angle1_m, angle2_m);
	ctx.arc(insideCircleCenter_m, petalInsideRadius_m, angle2_m, angle1_m);
	ctx.closePath();
}


Flower::~Flower()
{
}

Flower::Flower(int x, float radius, int numPetals, float hue) //: orginalXPos_m(x)
	: orginalXPos_m(x), hue_m(hue)
{
	color_m = ColorA(CM_HSV, hue, 1.f, 1.f, 0.65f);

	disappearanceRate_m = randFloat(.01f, 0.005f);

	float petalOutsideRadius = (2 * M_PI * radius) / numPetals / 2 * randFloat(0.9f, 1.0f);
	float petalInsideRadius = petalOutsideRadius * randFloat(0.2f, 0.4f);
	petals_m.reserve(numPetals - 1);
	for (size_t i = 0; i < numPetals; i++)
	{
		float petalAngle = (i / (float)numPetals) * 2 * M_PI;
		petals_m.emplace_back(make_unique<Petal>(petalAngle, radius, petalOutsideRadius, petalInsideRadius, color_m));
	}

	//get a velocity that is mostly 1, rarley 2 (random Gaussian)
	float value = rand_m.randGaussian();
	velocity_m = (int)(abs(value) + 1);
	if (velocity_m >= 3) velocity_m = 1;
}

void Flower::draw(cairo::Context &ctx)
{
	if (ypos_m < -30.0 || alpha_m <= 0.0f)

	{
		isVisible_m = false;
		return;
	}

	vec2 loc;
	if (isDestroyed_m)
	{
		alpha_m -= disappearanceRate_m;
		saturation_m -= disappearanceRate_m;
	}
	else
	{
		float x, xm = 0;

		x = simplexNoise_m.Evaluate(orginalXPos_m, xoffest_m);
		{
			xm = lmap<float>(x, -1.f, 1.f, 0, screenWidth_g);
			xoffest_m += 0.001f;
		}
		if (ypos_m == screenHeight_g - 229)
		{
			displacment_m = orginalXPos_m - xm;
		}
		loc = vec2(xm + displacment_m, ypos_m);
		ypos_m -= velocity_m;
	}

	for (auto& petal : petals_m)
	{
		// update the petals location
		petal->update(simplexNoise_m, loc);

		// draw the solid petals
		ctx.setSource(ColorA(CM_HSV, hue_m, saturation_m, 1.f, alpha_m));
		petal->draw(ctx);
		ctx.fill();

		// draw the petal outlines
		ctx.setSource(ColorAf(ColorA(CM_HSV, hue_m, saturation_m, 1.f)* 0.8f, alpha_m));
		petal->draw(ctx);
		ctx.stroke();
	}
}

void Flower::destroy()
{
	isDestroyed_m = true;
	for (auto& petal : petals_m)
		petal->isDestroyed_m = true;

}