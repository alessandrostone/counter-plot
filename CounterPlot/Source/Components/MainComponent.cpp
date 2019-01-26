#include "MainComponent.hpp"
#include "../Core/LookAndFeel.hpp"
#include "../Core/Main.hpp"
#include "../Viewers/Viewer.hpp"
#include "../Viewers/ColourMapViewer.hpp"
#include "../Viewers/UserExtensionView.hpp"




//=============================================================================
class MetaYamlViewer : public Viewer
{
public:

    //=========================================================================
    MetaYamlViewer (MessageSink* messageSink)
    {
        viewer.setMessageSink (messageSink);
        addAndMakeVisible (viewer);
    }

    //=========================================================================
    bool isInterestedInFile (File file) const override
    {
        return file.hasFileExtension (".yaml");
    }

    void loadFile (File file) override
    {
        if (file != currentFile)
        {
            currentFile = file;
            viewer.reset();
            reloadFile();
        }
    }

    void reloadFile() override
    {
        viewer.configure (currentFile);

        if (auto main = findParentComponentOfClass<MainComponent>())
        {
            main->refreshCurrentViewerName();
        }
    }

    String getViewerName() const override
    {
        return "Meta Viewer (" + viewer.getViewerName() + ")";
    }

    const Runtime::Kernel* getKernel() const override
    {
        return viewer.getKernel();
    }

    bool canReceiveMessages() const override
    {
        return viewer.canReceiveMessages();
    }

    bool receiveMessage (const String& message) override
    {
        return viewer.receiveMessage (message);
    }

    //=========================================================================
    void resized() override
    {
        viewer.setBounds (getLocalBounds());
    }

private:
    File currentFile;
    UserExtensionView viewer;
};




//=========================================================================
UserExtensionsDirectoryEditor::UserExtensionsDirectoryEditor()
{
    editor.addListener (this);
    editor.addKeyListener (&mappings);
    editor.setMultiLine (true);
    editor.setTabKeyUsedAsCharacter (false);
    editor.setFont (Font ("Menlo", 14, 0));
    mappings.returnKeyCallback = [this] () { return sendContentsToMainViewerCollection(); };
    setColours();
    addAndMakeVisible (editor);
}

void UserExtensionsDirectoryEditor::setDirectories (const Array<File>& directories)
{
    auto paths = StringArray();

    for (auto dir : directories)
        paths.add (toRelativePath (dir.getFullPathName()));
    editor.setText (paths.joinIntoString ("\n"));
}

Array<File> UserExtensionsDirectoryEditor::getDirectories() const
{
    auto directories = Array<File>();

    for (const auto& path : StringArray::fromLines (editor.getText()))
        if (path.isNotEmpty())
            directories.add (fromRelativePath (path));
    return directories;
}




//=========================================================================
void UserExtensionsDirectoryEditor::paint (Graphics& g)
{
    auto instructionsArea = getLocalBounds().withTop (editor.getBottom());
    auto press = KeyPress (KeyPress::returnKey, ModifierKeys::commandModifier, 0);

    g.setColour (findColour (LookAndFeelHelpers::statusBarBackground));
    g.fillRect (instructionsArea);

    g.setFont (Font().withHeight (11)); // TODO: add font preference to LAF
    g.setColour (findColour (LookAndFeelHelpers::statusBarText));
    g.drawText ("Directories to watch for .yaml extensions - one per line.", instructionsArea.withTrimmedLeft(6), Justification::centredLeft);
    g.drawText (press.getTextDescriptionWithIcons(), instructionsArea.withTrimmedRight(6), Justification::centredRight);
}

void UserExtensionsDirectoryEditor::resized()
{
    editor.setBounds (getLocalBounds().withTrimmedBottom (22));
}

void UserExtensionsDirectoryEditor::visibilityChanged()
{
    if (isVisible())
        if (auto main = findParentComponentOfClass<MainComponent>())
            setDirectories (main->getViewerCollection().getWatchedDirectories());
}

void UserExtensionsDirectoryEditor::colourChanged()
{
    setColours();
    repaint();
}

void UserExtensionsDirectoryEditor::lookAndFeelChanged()
{
    setColours();
    repaint();
}




