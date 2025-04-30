/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-04-30
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
 ******************************************************************************/
/*!
 * \file
 * \brief TV Control Point sample program
 */

#include "tv_ctrlpt.hpp"
#include "upnp.hpp"
#include <UPnPsdk/port.hpp>
#include <UPnPsdk/global.hpp>
#include <thread>

/*!
 * Mutex for protecting the global device list in a multi-threaded,
 * asynchronous environment. All functions should lock this mutex before
 * reading or writing the device list.
 */
pthread_mutex_t DeviceListMutex;

/// \cond
UpnpClient_Handle ctrlpt_handle = -1;
/// \endcond

/*! Device type for tv device. */
const char TvDeviceType[] = "urn:schemas-upnp-org:device:tvdevice:1";

/*! Service names.*/
const char* TvServiceName[] = {"Control", "Picture"};

/*!
   Global arrays for storing variable names and counts for
   TvControl and TvPicture services
 */
const char* TvVarName[TV_SERVICE_SERVCOUNT][TV_MAXVARS] = {
    {"Power", "Channel", "Volume", ""},
    {"Color", "Tint", "Contrast", "Brightness"}};

/// \brief TvVarCount
char TvVarCount[TV_SERVICE_SERVCOUNT] = {TV_CONTROL_VARCOUNT,
                                         TV_PICTURE_VARCOUNT};

/*!
   Timeout to request during subscriptions
 */
int default_timeout = 1801;

TvDeviceNode* GlobalDeviceList = NULL;

int TvCtrlPointDeleteNode(TvDeviceNode* node) {
    int rc, service, var;

    if (NULL == node) {
        SampleUtil_Print("ERROR: TvCtrlPointDeleteNode: Node is empty\n");
        return TV_ERROR;
    }

    for (service = 0; service < TV_SERVICE_SERVCOUNT; service++) {
        /*
           If we have a valid control SID, then unsubscribe
         */
        if (strcmp(node->device.TvService[service].SID, "") != 0) {
            rc = UpnpUnSubscribe(ctrlpt_handle,
                                 node->device.TvService[service].SID);
            if (UPNP_E_SUCCESS == rc) {
                SampleUtil_Print("Unsubscribed from Tv %s "
                                 "EventURL with SID=%s\n",
                                 TvServiceName[service],
                                 node->device.TvService[service].SID);
            } else {
                SampleUtil_Print("Error unsubscribing to Tv %s "
                                 "EventURL -- %d\n",
                                 TvServiceName[service], rc);
            }
        }

        for (var = 0; var < TvVarCount[service]; var++) {
            if (node->device.TvService[service].VariableStrVal[var]) {
                free(node->device.TvService[service].VariableStrVal[var]);
            }
        }
    }

    /*Notify New Device Added */
    SampleUtil_StateUpdate(NULL, NULL, node->device.UDN, DEVICE_REMOVED);
    free(node);
    node = NULL;

    return TV_SUCCESS;
}

int TvCtrlPointRemoveDevice(const char* UDN) {
    struct TvDeviceNode* curdevnode;
    struct TvDeviceNode* prevdevnode;

    pthread_mutex_lock(&DeviceListMutex);

    curdevnode = GlobalDeviceList;
    if (!curdevnode) {
        SampleUtil_Print("WARNING: TvCtrlPointRemoveDevice: Device "
                         "list empty\n");
    } else {
        if (0 == strcmp(curdevnode->device.UDN, UDN)) {
            GlobalDeviceList = curdevnode->next;
            TvCtrlPointDeleteNode(curdevnode);
        } else {
            prevdevnode = curdevnode;
            curdevnode = curdevnode->next;
            while (curdevnode) {
                if (strcmp(curdevnode->device.UDN, UDN) == 0) {
                    prevdevnode->next = curdevnode->next;
                    TvCtrlPointDeleteNode(curdevnode);
                    break;
                }
                prevdevnode = curdevnode;
                curdevnode = curdevnode->next;
            }
        }
    }

    pthread_mutex_unlock(&DeviceListMutex);

    return TV_SUCCESS;
}

int TvCtrlPointRemoveAll() {
    struct TvDeviceNode *curdevnode, *next;

    pthread_mutex_lock(&DeviceListMutex);

    curdevnode = GlobalDeviceList;
    GlobalDeviceList = NULL;

    while (curdevnode) {
        next = curdevnode->next;
        TvCtrlPointDeleteNode(curdevnode);
        curdevnode = next;
    }

    pthread_mutex_unlock(&DeviceListMutex);

    return TV_SUCCESS;
}

int TvCtrlPointRefresh() {
    int rc;

    TvCtrlPointRemoveAll();
    /* Search for all devices of type tvdevice version 1,
     * waiting for up to 5 seconds for the response */
    rc = UpnpSearchAsync(ctrlpt_handle, 5, TvDeviceType, NULL);
    if (UPNP_E_SUCCESS != rc) {
        SampleUtil_Print("Error sending search request%d\n", rc);

        return TV_ERROR;
    }

    return TV_SUCCESS;
}

int TvCtrlPointGetVar(int service, int devnum, const char* varname) {
    struct TvDeviceNode* devnode;
    int rc;

    pthread_mutex_lock(&DeviceListMutex);

    rc = TvCtrlPointGetDevice(devnum, &devnode);

    if (TV_SUCCESS == rc) {
        rc = UpnpGetServiceVarStatusAsync(
            ctrlpt_handle, devnode->device.TvService[service].ControlURL,
            varname, TvCtrlPointCallbackEventHandler, NULL);
        if (rc != UPNP_E_SUCCESS) {
            SampleUtil_Print("Error in UpnpGetServiceVarStatusAsync -- %d\n",
                             rc);
            rc = TV_ERROR;
        }
    }

    pthread_mutex_unlock(&DeviceListMutex);

    return rc;
}

int TvCtrlPointGetPower(int devnum) {
    return TvCtrlPointGetVar(TV_SERVICE_CONTROL, devnum, "Power");
}

int TvCtrlPointGetChannel(int devnum) {
    return TvCtrlPointGetVar(TV_SERVICE_CONTROL, devnum, "Channel");
}

