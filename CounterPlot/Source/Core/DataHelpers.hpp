#include "JuceHeader.h"

namespace YAML { class Node; }




//=============================================================================
class DataHelpers
{
public:
    static var varFromExpression (const crt::expression& expr);
    static var varFromYamlScalar (const YAML::Node& scalar);
    static var varFromYamlNode (const YAML::Node& node);
    static var varFromBorderSize (const BorderSize<int>&);

    static crt::expression expressionFromVar (const var& value);
    static YAML::Node yamlNodeFromVar (const var& value);
    static Array<Grid::TrackInfo> gridTrackInfoArrayFromVar (const var& value);
    static Colour colourFromVar (const var&);
};