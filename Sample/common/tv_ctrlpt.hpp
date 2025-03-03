#ifndef UPNPLIB_TV_CTRLPT_HPP
#define UPNPLIB_TV_CTRLPT_HPP
/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2022 GPL 3 and higher by Ingo Höft,  <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-03-03
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * - Neither name of Intel Corporation nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************/
/*!
 * \file
 * \brief TV Control Point sample program
 * \addtogroup UpnpSamples
 * @{
 */

#include "sample_util.hpp"

#include "UpnpString.hpp"
#include "upnp.hpp"
#include "upnptools.hpp"
#include "pthread.h" // To find pthreads4w don't use <pthread.h>

#include <signal.h>
#include <stdarg.h>
#include <stdio.h>

/// \cond
#define TV_SERVICE_SERVCOUNT 2
#define TV_SERVICE_CONTROL 0
#define TV_SERVICE_PICTURE 1

#define TV_CONTROL_VARCOUNT 3
#define TV_CONTROL_POWER 0
#define TV_CONTROL_CHANNEL 1
#define TV_CONTROL_VOLUME 2

#define TV_PICTURE_VARCOUNT 4
#define TV_PICTURE_COLOR 0
#define TV_PICTURE_TINT 1
#define TV_PICTURE_CONTRAST 2
#define TV_PICTURE_BRIGHTNESS 3

#define TV_MAX_VAL_LEN 5

#define TV_SUCCESS 0
#define TV_ERROR (-1)
#define TV_WARNING 1

/* This should be the maximum VARCOUNT from above */
#define TV_MAXVARS TV_PICTURE_VARCOUNT

extern const char* TvServiceName[];
extern const char* TvVarName[TV_SERVICE_SERVCOUNT][TV_MAXVARS];
extern char TvVarCount[];
/// \endcond

/// \brief Properties of a TV Service
struct tv_service {
    char ServiceId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    char* VariableStrVal[TV_MAXVARS];
    char EventURL[NAME_SIZE];
    char ControlURL[NAME_SIZE];
    char SID[NAME_SIZE];
};

/// \brief Properties of the TV Device
struct TvDevice {
    char UDN[250];
    char DescDocURL[250];
    char FriendlyName[250];
    char PresURL[250];
    int AdvrTimeOut;
    struct tv_service TvService[TV_SERVICE_SERVCOUNT];
};

/// \brief TV Device list node
struct TvDeviceNode {
    struct TvDevice device;
    struct TvDeviceNode* next;
};

/// The first node in the global device list, or NULL if empty
extern TvDeviceNode* GlobalDeviceList;

/// \cond
extern pthread_mutex_t DeviceListMutex;

extern UpnpClient_Handle ctrlpt_handle;
/// \endcond

// Seems this isn't used anywhere? --Ingo
// void TvCtrlPointPrintHelp();

/*!
 * \brief Delete a device node from the global device list
 *
 * \note This function is NOT thread safe, and should be called from
 * another function that has already locked the global device list.
 */
int TvCtrlPointDeleteNode(TvDeviceNode* node /*!< [in] The device node */);

/*!
 * \brief Remove a device from the global device list
 */
int TvCtrlPointRemoveDevice(
    /*! [in] The Unique Device Name for the device to remove */
    const char* UDN);

/*!
 * \brief Remove all devices from the global device list
 */
int TvCtrlPointRemoveAll();

/*!
 * \brief Clear the current global device list and issue new search requests to
 * build it up again from scratch
 */
int TvCtrlPointRefresh();

/*!
 * \brief Send an Action request to the specified service of a device
 */
int TvCtrlPointSendAction(
    int service, ///< [in] The service
    int devnum,  /*!< [in] The number of the device (order in the list, starting
                  * with 1) */
    const char* actionname,  ///< [in] The name of the action
    const char** param_name, ///< [in] An array of parameter names
    char** param_val,        ///< [in] The corresponding parameter values
    int param_count          ///< [in] The number of parameters
);

/*!
 * \brief Send an action with one argument to a device in the global device*
 * list
 */
int TvCtrlPointSendActionNumericArg(
    int devnum,  /*!< [in] The number of the device (order in the list, starting
                    with 1) */
    int service, /*!< [in] TV_SERVICE_CONTROL or TV_SERVICE_PICTURE */
    const char* actionName, /*!< [in] The device action, i.e., "SetChannel" */
    const char*
        paramName, /*!< [in] The name of the parameter that is being passed */
    int paramValue /*!< [in] Actual value of the parameter being passed */
);

/// \brief TvCtrlPointSendPowerOn
int TvCtrlPointSendPowerOn(int devnum);
/// \brief TvCtrlPointSendPowerOff
int TvCtrlPointSendPowerOff(int devnum);
/// \brief TvCtrlPointSendSetChannel
int TvCtrlPointSendSetChannel(int devnum, int channel);
/// \brief TvCtrlPointSendSetVolume
int TvCtrlPointSendSetVolume(int devnum, int volume);
/// \brief TvCtrlPointSendSetColor
int TvCtrlPointSendSetColor(int devnum, int color);
/// \brief TvCtrlPointSendSetTint
int TvCtrlPointSendSetTint(int devnum, int tint);
/// \brief TvCtrlPointSendSetContrast
int TvCtrlPointSendSetContrast(int devnum, int contrast);
/// \brief TvCtrlPointSendSetBrightness
int TvCtrlPointSendSetBrightness(int devnum, int brightness);

/*!
 * \brief Send a GetVar request to the specified service of a device.
 */