int TvCtrlPointGetVolume(int devnum) {
    return TvCtrlPointGetVar(TV_SERVICE_CONTROL, devnum, "Volume");
}

int TvCtrlPointGetColor(int devnum) {
    return TvCtrlPointGetVar(TV_SERVICE_PICTURE, devnum, "Color");
}

int TvCtrlPointGetTint(int devnum) {
    return TvCtrlPointGetVar(TV_SERVICE_PICTURE, devnum, "Tint");
}

int TvCtrlPointGetContrast(int devnum) {
    return TvCtrlPointGetVar(TV_SERVICE_PICTURE, devnum, "Contrast");
}

int TvCtrlPointGetBrightness(int devnum) {
    return TvCtrlPointGetVar(TV_SERVICE_PICTURE, devnum, "Brightness");
}

int TvCtrlPointSendAction(int service, int devnum, const char* actionname,
                          const char** param_name, char** param_val,
                          int param_count) {
    struct TvDeviceNode* devnode;
    IXML_Document* actionNode = NULL;
    int rc = TV_SUCCESS;
    int param;

    pthread_mutex_lock(&DeviceListMutex);

    rc = TvCtrlPointGetDevice(devnum, &devnode);
    if (TV_SUCCESS == rc) {
        if (0 == param_count) {
            actionNode =
                UpnpMakeAction(actionname, TvServiceType[service], 0, NULL);
        } else {
            for (param = 0; param < param_count; param++) {
                if (UpnpAddToAction(&actionNode, actionname,
                                    TvServiceType[service], param_name[param],
                                    param_val[param]) != UPNP_E_SUCCESS) {
                    SampleUtil_Print("ERROR: TvCtrlPointSendAction: "
                                     "Trying to add action param\n");
                    /*return -1; // TBD - BAD! leaves mutex
                     * locked */
                }
            }
        }

        rc = UpnpSendActionAsync(ctrlpt_handle,
                                 devnode->device.TvService[service].ControlURL,
                                 TvServiceType[service], NULL, actionNode,
                                 TvCtrlPointCallbackEventHandler, NULL);

        if (rc != UPNP_E_SUCCESS) {
            SampleUtil_Print("Error in UpnpSendActionAsync -- %d\n", rc);
            rc = TV_ERROR;
        }
    }

    pthread_mutex_unlock(&DeviceListMutex);

    if (actionNode)
        ixmlDocument_free(actionNode);

    return rc;
}

int TvCtrlPointSendActionNumericArg(int devnum, int service,
                                    const char* actionName,
                                    const char* paramName, int paramValue) {
    char param_val_a[50];
    char* param_val = param_val_a;

    snprintf(param_val_a, sizeof(param_val_a), "%d", paramValue);
    return TvCtrlPointSendAction(service, devnum, actionName, &paramName,
                                 &param_val, 1);
}

int TvCtrlPointSendPowerOn(int devnum) {
    return TvCtrlPointSendAction(TV_SERVICE_CONTROL, devnum, "PowerOn", NULL,
                                 NULL, 0);
}

int TvCtrlPointSendPowerOff(int devnum) {
    return TvCtrlPointSendAction(TV_SERVICE_CONTROL, devnum, "PowerOff", NULL,
                                 NULL, 0);
}

int TvCtrlPointSendSetChannel(int devnum, int channel) {
    return TvCtrlPointSendActionNumericArg(devnum, TV_SERVICE_CONTROL,
                                           "SetChannel", "Channel", channel);
}

int TvCtrlPointSendSetVolume(int devnum, int volume) {
    return TvCtrlPointSendActionNumericArg(devnum, TV_SERVICE_CONTROL,
                                           "SetVolume", "Volume", volume);
}

int TvCtrlPointSendSetColor(int devnum, int color) {
    return TvCtrlPointSendActionNumericArg(devnum, TV_SERVICE_PICTURE,
                                           "SetColor", "Color", color);
}

int TvCtrlPointSendSetTint(int devnum, int tint) {
    return TvCtrlPointSendActionNumericArg(devnum, TV_SERVICE_PICTURE,
                                           "SetTint", "Tint", tint);
}

int TvCtrlPointSendSetContrast(int devnum, int contrast) {
    return TvCtrlPointSendActionNumericArg(devnum, TV_SERVICE_PICTURE,
                                           "SetContrast", "Contrast", contrast);
}

int TvCtrlPointSendSetBrightness(int devnum, int brightness) {
    return TvCtrlPointSendActionNumericArg(
        devnum, TV_SERVICE_PICTURE, "SetBrightness", "Brightness", brightness);
}

int TvCtrlPointGetDevice(int devnum, struct TvDeviceNode** devnode) {
    int count = devnum;
    struct TvDeviceNode* tmpdevnode = NULL;

    if (count)
        tmpdevnode = GlobalDeviceList;
    while (--count && tmpdevnode) {
        tmpdevnode = tmpdevnode->next;
    }
    if (!tmpdevnode) {
        SampleUtil_Print("Error finding TvDevice number -- %d\n", devnum);
        return TV_ERROR;
    }
    *devnode = tmpdevnode;

    return TV_SUCCESS;
}

int TvCtrlPointPrintList() {
    struct TvDeviceNode* tmpdevnode;
    int i = 0;

    pthread_mutex_lock(&DeviceListMutex);

    SampleUtil_Print("TvCtrlPointPrintList:\n");
    tmpdevnode = GlobalDeviceList;
    while (tmpdevnode) {
        SampleUtil_Print(" %3d -- %s\n", ++i, tmpdevnode->device.UDN);
        tmpdevnode = tmpdevnode->next;
    }
    SampleUtil_Print("\n");
    pthread_mutex_unlock(&DeviceListMutex);

    return TV_SUCCESS;
}

