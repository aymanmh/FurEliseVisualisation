#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Perlin.h"
#include "cinder/Rand.h"
#include "cinder/Path2d.h"
#include "cinder/cairo/Cairo.h"
#include "opensimplexnois.h"
#include <unordered_map>
#include "MidiHeaders.h"

FILE _iob[] = { *stdin, *stdout, *stderr };
 
extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}

using namespace ci;
using namespace ci::app;
using namespace std;


class Petal
{
public:
	Petal(float petalAngle, float radius_m, float petalOutsideRadius_m, float petalInsideRadius_m, Color color);
	~Petal();
	void draw(cairo::Context& ctx);
	void update(OpenSimplexNoise& simplexNoise_m, vec2& orgLoc);
	bool isDestroyed_m = false;
private:
	float petalAngle_m;
	float angle1_m;
	float angle2_m;
	vec2 outsideCircleCenter_m;
	vec2 insideCircleCenter_m;
	vec2 direction_m = vec2(0,0);
	float radius_m, petalOutsideRadius_m, petalInsideRadius_m;
	Color color_m;
	float xoffest = 0.001f;
	float xdis_m = 0.f;
};

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
		//float x = simplexNoise.Evaluate(orgLoc.x, xoffest);
		//float xm = lmap<float>(x, -1.f, 1.f, 0, getWindowWidth());
		////float ym = lmap<float>(y, -1.f, 1.f, 0, getWindowHeight());
		//if (xdis_m == 0)
		//{
		//	xdis_m = orgLoc.x - xm;
		//	//	ydis = orgpos.y - ym;
		//}
		////console() << x <<"---" << y << endl;
		//outsideCircleCenter_m.x = xm + xdis_m;
		outsideCircleCenter_m -= direction_m;
		insideCircleCenter_m -= direction_m;

		xoffest += 0.0009f;
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
class Flower
{
public:
	Flower(int x, float radius, float petalOutsideRadius, float petalInsideRadius, int numPetals, ColorA color) //: orginalXPos_m(x)
		: orginalXPos_m(x),  color_m(color)
	{
		disappearanceRate = randFloat(.01f, 0.05f);

		petals_m.reserve(numPetals - 1);
		for (size_t i = 0; i < numPetals; i++)
		{
			float petalAngle = (i / (float)numPetals) * 2 * M_PI;
			petals_m.emplace_back(make_unique<Petal>(petalAngle, radius, petalOutsideRadius, petalInsideRadius, color));
		}
	}
	~Flower();

public:
	std::vector<unique_ptr<Petal>> petals_m;
	double orginalXPos_m;
	double ypos_m = getWindowHeight() - 229;
	//float xoffest = 0.00001f;
	float xoffest_m = 0.1f;
	bool isVisible_m = true;
	OpenSimplexNoise simplexNoise_m;
	float displacment_m = 0;
	ColorA		color_m;

	float alpha_m = 1.f;
	float disappearanceRate;
	bool isDestroyed_m = false;
	void destroy()
	{
		isDestroyed_m = true;
		for (auto& petal : petals_m)
			petal->isDestroyed_m = true;

	}

	void draw(cairo::Context &ctx)
	{
		if (ypos_m < -30.0 || alpha_m <= 0.0f)

		{
			isVisible_m = false;
			return;
		}

		vec2 loc;
		if (isDestroyed_m)
		{
			alpha_m -= disappearanceRate;
		}
		else
		{
			float x, xm = 0;

			x = simplexNoise_m.Evaluate(orginalXPos_m, xoffest_m);
			{
				xm = lmap<float>(x, -1.f, 1.f, 0, getWindowWidth());
				//float xm = lmap<float>(x, -1.f, 1.f, 0, circle->xpos*1.5f);
				//xm = lmap<float>(x, -1.f, 1.f, -10.0f, -20.0f);
				xoffest_m += 0.001f;
			}
			if (ypos_m == getWindowHeight() - 229)
			{
				displacment_m = orginalXPos_m - xm;
			}
			loc = vec2(xm + displacment_m, ypos_m--);
		}						
		
		for (auto& petal : petals_m)
		{
			// update the petals location
			petal->update(simplexNoise_m, loc);

			// draw the solid petals
			ctx.setSource(ColorAf(color_m,alpha_m));
			petal->draw(ctx);
			ctx.fill();

			// draw the petal outlines
			ctx.setSource(ColorAf(color_m * 0.8f, alpha_m));
			petal->draw(ctx);
			ctx.stroke();
		}		
	};
};

Flower::~Flower()
{
}

int screenWidth_g = 1280;
int screenHeight_g = 720;

class FurEliseVisualisationApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void renderScene(cairo::Context &ctx);
	void draw() override;
	Perlin					mPerlin;
	vec2 position;
	OpenSimplexNoise simplexNoise_m;
	std::unique_ptr<cairo::SurfaceImage> cairoSurface_m;
	gl::Texture2dRef piano_m;
	std::vector<unique_ptr<Flower>> flowers_m;
	void destoryFlower();
	int middleCPos_m = 622;
	int keyDisplacement = 14;
	std::unordered_map<int, int> keyPos_m;
	//std::unordered_map<unsigned char, int> keyPos_m;
	void midiListener(midi::Message msg);
	void populateKeyPos();
	midi::Input mInput;
	vector <int> notes;
	vector <int> cc;

};

void FurEliseVisualisationApp::setup()
{
	setWindowSize(screenWidth_g, screenHeight_g);
	Rand::randSeed(uint32_t(time(NULL)));
	cairoSurface_m = std::make_unique<cairo::SurfaceImage>(screenWidth_g, screenHeight_g, true);

	piano_m = gl::Texture::create(loadImage(loadAsset("piano.png")));

	populateKeyPos();

	/*float outerRadius = (2 * M_PI * 20) / 3 / 2 * randFloat(0.9f, 1.0f);
	float innerRadius = outerRadius * randFloat(0.2f, 0.4f);
	flowers_m.emplace_back(make_unique<Flower>(200, 20, outerRadius, innerRadius, 2, ColorA(CM_HSV, randFloat(), 1, 1, 0.65f)));


	return;*/

	mInput.listPorts();
	console() << "NUMBER OF PORTS: " << mInput.getNumPorts() << endl;

	if (mInput.getNumPorts() > 0)
	{
		for (int i = 0; i < mInput.getNumPorts(); i++)
			console() << mInput.getPortName(i) << endl;

		mInput.openPort(0);

		// Connect midi signal to our callback function
		// This connects to our main thread
		mInput.midiSignal.connect([this](midi::Message msg) { midiListener(msg); });

		// Optionally, this connects directly to the midi thread
		//mInput.midiThreadSignal.connect([this](midi::Message msg) { midiThreadListener(msg); });
	}
}

void FurEliseVisualisationApp::mouseDown( MouseEvent event )
{
	if (event.isRight())
		destoryFlower();
	else
	{
	float radius = randFloat(10, 35);
	int numPetals = randInt(5, 20);
	float outerRadius = (2 * M_PI * radius) / numPetals / 2 * randFloat(0.9f, 1.0f);
	float innerRadius = outerRadius * randFloat(0.2f, 0.4f);
	//mFlowers.push_back(Flower));
	flowers_m.emplace_back(make_unique<Flower>(event.getX(), radius, outerRadius, innerRadius, numPetals, ColorA(CM_HSV, randFloat(), 1, 1, 0.65f)));
	//	console() << event.getX() << endl;
	}
}

void FurEliseVisualisationApp::update()
{


}

void FurEliseVisualisationApp::renderScene(cairo::Context &ctx)
{
	std::vector<double> dashPatter = { 5,10.0,2 };
	cairo::GradientRadial radialGrad(getWindowCenter(), 0, getWindowCenter(), getWindowWidth());
	radialGrad.addColorStop(0, Color(1, 1, 1));
	radialGrad.addColorStop(1, Color(0.6, 0.6, 0.6));
	ctx.setSource(radialGrad);
	ctx.paint();
	
	ctx.setSource(Color(0.7f,0.3f,0.3f));
	
	ctx.setLineWidth(6.5);

	ctx.setLineCap(cinder::cairo::LINE_CAP_ROUND);//LINE_CAP_ROUND LINE_CAP_BUTT
	ctx.setLineJoin(cinder::cairo::LINE_JOIN_BEVEL);//LINE_CAP_ROUND LINE_CAP_BUTT


	for (auto& flower : flowers_m)
	{
		flower->draw(ctx);
		/*
		if (circle->ypos < -30.0)
		{
			circle->isVisible_m = false;
			continue;
		}
		float x, xm = 0;
	
			x = simplexNoise_m.Evaluate(circle->xpos, circle->xoffest);
		{
			xm = lmap<float>(x, -1.f, 1.f, 0, getWindowWidth());
			//float xm = lmap<float>(x, -1.f, 1.f, 0, circle->xpos*1.5f);
			//xm = lmap<float>(x, -1.f, 1.f, -10.0f, -20.0f);
			circle->xoffest += 0.001f;
		}
		if (circle->ypos == getWindowHeight() - 229)		
		{
			circle->displacment = circle->xpos - xm;
		}

		ctx.newSubPath();
		//circle->xpos = xm;
		ctx.arc(xm + circle->displacment, circle->ypos--, circle->r, circle->a1, circle->a2);
		//circle->xpos = xm;
		ctx.setSource(Color(0.3f, 0.7f, 0.25f));

		ctx.fill();

		ctx.closePath();

		ctx.stroke();
		*/
	}

	//remove invisible flowers
	flowers_m.erase(std::remove_if(flowers_m.begin(), flowers_m.end(), [](const std::unique_ptr<Flower>& flower) {return flower->isVisible_m == false; }), flowers_m.end());

}

