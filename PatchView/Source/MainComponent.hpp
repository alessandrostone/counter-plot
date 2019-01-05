#pragma once
#include "JuceHeader.h"
#include "FigureView.hpp"
#include "Views/DirectoryTree.hpp"
#include "Views/VariantView.hpp"




//=============================================================================
class PageView : public Component
{
public:
    PageView();
    void resized() override;
private:
    std::vector<std::unique_ptr<FigureView>> figures;
};




//=============================================================================
class MainComponent
: public Component
, public FigureView::Listener
, public DirectoryTree::Listener
{
public:
    //=========================================================================
    MainComponent();
    ~MainComponent();
    void setCurrentDirectory (File newCurrentDirectory);

    //=========================================================================
    void paint (Graphics&) override;
    void resized() override;
    bool keyPressed (const KeyPress& key) override;

    //=========================================================================
    void figureViewSetMargin (FigureView*, const BorderSize<int>&) override;
    void figureViewSetDomain (FigureView*, const Rectangle<double>&) override;
    void figureViewSetXlabel (FigureView*, const String&) override;
    void figureViewSetYlabel (FigureView*, const String&) override;
    void figureViewSetTitle (FigureView*, const String&) override;

    //=========================================================================
    void selectedFileChanged (DirectoryTree*, File) override;

private:
    //=========================================================================
    FigureModel model;

    FigureView figure;
    ImageComponent imageView;
    VariantView variantView;
    PageView page;

    DirectoryTree directoryTree;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