int TvCtrlPointPrintDevice(int devnum) {
    struct TvDeviceNode* tmpdevnode;
    int i = 0, service, var;
    char spacer[15];

    if (devnum <= 0) {
        SampleUtil_Print("Error in TvCtrlPointPrintDevice: "
                         "invalid devnum = %d\n",
                         devnum);
        return TV_ERROR;
    }

    pthread_mutex_lock(&DeviceListMutex);

    SampleUtil_Print("TvCtrlPointPrintDevice:\n");
    tmpdevnode = GlobalDeviceList;
    while (tmpdevnode) {
        i++;
        if (i == devnum)
            break;
        tmpdevnode = tmpdevnode->next;
    }
    if (!tmpdevnode) {
        SampleUtil_Print("Error in TvCtrlPointPrintDevice: "
                         "invalid devnum = %d  --  actual device count = %d\n",
                         devnum, i);
    } else {
        SampleUtil_Print(
            "  TvDevice -- %d\n"
            "    |                  \n"
            "    +- UDN        = %s\n"
            "    +- DescDocURL     = %s\n"
            "    +- FriendlyName   = %s\n"
            "    +- PresURL        = %s\n"
            "    +- Adver. TimeOut = %d\n",
            devnum, tmpdevnode->device.UDN, tmpdevnode->device.DescDocURL,
            tmpdevnode->device.FriendlyName, tmpdevnode->device.PresURL,
            tmpdevnode->device.AdvrTimeOut);
        for (service = 0; service < TV_SERVICE_SERVCOUNT; service++) {
            if (service < TV_SERVICE_SERVCOUNT - 1)
                snprintf(spacer, sizeof(spacer), "    |    ");
            else
                snprintf(spacer, sizeof(spacer), "         ");
            SampleUtil_Print(
                "    |                  \n"
                "    +- Tv %s Service\n"
                "%s+- ServiceId       = %s\n"
                "%s+- ServiceType     = %s\n"
                "%s+- EventURL        = %s\n"
                "%s+- ControlURL      = %s\n"
                "%s+- SID             = %s\n"
                "%s+- ServiceStateTable\n",
                TvServiceName[service], spacer,
                tmpdevnode->device.TvService[service].ServiceId, spacer,
                tmpdevnode->device.TvService[service].ServiceType, spacer,
                tmpdevnode->device.TvService[service].EventURL, spacer,
                tmpdevnode->device.TvService[service].ControlURL, spacer,
                tmpdevnode->device.TvService[service].SID, spacer);
            for (var = 0; var < TvVarCount[service]; var++) {
                SampleUtil_Print(
                    "%s     +- %-10s = %s\n", spacer, TvVarName[service][var],
                    tmpdevnode->device.TvService[service].VariableStrVal[var]);
            }
        }
    }
    SampleUtil_Print("\n");
    pthread_mutex_unlock(&DeviceListMutex);

    return TV_SUCCESS;
}

void TvCtrlPointAddDevice(IXML_Document* DescDoc, const char* location,
                          int expires) {
    char* deviceType{};
    char* friendlyName{};
    char* presURL{};
    char* baseURL{};
    char* relURL{};
    char* UDN{};
    char* serviceId[TV_SERVICE_SERVCOUNT]{};
    char* eventURL[TV_SERVICE_SERVCOUNT]{};
    char* controlURL[TV_SERVICE_SERVCOUNT]{};
    Upnp_SID eventSID[TV_SERVICE_SERVCOUNT];
    int TimeOut[TV_SERVICE_SERVCOUNT];
    struct TvDeviceNode* deviceNode{};
    struct TvDeviceNode* tmpdevnode{};
    int ret = 1;
    int found = 0;
    int service;
    int var;

    TimeOut[0] = default_timeout;
    TimeOut[1] = default_timeout;
    pthread_mutex_lock(&DeviceListMutex);

    /* Read key elements from description document */
    UDN = SampleUtil_GetFirstDocumentItem(DescDoc, "UDN");
    deviceType = SampleUtil_GetFirstDocumentItem(DescDoc, "deviceType");
    friendlyName = SampleUtil_GetFirstDocumentItem(DescDoc, "friendlyName");
    baseURL = SampleUtil_GetFirstDocumentItem(DescDoc, "URLBase");
    relURL = SampleUtil_GetFirstDocumentItem(DescDoc, "presentationURL");

    ret = UpnpResolveURL2((baseURL ? baseURL : location), relURL, &presURL);

    if (UPNP_E_SUCCESS != ret)
        SampleUtil_Print(
            "%s(%d): Error generating presURL from %s + %s, %s(%d)\n", __FILE__,
            __LINE__, baseURL, relURL, UpnpGetErrorMessage(ret), ret);

    if (strcmp(deviceType, TvDeviceType) == 0) {
        SampleUtil_Print("Found Tv device\n");

        /* Check if this device is already in the list */
        tmpdevnode = GlobalDeviceList;
        while (tmpdevnode) {
            if (strcmp(tmpdevnode->device.UDN, UDN) == 0) {
                found = 1;
                break;
            }
            tmpdevnode = tmpdevnode->next;
        }

        if (found) {
            /* The device is already there, so just update  */
            /* the advertisement timeout field */
            tmpdevnode->device.AdvrTimeOut = expires;
        } else {
            for (service = 0; service < TV_SERVICE_SERVCOUNT; service++) {
                if (SampleUtil_FindAndParseService(
                        DescDoc, location, TvServiceType[service],
                        &serviceId[service], &eventURL[service],
                        &controlURL[service])) {
                    SampleUtil_Print("Subscribing to "
                                     "EventURL %s...\n",
                                     eventURL[service]);
                    ret = UpnpSubscribe(ctrlpt_handle, eventURL[service],
                                        &TimeOut[service], eventSID[service]);
                    if (ret == UPNP_E_SUCCESS) {
                        SampleUtil_Print("Subscribed to "
                                         "EventURL with "
                                         "SID=%s\n",
                                         eventSID[service]);
                    } else {
                        SampleUtil_Print("Error Subscribing to "
                                         "EventURL -- %d\n",
                                         ret);
                        strcpy(eventSID[service], "");
                    }
                } else {
                    SampleUtil_Print("Error: Could not "
                                     "find Service: %s\n",
                                     TvServiceType[service]);
                }
            }
            /* Create a new device node */
            deviceNode =
                (struct TvDeviceNode*)malloc(sizeof(struct TvDeviceNode));
            strcpy(deviceNode->device.UDN, UDN);
            strcpy(deviceNode->device.DescDocURL, location);
            strcpy(deviceNode->device.FriendlyName, friendlyName);
            // BUGFIX: Next could segfault
            if (presURL)
                strcpy(deviceNode->device.PresURL, presURL);
            deviceNode->device.AdvrTimeOut = expires;
            for (service = 0; service < TV_SERVICE_SERVCOUNT; service++) {
                if (serviceId[service] == NULL) {
                    /* not found */
                    continue;
                }
                strcpy(deviceNode->device.TvService[service].ServiceId,
                       serviceId[service]);
                strcpy(deviceNode->device.TvService[service].ServiceType,
                       TvServiceType[service]);
                strcpy(deviceNode->device.TvService[service].ControlURL,
                       controlURL[service]);
                strcpy(deviceNode->device.TvService[service].EventURL,
                       eventURL[service]);
                strcpy(deviceNode->device.TvService[service].SID,
                       eventSID[service]);
                for (var = 0; var < TvVarCount[service]; var++) {
                    deviceNode->device.TvService[service].VariableStrVal[var] =
                        (char*)malloc(TV_MAX_VAL_LEN);
                    strcpy(deviceNode->device.TvService[service]
                               .VariableStrVal[var],
                           "");
                }
            }
            deviceNode->next = NULL;
            /* Insert the new device node in the list */
            if ((tmpdevnode = GlobalDeviceList) != nullptr) {
                while (tmpdevnode) {
                    if (tmpdevnode->next) {
                        tmpdevnode = tmpdevnode->next;
                    } else {
                        tmpdevnode->next = deviceNode;
                        break;
                    }
                }
            } else {
                GlobalDeviceList = deviceNode;
            }
            /*Notify New Device Added */
            SampleUtil_StateUpdate(NULL, NULL, deviceNode->device.UDN,
                                   DEVICE_ADDED);
        }
    }

    pthread_mutex_unlock(&DeviceListMutex);

    if (deviceType)
        free(deviceType);
    if (friendlyName)
        free(friendlyName);
    if (UDN)
        free(UDN);
    if (baseURL)
        free(baseURL);
    if (relURL)
        free(relURL);
    if (presURL)
        free(presURL);
    for (service = 0; service < TV_SERVICE_SERVCOUNT; service++) {
        if (serviceId[service])
            free(serviceId[service]);
        if (controlURL[service])
            free(controlURL[service]);
        if (eventURL[service])
            free(eventURL[service]);
    }
}

