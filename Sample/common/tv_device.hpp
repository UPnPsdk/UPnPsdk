#ifndef UPNPLIB_TV_DEVICE_HPP
#define UPNPLIB_TV_DEVICE_HPP
/**************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-06-09
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
 * \ingroup UPnPsamples
 * \brief Declarations for a simple UPnP device console application.
 */

#include "sample_util.hpp"
#include <UpnpSubscriptionRequest.hpp>
#include <UpnpStateVarRequest.hpp>
#include <UPnPsdk/port_sock.hpp>

/// \cond
#include "pthread.h" // To find pthreads4w don't use <pthread.h>
#include <signal.h>

/*! Number of services. */
#define TV_SERVICE_SERVCOUNT 2

/*! Index of control service */
#define TV_SERVICE_CONTROL 0

/*! Index of picture service */
#define TV_SERVICE_PICTURE 1

/*! Number of control variables */
#define TV_CONTROL_VARCOUNT 3

/*! Index of power variable */
#define TV_CONTROL_POWER 0

/*! Index of channel variable */
#define TV_CONTROL_CHANNEL 1

/*! Index of volume variable */
#define TV_CONTROL_VOLUME 2

/*! Number of picture variables */
#define TV_PICTURE_VARCOUNT 4

/*! Index of color variable */
#define TV_PICTURE_COLOR 0

/*! Index of tint variable */
#define TV_PICTURE_TINT 1

/*! Index of contrast variable */
#define TV_PICTURE_CONTRAST 2

/*! Index of brightness variable */
#define TV_PICTURE_BRIGHTNESS 3

/*! Max value length */
#define TV_MAX_VAL_LEN 5

/*! Max actions */
#define TV_MAXACTIONS 12

/*! This should be the maximum VARCOUNT from above */
#define TV_MAXVARS TV_PICTURE_VARCOUNT

enum Ip_mode { UNSPEC, IP4, LLA, GUA, BEST };
/// \endcond

/*!
 * \brief Prototype for all actions. For each action that a service
 * implements, there is a corresponding function with this prototype.
 *
 * Pointers to these functions, along with action names, are stored
 * in the service table. When an action request comes in the action
 * name is matched, and the appropriate function is called.
 * Each function returns UPNP_E_SUCCESS, on success, and a nonzero
 * error code on failure.
 */
typedef int (*upnp_action)(
    /*! [in] Document of action request. */
    IXML_Document* request,
    /*! [out] Action result. */
    IXML_Document** out,
    /*! [out] Error string in case action was unsuccessful. */
    const char** errorString);

/*! Structure for storing Tv Service identifiers and state table. */
struct TvService {
    /*! Universally Unique Device Name. */
    char UDN[NAME_SIZE];
    char ServiceId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    const char* VariableName[TV_MAXVARS];
    char* VariableStrVal[TV_MAXVARS];
    const char* ActionNames[TV_MAXACTIONS];
    upnp_action actions[TV_MAXACTIONS];
    int VariableCount;
};

/*! Array of service structures */
extern struct TvService tv_service_table[];

/*! Device handle returned from SDK */
extern UpnpDevice_Handle device_handle;

/*!
 * \brief Initializes the action table for the specified service.
 *
 * Note that knowledge of the service description is assumed.
 * Action names are hardcoded.
 */
int SetActionTable(
    /*! [in] one of TV_SERVICE_CONTROL or, TV_SERVICE_PICTURE. */
    int serviceType,
    /*! [in,out] service containing action table to set. */
    struct TvService* out);

/*!
 * \brief Initialize the device state table for this TvDevice, pulling
 * identifier info from the description Document.
 *
 * Note that knowledge of the service description is assumed.
 * State table variables and default values are currently hardcoded in
 * this file rather than being read from service description documents.
 */
int TvDeviceStateTableInit(
    /*! [in] The description document URL. */
    char* DescDocURL);

/*!
 * \brief Called during a subscription request callback.
 *
 * If the subscription request is for this device and either its
 * control service or picture service, then accept it.
 */
int TvDeviceHandleSubscriptionRequest(
    /*! [in] The subscription request event structure. */
    const UpnpSubscriptionRequest* sr_event);

/*!
 * \brief Called during a get variable request callback.
 *
 * If the request is for this device and either its control service or
 * picture service, then respond with the variable value.
 */
int TvDeviceHandleGetVarRequest(
    /*! [in,out] The control get variable request event structure. */
    UpnpStateVarRequest* cgv_event);

/*!
 * \brief Called during an action request callback.
 *
 * If the request is for this device and either its control service
 * or picture service, then perform the action and respond.
 */
int TvDeviceHandleActionRequest(
    /*! [in,out] The control action request event structure. */
    UpnpActionRequest* ca_event);

/*!
 * \brief The callback handler registered with the SDK while registering
 * root device.
 *
 * Dispatches the request to the appropriate procedure
 * based on the value of EventType. The four requests handled by the
 * device are:
 *  \li 1) Event Subscription requests.
 *  \li 2) Get Variable requests.
 *  \li 3) Action requests.
 */
