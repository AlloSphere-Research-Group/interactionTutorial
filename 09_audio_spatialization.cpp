
#include "al/core/app/al_App.hpp"
#include "al/core/graphics/al_Shapes.hpp"
#include "al/core/math/al_Random.hpp"

#include "al/core/sound/al_StereoPanner.hpp"
#include "al/core/sound/al_Vbap.hpp"
#include "al/core/sound/al_Dbap.hpp"
#include "al/core/sound/al_Ambisonics.hpp"


#include "al/util/ui/al_Parameter.hpp"
#include "al/util/ui/al_PresetSequencer.hpp"

#include "al/util/ui/al_SynthSequencer.hpp"
#include "al/util/ui/al_ControlGUI.hpp"

#include "Gamma/Oscillator.h"
#include "Gamma/Envelope.h"
#include "Gamma/Domain.h"

using namespace al;

/*
 * This tutorial shows how to use audio spatialization for SynthVoices and
 * PolySynth. It shows how each voice can have its own position in space
 * handled by a single spatializer object owned by the PolySynth.
*/

// Choose the spatializer type here:

//#define SpatializerType StereoPanner
#define SpatializerType Vbap
//#define SpatializerType Dbap
//#define SpatializerType AmbisonicsSpatializer

class MyVoice : public SynthVoice {
public:
    MyVoice() {
        addCone(mesh); // Prepare mesh to draw a cone

        mEnvelope.lengths(0.1f,  0.5f);
        mEnvelope.levels(0, 1, 0);
        mEnvelope.sustainPoint(1);
    }

    virtual void onProcess(AudioIOData &io) override {
        // First we will render the audio into bus 0
        // Note that we have allocated the bus on initialization by calling
        // channelsBus() for the AudioIO object.
        // We could run the spatializer in sample by sample mode here and
        // avoid using the bus altogether but this will be significantly
        // slower, so for efficiency, we render the output first to a bus
        // and then we use the spatializer on that buffer.

        while(io()) {
            io.bus(0) = mEnvelope() * mSource() * 0.05; // compute sample
        }
        // Then we will pass the bus buffer to the spatializer's
        // renderBuffer function. The spatializer will
        SpatializerType *spatializer = static_cast<SpatializerType *>(userData());
        spatializer->renderBuffer(io, mPose, io.busBuffer(0), io.framesPerBuffer());

        if (mEnvelope.done()) {
            free();
        }
    }

    virtual void onProcess(Graphics &g) {
        g.pushMatrix();
        g.translate(mPose.x(), mPose.y(), mPose.z());
        g.scale(mSize * mEnvelope.value());
        g.draw(mesh); // Draw the mesh
        g.popMatrix();
    }

    void set(float x, float y, float size, float frequency, float attackTime, float releaseTime) {
        mPose.pos(x, y, 0);
        mSize = size;
        mSource.freq(frequency);
        mEnvelope.lengths()[0] = attackTime;
        mEnvelope.lengths()[2] = releaseTime;
    }

    virtual bool setParamFields(float *pFields, int numFields) override {
        if (numFields != 6) { // Sanity check to make sure we are getting the right number of p-fields
            return false;
        }
        set(pFields[0], pFields[1], pFields[2], pFields[3], pFields[4], pFields[5]);
        return true;
    }

    virtual int getParamFields(float *pFields) override {
        // For getParamFields, we will copy the internal parameters into the pointer recieved.
        pFields[0] = mPose.x();
        pFields[1] = mPose.y();
        pFields[2] = mSize;
        pFields[3] = mSource.freq();
        pFields[4] = mEnvelope.lengths()[0];
        pFields[5] = mEnvelope.lengths()[2];

        return 6;
    }

    virtual void onTriggerOn() override {
        // We want to reset the envelope:
        mEnvelope.reset();
    }