void TvStateUpdate(char* UDN, int Service, IXML_Document* ChangedVariables,
                   char** State) {
    IXML_NodeList* properties;
    IXML_NodeList* variables;
    IXML_Element* property;
    IXML_Element* variable;
    long unsigned int length;
    long unsigned int length1;
    long unsigned int i;
    int j;
    char* tmpstate = NULL;
    (void)UDN;

    SampleUtil_Print("Tv State Update (service %d):\n", Service);
    /* Find all of the e:property tags in the document */
    properties =
        ixmlDocument_getElementsByTagName(ChangedVariables, "e:property");
    if (properties) {
        length = ixmlNodeList_length(properties);
        for (i = 0; i < length; i++) {
            /* Loop through each property change found */
            property = (IXML_Element*)ixmlNodeList_item(properties, i);
            /* For each variable name in the state table,
             * check if this is a corresponding property change */
            for (j = 0; j < TvVarCount[Service]; j++) {
                variables = ixmlElement_getElementsByTagName(
                    property, TvVarName[Service][j]);
                /* If a match is found, extract
                 * the value, and update the state table */
                if (variables) {
                    length1 = ixmlNodeList_length(variables);
                    if (length1) {
                        variable =
                            (IXML_Element*)ixmlNodeList_item(variables, 0);
                        tmpstate = SampleUtil_GetElementValue(variable);
                        if (tmpstate) {
                            strcpy(State[j], tmpstate);
                            SampleUtil_Print(" Variable "
                                             "Name: %s New "
                                             "Value:'%s'\n",
                                             TvVarName[Service][j], State[j]);
                        }
                        if (tmpstate)
                            free(tmpstate);
                        tmpstate = NULL;
                    }
                    ixmlNodeList_free(variables);
                    variables = NULL;
                }
            }
        }
        ixmlNodeList_free(properties);
    }
    return;
}

void TvCtrlPointHandleEvent(const char* sid, int evntkey,
                            IXML_Document* changes) {
    struct TvDeviceNode* tmpdevnode;
    int service;

    pthread_mutex_lock(&DeviceListMutex);

    tmpdevnode = GlobalDeviceList;
    while (tmpdevnode) {
        for (service = 0; service < TV_SERVICE_SERVCOUNT; ++service) {
            if (strcmp(tmpdevnode->device.TvService[service].SID, sid) == 0) {
                SampleUtil_Print("Received Tv %s Event: %d for SID %s\n",
                                 TvServiceName[service], evntkey, sid);
                TvStateUpdate(tmpdevnode->device.UDN, service, changes,
                              (char**)&tmpdevnode->device.TvService[service]
                                  .VariableStrVal);
                break;
            }
        }
        tmpdevnode = tmpdevnode->next;
    }

    pthread_mutex_unlock(&DeviceListMutex);
}

void TvCtrlPointHandleSubscribeUpdate(const char* eventURL, const Upnp_SID sid,
                                      int timeout) {
    struct TvDeviceNode* tmpdevnode;
    int service;
    (void)timeout;

    pthread_mutex_lock(&DeviceListMutex);

    tmpdevnode = GlobalDeviceList;
    while (tmpdevnode) {
        for (service = 0; service < TV_SERVICE_SERVCOUNT; service++) {
            if (strcmp(tmpdevnode->device.TvService[service].EventURL,
                       eventURL) == 0) {
                SampleUtil_Print("Received Tv %s Event Renewal "
                                 "for eventURL %s\n",
                                 TvServiceName[service], eventURL);
                strcpy(tmpdevnode->device.TvService[service].SID, sid);
                break;
            }
        }

        tmpdevnode = tmpdevnode->next;
    }

    pthread_mutex_unlock(&DeviceListMutex);

    return;
}

