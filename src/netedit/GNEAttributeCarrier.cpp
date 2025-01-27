/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GNEAttributeCarrier.cpp
/// @author  Jakob Erdmann
/// @date    Feb 2011
/// @version $Id$
///
// Abstract Base class for gui objects which carry attributes
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <algorithm>
#include <utils/common/MsgHandler.h>
#include <utils/xml/SUMOSAXAttributes.h>
#include <utils/gui/images/GUIIconSubSys.h>
#include <utils/gui/globjects/GUIGlObject.h>
#include <utils/geom/GeomConvHelper.h>
#include <netbuild/NBEdge.h>

#include "GNEAttributeCarrier.h"
#include "GNEUndoList.h"
#include "GNENet.h"
#include <netedit/netelements/GNEEdge.h>
#include <netedit/netelements/GNELane.h>

// ===========================================================================
// static members
// ===========================================================================

std::map<SumoXMLTag, std::vector<std::pair <SumoXMLAttr, std::string> > > GNEAttributeCarrier::_allowedAttributes;
std::vector<SumoXMLTag> GNEAttributeCarrier::myAllowedNetElementTags;
std::vector<SumoXMLTag> GNEAttributeCarrier::myAllowedAdditionalTags;
std::vector<SumoXMLTag> GNEAttributeCarrier::myAllowedShapeTags;
std::vector<SumoXMLTag> GNEAttributeCarrier::myBlockMovementTags;
std::vector<SumoXMLTag> GNEAttributeCarrier::myBlockShapeTags;
std::vector<SumoXMLTag> GNEAttributeCarrier::myCloseShapeTags;
std::vector<SumoXMLTag> GNEAttributeCarrier::myGeoPositionTags;
std::vector<SumoXMLTag> GNEAttributeCarrier::myGeoShapeTags;
std::vector<SumoXMLTag> GNEAttributeCarrier::myDialogTags;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myNumericalIntAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myNumericalFloatAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myTimeAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myBoolAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myColorAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myListAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myUniqueAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myNonEditableAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myPositiveAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myProbabilityAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::myFileAttrs;
std::map<SumoXMLTag, std::set<SumoXMLAttr> > GNEAttributeCarrier::mySVCPermissionsAttrs;
std::map<SumoXMLTag, SumoXMLTag> GNEAttributeCarrier::myAdditionalsWithParent;
std::map<SumoXMLTag, std::map<SumoXMLAttr, std::vector<std::string> > > GNEAttributeCarrier::myDiscreteChoices;
std::map<SumoXMLTag, std::map<SumoXMLAttr, std::pair<std::string, std::string> > > GNEAttributeCarrier::myAttrDefinitions;
int GNEAttributeCarrier::myMaxNumAttribute = 0;

const std::string GNEAttributeCarrier::LOADED = "loaded";
const std::string GNEAttributeCarrier::GUESSED = "guessed";
const std::string GNEAttributeCarrier::MODIFIED = "modified";
const std::string GNEAttributeCarrier::APPROVED = "approved";
const double GNEAttributeCarrier::INVALID_POSITION = -1000000;

#define NODEFAULTVALUE "<NODEFAULTVALUE>"
#define OPTIONALATTRIBUTE "-1"

// ===========================================================================
// method definitions
// ===========================================================================

GNEAttributeCarrier::GNEAttributeCarrier(SumoXMLTag tag, GUIIcon icon) :
    myTag(tag),
    myIcon(icon),
    mySelected(false)
{
}


template<> int
GNEAttributeCarrier::parse(const std::string& string) {
    return TplConvert::_str2int(string);
}


template<> double
GNEAttributeCarrier::parse(const std::string& string) {
    return TplConvert::_str2double(string);
}


template<> bool
GNEAttributeCarrier::parse(const std::string& string) {
    return TplConvert::_str2Bool(string);
}


template<> std::string
GNEAttributeCarrier::parse(const std::string& string) {
    return string;
}


template<> SUMOVehicleClass
GNEAttributeCarrier::parse(const std::string& string) {
    if (string.size() == 0) {
        throw EmptyData();
    } else if (SumoVehicleClassStrings.hasString(string) == false) {
        return SVC_IGNORING;
    } else {
        return SumoVehicleClassStrings.get(string);
    }
}


template<> RGBColor
GNEAttributeCarrier::parse(const std::string& string) {
    return RGBColor::parseColor(string);
}


template<> Position
GNEAttributeCarrier::parse(const std::string& string) {
    if (string.size() == 0) {
        throw EmptyData();
    } else {
        bool ok = true;
        PositionVector pos = GeomConvHelper::parseShapeReporting(string, "netedit-given", 0, ok, false, false);
        if(!ok || pos.size() != 1) {
            throw NumberFormatException();
        } else {
            return pos[0];
        }
    }
}


template<> SUMOVehicleShape
GNEAttributeCarrier::parse(const std::string& string) {
    if (string.size() == 0) {
        throw EmptyData();
    } else if ((string == "unknown") || (SumoVehicleShapeStrings.hasString(string) == false)) {
        return SVS_UNKNOWN;
    } else {
        return SumoVehicleShapeStrings.get(string);
    }
}


template<> std::vector<std::string>
GNEAttributeCarrier::parse(const std::string& string) {
    std::vector<std::string> parsedValues;
    SUMOSAXAttributes::parseStringVector(string, parsedValues);
    return parsedValues;
}


template<> std::vector<int>
GNEAttributeCarrier::parse(const std::string& string) {
    std::vector<std::string> parsedValues = parse<std::vector<std::string> >(string);
    std::vector<int> parsedIntValues;
    for (auto i : parsedValues) {
        parsedIntValues.push_back(parse<int>(i));
    }
    return parsedIntValues;
}


template<> std::vector<double>
GNEAttributeCarrier::parse(const std::string& string) {
    std::vector<std::string> parsedValues = parse<std::vector<std::string> >(string);
    std::vector<double> parsedDoubleValues;
    for (auto i : parsedValues) {
        parsedDoubleValues.push_back(parse<double>(i));
    }
    return parsedDoubleValues;
}


template<> std::vector<bool>
GNEAttributeCarrier::parse(const std::string& string) {
    std::vector<std::string> parsedValues = parse<std::vector<std::string> >(string);
    std::vector<bool> parsedBoolValues;
    for (auto i : parsedValues) {
        parsedBoolValues.push_back(parse<bool>(i));
    }
    return parsedBoolValues;
}


bool 
GNEAttributeCarrier::parseStringToANDBool(const std::string& string) {
    // obtain boolean vector (throw exception if string is empty)
    std::vector<bool> boolValues = GNEAttributeCarrier::parse<std::vector<bool> >(string);
    // set value of checkbox
    if (boolValues.size() == 1) {
        return boolValues.front();
    } else {
        int sum = 0;
        for (auto i : boolValues) {
            sum += (int)(i);
        }
        // only return true if all values are true
        if ((sum == 0) || (sum != (int)boolValues.size())) {
            return false;
        } else {
            return true;
        }
    }
}


std::string
GNEAttributeCarrier::getAttributeForSelection(SumoXMLAttr key) const {
    return getAttribute(key);
}


SumoXMLTag
GNEAttributeCarrier::getTag() const {
    return myTag;
}


FXIcon*
GNEAttributeCarrier::getIcon() const {
    return GUIIconSubSys::getIcon(myIcon);
}


GUIIcon
GNEAttributeCarrier::getGUIIcon() const {
    return myIcon;
}


std::vector<SumoXMLAttr>
GNEAttributeCarrier::getAttrs() const {
    std::vector<SumoXMLAttr> attr;
    for (auto i : allowedAttributes(myTag)) {
        attr.push_back(i.first);
    }
    return attr;
}


const std::string
GNEAttributeCarrier::getID() const {
    return getAttribute(SUMO_ATTR_ID);
}


std::string
GNEAttributeCarrier::getAttributeType(SumoXMLTag tag, SumoXMLAttr attr) {
    if (isInt(tag, attr)) {
        return "int";
    } else if (isFloat(tag, attr)) {
        return "float";
    } else if (isTime(tag, attr)) {
        return "time";
    } else if (isBool(tag, attr)) {
        return "bool";
    } else if (isColor(tag, attr)) {
        return "color";
    } else if (isString(tag, attr)) {
        return "string";
    } else if (isList(tag, attr)) {
        return "list";
    } else {
        throw ProcessError("Undeterminted type for '" + toString(tag) + "' '" + toString(attr) + "'");
    }
}


bool
GNEAttributeCarrier::isValidID(const std::string& value) {
    if(value.size() == 0) {
        return false;
    } else {
        return value.find_first_of(" \t\n\r@$%^&/|\\{}*'\";:<>") == std::string::npos;
    }
}


bool
GNEAttributeCarrier::isValidFilename(const std::string& value) {
    // @note Only characteres that aren't permited in a file path or belong
    // to XML sintax
    return (value.find_first_of("\t\n\r@$%^&|\\{}*'\";:<>") == std::string::npos);
}

// ===========================================================================
// static methods
// ===========================================================================