    virtual void onTriggerOff() override {
        // We want to force the envelope to release:

        mEnvelope.release();
    }


private:
    gam::Sine<> mSource; // Sine wave oscillator source
    gam::AD<> mEnvelope;

    Mesh mesh; // The mesh now belongs to the voice

    Pose mPose;
    float mSize {1.0}; // This are the internal parameters

};


class MyApp : public App
{
public:

    virtual void onInit() override {
        // We must call compile() once to prepare the spatializer
        // This must be done in onInit() to make sure it is called before
        // audio starts procesing
        mSpatializer.compile();
    }


    virtual void onCreate() override {
        nav().pos(Vec3d(0,0,8)); // Set the camera to view the scene
        Light::globalAmbient({0.2, 1, 0.2});

        gui << X << Y << Size << AttackTime << ReleaseTime; // Register the parameters with the GUI

        gui << mRecorder;
        gui << mSequencer;

        gui.init(); // Initialize GUI. Don't forget this!

        navControl().active(false); // Disable nav control (because we are using the control to drive the synth
        mRecorder << mSequencer.synth();


    }

//    virtual void onAnimate(double dt) override {
//        navControl().active(!gui.usingInput());
//    }

    virtual void onDraw(Graphics &g) override
    {
        g.clear();
        g.lighting(true);

        // We call the render method for the sequencer. This renders its
        // internal PolySynth
        mSequencer.render(g);

        gui.draw(g);
    }

    virtual void onSound(AudioIOData &io) override {
         // The spatializer must be "prepared" and "finalized" on every block.
        // We do it here once, independently of the number of voices.
        mSpatializer.prepare(io);
        mSequencer.render(io);
        mSpatializer.finalize(io);
    }

    virtual void onKeyDown(const Keyboard& k) override
    {
        MyVoice *voice = sequencer().synth().getVoice<MyVoice>();
        int midiNote = asciiToMIDI(k.key());
        float freq = 440.0f * powf(2, (midiNote - 69)/12.0f);
        voice->set(X.get(), Y.get(), Size.get(), freq, AttackTime.get(), ReleaseTime.get());
        // We will pass the spatializer as the "user data" to the synth voice
        // This way the voice will be spatialized within the voice's
        // onSound
        sequencer().synth().triggerOn(voice, 0, midiNote, &mSpatializer);
    }

    virtual void onKeyUp(const Keyboard &k) override {
        int midiNote = asciiToMIDI(k.key());
        sequencer().synth().triggerOff(midiNote);
    }


    SynthSequencer &sequencer() {
        return mSequencer;
    }

    SynthRecorder &recorder() {
        return mRecorder;
    }

private:
    Light light;

    Parameter X {"X", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Y {"Y", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Size {"Scale", "Size", 1.0, "", 0.1f, 3.0f};
    Parameter AttackTime {"AttackTime", "Sound", 0.1, "", 0.001f, 2.0f};
    Parameter ReleaseTime {"ReleaseTime", "Sound", 1.0, "", 0.001f, 5.0f};

    rnd::Random<> randomGenerator; // Random number generator

    ControlGUI gui;

    SynthRecorder mRecorder;
    SynthSequencer mSequencer;

    // A speaker layout and spatializer
    SpeakerLayout sl {StereoSpeakerLayout()};
    SpatializerType mSpatializer {sl};
};


int main(int argc, char *argv[])
{
    MyApp app;
    app.dimensions(800, 600);

    // We will render each voice's output to an internal bus within the
    // AudioIO object. We need to allocate this bus here, before audio
    // is opened by initAudio.
    app.audioIO().channelsBus(1);

    app.initAudio(44100, 256, 2, 0);
    gam::sampleRate(44100);

    // Before starting the application we need to register our voice in
    // the PolySynth (that is inside the sequencer). This allows
    // triggering the class from a text file.
    app.sequencer().synth().registerSynthClass<MyVoice>("MyVoice");

    app.start();
    return 0;
}