void FurEliseVisualisationApp::draw()
{
	//gl::clear( Color( 0, 0, 0 ) ); 
	cairo::Context ctx(*cairoSurface_m);
	//cairo::Context ctx(cairo::createWindowSurface());
	renderScene(ctx);
	gl::Texture2dRef texture = gl::Texture::create(cairoSurface_m->getSurface());
	gl::draw(texture);
	gl::draw(piano_m,vec2(0,491));
	
}

void FurEliseVisualisationApp::destoryFlower()
{
	if (flowers_m.empty())
		return;

	int index = randInt(0, flowers_m.size());
	flowers_m.at(index)->destroy();

	//circles_m.erase(circles_m.begin() + index);

}

void FurEliseVisualisationApp::midiListener(midi::Message msg)
{
	// This will be called on on the main thread and
	// safe to use with update and draw.

	switch (msg.status)
	{
	case MIDI_NOTE_ON:
		if (msg.pitch >= 60)
		{
			float radius = randFloat(10, 35);
			int numPetals = randInt(4, 15);
			float outerRadius = (2 * M_PI * radius) / numPetals / 2 * randFloat(0.9f, 1.0f);
			float innerRadius = outerRadius * randFloat(0.2f, 0.4f);
			//mFlowers.push_back(Flower));
			flowers_m.emplace_back(make_unique<Flower>(keyPos_m[msg.pitch], radius, outerRadius, innerRadius, numPetals, ColorA(CM_HSV, randFloat(), 1, 1, 0.65f)));
			//circles_m.emplace_back(make_unique<Flower>(keyPos_m[msg.pitch]));
		}
		else
			destoryFlower();
		console() << msg.pitch <<endl;
	default:
		break;
	}


}
void FurEliseVisualisationApp::populateKeyPos()
{
	//unsigned char lowestKey = 0x2d;
	int lowestKey = 33;

	keyPos_m[lowestKey++] = 30;
	keyPos_m[lowestKey++] = 53;
	keyPos_m[lowestKey++] = 75;
	keyPos_m[lowestKey++] = 96;
	keyPos_m[lowestKey++] = 117;
	keyPos_m[lowestKey++] = 138;
	keyPos_m[lowestKey++] = 159;
	keyPos_m[lowestKey++] = 185;
	keyPos_m[lowestKey++] = 204;
	keyPos_m[lowestKey++] = 224;
	keyPos_m[lowestKey++] = 246;
	keyPos_m[lowestKey++] = 268;
	keyPos_m[lowestKey++] = 290;
	keyPos_m[lowestKey++] = 310;
	keyPos_m[lowestKey++] = 331;
	keyPos_m[lowestKey++] = 355;
	keyPos_m[lowestKey++] = 374;
	keyPos_m[lowestKey++] = 398;
	keyPos_m[lowestKey++] = 417;
	keyPos_m[lowestKey++] = 437;
	keyPos_m[lowestKey++] = 463;
	keyPos_m[lowestKey++] = 481;
	keyPos_m[lowestKey++] = 502;
	keyPos_m[lowestKey++] = 525;
	keyPos_m[lowestKey++] = 547;
	keyPos_m[lowestKey++] = 564;
	keyPos_m[lowestKey++] = 586;
	keyPos_m[lowestKey++] = 607;
	keyPos_m[lowestKey++] = 631;
	keyPos_m[lowestKey++] = 652;
	keyPos_m[lowestKey++] = 674;
	keyPos_m[lowestKey++] = 692;
	keyPos_m[lowestKey++] = 715;
	keyPos_m[lowestKey++] = 736;
	keyPos_m[lowestKey++] = 758;
	keyPos_m[lowestKey++] = 778;
	keyPos_m[lowestKey++] = 797;
	keyPos_m[lowestKey++] = 819;
	keyPos_m[lowestKey++] = 839;
	keyPos_m[lowestKey++] = 861;
	keyPos_m[lowestKey++] = 883;
	keyPos_m[lowestKey++] = 903;
	keyPos_m[lowestKey++] = 928;
	keyPos_m[lowestKey++] = 945;
	keyPos_m[lowestKey++] = 970;
	keyPos_m[lowestKey++] = 991;
	keyPos_m[lowestKey++] = 1010;
	keyPos_m[lowestKey++] = 1032;
	keyPos_m[lowestKey++] = 1052;
	keyPos_m[lowestKey++] = 1073;
	keyPos_m[lowestKey++] = 1094;
	keyPos_m[lowestKey++] = 1114;
	keyPos_m[lowestKey++] = 1137;
	keyPos_m[lowestKey++] = 1158;
	keyPos_m[lowestKey++] = 1182;
	keyPos_m[lowestKey++] = 1199;
	keyPos_m[lowestKey++] = 1218;
	keyPos_m[lowestKey++] = 1241;
}

CINDER_APP( FurEliseVisualisationApp, RendererGl)
