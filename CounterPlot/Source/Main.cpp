#include "Main.hpp"
#include "MainComponent.hpp"
#include "Views/FigureView.hpp"
#include "Views/LookAndFeel.hpp"
#include "Views/BinaryTorquesView.hpp"




//=============================================================================
START_JUCE_APPLICATION (PatchViewApplication)

static herr_t h5_error_handler(hid_t estack, void*)
{
    // H5Eprint(estack, stdout);
    return 0;
}




//=============================================================================
PatchViewApplication::MainWindow::MainWindow (String name) : DocumentWindow (name, Colours::black, DocumentWindow::allButtons)
{
    content = std::make_unique<MainComponent>();

    addKeyListener (getApp().commandManager->getKeyMappings());
    setUsingNativeTitleBar (true);
    setContentNonOwned (content.get(), true);
    setResizable (true, true);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

PatchViewApplication::MainWindow::~MainWindow()
{
}

void PatchViewApplication::MainWindow::closeButtonPressed()
{
    JUCEApplication::getInstance()->systemRequestedQuit();
}




//=============================================================================
PatchViewApplication::MainMenu::MainMenu()
{
    setApplicationCommandManagerToWatch (getApp().commandManager.get());
}

StringArray PatchViewApplication::MainMenu::getMenuBarNames()
{
    return {"File", "View"};
}

PopupMenu PatchViewApplication::MainMenu::getMenuForIndex (int /*topLevelMenuIndex*/, const String& menuName)
{
    auto manager = &getApp().getCommandManager();
    PopupMenu menu;

    if (menuName == "File")
    {
        menu.addCommandItem (manager, Commands::openDirectory);
        menu.addSeparator();
        menu.addCommandItem (manager, BinaryTorquesView::Commands::makeSnapshotAndOpen);
        menu.addCommandItem (manager, BinaryTorquesView::Commands::saveSnapshotAs);
        return menu;
    }
    if (menuName == "View")
    {
        menu.addCommandItem (manager, Commands::toggleDirectoryView);
        menu.addSeparator();
        menu.addCommandItem (manager, BinaryTorquesView::Commands::nextColourMap);
        menu.addCommandItem (manager, BinaryTorquesView::Commands::prevColourMap);
        menu.addCommandItem (manager, BinaryTorquesView::Commands::resetScalarRange);
        return menu;
    }
    jassertfalse;
    return menu;
}

void PatchViewApplication::MainMenu::menuItemSelected (int menuItemID, int /*topLevelMenuIndex*/)
{
    // Not sure what this method is for...
}




//=============================================================================
PatchViewApplication& PatchViewApplication::getApp()
{
    return *dynamic_cast<PatchViewApplication*> (JUCEApplication::getInstance());
}

ApplicationCommandManager& PatchViewApplication::getCommandManager()
{
    return *commandManager;
}

PatchViewApplication::PatchViewApplication()
{
}

const String PatchViewApplication::getApplicationName()
{
    return ProjectInfo::projectName;
}

const String PatchViewApplication::getApplicationVersion()
{
    return ProjectInfo::versionString;
}

bool PatchViewApplication::moreThanOneInstanceAllowed()
{
    return true;
}

void PatchViewApplication::initialise (const String& commandLine)
{
    H5Eset_auto(H5E_DEFAULT, h5_error_handler, NULL);
    configureLookAndFeel();

    commandManager = std::make_unique<ApplicationCommandManager>();
    menu           = std::make_unique<MainMenu>();
    mainWindow     = std::make_unique<MainWindow> (getApplicationName());

    commandManager->registerAllCommandsForTarget (this);
    commandManager->registerAllCommandsForTarget (std::make_unique<BinaryTorquesView>().get());
    MenuBarModel::setMacMainMenu (menu.get(), nullptr);

    startTimer (500);
    settingsLastPolled = Time::getCurrentTime();
}

void PatchViewApplication::shutdown()
{
    MenuBarModel::setMacMainMenu (nullptr, nullptr);
}

void PatchViewApplication::systemRequestedQuit()
{
    quit();
}

void PatchViewApplication::anotherInstanceStarted (const String& commandLine)
{
}




//=============================================================================
void PatchViewApplication::getAllCommands (Array<CommandID>& commands)
{
    JUCEApplication::getAllCommands (commands);

    const CommandID ids[] = {
        Commands::openDirectory,
        Commands::reloadCurrentFile,
        Commands::toggleDirectoryView,
    };
    commands.addArray (ids, numElementsInArray (ids));
}

void PatchViewApplication::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    switch (commandID)
    {
        case Commands::openDirectory:
            result.setInfo ("Open...", "", "File", 0);
            result.defaultKeypresses.add (KeyPress ('o', ModifierKeys::commandModifier, 0));
            break;
        case Commands::reloadCurrentFile:
            result.setInfo ("Reload Current File", "", "File", 0);
            result.defaultKeypresses.add (KeyPress ('r', ModifierKeys::commandModifier, 0));
            break;
        case Commands::toggleDirectoryView:
            result.setInfo ("Side Bar", "", "View", mainWindow->content->isDirectoryTreeShowing() ? ApplicationCommandInfo::isTicked : 0);
            result.defaultKeypresses.add (KeyPress ('K', ModifierKeys::commandModifier, 0));
            break;
        default:
            JUCEApplication::getCommandInfo (commandID, result);
            break;
    }
}

