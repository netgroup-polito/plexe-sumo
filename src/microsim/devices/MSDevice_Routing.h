/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2007-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    MSDevice_Routing.h
/// @author  Michael Behrisch
/// @author  Daniel Krajzewicz
/// @author  Jakob Erdmann
/// @date    Tue, 04 Dec 2007
/// @version $Id$
///
// A device that performs vehicle rerouting based on current edge speeds
/****************************************************************************/
#ifndef MSDevice_Routing_h
#define MSDevice_Routing_h


// ===========================================================================
// included modules
// ===========================================================================
#include <config.h>

#include <set>
#include <vector>
#include <map>
#include <utils/common/SUMOTime.h>
#include <utils/common/WrappingCommand.h>
#include <utils/vehicle/SUMOAbstractRouter.h>
#include <utils/vehicle/AStarRouter.h>
#include <microsim/MSVehicle.h>
#include "MSDevice.h"

#ifdef HAVE_FOX
#include <utils/foxtools/FXWorkerThread.h>
#endif


// ===========================================================================
// class declarations
// ===========================================================================
class MSLane;

// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class MSDevice_Routing
 * @brief A device that performs vehicle rerouting based on current edge speeds
 *
 * The routing-device system consists of in-vehicle devices that perform a routing
 *  and a simulation-wide (static) methods for colecting edge weights.
 *
 * The edge weights container "myEdgeSpeeds" is pre-initialised as soon as one
 *  device is built and is kept updated via an event that adapts it to the current
 *  mean speed on the simulated network's edges.
 *
 * A device is assigned to a vehicle using the common explicit/probability - procedure.
 *
 * A device computes a new route for a vehicle as soon as the vehicle is inserted
 *  (within "enterLaneAtInsertion") - and, if the given period is larger than 0 - each
 *  x time steps where x is the period. This is triggered by an event that executes
 *  "wrappedRerouteCommandExecute".
 */
class MSDevice_Routing : public MSDevice {
public:
    /** @brief Inserts MSDevice_Routing-options
     * @param[filled] oc The options container to add the options to
     */
    static void insertOptions(OptionsCont& oc);

    /** @brief checks MSDevice_Routing-options
     * @param[filled] oc The options container with the user-defined options
     */
    static bool checkOptions(OptionsCont& oc);


    /** @brief Build devices for the given vehicle, if needed
     *
     * The options are read and evaluated whether rerouting-devices shall be built
     *  for the given vehicle.
     *
     * When the first device is built, the static container of edge weights
     *  used for routing is initialised with the mean speed the edges allow.
     *  In addition, an event is generated which updates these weights is
     *  built and added to the list of events to execute at a simulation end.
     *
     * For each seen vehicle, the global vehicle index is increased.
     *
     * The built device is stored in the given vector.
     *
     * @param[in] v The vehicle for which a device may be built
     * @param[filled] into The vector to store the built device in
     */
    static void buildVehicleDevices(SUMOVehicle& v, std::vector<MSDevice*>& into);


    /// @brief Destructor.
    ~MSDevice_Routing();



    /// @name Methods called on vehicle movement / state change, overwriting MSDevice
    /// @{

    /** @brief Computes a new route on vehicle insertion
     *
     * A new route is computed by calling the vehicle's "reroute" method, supplying
     *  "getEffort" as the edge effort retrieval method.
     *
     * If the reroute period is larger than 0, an event is generated and added
     *  to the list of simulation step begin events which executes
     *  "wrappedRerouteCommandExecute".
     *
     * @param[in] veh The entering vehicle.
     * @param[in] reason how the vehicle enters the lane
     * @return Always false
     * @see MSMoveReminder::notifyEnter
     * @see MSMoveReminder::Notification
     * @see MSVehicle::reroute
     * @see MSEventHandler
     * @see WrappingCommand
     */
    bool notifyEnter(SUMOVehicle& veh, MSMoveReminder::Notification reason, const MSLane* enteredLane = 0);
    /// @}

    /// @brief return the name for this type of device
    const std::string deviceName() const {
        return "rerouting";
    }

    /** @brief Saves the state of the device
     *
     * @param[in] out The OutputDevice to write the information into
     */
    void saveState(OutputDevice& out) const;

    /** @brief Loads the state of the device from the given description
     *
     * @param[in] attrs XML attributes describing the current state
     */
    void loadState(const SUMOSAXAttributes& attrs);

    /// @brief initiate the rerouting, create router / thread pool on first use
    void reroute(const SUMOTime currentTime, const bool onInit = false);


    /** @brief Labels the current time step as "unroutable".
     *
     * Sets mySkipRouting to the current time in order to skip rerouting.
     * This is useful for pre insertion routing when we know in advance
     * we cannot insert.
     *
     * @param[in] currentTime The current simulation time
     */
    void skipRouting(const SUMOTime currentTime) {
        mySkipRouting = currentTime;
    }

    /// @brief try to retrieve the given parameter from this device. Throw exception for unsupported key
    std::string getParameter(const std::string& key) const;

    /// @brief try to set the given parameter for this device. Throw exception for unsupported key
    void setParameter(const std::string& key, const std::string& value);


private:

    /** @brief Constructor
     *
     * @param[in] holder The vehicle that holds this device
     * @param[in] id The ID of the device
     * @param[in] period The period with which a new route shall be searched
     * @param[in] preInsertionPeriod The route search period before insertion
     */
    MSDevice_Routing(SUMOVehicle& holder, const std::string& id, SUMOTime period, SUMOTime preInsertionPeriod);

    /** @brief Performs rerouting before insertion into the network
     *
     * A new route is computed by calling the reroute method. If the routing
     *  involves taz the internal route cache is asked beforehand.
     *
     * @param[in] currentTime The current simulation time
     * @return The offset to the next call (the rerouting period "myPreInsertionPeriod")
     * @see MSVehicle::reroute
     * @see MSEventHandler
     * @see WrappingCommand
     */
    SUMOTime preInsertionReroute(const SUMOTime currentTime);

    /** @brief Performs rerouting after a period
     *
     * A new route is computed by calling the vehicle's "reroute" method, supplying
     *  "getEffort" as the edge effort retrieval method.
     *
     * This method is called from the event handler at the begin of a simulation
     *  step after the rerouting period is over. The reroute period is returned.
     *
     * @param[in] currentTime The current simulation time
     * @return The offset to the next call (the rerouting period "myPeriod")
     * @see MSVehicle::reroute
     * @see MSEventHandler
     * @see WrappingCommand
     */
    SUMOTime wrappedRerouteCommandExecute(SUMOTime currentTime);


private:
    /// @brief The period with which a vehicle shall be rerouted
    SUMOTime myPeriod;

    /// @brief The period with which a vehicle shall be rerouted before insertion
    SUMOTime myPreInsertionPeriod;

    /// @brief The last time a routing took place
    SUMOTime myLastRouting;

    /// @brief The time for which routing may be skipped because we cannot be inserted
    SUMOTime mySkipRouting;

    /// @brief The (optional) command responsible for rerouting
    WrappingCommand< MSDevice_Routing >* myRerouteCommand;

private:
    /// @brief Invalidated copy constructor.
    MSDevice_Routing(const MSDevice_Routing&);

    /// @brief Invalidated assignment operator.
    MSDevice_Routing& operator=(const MSDevice_Routing&);


};


#endif

/****************************************************************************/

