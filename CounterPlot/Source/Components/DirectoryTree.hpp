#pragma once
#include "JuceHeader.h"




//=============================================================================
class DirectoryTree : public Component, public AsyncUpdater
{
public:
    class Listener
    {
    public:
        virtual ~Listener() {}
        virtual void directoryTreeSelectedFileChanged (DirectoryTree*, File) = 0;
        virtual void directoryTreeWantsFileToBeSource (DirectoryTree*, File) = 0;
    };

    //=========================================================================
    DirectoryTree();
    ~DirectoryTree();
    void addListener (Listener*);
    void removeListener (Listener*);
    void setDirectoryToShow (File directoryToShow);
    void reloadAll();
    File getCurrentDirectory() const;
    TreeView& getTreeView() { return tree; }
    std::unique_ptr<XmlElement> getRootOpennessState() const;
    void restoreRootOpenness (const XmlElement& state);

    //=========================================================================
    void resized() override;
    void mouseEnter (const MouseEvent& e) override;
    void mouseExit (const MouseEvent& e) override;
    void mouseMove (const MouseEvent& e) override;
    bool keyPressed (const KeyPress& key) override;
    void colourChanged() override;
    void lookAndFeelChanged() override;

    //=========================================================================
    void handleAsyncUpdate() override;

private:
    //=========================================================================
    void sendSelectedFilesAsSources();
    void sendSelectedFilesChanged();
    void setMouseOverItem (TreeViewItem*);
    void setColours();
    class Item;
    friend class Item;
    TreeView tree;
    std::unique_ptr<Item> root;
    TreeViewItem* mouseOverItem = nullptr;
    File currentDirectory;
    ListenerList<Listener> listeners;
};
