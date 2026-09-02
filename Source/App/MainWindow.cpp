#include "MainWindow.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace aim
{
MainWindow::MainWindow (const ParameterRegistry& registry, IonMidiService& midiService, AppSettings& appSettings)
    : DocumentWindow ("AIM Editor",
                      juce::Colour::fromRGB (28, 28, 28),
                      juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar (true);
    setContentOwned (new MainEditor (registry, midiService, appSettings), true);
    setResizable (true, true);
    setResizeLimits (760, 520, 2400, 1600);

   #if JUCE_IOS
    setFullScreen (true);
   #else
    centreWithSize (1180, 720);
   #endif

    setVisible (true);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
}