//=========================================================================
void UserExtensionsDirectoryEditor::textEditorTextChanged (TextEditor&)
{
}

void UserExtensionsDirectoryEditor::textEditorReturnKeyPressed (TextEditor&)
{
    editor.insertTextAtCaret ("\n");
}

void UserExtensionsDirectoryEditor::textEditorEscapeKeyPressed (TextEditor&)
{
    setVisible (false);
}

void UserExtensionsDirectoryEditor::textEditorFocusLost (TextEditor&)
{
    setVisible (false);
}




//=========================================================================
void UserExtensionsDirectoryEditor::setColours()
{
    editor.setColour (TextEditor::textColourId, findColour (LookAndFeelHelpers::statusBarText).brighter());
    editor.setColour (TextEditor::backgroundColourId, findColour (LookAndFeelHelpers::statusBarBackground));
    editor.setColour (TextEditor::highlightColourId, findColour (LookAndFeelHelpers::statusBarBackground).brighter());
    editor.setColour (TextEditor::highlightedTextColourId, findColour (LookAndFeelHelpers::statusBarText).brighter());
    editor.setColour (TextEditor::focusedOutlineColourId, Colours::transparentBlack);
    editor.setColour (TextEditor::outlineColourId, Colours::transparentBlack);
}

bool UserExtensionsDirectoryEditor::areContentsValid() const
{
    for (auto dir : getDirectories())
        if (dir == File())
            return false;
    return true;
}

bool UserExtensionsDirectoryEditor::sendContentsToMainViewerCollection()
{
    if (auto main = findParentComponentOfClass<MainComponent>())
    {
        if (areContentsValid())
        {
            main->getViewerCollection().setWatchedDirectories (getDirectories());
            main->indicateSuccess ("Extension directories updated");
            setVisible (false);
            return true;
        }
        main->logErrorMessage ("Invalid directories");
    }
    return false;
}

String UserExtensionsDirectoryEditor::toRelativePath (const File& file) const
{
    auto home = File::getSpecialLocation (File::userHomeDirectory).getFullPathName();

    if (file.getFullPathName().startsWith (home))
        return file.getFullPathName().replaceFirstOccurrenceOf (home, "~");
    return file.getFullPathName();
}

File UserExtensionsDirectoryEditor::fromRelativePath (const String& path) const
{
    auto home = File::getSpecialLocation (File::userHomeDirectory).getFullPathName();

    if (path.startsWith ("~"))
        return path.replaceFirstOccurrenceOf ("~", home);
    else if (File::isAbsolutePath (path))
        return path;
    return File();
}




//=============================================================================
StatusBar::EnvironmentViewToggleButton::EnvironmentViewToggleButton() : Button ("")
{
    setWantsKeyboardFocus (false);
}

void StatusBar::EnvironmentViewToggleButton::paintButton (Graphics& g,
                                                          bool highlighted,
                                                          bool /*down*/)
{
    if (auto main = findParentComponentOfClass<MainComponent>())
    {
        auto text = String (main->isEnvironmentViewShowing() ? "Hide" : "Show") + " Viewer Environment";
        g.setFont (Font().withHeight (11));
        g.setColour (findColour (LookAndFeelHelpers::statusBarText).brighter (highlighted ? 0.2f : 0.0f));
        g.drawText (text, getLocalBounds().withTrimmedLeft(8), Justification::centredLeft);
    }
}




//=============================================================================
StatusBar::ViewerNamePopupButton::ViewerNamePopupButton() : Button ("")
{
    setWantsKeyboardFocus (false);
}

void StatusBar::ViewerNamePopupButton::paintButton (Graphics& g,
                                                    bool highlighted,
                                                    bool /*down*/)
{
    g.setFont (Font().withHeight (11));
    g.setColour (findColour (LookAndFeelHelpers::statusBarText).brighter (highlighted ? 0.2f : 0.0f));
    g.drawText (currentViewerName, getLocalBounds().withTrimmedRight(8), Justification::centredRight);
}