const std::vector<std::pair <SumoXMLAttr, std::string> >&
GNEAttributeCarrier::allowedAttributes(SumoXMLTag tag) {
    // define on first access
    if (!_allowedAttributes.count(tag)) {
        std::vector<std::pair<SumoXMLAttr, std::string> >& attrs = _allowedAttributes[tag];
        switch (tag) {
            case SUMO_TAG_EDGE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FROM, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TO, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SPEED, "13.89"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PRIORITY, "1"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_NUMLANES, "1"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TYPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ALLOW, "all"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DISALLOW, ""));
                //attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PREFER, ));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SHAPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LENGTH, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SPREADTYPE, "right"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_NAME, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_WIDTH, "default"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ENDOFFSET, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(GNE_ATTR_SHAPE_START, "")); // virtual attribute used to define an endPoint
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(GNE_ATTR_SHAPE_END, ""));   // virtual attribute from to define an endPoint
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(GNE_ATTR_BIDIR, ""));       // virtual attribute to check of this edge is part of a bidirectional railway
                break;
            case SUMO_TAG_JUNCTION:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, NODEFAULTVALUE)); // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TYPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SHAPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_RADIUS, "1.5"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_KEEP_CLEAR, "1"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TLTYPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TLID, ""));
                break;
            case SUMO_TAG_LANE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SPEED, "13.89"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ALLOW, "all"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DISALLOW, ""));
                //attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PREFER, ));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_WIDTH, "default"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ENDOFFSET, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ACCELERATION, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CUSTOMSHAPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_INDEX, NODEFAULTVALUE));
                break;
            case SUMO_TAG_POI:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, NODEFAULTVALUE)); // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_COLOR, "red"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TYPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LAYER, "4")); // needed to draw it over lane
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_WIDTH, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_HEIGHT, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_IMGFILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_RELATIVEPATH, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ANGLE, "0"));
                break;
            case SUMO_TAG_POILANE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION_LAT, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_COLOR, "red"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TYPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LAYER, "4")); // needed to draw it over lane
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_WIDTH, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_HEIGHT, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_IMGFILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_RELATIVEPATH, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ANGLE, "0"));
                break;
            case SUMO_TAG_POLY:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SHAPE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_COLOR, "green"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FILL, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LAYER, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TYPE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_IMGFILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_RELATIVEPATH, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ANGLE, "0"));
                break;
            case SUMO_TAG_CROSSING:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_EDGES, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PRIORITY, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_WIDTH, toString(OptionsCont::getOptions().getFloat("default.crossing-width"))));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TLLINKINDEX, "-1"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TLLINKINDEX2, "-1"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CUSTOMSHAPE, ""));
                break;
            case SUMO_TAG_CONNECTION:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FROM, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TO, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FROM_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TO_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PASS, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_KEEP_CLEAR, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CONTPOS, toString(NBEdge::UNSPECIFIED_CONTPOS)));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_UNCONTROLLED, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_VISIBILITY_DISTANCE, toString(NBEdge::UNSPECIFIED_VISIBILITY_DISTANCE)));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TLLINKINDEX, "-1"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SPEED, toString(NBEdge::UNSPECIFIED_SPEED)));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CUSTOMSHAPE, ""));
                break;
            case SUMO_TAG_BUS_STOP:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_STARTPOS, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ENDPOS, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_NAME, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LINES, ""));
                break;
            case SUMO_TAG_ACCESS:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                break;
            case SUMO_TAG_CONTAINER_STOP:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_STARTPOS, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ENDPOS, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_NAME, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LINES, ""));
                break;
            case SUMO_TAG_CHARGING_STATION:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_STARTPOS, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ENDPOS, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_NAME, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CHARGINGPOWER, "22000.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_EFFICIENCY, "0.95"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CHARGEINTRANSIT, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CHARGEDELAY, "0.00"));
                break;
            case SUMO_TAG_E1DETECTOR:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FREQUENCY, "100.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_VTYPES, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                break;
            case SUMO_TAG_E2DETECTOR:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LENGTH, "10.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FREQUENCY, "100.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CONT, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_HALTING_TIME_THRESHOLD, "1.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_HALTING_SPEED_THRESHOLD, "1.39"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_JAM_DIST_THRESHOLD, "10.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                break;
            case SUMO_TAG_E3DETECTOR:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, "0,0")); // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FREQUENCY, "100.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_HALTING_TIME_THRESHOLD, "1.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_HALTING_SPEED_THRESHOLD, "1.39"));
                break;
            case SUMO_TAG_DET_ENTRY:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                break;
            case SUMO_TAG_DET_EXIT:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                break;
            case SUMO_TAG_VSS:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, "0,0")); // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANES, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FILE, ""));
                break;
            case SUMO_TAG_CALIBRATOR:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_EDGE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FREQUENCY, "1.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ROUTEPROBE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_OUTPUT, ""));
                break;
            case SUMO_TAG_LANECALIBRATOR:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FREQUENCY, "100.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ROUTEPROBE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_OUTPUT, ""));
                break;
            case SUMO_TAG_REROUTER:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_EDGES, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_POSITION, "0,0")); // virtual attribute from the combination of the actually attributes SUMO_ATTR_X, SUMO_ATTR_Y
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PROB, "1.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_HALTING_TIME_THRESHOLD, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_OFF, "0"));
                break;
            case SUMO_TAG_ROUTEPROBE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_EDGE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FREQUENCY, OPTIONALATTRIBUTE));                
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_BEGIN, "0"));
                break;
            case SUMO_TAG_VAPORIZER:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_EDGE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_STARTTIME, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_END, "10.00"));
                break;
            case SUMO_TAG_PARKING_AREA:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_STARTPOS, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ENDPOS, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ROADSIDE_CAPACITY, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_FRIENDLY_POS, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_NAME, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_WIDTH, "3.2"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LENGTH, "5.0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ANGLE, "0.00"));
                break;
            case SUMO_TAG_PARKING_SPACE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_X, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_Y, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_Z, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_WIDTH, "3.2"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LENGTH, "5.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ANGLE, "0"));
                break;
            case SUMO_TAG_FLOW:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TYPE, DEFAULT_VTYPE_ID));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ROUTE, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_VEHSPERHOUR, OPTIONALATTRIBUTE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SPEED, OPTIONALATTRIBUTE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_COLOR, "yellow"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_BEGIN, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_END, "100.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DEPARTLANE, "first"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DEPARTPOS, "base"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DEPARTSPEED, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ARRIVALLANE, "current"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ARRIVALPOS, "max"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ARRIVALSPEED, "current"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LINE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PERSON_NUMBER, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CONTAINER_NUMBER, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_REROUTE, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DEPARTPOS_LAT, "center"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ARRIVALPOS_LAT, ""));
                break;
            case SUMO_TAG_ROUTE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_EDGES, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_COLOR, "yellow"));
                break;
            case SUMO_TAG_VTYPE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ACCEL, "2.60"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DECEL, "4.50"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SIGMA, "0.50"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TAU, "1.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LENGTH, "5.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_MINGAP, "2.50"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_MAXSPEED, "70.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SPEEDFACTOR, "1.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SPEEDDEV, "0.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_COLOR, "1,1,0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_VCLASS, "passenger"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_EMISSIONCLASS, "P_7_7"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_GUISHAPE, "passenger"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_WIDTH, "2.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_IMGFILE, ""));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_IMPATIENCE, "0.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LANE_CHANGE_MODEL, "LC2013"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CAR_FOLLOW_MODEL, "Krauss"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PERSON_CAPACITY, "4"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_CONTAINER_CAPACITY, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_BOARDING_DURATION, "0.50"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LOADING_DURATION, "90.00"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_LATALIGNMENT, "center"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_MINGAP_LAT, "0.12"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_MAXSPEED_LAT, "1.00"));
                break;
            case SUMO_TAG_STEP:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_TIME, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_SPEED, "50.00"));
                break;
            case SUMO_TAG_INTERVAL:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_BEGIN, "0"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_END, "100.00"));
                break;
            case SUMO_TAG_CLOSING_REROUTE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ALLOW, "all"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DISALLOW, ""));
                break;
            case SUMO_TAG_CLOSING_LANE_REROUTE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ALLOW, "all"));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_DISALLOW, ""));
                break;
            case SUMO_TAG_DEST_PROB_REROUTE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PROB, "1.00"));
                break;
            case SUMO_TAG_PARKING_ZONE_REROUTE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PROB, "1.00"));
                break;
            case SUMO_TAG_ROUTE_PROB_REROUTE:
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_ID, NODEFAULTVALUE));
                attrs.push_back(std::pair<SumoXMLAttr, std::string>(SUMO_ATTR_PROB, "1.00"));
                break;
            default:
                // Throw exception if tag isn't defined
                throw ProcessError("allowed attributes for tag '" + toString(tag) + "' not defined");
        }
    }
    return _allowedAttributes[tag];
}


std::vector<SumoXMLTag>
GNEAttributeCarrier::allowedTags() {
    std::vector<SumoXMLTag> tags;
    // append all net elements to tags
    tags.insert(std::end(tags), std::begin(allowedNetElementsTags()), std::end(allowedNetElementsTags()));
    tags.insert(std::end(tags), std::begin(allowedAdditionalTags()), std::end(allowedAdditionalTags()));
    tags.insert(std::end(tags), std::begin(allowedShapeTags()), std::end(allowedShapeTags()));
    return tags;
}


const std::vector<SumoXMLTag>&
GNEAttributeCarrier::allowedNetElementsTags() {
    // define on first access
    if (myAllowedNetElementTags.empty()) {
        myAllowedNetElementTags.push_back(SUMO_TAG_EDGE);
        myAllowedNetElementTags.push_back(SUMO_TAG_JUNCTION);
        myAllowedNetElementTags.push_back(SUMO_TAG_LANE);
        myAllowedNetElementTags.push_back(SUMO_TAG_CONNECTION);
        myAllowedNetElementTags.push_back(SUMO_TAG_CROSSING);
    }
    return myAllowedNetElementTags;
}


const std::vector<SumoXMLTag>&
GNEAttributeCarrier::allowedAdditionalTags() {
    // define on first access
    if (myAllowedAdditionalTags.empty()) {
        // Stopping Places
        myAllowedAdditionalTags.push_back(SUMO_TAG_BUS_STOP);
        myAllowedAdditionalTags.push_back(SUMO_TAG_ACCESS);
        myAllowedAdditionalTags.push_back(SUMO_TAG_CONTAINER_STOP);
        myAllowedAdditionalTags.push_back(SUMO_TAG_CHARGING_STATION);
        myAllowedAdditionalTags.push_back(SUMO_TAG_PARKING_AREA);
        myAllowedAdditionalTags.push_back(SUMO_TAG_PARKING_SPACE);
        // Detectors
        myAllowedAdditionalTags.push_back(SUMO_TAG_E1DETECTOR);
        myAllowedAdditionalTags.push_back(SUMO_TAG_E2DETECTOR);
        myAllowedAdditionalTags.push_back(SUMO_TAG_E3DETECTOR);
        myAllowedAdditionalTags.push_back(SUMO_TAG_DET_ENTRY);
        myAllowedAdditionalTags.push_back(SUMO_TAG_DET_EXIT);
        // Related with GNERoute Elements
        myAllowedAdditionalTags.push_back(SUMO_TAG_REROUTER);
        myAllowedAdditionalTags.push_back(SUMO_TAG_VSS);
        myAllowedAdditionalTags.push_back(SUMO_TAG_CALIBRATOR);
        myAllowedAdditionalTags.push_back(SUMO_TAG_LANECALIBRATOR);
        myAllowedAdditionalTags.push_back(SUMO_TAG_ROUTEPROBE);
        myAllowedAdditionalTags.push_back(SUMO_TAG_VAPORIZER);
    }
    return myAllowedAdditionalTags;
}


const std::vector<SumoXMLTag>&
GNEAttributeCarrier::allowedShapeTags() {
    // define on first access
    if (myAllowedShapeTags.empty()) {
        myAllowedShapeTags.push_back(SUMO_TAG_POLY);
        myAllowedShapeTags.push_back(SUMO_TAG_POI);
        myAllowedShapeTags.push_back(SUMO_TAG_POILANE);
    }
    return myAllowedShapeTags;
}


bool
GNEAttributeCarrier::canBlockMovement(SumoXMLTag tag) {
    // define on first access
    if (myBlockMovementTags.empty()) {
        myBlockMovementTags.push_back(SUMO_TAG_BUS_STOP);
        myBlockMovementTags.push_back(SUMO_TAG_ACCESS);
        myBlockMovementTags.push_back(SUMO_TAG_CONTAINER_STOP);
        myBlockMovementTags.push_back(SUMO_TAG_CHARGING_STATION);
        myBlockMovementTags.push_back(SUMO_TAG_E1DETECTOR);
        myBlockMovementTags.push_back(SUMO_TAG_E2DETECTOR);
        myBlockMovementTags.push_back(SUMO_TAG_E3DETECTOR);
        myBlockMovementTags.push_back(SUMO_TAG_DET_ENTRY);
        myBlockMovementTags.push_back(SUMO_TAG_DET_EXIT);
        myBlockMovementTags.push_back(SUMO_TAG_REROUTER);
        myBlockMovementTags.push_back(SUMO_TAG_VSS);
        myBlockMovementTags.push_back(SUMO_TAG_POI);
        myBlockMovementTags.push_back(SUMO_TAG_POILANE);
        myBlockMovementTags.push_back(SUMO_TAG_POLY);
        myBlockMovementTags.push_back(SUMO_TAG_PARKING_AREA);
        myBlockMovementTags.push_back(SUMO_TAG_PARKING_SPACE);
    }
    return std::find(myBlockMovementTags.begin(), myBlockMovementTags.end(), tag) != myBlockMovementTags.end();
}