void TvCtrlPointHandleGetVar(const char* controlURL, const char* varName,
                             const DOMString varValue) {

    struct TvDeviceNode* tmpdevnode;
    int service;

    pthread_mutex_lock(&DeviceListMutex);

    tmpdevnode = GlobalDeviceList;
    while (tmpdevnode) {
        for (service = 0; service < TV_SERVICE_SERVCOUNT; service++) {
            if (strcmp(tmpdevnode->device.TvService[service].ControlURL,
                       controlURL) == 0) {
                SampleUtil_StateUpdate(varName, varValue,
                                       tmpdevnode->device.UDN,
                                       GET_VAR_COMPLETE);
                break;
            }
        }
        tmpdevnode = tmpdevnode->next;
    }

    pthread_mutex_unlock(&DeviceListMutex);
}

int TvCtrlPointCallbackEventHandler(Upnp_EventType EventType, const void* Event,
                                    [[maybe_unused]] void* Cookie) {
    SampleUtil_PrintEvent(EventType, Event);
    switch (EventType) {
    /* SSDP Stuff */
    case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
    case UPNP_DISCOVERY_SEARCH_RESULT: {
        const UpnpDiscovery* d_event = (UpnpDiscovery*)Event;
        IXML_Document* DescDoc = NULL;
        const char* location = NULL;
        int errCode = UpnpDiscovery_get_ErrCode(d_event);

        if (errCode != UPNP_E_SUCCESS) {
            SampleUtil_Print("Error in Discovery Callback -- %d\n", errCode);
        }

        location = UpnpString_get_String(UpnpDiscovery_get_Location(d_event));
        errCode = UpnpDownloadXmlDoc(location, &DescDoc);
        if (errCode != UPNP_E_SUCCESS) {
            SampleUtil_Print("Error obtaining device description "
                             "from %s -- error = %d\n",
                             location, errCode);
        } else {
            TvCtrlPointAddDevice(DescDoc, location,
                                 UpnpDiscovery_get_Expires(d_event));
        }
        if (DescDoc) {
            ixmlDocument_free(DescDoc);
        }
        TvCtrlPointPrintList();
        break;
    }
    case UPNP_DISCOVERY_SEARCH_TIMEOUT:
        /* Nothing to do here... */
        break;
    case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE: {
        UpnpDiscovery* d_event = (UpnpDiscovery*)Event;
        int errCode = UpnpDiscovery_get_ErrCode(d_event);
        const char* deviceId =
            UpnpString_get_String(UpnpDiscovery_get_DeviceID(d_event));

        if (errCode != UPNP_E_SUCCESS) {
            SampleUtil_Print("Error in Discovery ByeBye Callback -- %d\n",
                             errCode);
        }
        SampleUtil_Print("Received ByeBye for Device: %s\n", deviceId);
        TvCtrlPointRemoveDevice(deviceId);
        SampleUtil_Print("After byebye:\n");
        TvCtrlPointPrintList();
        break;
    }
    /* SOAP Stuff */
    case UPNP_CONTROL_ACTION_COMPLETE: {
        UpnpActionComplete* a_event = (UpnpActionComplete*)Event;
        int errCode = UpnpActionComplete_get_ErrCode(a_event);
        if (errCode != UPNP_E_SUCCESS) {
            SampleUtil_Print("Error in  Action Complete Callback -- %d\n",
                             errCode);
        }
        /* No need for any processing here, just print out results.
         * Service state table updates are handled by events. */
        break;
    }
    case UPNP_CONTROL_GET_VAR_COMPLETE: {
        UpnpStateVarComplete* sv_event = (UpnpStateVarComplete*)Event;
        int errCode = UpnpStateVarComplete_get_ErrCode(sv_event);
        if (errCode != UPNP_E_SUCCESS) {
            SampleUtil_Print("Error in Get Var Complete Callback -- %d\n",
                             errCode);
        } else {
            TvCtrlPointHandleGetVar(
                UpnpString_get_String(
                    UpnpStateVarComplete_get_CtrlUrl(sv_event)),
                UpnpString_get_String(
                    UpnpStateVarComplete_get_StateVarName(sv_event)),
                UpnpStateVarComplete_get_CurrentVal(sv_event));
        }
        break;
    }
    /* GENA Stuff */
    case UPNP_EVENT_RECEIVED: {
        UpnpEvent* e_event = (UpnpEvent*)Event;
        TvCtrlPointHandleEvent(UpnpEvent_get_SID_cstr(e_event),
                               UpnpEvent_get_EventKey(e_event),
                               UpnpEvent_get_ChangedVariables(e_event));
        break;
    }
    case UPNP_EVENT_SUBSCRIBE_COMPLETE:
    case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
    case UPNP_EVENT_RENEWAL_COMPLETE: {
        UpnpEventSubscribe* es_event = (UpnpEventSubscribe*)Event;

        int errCode = UpnpEventSubscribe_get_ErrCode(es_event);
        if (errCode != UPNP_E_SUCCESS) {
            SampleUtil_Print("Error in Event Subscribe Callback -- %d\n",
                             errCode);
        } else {
            TvCtrlPointHandleSubscribeUpdate(
                UpnpString_get_String(
                    UpnpEventSubscribe_get_PublisherUrl(es_event)),
                UpnpString_get_String(UpnpEventSubscribe_get_SID(es_event)),
                UpnpEventSubscribe_get_TimeOut(es_event));
        }
        break;
    }
    case UPNP_EVENT_AUTORENEWAL_FAILED:
    case UPNP_EVENT_SUBSCRIPTION_EXPIRED: {
        UpnpEventSubscribe* es_event = (UpnpEventSubscribe*)Event;
        int TimeOut = default_timeout;
        Upnp_SID newSID;

        int errCode =
            UpnpSubscribe(ctrlpt_handle,
                          UpnpString_get_String(
                              UpnpEventSubscribe_get_PublisherUrl(es_event)),
                          &TimeOut, newSID);
        if (errCode == UPNP_E_SUCCESS) {
            SampleUtil_Print("Subscribed to EventURL with SID=%s\n", newSID);
            TvCtrlPointHandleSubscribeUpdate(
                UpnpString_get_String(
                    UpnpEventSubscribe_get_PublisherUrl(es_event)),
                newSID, TimeOut);
        } else {
            SampleUtil_Print("Error Subscribing to EventURL -- %d\n", errCode);
        }
        break;
    }
    /* ignore these cases, since this is not a device */
    case UPNP_EVENT_SUBSCRIPTION_REQUEST:
    case UPNP_CONTROL_GET_VAR_REQUEST:
    case UPNP_CONTROL_ACTION_REQUEST:
        break;
    }

    return 0;
}