void StatusBar::ViewerNamePopupButton::clicked()
{
    if (auto main = findParentComponentOfClass<MainComponent>())
    {
        PopupMenu menu;
        StringArray names;
        File currentFile = main->getCurrentFile();
        const Viewer* currentViewer = main->getCurrentViewer();

        for (auto viewer : main->getViewerCollection().getAllComponents())
        {
            auto name = viewer->getViewerName();
            names.add (name);
            menu.addItem (names.size(), name, viewer->isInterestedInFile (currentFile), viewer == currentViewer);
        }
        int result = menu.show();

        if (result != 0)
        {
            main->setCurrentViewer (names[result - 1]);
        }
    }
}




//=============================================================================
StatusBar::StatusBar()
{
    environmentViewToggleButton.setCommandToTrigger (&PatchViewApplication::getApp().getCommandManager(),
                                                     PatchViewApplication::Commands::toggleEnvironmentView,
                                                     true);
    addAndMakeVisible (environmentViewToggleButton);
    addAndMakeVisible (viewerNamePopupButton);
}

StatusBar::~StatusBar()
{
}

void StatusBar::incrementAsyncTaskCount()
{
    ++numberOfAsyncTasks;
    repaint();
}

void StatusBar::decrementAsyncTaskCount()
{
    --numberOfAsyncTasks;
    repaint();
}

void StatusBar::setMousePositionInFigure (Point<double> position)
{
    mousePositionInFigure = position;
    repaint();
}

void StatusBar::setCurrentViewerName (const String& viewerName)
{
    viewerNamePopupButton.currentViewerName = viewerName;
    viewerNamePopupButton.repaint();
}

void StatusBar::setCurrentErrorMessage (const String& what)
{
    currentErrorMessage = what;
    millisecondsToDisplayInfo = 4000;
    startTimer (40);
    repaint();
}

void StatusBar::setCurrentInfoMessage (const String& info, int milliseconds)
{
    if (milliseconds > 0)
    {
        millisecondsToDisplayInfo = milliseconds;
        startTimer (40);
    }
    currentInfoMessage = info;
    repaint();
}




//=============================================================================
void StatusBar::paint (Graphics& g)
{
    auto geom                = computeGeometry();
    auto backgroundColour    = findColour (LookAndFeelHelpers::statusBarBackground);
    auto fontColour          = findColour (LookAndFeelHelpers::statusBarText);
    auto errorColour         = findColour (LookAndFeelHelpers::statusBarErrorText);
    auto busyIndicatorColour = numberOfAsyncTasks ? Colours::yellow : Colours::transparentBlack;

    g.setColour (backgroundColour);
    g.fillAll();

    g.setColour (busyIndicatorColour);
    g.fillEllipse (geom.busyIndicatorArea.reduced (5).toFloat());

    g.setColour (fontColour);
    g.setFont (Font().withHeight (11));
    g.drawText (mousePositionInFigure.isOrigin() ? "" : mousePositionInFigure.toString(), geom.mousePositionArea, Justification::centredLeft);

    if (currentErrorMessage.isNotEmpty())
    {
        g.setColour (errorColour.withAlpha (jmin (1.f, millisecondsToDisplayInfo / 400.f)));
        g.drawText (currentErrorMessage, geom.messageArea, Justification::centredLeft);
    }
    else if (currentInfoMessage.isNotEmpty())
    {
        if (numberOfAsyncTasks > 0)
        {
            g.drawText (currentInfoMessage, geom.messageArea, Justification::centredLeft);
        }
        else if (millisecondsToDisplayInfo > 0)
        {
            g.setColour (fontColour.withAlpha (jmin (1.f, millisecondsToDisplayInfo / 400.f)));
            g.drawText (currentInfoMessage, geom.messageArea, Justification::centredLeft);
        }
    }
}

void StatusBar::resized()
{
    auto geom = computeGeometry();
    environmentViewToggleButton.setBounds (geom.environmentViewToggleArea);
    viewerNamePopupButton.setBounds (geom.viewerNamePopupArea);
}

void StatusBar::colourChanged()
{
    setColours();
    repaint();
}

void StatusBar::lookAndFeelChanged()
{
    setColours();
    repaint();
}




//=============================================================================
void StatusBar::timerCallback()
{
    millisecondsToDisplayInfo -= getTimerInterval();

    if (millisecondsToDisplayInfo <= 0)
    {
        millisecondsToDisplayInfo = 0;
        stopTimer();
    }
    repaint();
}