int TvDeviceCallbackEventHandler(
    /*! [in] The type of callback event. */
    Upnp_EventType,
    /*! [in] Data structure containing event data. */
    const void* Event,
    /*! [in] Optional data specified during callback registration. */
    void* Cookie);

/*!
 * \brief Update the TvDevice service state table, and notify all subscribed
 * control points of the updated state.
 *
 * Note that since this function blocks on the mutex tvDev_mutex,
 * to avoid a hang this function should not be called within any other
 * function that currently has this mutex locked.
 */
int TvDeviceSetServiceTableVar(
    /*! [in] The service number (TV_SERVICE_CONTROL or TV_SERVICE_PICTURE). */
    unsigned int service,
    /*! [in] The variable number (TV_CONTROL_POWER, TV_CONTROL_CHANNEL,
     * TV_CONTROL_VOLUME, TV_PICTURE_COLOR, TV_PICTURE_TINT,
     * TV_PICTURE_CONTRAST, or TV_PICTURE_BRIGHTNESS). */
    int variable,
    /*! [in] The string representation of the new value. */
    char* value);

/* Control Service Actions */

/*!
 * \brief Turn the power on.
 */
int TvDevicePowerOn(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Turn the power off.
 */
int TvDevicePowerOff(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Change the channel, update the TvDevice control service
 * state table, and notify all subscribed control points of the
 * updated state.
 */
int TvDeviceSetChannel(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Increase the channel.
 */
int TvDeviceIncreaseChannel(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Decrease the channel.
 */
int TvDeviceDecreaseChannel(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Change the volume, update the TvDevice control service
 *       state table, and notify all subscribed control points of the
 *       updated state.
 */
int TvDeviceSetVolume(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Increase the volume.
 */
int TvDeviceIncreaseVolume(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Decrease the volume.
 */
int TvDeviceDecreaseVolume(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*Picture Service Actions */

/*!
 * \brief Change the color, update the TvDevice picture service
 * state table, and notify all subscribed control points of the
 * updated state.
 */
int TvDeviceSetColor(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Increase the color.
 */
int TvDeviceIncreaseColor(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Decrease the color.
 */
int TvDeviceDecreaseColor(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Change the tint, update the TvDevice picture service
 * state table, and notify all subscribed control points of the
 * updated state.
 */
int TvDeviceSetTint(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Increase tint.
 */
int TvDeviceIncreaseTint(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Decrease tint.
 */
int TvDeviceDecreaseTint(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Change the contrast, update the TvDevice picture service
 * state table, and notify all subscribed control points of the
 * updated state.
 */
int TvDeviceSetContrast(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Increase the contrast.
 */
int TvDeviceIncreaseContrast(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Decrease the contrast.
 */
int TvDeviceDecreaseContrast(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Change the brightness, update the TvDevice picture service
 * state table, and notify all subscribed control points of the
 * updated state.
 */
int TvDeviceSetBrightness(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Increase brightnesss.
 */
int TvDeviceIncreaseBrightness(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Decrease brightnesss.
 */
int TvDeviceDecreaseBrightness(
    /*! [in] Document of action request. */
    IXML_Document* in,
    /*! [in] Action result. */
    IXML_Document** out,
    /*! [out] ErrorString in case action was unsuccessful. */
    const char** errorString);

/*!
 * \brief Initializes the UPnP SDK, registers the root-device, and sends out
 * advertisements.
 *
 * \returns
 *  On success: UPNPLIB_E_SUCCESS\n
 *  On error: Other UPnPlib error code.
 */
int TvDeviceStart(
    /*! [in] Network interface to initialize the SDK. If nullptr, then the first
       interface (specified by its index) except localhost is used. */
    const char* iface,
    /*! [in] Local port number to initialize the SDK. If 0, then a random number
       is used. */
    in_port_t port,
    /*! [in] Name of description document. May be a nullptr. Default is
       tvdevicedesc.xml. */
    const char* desc_doc_name,
    /*! [in] path of web directory.
     * may be a nullptr. Default is ./Sample/web (for Linux). */
    const char* web_dir_path,
    /*! [in] IP mode: IP_MODE_IPV4, IP_MODE_IPV6_LLA or
     * IP_MODE_IPV6_ULA_GUA. Default is IP_MODE_IPV6_LLA. */
    const int a_ip_mode,
    /*! [in] Non-zero if called from the combo application. */
    const int combo);

/*!
 * \brief Stops the device. Uninitializes the SDK.
 */
int TvDeviceStop(void);

/*!
 * \brief Function that receives commands from the user at the command prompt
 * during the lifetime of the device, and calls the appropriate
 * functions for those commands. Only one command, exit, is currently
 * defined.
 */
void* TvDeviceCommandLoop(void* args);

/*!
 * \brief Main entry point for tv device application.
 *
 * Initializes and registers with the SDK.
 * Initializes the state stables of the service.
 * Starts the command loop.
 *
 * Accepts the following optional arguments:
 *  \li \c -ip ipaddress
 *  \li \c -port port
 *  \li \c -desc desc_doc_name
 *  \li \c -webdir web_dir_path
 *  \li \c -help
 */
int device_main(int argc, char* argv[]);

#endif /* UPNPLIB_TV_DEVICE_HPP */