bool
GNEAttributeCarrier::canBlockShape(SumoXMLTag tag) {
    // define on first access
    if (myBlockShapeTags.empty()) {
        myBlockShapeTags.push_back(SUMO_TAG_POLY);
    }
    return std::find(myBlockShapeTags.begin(), myBlockShapeTags.end(), tag) != myBlockShapeTags.end();
}


bool 
GNEAttributeCarrier::canCloseShape(SumoXMLTag tag) {
    // define on first access
    if (myCloseShapeTags.empty()) {
        myCloseShapeTags.push_back(SUMO_TAG_POLY);
    }
    return std::find(myCloseShapeTags.begin(), myCloseShapeTags.end(), tag) != myCloseShapeTags.end();
}


bool 
GNEAttributeCarrier::canHaveParent(SumoXMLTag tag) {
    // define on first access
    if (myAdditionalsWithParent.empty()) {
        myAdditionalsWithParent[SUMO_TAG_DET_ENTRY] = SUMO_TAG_E3DETECTOR;
        myAdditionalsWithParent[SUMO_TAG_DET_EXIT] = SUMO_TAG_E3DETECTOR;
        myAdditionalsWithParent[SUMO_TAG_PARKING_SPACE] = SUMO_TAG_PARKING_AREA;
    }
    return myAdditionalsWithParent.count(tag) == 1;
}


bool 
GNEAttributeCarrier::canUseGeoPosition(SumoXMLTag tag) {
    // define on first access
    if (myGeoPositionTags.empty()) {
        myGeoPositionTags.push_back(SUMO_TAG_POI);
    }
    return std::find(myGeoPositionTags.begin(), myGeoPositionTags.end(), tag) != myGeoPositionTags.end();
}


bool 
GNEAttributeCarrier::canUseGeoShape(SumoXMLTag tag) {
    // define on first access
    if (myGeoShapeTags.empty()) {
        myGeoShapeTags.push_back(SUMO_TAG_POLY);
    }
    return std::find(myGeoShapeTags.begin(), myGeoShapeTags.end(), tag) != myGeoShapeTags.end();
}


bool
GNEAttributeCarrier::canOpenDialog(SumoXMLTag tag) {
    // define on first access
    if (myDialogTags.empty()) {
        myDialogTags.push_back(SUMO_TAG_CALIBRATOR);
        myDialogTags.push_back(SUMO_TAG_LANECALIBRATOR);
        myDialogTags.push_back(SUMO_TAG_REROUTER);
        myDialogTags.push_back(SUMO_TAG_VSS);
    }
    return std::find(myDialogTags.begin(), myDialogTags.end(), tag) != myDialogTags.end();
}


bool
GNEAttributeCarrier::isNumerical(SumoXMLTag tag, SumoXMLAttr attr) {
    return (isInt(tag, attr) || isFloat(tag, attr));
}


bool
GNEAttributeCarrier::isInt(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myNumericalIntAttrs.empty()) {
        // connection
        myNumericalIntAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_FROM_LANE);
        myNumericalIntAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_TO_LANE);
        myNumericalIntAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_TLLINKINDEX);
        // edge
        myNumericalIntAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_NUMLANES);
        myNumericalIntAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_PRIORITY);
        // lane
        myNumericalIntAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_INDEX);
        // crossing
        myNumericalIntAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_TLLINKINDEX);
        myNumericalIntAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_TLLINKINDEX2);
        // flow
        myNumericalIntAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_PERSON_NUMBER);
        myNumericalIntAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_CONTAINER_NUMBER);
        // vehicle type
        myNumericalIntAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_PERSON_CAPACITY);
        myNumericalIntAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_CONTAINER_CAPACITY);
        // POI
        myNumericalIntAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_LAYER);
        // POILane
        myNumericalIntAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_LAYER);
        // Layer
        myNumericalIntAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_LAYER);
        // ParkingArea
        myNumericalIntAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_ROADSIDE_CAPACITY);
    }
    return myNumericalIntAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isFloat(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myNumericalFloatAttrs.empty()) {
        // bus stop
        myNumericalFloatAttrs[SUMO_TAG_BUS_STOP].insert(SUMO_ATTR_STARTPOS);
        myNumericalFloatAttrs[SUMO_TAG_BUS_STOP].insert(SUMO_ATTR_ENDPOS);
        // stop access
        myNumericalFloatAttrs[SUMO_TAG_ACCESS].insert(SUMO_ATTR_POSITION);
        // charging station
        myNumericalFloatAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_STARTPOS);
        myNumericalFloatAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_ENDPOS);
        myNumericalFloatAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_CHARGINGPOWER);
        myNumericalFloatAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_EFFICIENCY);
        // connection
        myNumericalFloatAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_CONTPOS);
        myNumericalFloatAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_VISIBILITY_DISTANCE);
        myNumericalFloatAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_SPEED);
        // container stop
        myNumericalFloatAttrs[SUMO_TAG_CONTAINER_STOP].insert(SUMO_ATTR_STARTPOS);
        myNumericalFloatAttrs[SUMO_TAG_CONTAINER_STOP].insert(SUMO_ATTR_ENDPOS);
        // crossing
        myNumericalFloatAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_WIDTH);
        // E1
        myNumericalFloatAttrs[SUMO_TAG_E1DETECTOR].insert(SUMO_ATTR_POSITION);
        // E2
        myNumericalFloatAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_POSITION);
        myNumericalFloatAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_HALTING_SPEED_THRESHOLD);
        myNumericalFloatAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_JAM_DIST_THRESHOLD);
        myNumericalFloatAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_LENGTH);
        // E3
        myNumericalFloatAttrs[SUMO_TAG_E3DETECTOR].insert(SUMO_ATTR_HALTING_SPEED_THRESHOLD);
        // Entry
        myNumericalFloatAttrs[SUMO_TAG_DET_ENTRY].insert(SUMO_ATTR_POSITION);
        // Exit
        myNumericalFloatAttrs[SUMO_TAG_DET_EXIT].insert(SUMO_ATTR_POSITION);
        // Edge
        myNumericalFloatAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_ENDOFFSET);
        myNumericalFloatAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_LENGTH);
        myNumericalFloatAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_SPEED);
        // Junction
        myNumericalFloatAttrs[SUMO_TAG_JUNCTION].insert(SUMO_ATTR_RADIUS);
        // Lane
        myNumericalFloatAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_ENDOFFSET);
        myNumericalFloatAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_SPEED);
        // Rerouter
        myNumericalFloatAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_PROB);
        myNumericalFloatAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_HALTING_TIME_THRESHOLD);
        // Calibrator (edge)
        myNumericalFloatAttrs[SUMO_TAG_CALIBRATOR].insert(SUMO_ATTR_POSITION);
        // Calibrator (lane
        myNumericalFloatAttrs[SUMO_TAG_LANECALIBRATOR].insert(SUMO_ATTR_POSITION);
        // vehicle type
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_ACCEL);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_DECEL);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_SIGMA);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_TAU);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_LENGTH);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_MINGAP);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_MAXSPEED);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_SPEEDFACTOR);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_SPEEDDEV);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_WIDTH);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_MINGAP_LAT);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_MAXSPEED_LAT);
        myNumericalFloatAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_IMPATIENCE);
        // flow
        myNumericalFloatAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_VEHSPERHOUR);
        myNumericalFloatAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_SPEED);
        // step
        myNumericalFloatAttrs[SUMO_TAG_STEP].insert(SUMO_ATTR_SPEED);
        // POLY
        myNumericalFloatAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_ANGLE);
        // POI
        myNumericalFloatAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_ANGLE);
        myNumericalFloatAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_WIDTH);
        myNumericalFloatAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_HEIGHT);
        // POILane
        myNumericalFloatAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_POSITION);
        myNumericalFloatAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_POSITION_LAT);
        myNumericalFloatAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_ANGLE);
        myNumericalFloatAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_WIDTH);
        myNumericalFloatAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_HEIGHT);
        // offset value
        myNumericalFloatAttrs[SUMO_TAG_STOPOFFSET].insert(SUMO_ATTR_VALUE);
        // Parking Area
        myNumericalFloatAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_STARTPOS);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_ENDPOS);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_ANGLE);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_WIDTH);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_LENGTH);
        // Parking Space
        myNumericalFloatAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_X);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_Y);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_Z);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_ANGLE);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_WIDTH);
        myNumericalFloatAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_LENGTH);

    }
    return myNumericalFloatAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isTime(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myTimeAttrs.empty()) {
        // calibrator (edge)
        myTimeAttrs[SUMO_TAG_CALIBRATOR].insert(SUMO_ATTR_FREQUENCY);
        // calibrator (lane)
        myTimeAttrs[SUMO_TAG_LANECALIBRATOR].insert(SUMO_ATTR_FREQUENCY);
        // charging station
        myTimeAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_CHARGEDELAY);
        // E1
        myTimeAttrs[SUMO_TAG_E1DETECTOR].insert(SUMO_ATTR_FREQUENCY);
        // E2
        myTimeAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_FREQUENCY);
        myTimeAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_HALTING_TIME_THRESHOLD);
        // E3
        myTimeAttrs[SUMO_TAG_E3DETECTOR].insert(SUMO_ATTR_FREQUENCY);
        myTimeAttrs[SUMO_TAG_E3DETECTOR].insert(SUMO_ATTR_HALTING_TIME_THRESHOLD);
        // RouteProbe
        myTimeAttrs[SUMO_TAG_ROUTEPROBE].insert(SUMO_ATTR_BEGIN);
        myTimeAttrs[SUMO_TAG_ROUTEPROBE].insert(SUMO_ATTR_FREQUENCY);
        // Vaporizer
        myTimeAttrs[SUMO_TAG_VAPORIZER].insert(SUMO_ATTR_END);
        myTimeAttrs[SUMO_TAG_VAPORIZER].insert(SUMO_ATTR_STARTTIME);
        // Vehicle type
        myTimeAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_BOARDING_DURATION);
        myTimeAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_LOADING_DURATION);
        // Flow
        myTimeAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_BEGIN);
        myTimeAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_END);
        // step
        myTimeAttrs[SUMO_TAG_STEP].insert(SUMO_ATTR_TIME);
    }
    return myTimeAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isBool(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myBoolAttrs.empty()) {
        // edge
        myBoolAttrs[SUMO_TAG_EDGE].insert(GNE_ATTR_BIDIR);
        // bus stop
        myBoolAttrs[SUMO_TAG_BUS_STOP].insert(SUMO_ATTR_FRIENDLY_POS);
        // access
        myBoolAttrs[SUMO_TAG_ACCESS].insert(SUMO_ATTR_FRIENDLY_POS);
        // container stop
        myBoolAttrs[SUMO_TAG_CONTAINER_STOP].insert(SUMO_ATTR_FRIENDLY_POS);
        // charging station
        myBoolAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_CHARGEINTRANSIT);
        myBoolAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_FRIENDLY_POS);
        // lane
        myBoolAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_ACCELERATION);
        // connection
        myBoolAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_PASS);
        myBoolAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_UNCONTROLLED);
        myBoolAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_KEEP_CLEAR);
        // crossing
        myBoolAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_PRIORITY);
        // E1
        myBoolAttrs[SUMO_TAG_E1DETECTOR].insert(SUMO_ATTR_FRIENDLY_POS);
        // E2
        myBoolAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_CONT);
        myBoolAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_FRIENDLY_POS);
        // Entry
        myBoolAttrs[SUMO_TAG_DET_ENTRY].insert(SUMO_ATTR_FRIENDLY_POS);
        // Exit
        myBoolAttrs[SUMO_TAG_DET_EXIT].insert(SUMO_ATTR_FRIENDLY_POS);
        // junction
        myBoolAttrs[SUMO_TAG_JUNCTION].insert(SUMO_ATTR_KEEP_CLEAR);
        // rerouter
        myBoolAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_OFF);
        // flow
        myBoolAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_REROUTE);
        // Poly
        myBoolAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_FILL);
        myBoolAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_GEO);
        myBoolAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_RELATIVEPATH);
        // POI
        myBoolAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_GEO);
        myBoolAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_RELATIVEPATH);
        // POILane
        myBoolAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_GEO);
        myBoolAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_RELATIVEPATH);
        // Parking Area
        myBoolAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_FRIENDLY_POS);
    }
    return myBoolAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isColor(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myColorAttrs.empty()) {
        // POI
        myColorAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_COLOR);
        // POILane
        myColorAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_COLOR);
        // Poly
        myColorAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_COLOR);
        // Route
        myColorAttrs[SUMO_TAG_ROUTE].insert(SUMO_ATTR_COLOR);
        // Color
        myColorAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_COLOR);
    }
    return myColorAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isString(SumoXMLTag tag, SumoXMLAttr attr) {
    return (!isNumerical(tag, attr) && !isBool(tag, attr) && !isTime(tag, attr));
}