//=============================================================================
StatusBar::Geometry StatusBar::computeGeometry() const
{
    auto geom = Geometry();
    auto area = getLocalBounds();
    geom.busyIndicatorArea         = area.removeFromRight (getHeight());
    geom.viewerNamePopupArea       = area.removeFromRight (200);
    geom.mousePositionArea         = area.removeFromRight (120);
    geom.environmentViewToggleArea = area.removeFromLeft  (150);
    geom.messageArea               = area;
    return geom;
}

void StatusBar::setColours()
{
}




//=============================================================================
EnvironmentView::EnvironmentView()
{
    list.setModel (this);
    list.getViewport()->setWantsKeyboardFocus (false);
    setColours();
    addAndMakeVisible (list);
}

void EnvironmentView::setKernel (const Runtime::Kernel* kernelToView)
{
    kernel = kernelToView;
    refresh();
}

void EnvironmentView::refresh()
{
    keys.clear();

    if (kernel)
        for (const auto& item : *kernel)
            if ((item.second.flags & Runtime::builtin) == 0)
                keys.add (item.first);

    keys.sort();
    list.updateContent();
    repaint();
}




//=============================================================================
void EnvironmentView::resized()
{
    list.setBounds (getLocalBounds());
}

void EnvironmentView::colourChanged()
{
    setColours();
    repaint();
}

void EnvironmentView::lookAndFeelChanged()
{
    setColours();
    repaint();
}




//=============================================================================
void EnvironmentView::setColours()
{
    list.setColour (ListBox::backgroundColourId, findColour (LookAndFeelHelpers::environmentViewBackground));
}




//=============================================================================
int EnvironmentView::getNumRows()
{
    return keys.size();
}

void EnvironmentView::paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
    g.fillAll (rowIsSelected ? findColour (ListBox::backgroundColourId).darker() : Colours::transparentBlack);

    auto key = keys[rowNumber];
    auto err = kernel->error_at (key.toStdString());
    auto repr = Runtime::represent (kernel->at (keys[rowNumber].toStdString()));
    auto text1 = findColour (LookAndFeelHelpers::environmentViewText1);
    auto text2 = findColour (LookAndFeelHelpers::environmentViewText2);

    g.setFont (Font ("Menlo", 11, 0));
    g.setColour (text1);
    g.drawText (keys[rowNumber], 8, 0, width - 16, height, Justification::centredLeft);

    if (err.empty())
    {
        g.setColour (text2);
        g.drawText (repr, 8, 0, width - 16, height, Justification::centredRight);
    }
    else
    {
        g.setColour (Colours::orange);
        g.drawText (err, 8, 0, width - 16, height, Justification::centredRight);
    }
}

void EnvironmentView::selectedRowsChanged (int lastRowSelected)
{
    if (auto main = findParentComponentOfClass<MainComponent>())
    {
        main->showKernelRule (keys[lastRowSelected]);
    }
}




//=========================================================================
KernelRuleEntry::KernelRuleEntry()
{
    editor.setTextToShowWhenEmpty ("key: value", Colours::grey);
    editor.setFont (Font ("Menlo", 15, 0));
    editor.addListener (this);
    editor.addKeyListener (&keyMappings);
    keyMappings.nextCallback = [this] () { return recallNext(); };
    keyMappings.prevCallback = [this] () { return recallPrev(); };
    setColours();
    addAndMakeVisible (editor);
}

void KernelRuleEntry::loadRule (const std::string& rule, const Runtime::Kernel& kernel)
{
    if (kernel.contains (rule))
    {
        if (! kernel.expr_at (rule).empty())
            editor.setText (rule + ": " + kernel.expr_at (rule).str());
        else
            editor.setText (rule + ": " + kernel.at (rule).toString().toStdString());

        loadedRule = rule;
        loadedText = editor.getText();
    }
}

void KernelRuleEntry::refresh (const Runtime::Kernel* kernel)
{
    if (kernel && editor.getText() == loadedText)
    {
        loadRule (loadedRule.toStdString(), *kernel);
    }
}

bool KernelRuleEntry::recallNext()
{
    if (indexInHistory < history.size())
    {
        editor.setText (history[++indexInHistory]);
        return true;
    }
    return false;
}

