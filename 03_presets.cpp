
#include "al/core/app/al_App.hpp"
#include "al/core/graphics/al_Shapes.hpp"
#include "al/util/ui/al_Parameter.hpp"
#include "al/util/ui/al_Preset.hpp"
#include "al/core/math/al_Random.hpp"
#include "al/util/ui/al_SynthGUI.hpp"

using namespace al;

/* A simple App that draws a cone on the screen
 * Parameters are exposed via OSC.
*/

class MyApp : public App
{
public:

    virtual void onCreate() override {
        // Set the camera to view the scene
        nav().pos(Vec3d(0,0,8));
        // Prepare mesh to draw a cone
        addCone(mesh);
        Light::globalAmbient({0.2, 1, 0.2});

        // Register the parameters with the GUI
        gui << X << Y << Size;
        // Register the preset handler with GUI to have control of the presets
        gui << presetHandler;
        gui.init(); // Initialize GUI. Don't forget this!

        /*
            To register Parameters with a PresetHandler, you use the streaming
            operator, just as you did for the ParameterServer.
        */
        presetHandler << X << Y << Size;
        presetHandler.setMorphTime(2.0); // Presets will take 2 seconds to "morph"
    }

    virtual void onDraw(Graphics &g) override
    {
        g.clear();
        g.lighting(true);
        g.light(light);

        g.pushMatrix();
        // You can get a parameter's value using the get() member function
        g.translate(X.get(), Y.get(), 0);
        g.scale(Size.get());
        g.draw(mesh); // Draw the mesh
        g.popMatrix();

        // Draw th GUI
        gui.onDraw(g);
    }

    /*
        The keyboard is used here to store and recall presets, and also to
        randomize the parameter values. See instructions below.

        The storePreset() function can be used passing only a string, but you can
        also assign a number index to each particular preset. The number index will
        become useful in the next example. For simplicity, the preset name and the
        preset index will be the same (although one is an int and the other a
        string).
    */
    virtual void onKeyDown(const Keyboard& k) override
    {
        std::string presetName = std::to_string(k.keyAsNumber());
        if (k.alt()) {
            if (k.isNumber()) { // Use alt + any number key to store preset
                presetHandler.storePreset(k.keyAsNumber(), presetName);
                std::cout << "Storing preset:" << presetName << std::endl;
            }
        } else {
            if (k.isNumber()) { // Recall preset using the number keys
                presetHandler.recallPreset(k.keyAsNumber());
                std::cout << "Recalling preset:" << presetName << std::endl;
            } else if (k.key() == ' ') { // Randomize parameters
                X = randomGenerator.uniformS();
                Y = randomGenerator.uniformS();
                Size = 0.1 + randomGenerator.uniform() * 2.0;
            }

        }
    }

private:
    Light light;
    Mesh mesh;

    Parameter X {"X", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Y {"Y", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Size {"Scale", "Size", 1.0, "", 0.1f, 3.0f};

    /*
        A PresetHandler object groups parameters and stores presets for them.

        Parameters are registered using the streaming operator.

        You need to specify the path where presets will be stored as the first
        argument to the constructor.

        A PresetHandler can store and recall presets using the storePreset() and
        recallPreset() functions. When a preset is recalled, the values are
        gradually "morphed" (i.e. interpolated linearly) until they reach their
        destination. The time of this morph is set using the setMorphTime()
        function.
    */
    PresetHandler presetHandler {"sequencerPresets"};

    rnd::Random<> randomGenerator; // Random number generator

    SynthGUI gui;
};


int main(int argc, char *argv[])
{
    MyApp app;
    app.dimensions(800, 600);
    app.start();
    return 0;
}