void TvCtrlPointVerifyTimeouts(int incr) {
    struct TvDeviceNode* prevdevnode;
    struct TvDeviceNode* curdevnode;
    int ret;

    pthread_mutex_lock(&DeviceListMutex);

    prevdevnode = NULL;
    curdevnode = GlobalDeviceList;
    while (curdevnode) {
        curdevnode->device.AdvrTimeOut -= incr;
        /*SampleUtil_Print("Advertisement Timeout: %d\n",
         * curdevnode->device.AdvrTimeOut); */
        if (curdevnode->device.AdvrTimeOut <= 0) {
            /* This advertisement has expired, so we should remove
             * the device from the list */
            if (GlobalDeviceList == curdevnode)
                GlobalDeviceList = curdevnode->next;
            else
                prevdevnode->next = curdevnode->next;
            TvCtrlPointDeleteNode(curdevnode);
            if (prevdevnode)
                curdevnode = prevdevnode->next;
            else
                curdevnode = GlobalDeviceList;
        } else {
            if (curdevnode->device.AdvrTimeOut < 2 * incr) {
                /* This advertisement is about to expire, so
                 * send out a search request for this device
                 * UDN to try to renew */
                ret = UpnpSearchAsync(ctrlpt_handle, incr,
                                      curdevnode->device.UDN, NULL);
                if (ret != UPNP_E_SUCCESS)
                    SampleUtil_Print("Error sending search request "
                                     "for Device UDN: %s -- err = "
                                     "%d\n",
                                     curdevnode->device.UDN, ret);
            }
            prevdevnode = curdevnode;
            curdevnode = curdevnode->next;
        }
    }

    pthread_mutex_unlock(&DeviceListMutex);
}

/// \brief TvCtrlPointTimerLoopRun
static int TvCtrlPointTimerLoopRun = 1;
/*!
 * \brief Function that runs in its own thread and monitors advertisement
 * and subscription timeouts for devices in the global device list.
 */
void* TvCtrlPointTimerLoop([[maybe_unused]] void* args) {
    /* how often to verify the timeouts, in seconds */
    int incr = 30;

    while (TvCtrlPointTimerLoopRun) {
        std::this_thread::sleep_for(std::chrono::seconds(incr));
        TvCtrlPointVerifyTimeouts(incr);
    }

    return NULL;
}

int TvCtrlPointStart(char* iface, state_update updateFunctionPtr, int combo) {
    pthread_t timer_thread;
    int rc;
    unsigned short port = 0;

    SampleUtil_RegisterUpdateFunction(updateFunctionPtr);

    pthread_mutex_init(&DeviceListMutex, 0);

    SampleUtil_Print("Initializing UPnP Sdk with\n"
                     "\tinterface = %s port = %u\n",
                     iface ? iface : "{NULL}", port);

    rc = UpnpInit2(iface, port);
    if (rc != UPNP_E_SUCCESS) {
        SampleUtil_Print("WinCEStart: UpnpInit2() Error: %d\n", rc);
        if (!combo) {
            UpnpFinish();

            return TV_ERROR;
        }
    }

    SampleUtil_Print("UPnP Initialized\n"
                     "\tipv4 address = %s port = %u\n"
                     "\tipv6 address = %s port = %u\n"
                     "\tipv6ulagua address = %s port = %u\n",
                     UpnpGetServerIpAddress(), UpnpGetServerPort(),
                     UpnpGetServerIp6Address(), UpnpGetServerPort6(),
                     UpnpGetServerUlaGuaIp6Address(),
                     UpnpGetServerUlaGuaPort6());
    SampleUtil_Print("Registering Control Point\n");
    rc = UpnpRegisterClient(TvCtrlPointCallbackEventHandler, &ctrlpt_handle,
                            &ctrlpt_handle);
    if (rc != UPNP_E_SUCCESS) {
        SampleUtil_Print("Error registering CP: %d\n", rc);
        UpnpFinish();

        return TV_ERROR;
    }

    SampleUtil_Print("Control Point Registered\n");

    TvCtrlPointRefresh();

    /* start a timer thread */
    pthread_create(&timer_thread, NULL, TvCtrlPointTimerLoop, NULL);
    pthread_detach(timer_thread);

    return TV_SUCCESS;
}

int TvCtrlPointStop() {
    TvCtrlPointTimerLoopRun = 0;
    TvCtrlPointRemoveAll();
    UpnpUnRegisterClient(ctrlpt_handle);
    UpnpFinish();
    SampleUtil_Finish();

    return TV_SUCCESS;
}

void TvCtrlPointPrintShortHelp() {
    SampleUtil_Print("Commands:\n"
                     "  Help\n"
                     "  HelpFull\n"
                     "  ToggleVerbose\n"
                     "  ListDev\n"
                     "  Refresh\n"
                     "  PrintDev      <devnum>\n"
                     "  PowerOn       <devnum>\n"
                     "  PowerOff      <devnum>\n"
                     "  SetChannel    <devnum> <channel>\n"
                     "  SetVolume     <devnum> <volume>\n"
                     "  SetColor      <devnum> <color>\n"
                     "  SetTint       <devnum> <tint>\n"
                     "  SetContrast   <devnum> <contrast>\n"
                     "  SetBrightness <devnum> <brightness>\n"
                     "  CtrlAction    <devnum> <action>\n"
                     "  PictAction    <devnum> <action>\n"
                     "  CtrlGetVar    <devnum> <varname>\n"
                     "  PictGetVar    <devnum> <action>\n"
                     "  Exit\n");
}