bool KernelRuleEntry::recallPrev()
{
    if (indexInHistory > 0)
    {
        editor.setText (history[--indexInHistory]);
        return true;
    }
    return false;
}




//=========================================================================
void KernelRuleEntry::resized()
{
    editor.setBounds (getLocalBounds());
    editor.setIndents (16, (getHeight() - editor.getFont().getHeight()) / 2);
}

void KernelRuleEntry::colourChanged()
{
    setColours();
    repaint();
}

void KernelRuleEntry::lookAndFeelChanged()
{
    setColours();
    repaint();
}




//=========================================================================
void KernelRuleEntry::textEditorTextChanged (TextEditor&)
{
}

void KernelRuleEntry::textEditorReturnKeyPressed (TextEditor&)
{
    if (auto main = findParentComponentOfClass<MainComponent>())
    {
        if (! editor.isEmpty())
        {
            auto text = editor.getText();

            if (main->sendMessageToCurrentViewer (text))
            {
                editor.setText (String());
                history.add (text);
                indexInHistory = history.size();
            }
        }
    }
}

void KernelRuleEntry::textEditorEscapeKeyPressed (TextEditor&)
{
    if (auto main = findParentComponentOfClass<MainComponent>())
    {
        main->toggleKernelRuleEntryShown();
    }
}

void KernelRuleEntry::textEditorFocusLost (TextEditor&)
{
}




//=============================================================================
void KernelRuleEntry::setColours()
{
    editor.setColour (TextEditor::textColourId, findColour (LookAndFeelHelpers::statusBarText).brighter());
    editor.setColour (TextEditor::backgroundColourId, findColour (LookAndFeelHelpers::statusBarBackground));
    editor.setColour (TextEditor::highlightColourId, Colours::black.withAlpha (0.15f));
    editor.setColour (TextEditor::highlightedTextColourId, findColour (LookAndFeelHelpers::statusBarText).brighter());
    editor.setColour (TextEditor::focusedOutlineColourId, Colours::transparentBlack);
    editor.setColour (TextEditor::outlineColourId, Colours::transparentBlack);
}




//=============================================================================
MainComponent::MainComponent()
{
    filePoller.setCallback ([this] (File) { reloadCurrentFile(); });
    directoryTree.addListener (this);
    directoryTree.getTreeView().setWantsKeyboardFocus (directoryTreeShowing);
    environmentView.getListBox().setWantsKeyboardFocus (environmentViewShowing);
    statusBar.setCurrentViewerName ("Viewer List");

    viewers.addListener (this);
    viewers.add (std::make_unique<JsonFileViewer>());
    viewers.add (std::make_unique<ImageFileViewer>());
    viewers.add (std::make_unique<ColourMapViewer>());
    viewers.add (std::make_unique<PDFViewer>());
    viewers.add (std::make_unique<MetaYamlViewer> (this));

#if (JUCE_DEBUG == 0)
     viewers.loadFromYamlString (BinaryData::BinaryTorque_yaml);
     viewers.loadFromYamlString (BinaryData::JetInCloud_yaml);
#warning("Loading hard-coded viewers")
#endif

    addAndMakeVisible (directoryTree);
    addAndMakeVisible (environmentView);
    addAndMakeVisible (statusBar);
    addChildComponent (kernelRuleEntry);
    addChildComponent (userExtensionsDirectoryEditor);
    setSize (1024, 768 - 64);
}

MainComponent::~MainComponent()
{
}

void MainComponent::setCurrentDirectory (File newCurrentDirectory)
{
    directoryTree.setDirectoryToShow (newCurrentDirectory);
}

void MainComponent::reloadCurrentFile()
{
    if (currentViewer)
        currentViewer->reloadFile();
}

void MainComponent::reloadDirectoryTree()
{
    directoryTree.reloadAll();
}

void MainComponent::toggleDirectoryTreeShown (bool animated)
{
    directoryTreeShowing = ! directoryTreeShowing;
    directoryTree.getTreeView().setWantsKeyboardFocus (directoryTreeShowing);
    layout (animated);
}

void MainComponent::toggleEnvironmentViewShown (bool animated)
{
    environmentViewShowing = ! environmentViewShowing;
    environmentView.getListBox().setWantsKeyboardFocus (environmentViewShowing);
    layout (animated);
}

