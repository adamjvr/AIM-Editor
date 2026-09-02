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
    requestQuit();
}

void MainWindow::openDocument (const juce::File& file)
{
    if (auto* editor = dynamic_cast<MainEditor*> (getContentComponent()))
    {
        editor->openDocumentFile (file);
        toFront (true);
    }
}

void MainWindow::requestQuit()
{
    const auto* editor = dynamic_cast<MainEditor*> (getContentComponent());
    if (editor == nullptr || ! editor->hasUnsavedChanges())
    {
        juce::JUCEApplication::getInstance()->quit();
        return;
    }

    const auto message = "AIM Editor has unsaved changes in the "
                       + editor->unsavedChangesDescription()
                       + ".\n\nSave the native JSON document(s) before quitting?";

    const auto options = juce::MessageBoxOptions()
                           .withIconType (juce::MessageBoxIconType::WarningIcon)
                           .withTitle ("Unsaved AIM Editor changes")
                           .withMessage (message)
                           .withButton ("Save & Quit")
                           .withButton ("Quit Without Saving")
                           .withButton ("Cancel")
                           .withAssociatedComponent (this);

    // JUCE 9 NativeMessageBox::showAsync returns the zero-based button index.
    // Do not replace this with AlertWindow::showAsync: its legacy non-native
    // AlertWindow result mapping is 1/2/0 for a three-button box.
    juce::NativeMessageBox::showAsync (options,
                                  [safe = juce::Component::SafePointer<MainWindow> (this)] (int buttonIndex)
                                  {
                                      if (safe == nullptr)
                                          return;

                                      if (buttonIndex == 0)
                                      {
                                          if (auto* currentEditor = dynamic_cast<MainEditor*> (safe->getContentComponent()))
                                              currentEditor->saveUnsavedChanges ([] (bool saved)
                                              {
                                                  if (saved)
                                                      juce::MessageManager::callAsync ([]
                                                      {
                                                          if (auto* app = juce::JUCEApplication::getInstance())
                                                              app->quit();
                                                      });
                                              });
                                      }
                                      else if (buttonIndex == 1)
                                      {
                                          if (auto* app = juce::JUCEApplication::getInstance())
                                              app->quit();
                                      }
                                  });
}
}