void TvCtrlPointPrintLongHelp() {
    SampleUtil_Print(
        "\n"
        "******************************\n"
        "* TV Control Point Help Info *\n"
        "******************************\n"
        "\n"
        "This sample control point application automatically searches\n"
        "for and subscribes to the services of television device emulator\n"
        "devices, described in the tvdevicedesc.xml description document.\n"
        "It also registers itself as a tv device.\n"
        "\n"
        "Commands:\n"
        "  Help\n"
        "       Print a short help info.\n"
        "  HelpFull\n"
        "       Print this extended help info.\n"
        "  ToggleVerbose\n"
        "       Toggle (on/off) verbose debug output.\n"
        "  ListDev\n"
        "       Print the current list of TV Device Emulators that this\n"
        "         control point is aware of. Each device is preceded by a\n"
        "         device number which corresponds to the devnum argument of\n"
        "         commands listed below.\n"
        "  Refresh\n"
        "       Delete all of the devices from the device list and issue new\n"
        "         search request to rebuild the list from scratch.\n"
        "  PrintDev       <devnum>\n"
        "       Print the state table for the device <devnum>.\n"
        "         e.g., 'PrintDev 1' prints the state table for the first\n"
        "         device in the device list.\n"
        "  PowerOn        <devnum>\n"
        "       Sends the PowerOn action to the Control Service of\n"
        "         device <devnum>.\n"
        "  PowerOff       <devnum>\n"
        "       Sends the PowerOff action to the Control Service of\n"
        "         device <devnum>.\n"
        "  SetChannel     <devnum> <channel>\n"
        "       Sends the SetChannel action to the Control Service of\n"
        "         device <devnum>, requesting the channel to be changed\n"
        "         to <channel>.\n"
        "  SetVolume      <devnum> <volume>\n"
        "       Sends the SetVolume action to the Control Service of\n"
        "         device <devnum>, requesting the volume to be changed\n"
        "         to <volume>.\n"
        "  SetColor       <devnum> <color>\n"
        "       Sends the SetColor action to the Control Service of\n"
        "         device <devnum>, requesting the color to be changed\n"
        "         to <color>.\n"
        "  SetTint        <devnum> <tint>\n"
        "       Sends the SetTint action to the Control Service of\n"
        "         device <devnum>, requesting the tint to be changed\n"
        "         to <tint>.\n"
        "  SetContrast    <devnum> <contrast>\n"
        "       Sends the SetContrast action to the Control Service of\n"
        "         device <devnum>, requesting the contrast to be changed\n"
        "         to <contrast>.\n"
        "  SetBrightness  <devnum> <brightness>\n"
        "       Sends the SetBrightness action to the Control Service of\n"
        "         device <devnum>, requesting the brightness to be changed\n"
        "         to <brightness>.\n"
        "  CtrlAction     <devnum> <action>\n"
        "       Sends an action request specified by the string <action>\n"
        "         to the Control Service of device <devnum>.  This command\n"
        "         only works for actions that have no arguments.\n"
        "         (e.g., \"CtrlAction 1 IncreaseChannel\")\n"
        "  PictAction     <devnum> <action>\n"
        "       Sends an action request specified by the string <action>\n"
        "         to the Picture Service of device <devnum>.  This command\n"
        "         only works for actions that have no arguments.\n"
        "         (e.g., \"PictAction 1 DecreaseContrast\")\n"
        "  CtrlGetVar     <devnum> <varname>\n"
        "       Requests the value of a variable specified by the string\n"
        "         <varname> from the Control Service of device <devnum>.\n"
        "         (e.g., \"CtrlGetVar 1 Volume\")\n"
        "  PictGetVar     <devnum> <action>\n"
        "       Requests the value of a variable specified by the string\n"
        "         <varname> from the Picture Service of device <devnum>.\n"
        "         (e.g., \"PictGetVar 1 Tint\")\n"
        "  Exit\n"
        "       Exits the control point application.\n");
}

/*! Tags for valid commands issued at the command prompt. */
enum cmdloop_tvcmds {
    PRTHELP = 0,
    PRTFULLHELP,
    VERBOSE,
    POWON,
    POWOFF,
    SETCHAN,
    SETVOL,
    SETCOL,
    SETTINT,
    SETCONT,
    SETBRT,
    CTRLACTION,
    PICTACTION,
    CTRLGETVAR,
    PICTGETVAR,
    PRTDEV,
    LSTDEV,
    REFRESH,
    EXITCMD
};

/*! Data structure for parsing commands from the command line. */
struct cmdloop_commands {
    /* the string  */
    const char* str;
    /* the command */
    int cmdnum;
    /* the number of arguments */
    int numargs;
    /* the args */
    const char* args;
};
/// \brief cmdloop_commands
cmdloop_commands cmdloop_commands;

/*! Mappings between command text names, command tag,
 * and required command arguments for command line
 * commands */
static struct cmdloop_commands cmdloop_cmdlist[] = {
    {"Help", PRTHELP, 1, ""},
    {"HelpFull", PRTFULLHELP, 1, ""},
    {"ToggleVerbose", VERBOSE, 1, ""},
    {"ListDev", LSTDEV, 1, ""},
    {"Refresh", REFRESH, 1, ""},
    {"PrintDev", PRTDEV, 2, "<devnum>"},
    {"PowerOn", POWON, 2, "<devnum>"},
    {"PowerOff", POWOFF, 2, "<devnum>"},
    {"SetChannel", SETCHAN, 3, "<devnum> <channel (int)>"},
    {"SetVolume", SETVOL, 3, "<devnum> <volume (int)>"},
    {"SetColor", SETCOL, 3, "<devnum> <color (int)>"},
    {"SetTint", SETTINT, 3, "<devnum> <tint (int)>"},
    {"SetContrast", SETCONT, 3, "<devnum> <contrast (int)>"},
    {"SetBrightness", SETBRT, 3, "<devnum> <brightness (int)>"},
    {"CtrlAction", CTRLACTION, 2, "<devnum> <action (string)>"},
    {"PictAction", PICTACTION, 2, "<devnum> <action (string)>"},
    {"CtrlGetVar", CTRLGETVAR, 2, "<devnum> <varname (string)>"},
    {"PictGetVar", PICTGETVAR, 2, "<devnum> <varname (string)>"},
    {"Exit", EXITCMD, 1, ""}};