void MainComponent::toggleKernelRuleEntryShown()
{
    kernelRuleEntryShowing = ! kernelRuleEntryShowing;
    kernelRuleEntry.setVisible (kernelRuleEntryShowing);
    layout (false);

    if (isKernelRuleEntryShowing())
    {
        kernelRuleEntry.grabKeyboardFocus();
    }
}

void MainComponent::toggleUserExtensionsDirectoryEditor()
{
    if (userExtensionsDirectoryEditor.isVisible())
    {
        userExtensionsDirectoryEditor.setVisible (false);
    }
    else
    {
        userExtensionsDirectoryEditor.setVisible (true);
        userExtensionsDirectoryEditor.grabKeyboardFocus();
    }
}

bool MainComponent::hideExtraComponents()
{
    if (userExtensionsDirectoryEditor.isVisible())
    {
        userExtensionsDirectoryEditor.setVisible (false);
        return true;
    }
    if (kernelRuleEntryShowing)
    {
        toggleKernelRuleEntryShown();
        return true;
    }
    if (environmentViewShowing)
    {
        toggleEnvironmentViewShown (false);
        return true;
    }
    return false;
}

bool MainComponent::isDirectoryTreeShowing() const
{
    return directoryTreeShowing;
}

bool MainComponent::isEnvironmentViewShowing() const
{
    return environmentViewShowing;
}

bool MainComponent::isKernelRuleEntryShowing() const
{
    return kernelRuleEntryShowing;
}

bool MainComponent::isUserExtensionsDirectoryEditorShowing() const
{
    return userExtensionsDirectoryEditor.isVisible();
}

File MainComponent::getCurrentDirectory() const
{
    return directoryTree.getCurrentDirectory();
}

File MainComponent::getCurrentFile() const
{
    return currentFile;
}

const Viewer* MainComponent::getCurrentViewer() const
{
    return currentViewer;
}

ViewerCollection& MainComponent::getViewerCollection()
{
    return viewers;
}

DirectoryTree& MainComponent::getDirectoryTree()
{
    return directoryTree;
}

void MainComponent::setCurrentViewer (const String& viewerName)
{
    if (auto viewer = viewers.findViewerWithName (viewerName))
    {
        makeViewerCurrent (viewer);
    }
}

bool MainComponent::isViewerSuitable (Viewer* viewer) const
{
    if (viewer == nullptr)
        return false;
    if (viewer->isInterestedInFile (currentFile))
        return true;
    return false;
}

void MainComponent::makeViewerCurrent (Viewer* viewer)
{
    if (viewer != currentViewer)
    {
        if (viewer)
        {
            viewer->loadFile (currentFile);
            statusBar.setCurrentViewerName (viewer->getViewerName());
            environmentView.setKernel (viewer->getKernel());
        }
        else
        {
            statusBar.setCurrentViewerName ("Viewer List");
            environmentView.setKernel (nullptr);
        }
        if (isKernelRuleEntryShowing() && ! viewer->canReceiveMessages())
        {
            toggleKernelRuleEntryShown();
        }
        viewers.showOnly (viewer);
        currentViewer = viewer;
        PatchViewApplication::getApp().getCommandManager().commandStatusChanged();
    }
}

void MainComponent::refreshCurrentViewerName()
{
    if (currentViewer)
        statusBar.setCurrentViewerName (currentViewer->getViewerName());
    else
        statusBar.setCurrentViewerName (String());
}

bool MainComponent::sendMessageToCurrentViewer (String& message)
{
    if (currentViewer)
    {
        return currentViewer->receiveMessage (message);
    }
    return false;
}

bool MainComponent::canSendMessagesToCurrentViewer() const
{
    if (currentViewer)
    {
        return currentViewer->canReceiveMessages();
    }
    return false;
}

void MainComponent::showKernelRule (const String& rule)
{
    if (currentViewer)
    {
        if (auto kernel = currentViewer->getKernel())
        {
            kernelRuleEntry.loadRule (rule.toStdString(), *kernel);
        }
    }
}

void MainComponent::indicateSuccess (const String& info)
{
    statusBar.setCurrentErrorMessage (String());
    statusBar.setCurrentInfoMessage (info, 3000);
}

