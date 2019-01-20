#pragma once
#include "JuceHeader.h"

namespace YAML { class Node; }




//=============================================================================
class DataHelpers
{
public:
    static void updateDict (var& dictToUpdate, const var& other);

    static var varFromExpression (const crt::expression& expr);
    static var varFromYamlScalar (const YAML::Node& scalar);
    static var varFromYamlNode (const YAML::Node& node);
    static var varFromBorderSize (const BorderSize<int>&);
    static var varFromStringPairArray (const StringPairArray& value);

    static crt::expression expressionFromVar (const var& value);
    static YAML::Node yamlNodeFromVar (const var& value);
    static Array<Grid::TrackInfo> gridTrackInfoArrayFromVar (const var& value);
    static Colour colourFromVar (const var&);
    static StringArray stringArrayFromVar (const var&);
    static StringPairArray stringPairArrayFromVar (const var&);

    static CriticalSection& getCriticalSectionForHDF5();
};




//=============================================================================
class FilePoller : public Timer
{
public:

    //=========================================================================
    void setCallback (std::function<void(File)> callbackToInvoke)
    {
        callback = callbackToInvoke;
    }

    void setFileToPoll (File fileToPoll)
    {
        startTimer (100);
        file = fileToPoll;
        lastNotified = Time::getCurrentTime();
    }

    //=========================================================================
    void timerCallback() override
    {
        if (lastNotified < file.getLastModificationTime())
        {
            lastNotified = Time::getCurrentTime();

            if (callback)
                callback (file);
        }
    }

private:
    Time lastNotified;
    File file;
    std::function<void(File)> callback = nullptr;
};
