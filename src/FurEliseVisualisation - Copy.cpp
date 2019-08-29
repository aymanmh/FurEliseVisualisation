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

class Flower {
public:
	Flower(vec2 loc, float radius, float petalOutsideRadius, float petalInsideRadius, int numPetals, ColorA color)
		: mLoc(loc), mRadius(radius), mPetalOutsideRadius(petalOutsideRadius), mPetalInsideRadius(petalInsideRadius), mNumPetals(numPetals), mColor(color)
	{}





private:
	vec2		mLoc;

};

class Flower
{
public:
	Flower(double x);
	~Flower();

public:
	double xpos;
	double ypos = getWindowHeight() - 229;
	double r;
	double a1;
	double a2;
	//float xoffest = 0.00001f;
	float xoffest = 0.1f;
	bool isVisible_m = true;
	OpenSimplexNoise simplexNoise_m;
	float displacment = 0;
	float		mRadius, mPetalOutsideRadius, mPetalInsideRadius;
	int			mNumPetals;
	ColorA		mColor;
	void makePath(cairo::Context &ctx) const
	{
		for (int petal = 0; petal < mNumPetals; ++petal) {
			ctx.newSubPath();
			float petalAngle = (petal / (float)mNumPetals) * 2 * M_PI;
			vec2 outsideCircleCenter = mLoc + vec2(1, 0) * (float)cos(petalAngle) * mRadius + vec2(0, 1) * (float)sin(petalAngle) * mRadius;
			vec2 insideCircleCenter = mLoc + vec2(1, 0) * (float)cos(petalAngle) * mPetalInsideRadius + vec2(0, 1) * (float)sin(petalAngle) * mPetalInsideRadius;
			ctx.arc(outsideCircleCenter, mPetalOutsideRadius, petalAngle + M_PI / 2 + M_PI, petalAngle + M_PI / 2);
			ctx.arc(insideCircleCenter, mPetalInsideRadius, petalAngle + M_PI / 2, petalAngle + M_PI / 2 + M_PI);
			ctx.closePath();
		}
	}

	void draw(cairo::Context &ctx) const
	{
		// draw the solid petals
		ctx.setSource(mColor);
		makePath(ctx);
		ctx.fill();

		// draw the petal outlines
		ctx.setSource(mColor * 0.8f);
		makePath(ctx);
		ctx.stroke();
	};
};

Flower::Flower(double x)
{
	xpos = x;
	r = 20.0;
	a1 = 0.0;
	a2 = 2 * M_PI;
	
}

Flower::~Flower()
{
}

int screenWidth_g = 1280;
int screenHeight_g = 720;

class CairoProjectApp : public App {
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
	std::vector<unique_ptr<Flower>> circles_m;
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

void CairoProjectApp::setup()
{
	setWindowSize(screenWidth_g, screenHeight_g);
	Rand::randSeed(uint32_t(time(NULL)));
	cairoSurface_m = std::make_unique<cairo::SurfaceImage>(screenWidth_g, screenHeight_g, true);

	piano_m = gl::Texture::create(loadImage(loadAsset("piano.png")));

	populateKeyPos();

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

void CairoProjectApp::mouseDown( MouseEvent event )
{
	//if (event.isRight())
		//destoryFlower();
	//else
	{
		//circles_m.emplace_back(make_unique<Flower>(event.getX()));
		console() << event.getX() << endl;
	}
}

void CairoProjectApp::update()
{


}

void CairoProjectApp::renderScene(cairo::Context &ctx)
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


	for (auto& circle : circles_m)
	{
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
	}

	//remove invisible flowers
	circles_m.erase(std::remove_if(circles_m.begin(), circles_m.end(), [](const std::unique_ptr<Flower>& flower) {return flower->isVisible_m == false; }), circles_m.end());

}

void CairoProjectApp::draw()
{
	//gl::clear( Color( 0, 0, 0 ) ); 
	cairo::Context ctx(*cairoSurface_m);
	//cairo::Context ctx(cairo::createWindowSurface());
	renderScene(ctx);
	gl::Texture2dRef texture = gl::Texture::create(cairoSurface_m->getSurface());
	gl::draw(texture);
	gl::draw(piano_m,vec2(0,491));
	
}

void CairoProjectApp::destoryFlower()
{
	if (circles_m.empty())
		return;

	int index = randInt(0, circles_m.size());
	circles_m.erase(circles_m.begin() + index);

}

void CairoProjectApp::midiListener(midi::Message msg)
{
	// This will be called on on the main thread and
	// safe to use with update and draw.

	switch (msg.status)
	{
	case MIDI_NOTE_ON:
		if (msg.pitch >= 60)
			circles_m.emplace_back(make_unique<Flower>(keyPos_m[msg.pitch]));
		else
			destoryFlower();
		console() << msg.pitch <<endl;
	default:
		break;
	}


}
void CairoProjectApp::populateKeyPos()
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

CINDER_APP( CairoProjectApp, RendererGl)