void MainComponent::logErrorMessage (const String& what)
{
    statusBar.setCurrentErrorMessage (what);
}




//=============================================================================
void MainComponent::resized()
{
    layout (false);
}




//=============================================================================
void MainComponent::selectedFileChanged (DirectoryTree*, File file)
{
    currentFile = file;
    filePoller.setFileToPoll (currentFile);

    if (currentViewer && currentViewer->isInterestedInFile (currentFile))
    {
        currentViewer->loadFile (currentFile);
    }
    else if (auto viewer = viewers.findViewerForFile (file))
    {
        makeViewerCurrent (viewer);
    }
}




//=============================================================================
void MainComponent::figureMousePosition (Point<double> position)
{
    statusBar.setMousePositionInFigure (position);
}




//=============================================================================
void MainComponent::viewerAsyncTaskStarted (const String& name)
{
    statusBar.incrementAsyncTaskCount();
    statusBar.setCurrentInfoMessage ("Started " + name);
}

void MainComponent::viewerAsyncTaskCompleted (const String& name)
{
    statusBar.decrementAsyncTaskCount();
    statusBar.setCurrentInfoMessage ("Completed " + name);
}

void MainComponent::viewerAsyncTaskCancelled (const String& name)
{
    statusBar.decrementAsyncTaskCount();
    // statusBar.setCurrentInfoMessage ("Cancelled " + name);
}

void MainComponent::viewerLogErrorMessage (const String& what)
{
    statusBar.setCurrentErrorMessage (what);
}

void MainComponent::viewerIndicateSuccess()
{
    statusBar.setCurrentErrorMessage (String());
}

void MainComponent::viewerEnvironmentChanged()
{
    if (currentViewer)
    {
        environmentView.refresh();
        kernelRuleEntry.refresh (currentViewer->getKernel());
    }
}




//=============================================================================
void MainComponent::viewerCollectionViewerReconfigured (Viewer *viewer)
{
    statusBar.setCurrentInfoMessage ("Configure viewer " + viewer->getViewerName(), 3000);

    if (viewer == currentViewer)
        if (! isViewerSuitable (viewer))
            makeViewerCurrent (viewers.findViewerForFile (currentFile));
}

void MainComponent::viewerCollectionViewerAdded (Viewer *viewer)
{
    statusBar.setCurrentInfoMessage ("Load viewer: " + viewer->getViewerName(), 3000);
    addChildComponent (viewer, 0);
    layout (false);

    if (currentViewer == nullptr && isViewerSuitable (viewer))
        makeViewerCurrent (viewer);
}

void MainComponent::viewerCollectionViewerRemoved (Viewer *viewer)
{
    statusBar.setCurrentInfoMessage ("Unload viewer " + viewer->getViewerName(), 3000);

    if (viewer == currentViewer)
        makeViewerCurrent (nullptr);
}




//=============================================================================
void MainComponent::layout (bool animated)
{
    auto setBounds = [animated] (Component& component, const Rectangle<int>& bounds)
    {
        if (animated && component.isVisible())
            Desktop::getInstance().getAnimator().animateComponent (&component, bounds, 1.f, 200, false, 1.f, 1.f);
        else
            component.setBounds (bounds);
    };

    auto area = getLocalBounds();
    auto statusBarArea = area.removeFromBottom (22);
    auto directoryTreeArea = directoryTreeShowing ? area.removeFromLeft (300) : area.withWidth (300).translated (-300, 0);
    auto environmentViewArea = Rectangle<int> (0, 0, 300, 330)
    .withBottomY (statusBarArea.getY())
    .translated (0, environmentViewShowing ? 0 : 330 + 22); // the 22 is to ensure it's offscreen, so not painted

    if (kernelRuleEntryShowing)
    {
        kernelRuleEntry.setBounds (area.removeFromBottom (32));
    }
    setBounds (statusBar, statusBarArea);
    setBounds (directoryTree, directoryTreeArea);
    setBounds (environmentView, environmentViewArea);
    viewers.setBounds (area, animated);

    userExtensionsDirectoryEditor.setBounds (getLocalBounds().withSizeKeepingCentre (400, 300));
}