bool
GNEAttributeCarrier::isList(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myListAttrs.empty()) {
        // bus stop
        myListAttrs[SUMO_TAG_BUS_STOP].insert(SUMO_ATTR_LINES);
        // container stop
        myListAttrs[SUMO_TAG_CONTAINER_STOP].insert(SUMO_ATTR_LINES);
        // crossing
        myListAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_EDGES);
        // rerouter
        myListAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_EDGES);
        // variable speed signal
        myListAttrs[SUMO_TAG_VSS].insert(SUMO_ATTR_LANES);
        // route
        myListAttrs[SUMO_TAG_ROUTE].insert(SUMO_ATTR_EDGES);
        // POLY
        myListAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_SHAPE);
        // vClasses
        myListAttrs[SUMO_TAG_STOPOFFSET].insert(SUMO_ATTR_VCLASSES);
        // Exception vClasses
        myListAttrs[SUMO_TAG_STOPOFFSET].insert(SUMO_ATTR_EXCEPTIONS);
    }
    return myListAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isUnique(SumoXMLTag tag, SumoXMLAttr attr) {
    // ID is an atribute that always is unique
    if (attr == SUMO_ATTR_ID) {
        return true;
    } else {
        // define on first access
        if (myUniqueAttrs.empty()) {
            // connection
            myUniqueAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_FROM);
            myUniqueAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_FROM_LANE);
            myUniqueAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_TO);
            myUniqueAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_TO_LANE);
            myUniqueAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_CUSTOMSHAPE);
            // edge
            myUniqueAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_FROM);
            myUniqueAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_TO);
            // busstop
            myUniqueAttrs[SUMO_TAG_BUS_STOP].insert(SUMO_ATTR_STARTPOS);
            myUniqueAttrs[SUMO_TAG_BUS_STOP].insert(SUMO_ATTR_ENDPOS);
            myUniqueAttrs[SUMO_TAG_BUS_STOP].insert(SUMO_ATTR_LANE);
            // access
            myUniqueAttrs[SUMO_TAG_ACCESS].insert(SUMO_ATTR_LANE);
            myUniqueAttrs[SUMO_TAG_ACCESS].insert(SUMO_ATTR_POSITION);
            // container stop
            myUniqueAttrs[SUMO_TAG_CONTAINER_STOP].insert(SUMO_ATTR_STARTPOS);
            myUniqueAttrs[SUMO_TAG_CONTAINER_STOP].insert(SUMO_ATTR_ENDPOS);
            myUniqueAttrs[SUMO_TAG_CONTAINER_STOP].insert(SUMO_ATTR_LANE);
            // charging station
            myUniqueAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_STARTPOS);
            myUniqueAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_ENDPOS);
            myUniqueAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_LANE);
            // calibrator (edge)
            myUniqueAttrs[SUMO_TAG_CALIBRATOR].insert(SUMO_ATTR_EDGE);
            // calibrator (lane)
            myUniqueAttrs[SUMO_TAG_LANECALIBRATOR].insert(SUMO_ATTR_LANE);
            // crossing
            myUniqueAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_EDGES);
            myUniqueAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_CUSTOMSHAPE);
            // det entry
            myUniqueAttrs[SUMO_TAG_DET_ENTRY].insert(SUMO_ATTR_LANE);
            myUniqueAttrs[SUMO_TAG_DET_ENTRY].insert(SUMO_ATTR_POSITION);
            // det exit
            myUniqueAttrs[SUMO_TAG_DET_EXIT].insert(SUMO_ATTR_LANE);
            myUniqueAttrs[SUMO_TAG_DET_EXIT].insert(SUMO_ATTR_POSITION);
            // E1
            myUniqueAttrs[SUMO_TAG_E1DETECTOR].insert(SUMO_ATTR_FILE);
            myUniqueAttrs[SUMO_TAG_E1DETECTOR].insert(SUMO_ATTR_LANE);
            myUniqueAttrs[SUMO_TAG_E1DETECTOR].insert(SUMO_ATTR_POSITION);
            // E2
            myUniqueAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_FILE);
            myUniqueAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_LANE);
            myUniqueAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_POSITION);
            // E3
            myUniqueAttrs[SUMO_TAG_E3DETECTOR].insert(SUMO_ATTR_FILE);
            myUniqueAttrs[SUMO_TAG_E3DETECTOR].insert(SUMO_ATTR_POSITION);
            // Edge
            myUniqueAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_SHAPE);
            // Junction
            myUniqueAttrs[SUMO_TAG_JUNCTION].insert(SUMO_ATTR_POSITION);
            myUniqueAttrs[SUMO_TAG_JUNCTION].insert(SUMO_ATTR_SHAPE);
            myUniqueAttrs[SUMO_TAG_JUNCTION].insert(SUMO_ATTR_TLID);
            // Rerouter
            myUniqueAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_EDGES);
            myUniqueAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_FILE);
            myUniqueAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_POSITION);
            // Routeprobe
            myUniqueAttrs[SUMO_TAG_ROUTEPROBE].insert(SUMO_ATTR_EDGE);
            myUniqueAttrs[SUMO_TAG_ROUTEPROBE].insert(SUMO_ATTR_FILE);
            // Vaporizer
            myUniqueAttrs[SUMO_TAG_VAPORIZER].insert(SUMO_ATTR_EDGE);
            myUniqueAttrs[SUMO_TAG_VAPORIZER].insert(SUMO_ATTR_FILE);
            // VSS
            myUniqueAttrs[SUMO_TAG_VSS].insert(SUMO_ATTR_POSITION);
            myUniqueAttrs[SUMO_TAG_VSS].insert(SUMO_ATTR_FILE);
            // POI
            myUniqueAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_POSITION);
            myUniqueAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_GEOPOSITION);
            // POILane
            myUniqueAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_LANE);
            myUniqueAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_POSITION);
            // POLY
            myUniqueAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_SHAPE);
            myUniqueAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_GEOSHAPE);
            // Parking Area
            myUniqueAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_STARTPOS);
            myUniqueAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_ENDPOS);
            myUniqueAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_LANE);
            // Parking Space
            myUniqueAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_X);
            myUniqueAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_Y);
        }
        return myUniqueAttrs[tag].count(attr) == 1;
    }
}


bool
GNEAttributeCarrier::isDiscrete(SumoXMLTag tag, SumoXMLAttr attr) {
    if (discreteChoices(tag, attr).size() > 0) {
        return true;
    } else {
        return false;
    }
}


bool
GNEAttributeCarrier::isPositive(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myPositiveAttrs.empty()) {
        // edge
        myPositiveAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_SPEED);
        myPositiveAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_PRIORITY);
        myPositiveAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_NUMLANES);
        myPositiveAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_LENGTH);
        myPositiveAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_WIDTH);
        myPositiveAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_ENDOFFSET);
        // junction
        myPositiveAttrs[SUMO_TAG_JUNCTION].insert(SUMO_ATTR_RADIUS);
        // lane
        myPositiveAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_SPEED);
        myPositiveAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_WIDTH);
        myPositiveAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_ENDOFFSET);
        // crossing
        myPositiveAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_PRIORITY);
        // connection
        myPositiveAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_CONTPOS);
        myPositiveAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_VISIBILITY_DISTANCE);
        myPositiveAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_SPEED);
        // charging station
        myPositiveAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_CHARGINGPOWER);
        myPositiveAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_CHARGEINTRANSIT);
        // E2
        myPositiveAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_LENGTH);
        myPositiveAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_HALTING_TIME_THRESHOLD);
        myPositiveAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_HALTING_SPEED_THRESHOLD);
        myPositiveAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_JAM_DIST_THRESHOLD);
        // calibrator (edge)
        myPositiveAttrs[SUMO_TAG_CALIBRATOR].insert(SUMO_ATTR_POSITION);
        // calibrator (lane)
        myPositiveAttrs[SUMO_TAG_LANECALIBRATOR].insert(SUMO_ATTR_POSITION);
        // flow
        myPositiveAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_PERSON_NUMBER);
        myPositiveAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_CONTAINER_NUMBER);
        myPositiveAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_VEHSPERHOUR);
        myPositiveAttrs[SUMO_TAG_FLOW].insert(SUMO_ATTR_SPEED);
        // vehicle type
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_ACCEL);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_DECEL);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_SIGMA);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_TAU);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_LENGTH);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_MINGAP);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_MAXSPEED);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_SPEEDFACTOR);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_SPEEDDEV);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_WIDTH);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_MINGAP_LAT);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_MAXSPEED_LAT);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_PERSON_CAPACITY);
        myPositiveAttrs[SUMO_TAG_VTYPE].insert(SUMO_ATTR_CONTAINER_CAPACITY);
        // POI
        myPositiveAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_WIDTH);
        myPositiveAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_HEIGHT);
        // POILane
        myPositiveAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_POSITION);
        // offset value
        myPositiveAttrs[SUMO_TAG_STOPOFFSET].insert(SUMO_ATTR_VALUE);
        // Parking Area
        myPositiveAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_ROADSIDE_CAPACITY);
        myPositiveAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_WIDTH);
        myPositiveAttrs[SUMO_TAG_PARKING_AREA].insert(SUMO_ATTR_LENGTH);
        // Parking Space
        myPositiveAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_WIDTH);
        myPositiveAttrs[SUMO_TAG_PARKING_SPACE].insert(SUMO_ATTR_LENGTH);
    }
    return myPositiveAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isProbability(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myProbabilityAttrs.empty()) {
        // charging station
        myProbabilityAttrs[SUMO_TAG_CHARGING_STATION].insert(SUMO_ATTR_EFFICIENCY);
        // rerouter
        myProbabilityAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_PROB);
        // destiny probability reroute
        myProbabilityAttrs[SUMO_TAG_DEST_PROB_REROUTE].insert(SUMO_ATTR_PROB);
        // destiny probability reroute
        myProbabilityAttrs[SUMO_TAG_PARKING_ZONE_REROUTE].insert(SUMO_ATTR_PROB);
        // route prob reroute
        myProbabilityAttrs[SUMO_TAG_ROUTE_PROB_REROUTE].insert(SUMO_ATTR_PROB);
    }
    return myProbabilityAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isFilename(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myFileAttrs.empty()) {
        // E1
        myFileAttrs[SUMO_TAG_E1DETECTOR].insert(SUMO_ATTR_FILE);
        // E2
        myFileAttrs[SUMO_TAG_E2DETECTOR].insert(SUMO_ATTR_FILE);
        // E3
        myFileAttrs[SUMO_TAG_E3DETECTOR].insert(SUMO_ATTR_FILE);
        // calibrator (edge)
        myFileAttrs[SUMO_TAG_CALIBRATOR].insert(SUMO_ATTR_OUTPUT);
        // calibrator (lane)
        myFileAttrs[SUMO_TAG_LANECALIBRATOR].insert(SUMO_ATTR_OUTPUT);
        // rerouter
        myFileAttrs[SUMO_TAG_REROUTER].insert(SUMO_ATTR_FILE);
        // routeprobe
        myFileAttrs[SUMO_TAG_ROUTEPROBE].insert(SUMO_ATTR_FILE);
        // Variable Speed Signal
        myFileAttrs[SUMO_TAG_VSS].insert(SUMO_ATTR_FILE);
        // POI
        myFileAttrs[SUMO_TAG_POI].insert(SUMO_ATTR_IMGFILE);
        // POILane
        myFileAttrs[SUMO_TAG_POILANE].insert(SUMO_ATTR_IMGFILE);
        // POLY
        myFileAttrs[SUMO_TAG_POLY].insert(SUMO_ATTR_IMGFILE);
    }
    return myFileAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isSVCPermissions(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (mySVCPermissionsAttrs.empty()) {
        // Edge
		mySVCPermissionsAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_ALLOW);
		mySVCPermissionsAttrs[SUMO_TAG_EDGE].insert(SUMO_ATTR_DISALLOW);
        // Lane
		mySVCPermissionsAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_ALLOW);
		mySVCPermissionsAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_DISALLOW);
        // Closing Reroute
		mySVCPermissionsAttrs[SUMO_TAG_CLOSING_REROUTE].insert(SUMO_ATTR_ALLOW);
		mySVCPermissionsAttrs[SUMO_TAG_CLOSING_REROUTE].insert(SUMO_ATTR_DISALLOW);
        // Closing Lane Reroute
		mySVCPermissionsAttrs[SUMO_TAG_CLOSING_LANE_REROUTE].insert(SUMO_ATTR_ALLOW);
		mySVCPermissionsAttrs[SUMO_TAG_CLOSING_LANE_REROUTE].insert(SUMO_ATTR_DISALLOW);
    }
    return mySVCPermissionsAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::isNonEditable(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myNonEditableAttrs.empty()) {
        // connection
        myNonEditableAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_FROM);
        myNonEditableAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_FROM_LANE);
        myNonEditableAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_TO);
        myNonEditableAttrs[SUMO_TAG_CONNECTION].insert(SUMO_ATTR_TO_LANE);
        // crossing
        myNonEditableAttrs[SUMO_TAG_CROSSING].insert(SUMO_ATTR_ID);
        // lane
        myNonEditableAttrs[SUMO_TAG_LANE].insert(SUMO_ATTR_INDEX);
    }
    return myNonEditableAttrs[tag].count(attr) == 1;
}