void TvCtrlPointPrintCommands() {
    int i;
    int numofcmds = (sizeof cmdloop_cmdlist) / sizeof(cmdloop_commands);

    SampleUtil_Print("Valid Commands:\n");
    for (i = 0; i < numofcmds; ++i) {
        SampleUtil_Print("  %-14s %s\n", cmdloop_cmdlist[i].str,
                         cmdloop_cmdlist[i].args);
    }
    SampleUtil_Print("\n");
}

void* TvCtrlPointCommandLoop(void* args) {
    char cmdline[100];
    char* s;
    (void)args;

    while (1) {
        SampleUtil_Print("\n>> ");
        s = fgets(cmdline, 100, stdin);
        if (!s)
            break;
        TvCtrlPointProcessCommand(cmdline);
#if 0
        // TODO: instead; work in progress.
        int c = std::getchar();
        if (c == EOF)
            break;
        if (c == '\n') {
            SampleUtil_Print(":");
            // gPrintFun = nullptr;
            s = fgets(cmdline, sizeof(cmdline), stdin);
            if (!s)
                break;
            TvCtrlPointProcessCommand(cmdline);
            // gPrintFun = linux_print;
        }
#endif
    }

    return NULL;
}

int TvCtrlPointProcessCommand(char* cmdline) {
    char cmd[100];
    char strarg[100];
    int arg_val_err = -99999;
    int arg1 = arg_val_err;
    int arg2 = arg_val_err;
    int cmdnum = -1;
    int numofcmds = (sizeof cmdloop_cmdlist) / sizeof(cmdloop_commands);
    int cmdfound = 0;
    int i;
    int rc;
    int invalidargs = 0;
    int validargs;

#ifdef _WIN32
    validargs = sscanf_s(cmdline, "%s %d %d", cmd, (unsigned)_countof(cmd),
                         &arg1, &arg2);
#else
    validargs = sscanf(cmdline, "%s %d %d", cmd, &arg1, &arg2);
#endif
    for (i = 0; i < numofcmds; ++i) {
        if (strcasecmp(cmd, cmdloop_cmdlist[i].str) == 0) {
            cmdnum = cmdloop_cmdlist[i].cmdnum;
            cmdfound++;
            if (validargs != cmdloop_cmdlist[i].numargs)
                invalidargs++;
            break;
        }
    }
    if (!cmdfound) {
        SampleUtil_Print("Command not found; try 'Help'\n");
        return TV_SUCCESS;
    }
    if (invalidargs) {
        SampleUtil_Print("Invalid arguments; try 'Help'\n");
        return TV_SUCCESS;
    }
    switch (cmdnum) {
    case PRTHELP:
        TvCtrlPointPrintShortHelp();
        break;
    case PRTFULLHELP:
        TvCtrlPointPrintLongHelp();
        break;
    case VERBOSE:
        UPnPsdk::g_dbug = !UPnPsdk::g_dbug;
        break;
    case POWON:
        TvCtrlPointSendPowerOn(arg1);
        break;
    case POWOFF:
        TvCtrlPointSendPowerOff(arg1);
        break;
    case SETCHAN:
        TvCtrlPointSendSetChannel(arg1, arg2);
        break;
    case SETVOL:
        TvCtrlPointSendSetVolume(arg1, arg2);
        break;
    case SETCOL:
        TvCtrlPointSendSetColor(arg1, arg2);
        break;
    case SETTINT:
        TvCtrlPointSendSetTint(arg1, arg2);
        break;
    case SETCONT:
        TvCtrlPointSendSetContrast(arg1, arg2);
        break;
    case SETBRT:
        TvCtrlPointSendSetBrightness(arg1, arg2);
        break;
    case CTRLACTION:
        /* re-parse commandline since second arg is string. */
#ifdef _WIN32
        validargs = sscanf_s(cmdline, "%s %d %s", cmd, (unsigned)_countof(cmd),
                             &arg1, strarg, (unsigned)_countof(strarg));
#else
        validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
#endif
        if (validargs == 3)
            TvCtrlPointSendAction(TV_SERVICE_CONTROL, arg1, strarg, NULL, NULL,
                                  0);
        else
            invalidargs++;
        break;
    case PICTACTION:
        /* re-parse commandline since second arg is string. */
#ifdef _WIN32
        validargs = sscanf_s(cmdline, "%s %d %s", cmd, (unsigned)_countof(cmd),
                             &arg1, strarg, (unsigned)_countof(strarg));
#else
        validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
#endif
        if (validargs == 3)
            TvCtrlPointSendAction(TV_SERVICE_PICTURE, arg1, strarg, NULL, NULL,
                                  0);
        else
            invalidargs++;
        break;
    case CTRLGETVAR:
        /* re-parse commandline since second arg is string. */
#ifdef _WIN32
        validargs = sscanf_s(cmdline, "%s %d %s", cmd, (unsigned)_countof(cmd),
                             &arg1, strarg, (unsigned)_countof(strarg));
#else
        validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
#endif
        if (validargs == 3)
            TvCtrlPointGetVar(TV_SERVICE_CONTROL, arg1, strarg);
        else
            invalidargs++;
        break;
    case PICTGETVAR:
        /* re-parse commandline since second arg is string. */
#ifdef _WIN32
        validargs = sscanf_s(cmdline, "%s %d %s", cmd, (unsigned)_countof(cmd),
                             &arg1, strarg, (unsigned)_countof(strarg));
#else
        validargs = sscanf(cmdline, "%s %d %s", cmd, &arg1, strarg);
#endif
        if (validargs == 3)
            TvCtrlPointGetVar(TV_SERVICE_PICTURE, arg1, strarg);
        else
            invalidargs++;
        break;
    case PRTDEV:
        TvCtrlPointPrintDevice(arg1);
        break;
    case LSTDEV:
        TvCtrlPointPrintList();
        break;
    case REFRESH:
        TvCtrlPointRefresh();
        break;
    case EXITCMD:
        rc = TvCtrlPointStop();
        exit(rc);
        break;
    default:
        SampleUtil_Print("Command not implemented; see 'Help'\n");
        break;
    }
    if (invalidargs)
        SampleUtil_Print("Invalid args in command; see 'Help'\n");

    return TV_SUCCESS;
}
