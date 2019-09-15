#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include <unordered_map>
#include "MidiHeaders.h"
#include "Flower.h"

FILE _iob[] = { *stdin, *stdout, *stderr };
 
extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}


using namespace ci::app;

class FurEliseVisualisationApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void renderScene(cairo::Context &ctx);
	void draw() override;
	void midiListener(midi::Message msg);

private:
	void destoryFlower();
	void populateKeyPos();
	void keyDown(KeyEvent event) override;
	std::unique_ptr<cairo::SurfaceImage> cairoSurface_m;
	gl::Texture2dRef piano_m;
	std::vector<unique_ptr<Flower>> flowers_m;
	const int middleCPos_m = 622;
	std::unordered_map<int, int> keyPos_m;
	midi::Input Input_m;
	int colorCounter_m = 0;
	int lastNote_m = -1;
};

void FurEliseVisualisationApp::setup()
{
	setWindowSize(screenWidth_g, screenHeight_g);
	Rand::randSeed(uint32_t(time(NULL)));
	cairoSurface_m = std::make_unique<cairo::SurfaceImage>(screenWidth_g, screenHeight_g, true);

	piano_m = gl::Texture::create(loadImage(loadAsset("piano.png")));

	populateKeyPos();

	Input_m.listPorts();
	console() << "NUMBER OF PORTS: " << Input_m.getNumPorts() << endl;

	if (Input_m.getNumPorts() > 0)
	{
		for (int i = 0; i < Input_m.getNumPorts(); i++)
			console() << Input_m.getPortName(i) << endl;

		Input_m.openPort(0);

		// Connect midi signal to our callback function
		// This connects to our main thread
		Input_m.midiSignal.connect([this](midi::Message msg) { midiListener(msg); });

		// Optionally, this connects directly to the midi thread
		//mInput.midiThreadSignal.connect([this](midi::Message msg) { midiThreadListener(msg); });
	}
}

void FurEliseVisualisationApp::mouseDown(MouseEvent event)
{
	if (event.isRight())
		destoryFlower();
	else
	{
		float radius = randFloat(5, 40);
		int numPetals;
		if (radius <= 8)
			numPetals = randInt(3, 6);
		else if (radius < 30)
			numPetals = randInt(3, 12);
		else
			numPetals = randInt(6, 20);

		flowers_m.emplace_back(make_unique<Flower>(event.getX(), radius, numPetals, lmap<float>(colorCounter_m % 256, 0, 255, 0.f, 1.f)));
		colorCounter_m += 4;
	}
}

void FurEliseVisualisationApp::update()
{
}

void FurEliseVisualisationApp::renderScene(cairo::Context &ctx)
{
	//cairo::GradientLinear radialGrad(getWindowCenter(), 0, getWindowCenter(), getWindowWidth());
	//cairo::GradientLinear radialLinear(0, 0, getWindowHeight(), getWindowWidth());
	cairo::GradientLinear radialLinear(getWindowHeight(), getWindowWidth() / 2, getWindowWidth() / 2, 0);
	//radialLinear.addColorStop(0, Color(.6, .6, .6));
	radialLinear.addColorStop(0, Color(0.0, 0.455, 0.90)); //52.9, 80.8, 92.2
	radialLinear.addColorStop(1, Color(0.0, 0.355, 0.85)); //52.9, 80.8, 92.2
	ctx.setSource(radialLinear);
	ctx.paint();
	
	//ctx.setSource(Color(0.7f,0.3f,0.3f));
	
	ctx.setLineWidth(5);

	ctx.setLineCap(cinder::cairo::LINE_CAP_ROUND);
	ctx.setLineJoin(cinder::cairo::LINE_JOIN_BEVEL);


	for (auto& flower : flowers_m)
	{
		flower->draw(ctx);
	}

	//remove invisible/out of screen flowers
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
	//gl::draw(piano_m,vec2(0,491)); // for calibration
	
}

void FurEliseVisualisationApp::destoryFlower()
{
	if (flowers_m.empty())
		return;

	int index = randInt(0, flowers_m.size());
	while (flowers_m.at(index)->isDestroyed_m)//don't destory a flower that is being destroyed.
	{
		// if the all the remaining flowers are being destroyed, skip
		auto flower = std::find_if(flowers_m.begin(), flowers_m.end(), [](const std::unique_ptr<Flower>& flower) {return flower->isDestroyed_m == false; });
		if (flower == flowers_m.end())
			break;
		index = randInt(0, flowers_m.size());
	}

	flowers_m.at(index)->destroy();
}

void FurEliseVisualisationApp::keyDown(KeyEvent event)
{
	if (event.getChar() == 'c')
	{
		flowers_m.clear();
		colorCounter_m = 0;
		lastNote_m = -1;
	}
}
void FurEliseVisualisationApp::midiListener(midi::Message msg)
{
	// This will be called on on the main thread and
	// safe to use with update and draw.

	switch (msg.status)
	{
	case MIDI_NOTE_ON:
		if (msg.pitch >= 60) //middle C and above
		//if (msg.pitch >= lastNote_m) // going up the scale
		{
			float radius;
			if(msg.velocity < 40)
				radius = randFloat(5, 25);
			else
				radius = randFloat(20, 50);

			int numPetals;
			if (radius <= 8)
				numPetals = randInt(3, 6);
			else if (radius < 30)
				numPetals = randInt(3, 12);
			else
				numPetals = randInt(6, 20);

			flowers_m.emplace_back(make_unique<Flower>(keyPos_m[msg.pitch], radius, numPetals, lmap<float>(colorCounter_m % 256, 0, 255, 0.f, 1.f)));
			colorCounter_m += 3;
		}
		else
			destoryFlower();
		//console() << msg.velocity <<endl;
		lastNote_m = msg.pitch;
	default:
		break;
	}


}
void FurEliseVisualisationApp::populateKeyPos()
{
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
