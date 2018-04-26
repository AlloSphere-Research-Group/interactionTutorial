
#include "al/core/app/al_App.hpp"
#include "al/core/graphics/al_Shapes.hpp"
#include "al/core/math/al_Random.hpp"
#include "al/util/ui/al_Parameter.hpp"
#include "al/util/ui/al_PresetSequencer.hpp"

#include "al/util/ui/al_SynthSequencer.hpp"
#include "al/util/ui/al_SynthRecorder.hpp"
#include "al/util/ui/al_ControlGUI.hpp"

#include "Gamma/Oscillator.h"
#include "Gamma/Envelope.h"
#include "Gamma/Domain.h"

using namespace al;

/*
 * This tutorial shows how to use the SynthRecorder class
 *
 * This class allows recording and playback of a PolySynth to a text file.
 * To allow the SynthVoice to read and write you must implement the
 * setParamFields and getParamFields function and register the SynthVoice to
 * allow instantiation from a text file.
*/

class MyVoice : public SynthVoice {
public:
    MyVoice() {
        addCone(mesh); // Prepare mesh to draw a cone

        mEnvelope.lengths(0.1f,  0.5f);
        mEnvelope.levels(0, 1, 0);
        mEnvelope.sustainPoint(1);
    }

    virtual void onProcess(AudioIOData &io) override {
        while(io()) {
            io.out(0) += mEnvelope() * mSource() * 0.05; // Output on the first channel scaled by 0.05;
        }
        if (mEnvelope.done()) {
            free();
        }
    }

    virtual void onProcess(Graphics &g) {
        g.pushMatrix();
        // You can get a parameter's value using the get() member function
        g.translate(mX, mY, 0);
        g.scale(mSize * mEnvelope.value());
        g.draw(mesh); // Draw the mesh
        g.popMatrix();
    }

    void set(float x, float y, float size, float frequency, float attackTime, float releaseTime) {
        mX = x;
        mY = y;
        mSize = size;
        mSource.freq(frequency);
        mEnvelope.lengths()[0] = attackTime;
        mEnvelope.lengths()[2] = releaseTime;
    }

    /*
     * You need to implement the setParamFields and getParamFields functions
     * to be able to communicate to the Sequencer. These pFields capture the
     * internal parameters that are sequenced.
     * For set parameters, we can use it to directly call set():
     */
    virtual bool setParamFields(float *pFields, int numFields) override {
        if (numFields != 6) { // Sanity check to make sure we are getting the right number of p-fields
            return false;
        }
        set(pFields[0], pFields[1], pFields[2], pFields[3], pFields[4], pFields[5]);
        return true;
    }

    virtual int getParamFields(float *pFields) override {
        // For getParamFields, we will copy the internal parameters into the pointer recieved.
        pFields[0] = mX;
        pFields[1] = mY;
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

    float mX {0}, mY {0}, mSize {1.0}; // This are the internal parameters

};


class MyApp : public App
{
public:

    virtual void onCreate() override {
        nav().pos(Vec3d(0,0,8)); // Set the camera to view the scene
        Light::globalAmbient({0.2, 1, 0.2});

        gui << X << Y << Size << AttackTime << ReleaseTime; // Register the parameters with the GUI

        /*
         * The SynthRecorder object can be passed to a ControlGUI object to
         * generate a GUI interface that can be controlled via the mouse
         */

        gui << mRecorder;
        gui << mSequencer;

        gui.init(); // Initialize GUI. Don't forget this!

        navControl().active(false); // Disable nav control (because we are using the control to drive the synth

        // We need to register a PolySynth with the recorder
        // We could use PolySynth directly and we can also use the PolySynth
        // contained within the sequencer accesing it through the synth()
        // function. Using the PolySynth from the sequencer allows both
        // text file based and programmatic (C++ based) sequencing.
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
        // We call the render method for the sequencer to render audio
        mSequencer.render(io);
    }

    /*
     * Put back the functions to trigger the PolySynth in real time.
     * Notice that we are using the PolySynth found within the
     * sequencer instead of directly
     */
    virtual void onKeyDown(const Keyboard& k) override
    {
        MyVoice *voice = sequencer().synth().getVoice<MyVoice>();
        int midiNote = asciiToMIDI(k.key());
        float freq = 440.0f * powf(2, (midiNote - 69)/12.0f);
        voice->set(X.get(), Y.get(), Size.get(), freq, AttackTime.get(), ReleaseTime.get());
        sequencer().synth().triggerOn(voice, 0, midiNote);
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

};


int main(int argc, char *argv[])
{
    MyApp app;
    app.dimensions(800, 600);
    app.initAudio(44100, 256, 2, 0);
    gam::sampleRate(44100);

    // Before starting the application we need to register our voice in
    // the PolySynth (that is inside the sequencer). This allows
    // triggering the class from a text file.
    app.sequencer().synth().registerSynthClass<MyVoice>("MyVoice");

    app.start();
    return 0;
}