bool
GNEAttributeCarrier::hasAttribute(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if (i.first == attr) {
            return true;
        }
    }
    return false;
}


bool
GNEAttributeCarrier::hasDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if (i.first == attr) {
            return (i.second != NODEFAULTVALUE);
        }
    }
    throw ProcessError("Attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' not defined");
}


const std::vector<std::string>&
GNEAttributeCarrier::discreteChoices(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myDiscreteChoices.empty()) {
        std::vector<std::string> choices;
        // Get type of nodes
        choices = SUMOXMLDefinitions::NodeTypes.getStrings();
        for (auto it : choices) {
            if (it != toString(NODETYPE_DEAD_END_DEPRECATED)) {
                // junction
                myDiscreteChoices[SUMO_TAG_JUNCTION][SUMO_ATTR_TYPE].push_back(it);
            }
        }
        // Get types of traffic lights
        choices = SUMOXMLDefinitions::TrafficLightTypes.getStrings();
        for (auto it : choices) {
            if (it != toString(TLTYPE_INVALID)) {
                // junction
                myDiscreteChoices[SUMO_TAG_JUNCTION][SUMO_ATTR_TLTYPE].push_back(it);
            }
        }
        // get type of lane spread functions
        choices = SUMOXMLDefinitions::LaneSpreadFunctions.getStrings();
        for (auto it : choices) {
            // edge
            myDiscreteChoices[SUMO_TAG_EDGE][SUMO_ATTR_SPREADTYPE].push_back(it);
        }
        // get vehicle types
        choices = SumoVehicleClassStrings.getStrings();
        for (auto it : choices) {
            // edge
            myDiscreteChoices[SUMO_TAG_EDGE][SUMO_ATTR_ALLOW].push_back(it);
            myDiscreteChoices[SUMO_TAG_EDGE][SUMO_ATTR_DISALLOW].push_back(it);
            // lane
            myDiscreteChoices[SUMO_TAG_LANE][SUMO_ATTR_ALLOW].push_back(it);
            myDiscreteChoices[SUMO_TAG_LANE][SUMO_ATTR_DISALLOW].push_back(it);
            // vehicle type
            myDiscreteChoices[SUMO_TAG_VTYPE][SUMO_ATTR_VCLASS].push_back(it);
        }
        // get vehicle shapes
        choices = SumoVehicleShapeStrings.getStrings();
        for (auto it : choices) {
            // vehicle type
            myDiscreteChoices[SUMO_TAG_VTYPE][SUMO_ATTR_GUISHAPE].push_back(it);
        }
        // lat alignments of vehicle types
        myDiscreteChoices[SUMO_TAG_VTYPE][SUMO_ATTR_LATALIGNMENT].push_back("left");
        myDiscreteChoices[SUMO_TAG_VTYPE][SUMO_ATTR_LATALIGNMENT].push_back("right");
        myDiscreteChoices[SUMO_TAG_VTYPE][SUMO_ATTR_LATALIGNMENT].push_back("center");
        myDiscreteChoices[SUMO_TAG_VTYPE][SUMO_ATTR_LATALIGNMENT].push_back("compact");
        myDiscreteChoices[SUMO_TAG_VTYPE][SUMO_ATTR_LATALIGNMENT].push_back("nice");
        myDiscreteChoices[SUMO_TAG_VTYPE][SUMO_ATTR_LATALIGNMENT].push_back("arbitrary");
    }
    return myDiscreteChoices[tag][attr];
}


SumoXMLTag 
GNEAttributeCarrier::getAdditionalParentTag(SumoXMLTag tag) {
    if(canHaveParent(tag)) {
        return myAdditionalsWithParent[tag];
    } else {
        return SUMO_TAG_NOTHING;
    }
}


bool
GNEAttributeCarrier::discreteCombinableChoices(SumoXMLAttr attr) {
    return (attr == SUMO_ATTR_ALLOW || attr == SUMO_ATTR_DISALLOW);
}


