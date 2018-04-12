
#include "al/core/app/al_App.hpp"
#include "al/core/graphics/al_Shapes.hpp"
#include "al/util/ui/al_Parameter.hpp"
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
        gui.init(); // Initialize GUI. Don't forget this!

        /*
        Parameters need to be added to the ParameterServer by using the
        streaming operator <<.
        */
        paramServer << X << Y << Size; // Add parameters to parameter server

        /*
        You can add an OSC listener to a parameter server. Any change to any
        parameter will be broadcast to the OSC address and port registered using the
        addListener() function.
        */
        paramServer.addListener("127.0.0.1", 13560);

        /*
        The print function of the ParameterServer object provides information
        on the server, including the paths to all registered parameters.

        You will see a cone on the screen. I will not move until you send OSC
        messages with values to any of these addresses:

        Parameter X : /Position/X
        Parameter Y : /Position/Y
        Parameter Scale : /Size/Scale
        */
        paramServer.print();
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

private:
    Light light;
    Mesh mesh;


    /*
        The parameter's name is the first argument, followed by the name of the
        group it belongs to. The group is used in particular by OSC to construct
        the access address. The "prefix" is similar to group in this respect
        as is prefixes the text to the address.
    */
    Parameter X {"X", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Y {"Y", "Position", 0.0, "", -1.0f, 1.0f};
    Parameter Size {"Scale", "Size", 1.0, "", 0.1f, 3.0f};


    /* Once the parameters have been declared they can easily be exposed via OSC
     using the ParameterServer class. You can specify the IP Address and the
     network port to listen on.

     To register the parameters to the parameter server, you need to add them
     using the << streaming operator. See the main function below.
    */
    ParameterServer paramServer {"127.0.0.1", 9010};

    SynthGUI gui;
};


int main(int argc, char *argv[])
{
    MyApp app;
    app.dimensions(800, 600);
    app.start();
    return 0;
}