int TvCtrlPointGetVar(
    int service, ///< [in] The service
    int devnum,  /*!< [in] The number of the device (order in the list, starting
                    with 1) */
    const char* varname ///< [in] The name of the variable to request
);

/// \brief TvCtrlPointGetPower
int TvCtrlPointGetPower(int devnum);
/// \brief TvCtrlPointGetChannel
int TvCtrlPointGetChannel(int devnum);
/// \brief TvCtrlPointGetVolume
int TvCtrlPointGetVolume(int devnum);
/// \brief TvCtrlPointGetColor
int TvCtrlPointGetColor(int devnum);
/// \brief TvCtrlPointGetTint
int TvCtrlPointGetTint(int devnum);
/// \brief TvCtrlPointGetContrast
int TvCtrlPointGetContrast(int devnum);
/// \brief TvCtrlPointGetBrightness
int TvCtrlPointGetBrightness(int devnum);

/*!
 * \brief Get Control Point Device
 *
 * Given a list number, returns the pointer to the device node at that position
 * in the global device list.
 * \note that this function is not thread safe. It must be called from a
 * function that has locked the global device list.
 */
int TvCtrlPointGetDevice(
    int devnum, /*!< [in] The number of the device (order in the list, starting
                   with 1) */
    struct TvDeviceNode** devnode /*!< [in] The output device node pointer */
);

/*!
 * \brief Print the universal device names for each device in the global device
 * list
 */
int TvCtrlPointPrintList();

/*!
 * \brief Print the identifiers and state table for a device from the global
 * device list
 */
int TvCtrlPointPrintDevice(
    /*! [in] The number of the device (order in the list, starting with 1) */
    int devnum);

/*!
 * \brief Add the UPnP device to the global device list
 *
 * If the device is not already included in the global device list, add it.
 * Otherwise, update its advertisement expiration timeout.
 */
void TvCtrlPointAddDevice(
    IXML_Document* DescDoc, ///< [in] The description document for the device
    const char* location, ///< [in] The location of the description document URL
    int expires           ///< [in] The expiration time for this advertisement
);

/// \brief TvCtrlPointHandleGetVar
void TvCtrlPointHandleGetVar(const char* controlURL, const char* varName,
                             const DOMString varValue);

/*!
 * \brief Update a Tv state table. Called when an event is received.
 *
 * \note: this function is NOT thread save. It must be called from another
 * function that has locked the global device list.
 */
void TvStateUpdate(
    /*! [in] The UDN of the parent device. */
    char* UDN,
    /*! [in] The service state table to update. */
    int Service,
    /*! [out] DOM document representing the XML received with the event. */
    IXML_Document* ChangedVariables,
    /*! [out] pointer to the state table for the Tv  service to update. */
    char** State);

/*!
 * \brief Handle a UPnP event that was received
 *
 * Process the event and update the appropriate service state table.
 */
void TvCtrlPointHandleEvent(
    const char* sid,       ///< [in] The subscription id for the event
    int evntkey,           ///< [in] The eventkey number for the event
    IXML_Document* changes ///< [in] The DOM document representing the changes
);

/*!
 * \brief Handle a UPnP subscription update that was received
 *
 * Find the service the update belongs to, and update its subscription timeout.
 */
void TvCtrlPointHandleSubscribeUpdate(
    const char* eventURL, ///< [in] The event URL for the subscription
    const Upnp_SID sid,   ///< [in] The subscription id for the subscription
    int timeout           ///< [in] The new timeout for the subscription
);

/*!
 * \brief The callback handler registered with the SDK while registering the
 * control point
 *
 * Detects the type of callback, and passes the request on to the appropriate
 * function.
 */
int TvCtrlPointCallbackEventHandler(
    Upnp_EventType EventType, ///< [in] The type of callback event
    const void* Event,        ///< [in] Data structure containing event data
    [[maybe_unused]] void*
        Cookie ///< [in] Optional data specified during callback registration
);

/*!
 * \brief Checks the advertisement each device in the global device list.
 *
 * If an advertisement expires, the device is removed from the list. If an
 * advertisement is about to expire, a search request is sent for that device.
 */
void TvCtrlPointVerifyTimeouts(
    /*! [in] The increment to subtract from the timeouts each time the
     * function is called. */
    int incr);

/*!
 * \brief Call this function to initialize the UPnP library and start the TV
 * Control Point.
 *
 * This function creates a timer thread and provides a callback handler to
 * process any UPnP events that are received.
 *
 * \return TV_SUCCESS if everything went well, else TV_ERROR.
 */
int TvCtrlPointStart(char* iface, state_update updateFunctionPtr, int combo);

/// \brief Finish execution of the control point
int TvCtrlPointStop();

/*!
 * \brief Print help info for this application.
 */
void TvCtrlPointPrintShortHelp();

/*!
 * \brief Print long help info for this application.
 */
void TvCtrlPointPrintLongHelp();

/*!
 * \brief Print the list of valid command line commands to the user
 */
void TvCtrlPointPrintCommands();

/*!
 * \brief Function that receives commands from the user
 *
 * Accepting commands at the command prompt during the lifetime of the device,
 * and calls the appropriate functions for those commands.
 */
void* TvCtrlPointCommandLoop(void* args);

/*!
 * \brief TvCtrlPointProcessCommand
 */
int TvCtrlPointProcessCommand(char* cmdline);

/*! @} UpnpSamples */

#endif /* UPNPLIB_TV_CTRLPT_HPP */