const std::string&
GNEAttributeCarrier::getDefinition(SumoXMLTag tag, SumoXMLAttr attr) {
    // define on first access
    if (myAttrDefinitions.empty()) {
        // Edge
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_ID] = setAttrDefinition("The id of the edge.", "Unique");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_FROM] = setAttrDefinition("The name of a node within the nodes-file the edge shall start at.");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_TO] = setAttrDefinition("The name of a node within the nodes-file the edge shall end at.");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_SPEED] = setAttrDefinition("The maximum speed allowed on the edge in m/s.");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_PRIORITY] = setAttrDefinition("The priority of the edge.");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_NUMLANES] = setAttrDefinition("The number of lanes of the edge.");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_TYPE] = setAttrDefinition("The name of a type within the SUMO edge type file");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_ALLOW] = setAttrDefinition("Explicitly allows the given vehicle classes (not given will be not allowed).");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_DISALLOW] = setAttrDefinition("Explicitly disallows the given vehicle classes (not given will be allowed).");
        //myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_PREFER] = setAttrDefinition("");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_SHAPE] = setAttrDefinition("If the shape is given it should start and end with the positions of the from-node and to-node.");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_LENGTH] = setAttrDefinition("The length of the edge in meter");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_SPREADTYPE] = setAttrDefinition("Lane width for all lanes of this edge in meters (used for visualization).");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_NAME] = setAttrDefinition("street name (need not be unique, used for visualization).");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_WIDTH] = setAttrDefinition("Lane width for all lanes of this edge in meters (used for visualization).");
        myAttrDefinitions[SUMO_TAG_EDGE][SUMO_ATTR_ENDOFFSET] = setAttrDefinition("Move the stop line back from the intersection by the given amount.");
        // Junction
        myAttrDefinitions[SUMO_TAG_JUNCTION][SUMO_ATTR_ID] = setAttrDefinition("The id of the node", "Unique");
        myAttrDefinitions[SUMO_TAG_JUNCTION][SUMO_ATTR_POSITION] = setAttrDefinition("The x-y-z position of the node on the plane in meters.");
        myAttrDefinitions[SUMO_TAG_JUNCTION][SUMO_ATTR_TYPE] = setAttrDefinition("An optional type for the node.");
        myAttrDefinitions[SUMO_TAG_JUNCTION][SUMO_ATTR_SHAPE] = setAttrDefinition("A custom shape for that node.");
        myAttrDefinitions[SUMO_TAG_JUNCTION][SUMO_ATTR_RADIUS] = setAttrDefinition("Optional turning radius (for all corners) for that node in meters.");
        myAttrDefinitions[SUMO_TAG_JUNCTION][SUMO_ATTR_KEEP_CLEAR] = setAttrDefinition("Whether the junction-blocking-heuristic should be activated at this node.");
        myAttrDefinitions[SUMO_TAG_JUNCTION][SUMO_ATTR_TLTYPE] = setAttrDefinition("An optional type for the traffic light algorithm.");
        myAttrDefinitions[SUMO_TAG_JUNCTION][SUMO_ATTR_TLID] = setAttrDefinition("An optional id for the traffic light program.");
        // Lane
        myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_ID] = setAttrDefinition("ID of lane (Automatic)");
        myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_SPEED] = setAttrDefinition("Speed in meters per second");
        myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_ALLOW] = setAttrDefinition("Explicitly allows the given vehicle classes (not given will be not allowed).");
        myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_DISALLOW] = setAttrDefinition("Explicitly disallows the given vehicle classes (not given will be allowed).");
        //myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_PREFER] = setAttrDefinition("");
        myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_WIDTH] = setAttrDefinition("Width in meters (used for visualization).");
        myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_ENDOFFSET] = setAttrDefinition("Move the stop line back from the intersection by the given amount.");
        myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_INDEX] = setAttrDefinition("The enumeration index of the lane (0 is the rightmost lane, <NUMBER_LANES>-1 is the leftmost one).");
        myAttrDefinitions[SUMO_TAG_LANE][SUMO_ATTR_CUSTOMSHAPE] = setAttrDefinition("If the shape is given it overrides the computation based on edge shape.");
        // POI
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_ID] = setAttrDefinition("The id of the POI", "Unique");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_COLOR] = setAttrDefinition("The color with which the poi shall be displayed");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_POSITION] = setAttrDefinition("The position in view");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_GEOPOSITION] = setAttrDefinition("The position in view in GEO coordinates");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_TYPE] = setAttrDefinition("A typename for the poi");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_LAYER] = setAttrDefinition("The layer of the poi for drawing and selecting");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_IMGFILE] = setAttrDefinition("A bitmap to use for rendering this poi");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_RELATIVEPATH] = setAttrDefinition("Enable or disbale use image file as a relative path");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_WIDTH] = setAttrDefinition("Width of rendered image in meters");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_HEIGHT] = setAttrDefinition("Height of rendered image in meters");
        myAttrDefinitions[SUMO_TAG_POI][SUMO_ATTR_ANGLE] = setAttrDefinition("Angle of rendered image in degree");
        // POILane
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_ID] = setAttrDefinition("The id of the POI", "Unique");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_COLOR] = setAttrDefinition("The color with which the poi shall be displayed");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_LANE] = setAttrDefinition("The name of the lane the poi is located at); the lane must be a part of the loaded network");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_POSITION] = setAttrDefinition("The position on the named lane or in the net in meters at which the poi is located at");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_POSITION_LAT] = setAttrDefinition("The lateral offset on the named lane at which the poi is located at");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_TYPE] = setAttrDefinition("A typename for the poi");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_LAYER] = setAttrDefinition("The layer of the poi for drawing and selecting");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_IMGFILE] = setAttrDefinition("A bitmap to use for rendering this poi");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_RELATIVEPATH] = setAttrDefinition("Enable or disbale use image file as a relative path");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_WIDTH] = setAttrDefinition("Width of rendered image in meters");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_HEIGHT] = setAttrDefinition("Height of rendered image in meters");
        myAttrDefinitions[SUMO_TAG_POILANE][SUMO_ATTR_ANGLE] = setAttrDefinition("Angle of rendered image in degree");
        // Polygon
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_ID] = setAttrDefinition("The id of the polygon", "Unique");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_COLOR] = setAttrDefinition("The RGBA color with which the polygon shall be displayed");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_SHAPE] = setAttrDefinition("The shape of the polygon");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_GEOSHAPE] = setAttrDefinition("The shape of the polygon in GEO format");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_GEO] = setAttrDefinition("Enable or disable save shape in GEO format");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_FILL] = setAttrDefinition("An information whether the polygon shall be filled");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_LAYER] = setAttrDefinition("The layer in which the polygon lies");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_TYPE] = setAttrDefinition("A typename for the polygon");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_IMGFILE] = setAttrDefinition("A bitmap to use for rendering this polygon");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_RELATIVEPATH] = setAttrDefinition("Enable or disbale use image file as a relative path");
        myAttrDefinitions[SUMO_TAG_POLY][SUMO_ATTR_ANGLE] = setAttrDefinition("Angle of rendered image in degree");
        // Crossing
        myAttrDefinitions[SUMO_TAG_CROSSING][SUMO_ATTR_ID] = setAttrDefinition("The ID of Crossing", "Unique");
        myAttrDefinitions[SUMO_TAG_CROSSING][SUMO_ATTR_PRIORITY] = setAttrDefinition("Whether the pedestrians have priority over the vehicles (automatically set to true at tls-controlled intersections).");
        myAttrDefinitions[SUMO_TAG_CROSSING][SUMO_ATTR_WIDTH] = setAttrDefinition("The width of the crossings.");
        myAttrDefinitions[SUMO_TAG_CROSSING][SUMO_ATTR_EDGES] = setAttrDefinition("The (road) edges which are crossed.");
        myAttrDefinitions[SUMO_TAG_CROSSING][SUMO_ATTR_TLLINKINDEX] = setAttrDefinition("sets the tls-index for this crossing.");
        myAttrDefinitions[SUMO_TAG_CROSSING][SUMO_ATTR_TLLINKINDEX2] = setAttrDefinition("sets the opposite-direction tls-index for this crossing.");
        myAttrDefinitions[SUMO_TAG_CROSSING][SUMO_ATTR_CUSTOMSHAPE] = setAttrDefinition("Overrids default shape of pedestrian crossing.");
        // Connection
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_FROM] = setAttrDefinition("The name of the edge the vehicles leave ");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_TO] = setAttrDefinition("The name of the edge the vehicles may reach when leaving 'from'");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_FROM_LANE] = setAttrDefinition("the lane index of the incoming lane (numbers starting with 0)");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_TO_LANE] = setAttrDefinition("the lane index of the outgoing lane (numbers starting with 0)");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_PASS] = setAttrDefinition("if set, vehicles which pass this (lane-2-lane) connection) will not wait");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_KEEP_CLEAR] = setAttrDefinition("if set to false, vehicles which pass this (lane-2-lane) connection) will not worry about blocking the intersection.");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_CONTPOS] = setAttrDefinition("If set to a more than 0 value, an internal junction will be built at this position (in m) from the start of the internal lane for this connection.");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_UNCONTROLLED] = setAttrDefinition("If set to true, This connection will not be TLS-controlled despite its node being controlled.");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_VISIBILITY_DISTANCE] = setAttrDefinition("sets the distance to the connection at which all relevant foes are visible.");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_TLLINKINDEX] = setAttrDefinition("sets the tls-index for this connection.");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_SPEED] = setAttrDefinition("sets custom speed limit for the connection.");
        myAttrDefinitions[SUMO_TAG_CONNECTION][SUMO_ATTR_CUSTOMSHAPE] = setAttrDefinition("sets custom shape for the connection.");
        // Bus Stop
        myAttrDefinitions[SUMO_TAG_BUS_STOP][SUMO_ATTR_ID] = setAttrDefinition("The id of bus stop", "Unique");
        myAttrDefinitions[SUMO_TAG_BUS_STOP][SUMO_ATTR_LANE] = setAttrDefinition("The name of the lane the bus stop shall be located at");
        myAttrDefinitions[SUMO_TAG_BUS_STOP][SUMO_ATTR_STARTPOS] = setAttrDefinition("The begin position on the lane (the lower position on the lane) in meters");
        myAttrDefinitions[SUMO_TAG_BUS_STOP][SUMO_ATTR_ENDPOS] = setAttrDefinition("The end position on the lane (the higher position on the lane) in meters, must be larger than startPos by more than 0.1m");
        myAttrDefinitions[SUMO_TAG_BUS_STOP][SUMO_ATTR_NAME] = setAttrDefinition("Name of bus stop");
        myAttrDefinitions[SUMO_TAG_BUS_STOP][SUMO_ATTR_LINES] = setAttrDefinition("Meant to be the names of the bus lines that stop at this bus stop. This is only used for visualization purposes");
        myAttrDefinitions[SUMO_TAG_BUS_STOP][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        // stop access
        myAttrDefinitions[SUMO_TAG_ACCESS][SUMO_ATTR_LANE] = setAttrDefinition("The name of the lane the stop access shall be located at");
        myAttrDefinitions[SUMO_TAG_ACCESS][SUMO_ATTR_POSITION] = setAttrDefinition("The position on the lane (the lower position on the lane) in meters");
        myAttrDefinitions[SUMO_TAG_ACCESS][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        // container Stop
        myAttrDefinitions[SUMO_TAG_CONTAINER_STOP][SUMO_ATTR_ID] = setAttrDefinition("The id of container stop", "Unique");
        myAttrDefinitions[SUMO_TAG_CONTAINER_STOP][SUMO_ATTR_LANE] = setAttrDefinition("The name of the lane the container stop shall be located at");
        myAttrDefinitions[SUMO_TAG_CONTAINER_STOP][SUMO_ATTR_STARTPOS] = setAttrDefinition("The begin position on the lane (the lower position on the lane) in meters");
        myAttrDefinitions[SUMO_TAG_CONTAINER_STOP][SUMO_ATTR_ENDPOS] = setAttrDefinition("The end position on the lane (the higher position on the lane) in meters, must be larger than startPos by more than 0.1m");
        myAttrDefinitions[SUMO_TAG_CONTAINER_STOP][SUMO_ATTR_NAME] = setAttrDefinition("Name of Container Stop");
        myAttrDefinitions[SUMO_TAG_CONTAINER_STOP][SUMO_ATTR_LINES] = setAttrDefinition("meant to be the names of the bus lines that stop at this container stop. This is only used for visualization purposes");
        myAttrDefinitions[SUMO_TAG_CONTAINER_STOP][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        // Charging Station
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_ID] = setAttrDefinition("The id of charging station", "Unique");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_LANE] = setAttrDefinition("Lane of the charging station location");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_STARTPOS] = setAttrDefinition("Begin position in the specified lane");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_ENDPOS] = setAttrDefinition("End position in the specified lane");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_NAME] = setAttrDefinition("Name of Charging Station");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_CHARGINGPOWER] = setAttrDefinition("Charging power in W");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_EFFICIENCY] = setAttrDefinition("Charging efficiency [0,1]");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_CHARGEINTRANSIT] = setAttrDefinition("Enable or disable charge in transit, i.e. vehicle must or must not to stop for charging");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_CHARGEDELAY] = setAttrDefinition("Time delay after the vehicles has reached / stopped on the charging station, before the energy transfer (charging) begins");
        myAttrDefinitions[SUMO_TAG_CHARGING_STATION][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        // E1
        myAttrDefinitions[SUMO_TAG_E1DETECTOR][SUMO_ATTR_ID] = setAttrDefinition("The id of E1", "Unique");
        myAttrDefinitions[SUMO_TAG_E1DETECTOR][SUMO_ATTR_LANE] = setAttrDefinition("The id of the lane the detector shall be laid on. The lane must be a part of the network used");
        myAttrDefinitions[SUMO_TAG_E1DETECTOR][SUMO_ATTR_POSITION] = setAttrDefinition("The position on the lane the detector shall be laid on in meters. The position must be a value between -1*lane's length and the lane's length");
        myAttrDefinitions[SUMO_TAG_E1DETECTOR][SUMO_ATTR_FREQUENCY] = setAttrDefinition("The aggregation period the values the detector collects shall be summed up");
        myAttrDefinitions[SUMO_TAG_E1DETECTOR][SUMO_ATTR_FILE] = setAttrDefinition("The path to the output file");
        myAttrDefinitions[SUMO_TAG_E1DETECTOR][SUMO_ATTR_VTYPES] = setAttrDefinition("Space separated list of vehicle type ids to consider");
        myAttrDefinitions[SUMO_TAG_E1DETECTOR][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        // E2
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_ID] = setAttrDefinition("The id of E2", "Unique");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_LANE] = setAttrDefinition("The id of the lane the detector shall be laid on. The lane must be a part of the network used");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_POSITION] = setAttrDefinition("The position on the lane the detector shall be laid on in meters");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_LENGTH] = setAttrDefinition("The length of the detector in meters");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_FREQUENCY] = setAttrDefinition("The aggregation period the values the detector collects shall be summed up");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_FILE] = setAttrDefinition("The path to the output file");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_CONT] = setAttrDefinition("Holds the information whether detectors longer than a lane shall be cut off or continued (set it to true for the second case)); default: false");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_HALTING_TIME_THRESHOLD] = setAttrDefinition("The time-based threshold that describes how much time has to pass until a vehicle is recognized as halting); in s, default: 1s");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_HALTING_SPEED_THRESHOLD] = setAttrDefinition("The speed-based threshold that describes how slow a vehicle has to be to be recognized as halting); in m/s, default: 5/3.6m/s");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_JAM_DIST_THRESHOLD] = setAttrDefinition("The minimum distance to the next standing vehicle in order to make this vehicle count as a participant to the jam); in m, default: 10m");
        myAttrDefinitions[SUMO_TAG_E2DETECTOR][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        // E3
        myAttrDefinitions[SUMO_TAG_E3DETECTOR][SUMO_ATTR_ID] = setAttrDefinition("The id of E3", "Unique");
        myAttrDefinitions[SUMO_TAG_E3DETECTOR][SUMO_ATTR_FREQUENCY] = setAttrDefinition("The aggregation period the values the detector collects shall be summed up");
        myAttrDefinitions[SUMO_TAG_E3DETECTOR][SUMO_ATTR_FILE] = setAttrDefinition("The path to the output file");
        myAttrDefinitions[SUMO_TAG_E3DETECTOR][SUMO_ATTR_HALTING_TIME_THRESHOLD] = setAttrDefinition("The time-based threshold that describes how much time has to pass until a vehicle is recognized as halting); in s, default: 1s");
        myAttrDefinitions[SUMO_TAG_E3DETECTOR][SUMO_ATTR_HALTING_SPEED_THRESHOLD] = setAttrDefinition("The speed-based threshold that describes how slow a vehicle has to be to be recognized as halting); in m/s, default: 5/3.6m/s");
        myAttrDefinitions[SUMO_TAG_E3DETECTOR][SUMO_ATTR_POSITION] = setAttrDefinition("X-Y position of detector in editor (Only used in netedit)");
        // Entry
        myAttrDefinitions[SUMO_TAG_DET_ENTRY][SUMO_ATTR_LANE] = setAttrDefinition("The id of the lane the detector shall be laid on. The lane must be a part of the network used");
        myAttrDefinitions[SUMO_TAG_DET_ENTRY][SUMO_ATTR_POSITION] = setAttrDefinition("The position on the lane the detector shall be laid on in meters");
        myAttrDefinitions[SUMO_TAG_DET_ENTRY][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        // Exit
        myAttrDefinitions[SUMO_TAG_DET_EXIT][SUMO_ATTR_LANE] = setAttrDefinition("The id of the lane the detector shall be laid on. The lane must be a part of the network used");
        myAttrDefinitions[SUMO_TAG_DET_EXIT][SUMO_ATTR_POSITION] = setAttrDefinition("The position on the lane the detector shall be laid on in meters");
        myAttrDefinitions[SUMO_TAG_DET_EXIT][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        // Variable Speed Signal
        myAttrDefinitions[SUMO_TAG_VSS][SUMO_ATTR_ID] = setAttrDefinition("The id of Variable Speed Signal", "Unique");
        myAttrDefinitions[SUMO_TAG_VSS][SUMO_ATTR_LANES] = setAttrDefinition("list of lanes of Variable Speed Sign");
        myAttrDefinitions[SUMO_TAG_VSS][SUMO_ATTR_FILE] = setAttrDefinition("The path to the output file");
        myAttrDefinitions[SUMO_TAG_VSS][SUMO_ATTR_POSITION] = setAttrDefinition("X-Y position of detector in editor (Only used in netedit)");
        // Calibrator
        myAttrDefinitions[SUMO_TAG_CALIBRATOR][SUMO_ATTR_ID] = setAttrDefinition("The id of Calibrator", "Unique");
        myAttrDefinitions[SUMO_TAG_CALIBRATOR][SUMO_ATTR_EDGE] = setAttrDefinition("The id of edge in the simulation network");
        myAttrDefinitions[SUMO_TAG_CALIBRATOR][SUMO_ATTR_POSITION] = setAttrDefinition("The position of the calibrator on the specified lane");
        myAttrDefinitions[SUMO_TAG_CALIBRATOR][SUMO_ATTR_FREQUENCY] = setAttrDefinition("The aggregation interval in which to calibrate the flows. default is step-length");
        myAttrDefinitions[SUMO_TAG_CALIBRATOR][SUMO_ATTR_ROUTEPROBE] = setAttrDefinition("The id of the routeProbe element from which to determine the route distribution for generated vehicles");
        myAttrDefinitions[SUMO_TAG_CALIBRATOR][SUMO_ATTR_OUTPUT] = setAttrDefinition("The output file for writing calibrator information or NULL");
        // LaneCalibrator
        myAttrDefinitions[SUMO_TAG_LANECALIBRATOR][SUMO_ATTR_ID] = setAttrDefinition("The id of Calibrator", "Unique");
        myAttrDefinitions[SUMO_TAG_LANECALIBRATOR][SUMO_ATTR_LANE] = setAttrDefinition("The id of lane in the simulation network");
        myAttrDefinitions[SUMO_TAG_LANECALIBRATOR][SUMO_ATTR_POSITION] = setAttrDefinition("The position of the calibrator on the specified lane");
        myAttrDefinitions[SUMO_TAG_LANECALIBRATOR][SUMO_ATTR_FREQUENCY] = setAttrDefinition("The aggregation interval in which to calibrate the flows. default is step-length");
        myAttrDefinitions[SUMO_TAG_LANECALIBRATOR][SUMO_ATTR_ROUTEPROBE] = setAttrDefinition("The id of the routeProbe element from which to determine the route distribution for generated vehicles");
        myAttrDefinitions[SUMO_TAG_LANECALIBRATOR][SUMO_ATTR_OUTPUT] = setAttrDefinition("The output file for writing calibrator information or NULL");
        // Rerouter
        myAttrDefinitions[SUMO_TAG_REROUTER][SUMO_ATTR_ID] = setAttrDefinition("The id of Rerouter", "Unique");
        myAttrDefinitions[SUMO_TAG_REROUTER][SUMO_ATTR_EDGES] = setAttrDefinition("An edge id or a list of edge ids where vehicles shall be rerouted");
        myAttrDefinitions[SUMO_TAG_REROUTER][SUMO_ATTR_FILE] = setAttrDefinition("The path to the definition file (alternatively, the intervals may defined as children of the rerouter)");
        myAttrDefinitions[SUMO_TAG_REROUTER][SUMO_ATTR_PROB] = setAttrDefinition("The probability for vehicle rerouting (0-1), default 1");
        myAttrDefinitions[SUMO_TAG_REROUTER][SUMO_ATTR_HALTING_TIME_THRESHOLD] = setAttrDefinition("The waiting time threshold (in s) that must be reached to activate rerouting (default -1 which disables the threshold)");
        myAttrDefinitions[SUMO_TAG_REROUTER][SUMO_ATTR_OFF] = setAttrDefinition("Whether the router should be inactive initially (and switched on in the gui), default:false");
        myAttrDefinitions[SUMO_TAG_REROUTER][SUMO_ATTR_POSITION] = setAttrDefinition("X,Y position in editor (Only used in netedit)");
        // route probe
        myAttrDefinitions[SUMO_TAG_ROUTEPROBE][SUMO_ATTR_ID] = setAttrDefinition("The id of RouteProbe", "Unique");
        myAttrDefinitions[SUMO_TAG_ROUTEPROBE][SUMO_ATTR_EDGE] = setAttrDefinition("The id of an edge in the simulation network");
        myAttrDefinitions[SUMO_TAG_ROUTEPROBE][SUMO_ATTR_FREQUENCY] = setAttrDefinition("The frequency in which to report the distribution");
        myAttrDefinitions[SUMO_TAG_ROUTEPROBE][SUMO_ATTR_FILE] = setAttrDefinition("The file for generated output");
        myAttrDefinitions[SUMO_TAG_ROUTEPROBE][SUMO_ATTR_BEGIN] = setAttrDefinition("The time at which to start generating output");
        // Parking Area
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_ID] = setAttrDefinition("The id of ParkingArea", "Unique");
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_LANE] = setAttrDefinition("The name of the lane the Parking Area shall be located at");
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_STARTPOS] = setAttrDefinition("The begin position on the lane (the lower position on the lane) in meters");
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_ENDPOS] = setAttrDefinition("The end position on the lane (the higher position on the lane) in meters, must be larger than startPos by more than 0.1m");
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_NAME] = setAttrDefinition("Name of Parking Area");
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_FRIENDLY_POS] = setAttrDefinition("If set, no error will be reported if element is placed behind the lane. Instead,it will be placed 0.1 meters from the lanes end or at position 0.1, if the position was negative and larger than the lanes length after multiplication with - 1");
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_WIDTH] = setAttrDefinition("The width of the road-side parking spaces");
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_LENGTH] = setAttrDefinition("The length of the road-side parking spaces");
        myAttrDefinitions[SUMO_TAG_PARKING_AREA][SUMO_ATTR_ANGLE] = setAttrDefinition("The angle of the road-side parking spaces relative to the lane angle, positive means clockwise");
        // Parking Space
        myAttrDefinitions[SUMO_TAG_PARKING_SPACE][SUMO_ATTR_X] = setAttrDefinition("The X position in meters of the parking vehicle");
        myAttrDefinitions[SUMO_TAG_PARKING_SPACE][SUMO_ATTR_Y] = setAttrDefinition("The Y positions in meters of the parking vehicle");
        myAttrDefinitions[SUMO_TAG_PARKING_SPACE][SUMO_ATTR_Z] = setAttrDefinition("The Z position in meters of the parking vehicle");
        myAttrDefinitions[SUMO_TAG_PARKING_SPACE][SUMO_ATTR_WIDTH] = setAttrDefinition("The width of the road-side parking spaces");
        myAttrDefinitions[SUMO_TAG_PARKING_SPACE][SUMO_ATTR_LENGTH] = setAttrDefinition("The length of the road-side parking spaces");
        myAttrDefinitions[SUMO_TAG_PARKING_SPACE][SUMO_ATTR_ANGLE] = setAttrDefinition("The angle of the road-side parking spaces relative to the lane angle, positive means clockwise");
        // flow
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_TYPE] = setAttrDefinition("The id of the vehicle type to use for this vehicle.");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_VEHSPERHOUR] = setAttrDefinition("Number of vehicles per hour, equally spaced");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_SPEED] = setAttrDefinition("Speed of vehicles");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_ROUTE] = setAttrDefinition("The id of the route the vehicle shall drive along");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_BEGIN] = setAttrDefinition("First vehicle departure time");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_END] = setAttrDefinition("End of departure interval");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_DEPARTLANE] = setAttrDefinition("The lane on which the vehicle shall be inserted");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_DEPARTPOS] = setAttrDefinition("The position at which the vehicle shall enter the net");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_DEPARTSPEED] = setAttrDefinition("The speed with which the vehicle shall enter the network");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_ARRIVALLANE] = setAttrDefinition("The lane at which the vehicle shall leave the network");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_ARRIVALPOS] = setAttrDefinition("The position at which the vehicle shall leave the network");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_ARRIVALSPEED] = setAttrDefinition("The speed with which the vehicle shall leave the network");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_LINE] = setAttrDefinition("A string specifying the id of a public transport line which can be used when specifying person rides");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_PERSON_NUMBER] = setAttrDefinition("The number of occupied seats when the vehicle is inserted");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_CONTAINER_NUMBER] = setAttrDefinition("The number of occupied container places when the vehicle is inserted");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_REROUTE] = setAttrDefinition(" Whether the vehicle should be equipped with a rerouting device");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_DEPARTPOS_LAT] = setAttrDefinition("The lateral position on the departure lane at which the vehicle shall enter the net");
        myAttrDefinitions[SUMO_TAG_FLOW][SUMO_ATTR_ARRIVALPOS_LAT] = setAttrDefinition("The lateral position on the arrival lane at which the vehicle shall arrive");
        // route
        myAttrDefinitions[SUMO_TAG_ROUTE][SUMO_ATTR_ID] = setAttrDefinition("The id of Route", "Unique");
        myAttrDefinitions[SUMO_TAG_ROUTE][SUMO_ATTR_EDGES] = setAttrDefinition("The edges the vehicle shall drive along, given as their ids, separated using spaces");
        myAttrDefinitions[SUMO_TAG_ROUTE][SUMO_ATTR_COLOR] = setAttrDefinition("This route's color");
        // vehicle type
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_ID] = setAttrDefinition("The id of VehicleType", "Unique");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_ACCEL] = setAttrDefinition("The acceleration ability of vehicles of this type [m/s^2]");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_DECEL] = setAttrDefinition("The deceleration ability of vehicles of this type [m/s^2]");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_SIGMA] = setAttrDefinition("Car-following model parameter");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_TAU] = setAttrDefinition("Car-following model parameter");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_LENGTH] = setAttrDefinition("The vehicle's netto-length (length) [m]");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_MINGAP] = setAttrDefinition("Empty space after leader [m]");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_MAXSPEED] = setAttrDefinition("The vehicle's maximum velocity [m/s]");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_SPEEDFACTOR] = setAttrDefinition("The vehicles expected multiplicator for lane speed limits");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_SPEEDDEV] = setAttrDefinition("The deviation of the speedFactor");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_COLOR] = setAttrDefinition("This vehicle type's color");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_VCLASS] = setAttrDefinition("An abstract vehicle class");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_EMISSIONCLASS] = setAttrDefinition("An abstract emission class");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_GUISHAPE] = setAttrDefinition("How this vehicle is rendered");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_WIDTH] = setAttrDefinition("The vehicle's width [m] (only used for drawing)");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_IMGFILE] = setAttrDefinition("Image file for rendering vehicles of this type (should be grayscale to allow functional coloring)");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_IMPATIENCE] = setAttrDefinition("Willingess of drivers to impede vehicles with higher priority");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_LANE_CHANGE_MODEL] = setAttrDefinition("The model used for changing lanes");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_CAR_FOLLOW_MODEL] = setAttrDefinition("The model used for car following");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_PERSON_CAPACITY] = setAttrDefinition("The number of persons (excluding an autonomous driver) the vehicle can transport");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_CONTAINER_CAPACITY] = setAttrDefinition("The number of containers the vehicle can transport");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_BOARDING_DURATION] = setAttrDefinition("The time required by a person to board the vehicle");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_LOADING_DURATION] = setAttrDefinition("The time required to load a container onto the vehicle");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_LATALIGNMENT] = setAttrDefinition("The preferred lateral alignment when using the sublane-model");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_MINGAP_LAT] = setAttrDefinition("The minimum lateral gap at a speed difference of 50km/h when using the sublane-model");
        myAttrDefinitions[SUMO_TAG_VTYPE][SUMO_ATTR_MAXSPEED_LAT] = setAttrDefinition("The maximum lateral speed when using the sublane-model");
        // step
        myAttrDefinitions[SUMO_TAG_STEP][SUMO_ATTR_TIME] = setAttrDefinition("Time");
        myAttrDefinitions[SUMO_TAG_STEP][SUMO_ATTR_SPEED] = setAttrDefinition("Speed");
        // interval
        myAttrDefinitions[SUMO_TAG_INTERVAL][SUMO_ATTR_BEGIN] = setAttrDefinition("Begin");
        myAttrDefinitions[SUMO_TAG_INTERVAL][SUMO_ATTR_END] = setAttrDefinition("End");
        // closing reroute
        myAttrDefinitions[SUMO_TAG_CLOSING_REROUTE][SUMO_ATTR_ID] = setAttrDefinition("Edge ID");
        myAttrDefinitions[SUMO_TAG_CLOSING_REROUTE][SUMO_ATTR_ALLOW] = setAttrDefinition("allowed vehicles");
        myAttrDefinitions[SUMO_TAG_CLOSING_REROUTE][SUMO_ATTR_DISALLOW] = setAttrDefinition("disallowed vehicles");
        // closing lane reroute
        myAttrDefinitions[SUMO_TAG_CLOSING_LANE_REROUTE][SUMO_ATTR_ID] = setAttrDefinition("Lane ID");
        myAttrDefinitions[SUMO_TAG_CLOSING_LANE_REROUTE][SUMO_ATTR_ALLOW] = setAttrDefinition("allowed vehicles");
        myAttrDefinitions[SUMO_TAG_CLOSING_LANE_REROUTE][SUMO_ATTR_DISALLOW] = setAttrDefinition("disallowed vehicles");
        // destiny probability reroute
        myAttrDefinitions[SUMO_TAG_DEST_PROB_REROUTE][SUMO_ATTR_ID] = setAttrDefinition("Edge ID");
        myAttrDefinitions[SUMO_TAG_DEST_PROB_REROUTE][SUMO_ATTR_PROB] = setAttrDefinition("probability");
        // parkingAreaReroute
        myAttrDefinitions[SUMO_TAG_PARKING_ZONE_REROUTE][SUMO_ATTR_ID] = setAttrDefinition("ParkingArea ID");
        myAttrDefinitions[SUMO_TAG_PARKING_ZONE_REROUTE][SUMO_ATTR_PROB] = setAttrDefinition("probability");
        // route probability reroute
        myAttrDefinitions[SUMO_TAG_ROUTE_PROB_REROUTE][SUMO_ATTR_ID] = setAttrDefinition("Route");
        myAttrDefinitions[SUMO_TAG_ROUTE_PROB_REROUTE][SUMO_ATTR_PROB] = setAttrDefinition("probability");
    }
    return myAttrDefinitions[tag][attr].first;
}