bool PatchViewApplication::perform (const InvocationInfo& info)
{
    switch (info.commandID)
    {
        case Commands::openDirectory:             return presentOpenDirectoryDialog();
        case Commands::reloadCurrentFile:         mainWindow->content->reloadCurrentFile(); return true;
        case Commands::toggleDirectoryView:       mainWindow->content->toggleDirectoryTreeShown(); return true;
        default:                                  return JUCEApplication::perform (info);
    }
}

void PatchViewApplication::timerCallback()
{
    auto settingsFile = File::getSpecialLocation (File::userHomeDirectory).getChildFile ("patch_view_settings.json");

    if (settingsLastPolled > settingsFile.getLastModificationTime())
    {
        return;
    }

    settingsLastPolled = Time::getCurrentTime();
    var settings = JSON::parse (settingsFile);
    auto& laf = Desktop::getInstance().getDefaultLookAndFeel();

    LookAndFeelHelpers::setLookAndFeelDefaults (laf, LookAndFeelHelpers::BackgroundScheme::dark);
    LookAndFeelHelpers::setLookAndFeelDefaults (laf, LookAndFeelHelpers::TextColourScheme::pastels2);
    FigureView        ::setLookAndFeelDefaults (laf, FigureView::ColourScheme::dark);

    if (auto obj = settings.getDynamicObject())
    {
        for (auto item : obj->getProperties())
        {
            auto id = LookAndFeelHelpers::colourIdFromString (item.name.toString());
            auto colour = LookAndFeelHelpers::colourFromVariant (item.value);

            if (colour != Colours::transparentWhite)
            {
                laf.setColour (id, colour);
            }
        }
    }
    mainWindow->sendLookAndFeelChange();
}




//=============================================================================
void PatchViewApplication::configureLookAndFeel()
{
    auto& laf = Desktop::getInstance().getDefaultLookAndFeel();
    laf.setColour (TextEditor::backgroundColourId, Colours::white);
    laf.setColour (TextEditor::textColourId, Colours::black);
    laf.setColour (TextEditor::highlightColourId, Colours::lightblue);
    laf.setColour (TextEditor::highlightedTextColourId, Colours::black);
    laf.setColour (TextEditor::outlineColourId, Colours::transparentBlack);
    laf.setColour (TextEditor::focusedOutlineColourId, Colours::lightblue);
    laf.setColour (ListBox::backgroundColourId, Colours::white);

    laf.setColour (Label::ColourIds::textColourId, Colours::lightgrey);
    laf.setColour (Label::ColourIds::textWhenEditingColourId, Colours::lightgrey);
    laf.setColour (Label::ColourIds::backgroundWhenEditingColourId, Colours::white);

    LookAndFeelHelpers::setLookAndFeelDefaults (laf, LookAndFeelHelpers::BackgroundScheme::dark);
    LookAndFeelHelpers::setLookAndFeelDefaults (laf, LookAndFeelHelpers::TextColourScheme::pastels2);
    FigureView        ::setLookAndFeelDefaults (laf, FigureView::ColourScheme::dark);
}

bool PatchViewApplication::presentOpenDirectoryDialog()
{
    FileChooser chooser ("Open directory...", currentDirectory, "", true, false, nullptr);

    if (chooser.browseForDirectory())
    {
        mainWindow->content->setCurrentDirectory (chooser.getResult());
    }
    return true;
}