const std::string&
GNEAttributeCarrier::getRestriction(SumoXMLTag tag, SumoXMLAttr attr) {
    // check if definitions and restrictions are already defined
    if(myAttrDefinitions.empty()) {
        // call getDefinition to fill myAttrDefinitions
        getDefinition(tag, attr);
    }
    return myAttrDefinitions[tag][attr].second;
}

int
GNEAttributeCarrier::getHigherNumberOfAttributes() {
    if (myMaxNumAttribute == 0) {
        for (auto i : allowedNetElementsTags()) {
            myMaxNumAttribute = MAX2(myMaxNumAttribute, (int)allowedAttributes(i).size());
        }
        for (auto i : allowedAdditionalTags()) {
            myMaxNumAttribute = MAX2(myMaxNumAttribute, (int)allowedAttributes(i).size());
        }
        for (auto i : allowedShapeTags()) {
            myMaxNumAttribute = MAX2(myMaxNumAttribute, (int)allowedAttributes(i).size());
        }
    }
    return myMaxNumAttribute;
}


template<> int
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<int>(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> double
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<double>(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> bool
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<bool>(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> std::string
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return (i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> Position
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<Position>(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> SUMOVehicleClass
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<SUMOVehicleClass>(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> SUMOVehicleShape
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<SUMOVehicleShape>(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> RGBColor
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<RGBColor>(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> std::vector<int>
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<std::vector<int> >(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> std::vector<double>
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<std::vector<double> >(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> std::vector<bool>
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<std::vector<bool> >(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


template<> std::vector<std::string>
GNEAttributeCarrier::getDefaultValue(SumoXMLTag tag, SumoXMLAttr attr) {
    for (auto i : allowedAttributes(tag)) {
        if ((i.first == attr) && (i.second != NODEFAULTVALUE)) {
            return parse<std::vector<std::string> >(i.second);
        }
    }
    // throw exception if attribute doesn't have a default value and return a empty value to avoid warnings
    throw ProcessError("attribute '" + toString(attr) + "' for tag '" + toString(tag) + "' doesn't have a default value");
}


bool
GNEAttributeCarrier::checkGNEEdgesValid(GNENet* net, const std::string& value, bool report) {
    // Declare string vector
    std::vector<std::string> edgeIds = GNEAttributeCarrier::parse<std::vector<std::string> > (value);
    // Iterate over edges IDs and check if edge exist
    for (auto i : edgeIds) {
        if (net->retrieveEdge(i, false) == nullptr) {
            if (report) {
                WRITE_WARNING("Error parsing parameter " + toString(SUMO_ATTR_EDGES) + ". " + toString(SUMO_TAG_EDGE) + " '" + i + "'  doesn't exist.");
            }
            return false;
        }
    }
    // all edges exist, then return true
    return true;
}


bool
GNEAttributeCarrier::checkGNELanesValid(GNENet* net, const std::string& value, bool report) {
    // Declare string vector
    std::vector<std::string> laneIds = GNEAttributeCarrier::parse<std::vector<std::string> > (value);
    // Iterate over lanes IDs and check if lane exist
    for (auto i : laneIds) {
        if (net->retrieveLane(i, false) == nullptr) {
            if (report) {
                WRITE_WARNING("Error parsing parameter " + toString(SUMO_ATTR_LANES) + ". " + toString(SUMO_TAG_LANE) + " '" + i + "'  doesn't exist.");
            }
            return false;
        }
    }
    // all lanes exist, then return true
    return true;
}


std::vector<GNEEdge*>
GNEAttributeCarrier::parseGNEEdges(GNENet* net, const std::string& value) {
    // Declare string vector
    std::vector<std::string> edgeIds = GNEAttributeCarrier::parse<std::vector<std::string> > (value);
    std::vector<GNEEdge*> parsedEdges;
    // Iterate over edges IDs, retrieve Edges and add it into parsedEdges
    for (auto i : edgeIds) {
        parsedEdges.push_back(net->retrieveEdge(i));
    }
    return parsedEdges;
}


std::vector<GNELane*>
GNEAttributeCarrier::parseGNELanes(GNENet* net, const std::string& value) {
    // Declare string vector
    std::vector<std::string> laneIds = GNEAttributeCarrier::parse<std::vector<std::string> > (value);
    std::vector<GNELane*> parsedLanes;
    // Iterate over lanes IDs, retrieve Lanes and add it into parsedLanes
    for (auto i : laneIds) {
        parsedLanes.push_back(net->retrieveLane(i));
    }
    return parsedLanes;
}


std::string
GNEAttributeCarrier::parseGNEEdges(const std::vector<GNEEdge*>& edges) {
    // obtain ID's of edges and return their join
    std::vector<std::string> edgeIDs;
    for (auto i : edges) {
        edgeIDs.push_back(i->getID());
    }
    return joinToString(edgeIDs, " ");
}


std::string
GNEAttributeCarrier::parseGNELanes(const std::vector<GNELane*>& lanes) {
    // obtain ID's of lanes and return their join
    std::vector<std::string> laneIDs;
    for (auto i : lanes) {
        laneIDs.push_back(i->getID());
    }
    return joinToString(laneIDs, " ");
}


int 
GNEAttributeCarrier::getCircleResolution(const GUIVisualizationSettings& settings) {
    if(settings.drawForSelecting) {
        return 8;
    } else if (settings.scale >= 10) {
        return 32;
    } else if (settings.scale >= 2) {
        return 16;
    } else {
        return 8;
    }
}


void 
GNEAttributeCarrier::writeAttribute(OutputDevice& device, SumoXMLAttr key) const {
    std::string attribute = getAttribute(key);
    if(key == GNE_ATTR_BLOCK_MOVEMENT) {
        // only write Block Movement if is enabled
        if(attribute == "1") {
            device.writeAttr(key, attribute);
        }
    } else if(hasDefaultValue(getTag(), key)) {
        // only write optional attributes (i.e attributes with default value) if are differents
        if(getDefaultValue<std::string>(getTag(), key) != attribute) {
            device.writeAttr(key, attribute);
        }
    } else {
        device.writeAttr(key, attribute);
    }
}


std::pair<std::string, std::string> 
GNEAttributeCarrier::setAttrDefinition(const std::string &definition, const std::string &restriction) {
    return std::pair<std::string, std::string>(definition, restriction);
}

/****************************************************************************/

