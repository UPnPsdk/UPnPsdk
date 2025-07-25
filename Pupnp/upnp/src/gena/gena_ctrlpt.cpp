/*******************************************************************************
 *
 * Copyright (c) 2000-2003 Intel Corporation
 * All rights reserved.
 * Copyright (c) 2012 France Telecom All rights reserved.
 * Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
 * Redistribution only with this Copyright remark. Last modified: 2025-07-17
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
// Last compare with pupnp original source file on 2025-07-17, ver 1.14.21

#include "config.hpp"

/*!
 * \file
 */

#if EXCLUDE_GENA == 0
#ifdef INCLUDE_CLIENT_APIS

#include "UpnpEventSubscribe.hpp"
#include "client_table.hpp"
#include "gena.hpp"
#include "httpparser.hpp"
#include "httpreadwrite.hpp"
#include "parsetools.hpp"
#include "statcodes.hpp"
#include "upnpapi.hpp"
#include "uuid.hpp"

#include "posix_overwrites.hpp" // IWYU pragma: keep

extern ithread_mutex_t GlobalClientSubscribeMutex;

typedef struct {
    int handle;
    int eventId;
    void* Event;
} job_arg;

/*!
 * \brief Free memory associated with job's argument
 */
static void free_subscribe_arg(void* arg) {
    if (arg) {
        job_arg* p = (job_arg*)arg;
        if (p->Event) {
            UpnpEventSubscribe_delete((UpnpEventSubscribe*)p->Event);
        }
        free(p);
    }
}

/*!
 * \brief This is a thread function to send the renewal just before the
 * subscription times out.
 */
static void GenaAutoRenewSubscription(
    /*! [in] Thread data(job_arg *) needed to send the renewal. */
    void* input) {
    job_arg* arg = (job_arg*)input;
    UpnpEventSubscribe* sub_struct = (UpnpEventSubscribe*)arg->Event;
    void* cookie;
    Upnp_FunPtr callback_fun;
    struct Handle_Info* handle_info;
    int send_callback = 0;
    Upnp_EventType eventType{};
    int timeout = 0;
    int errCode = 0;

    if (AUTO_RENEW_TIME == 0) {
        UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__, "GENA SUB EXPIRED\n");
        UpnpEventSubscribe_set_ErrCode(sub_struct, UPNP_E_SUCCESS);
        send_callback = 1;
        eventType = UPNP_EVENT_SUBSCRIPTION_EXPIRED;
    } else {
        UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__, "GENA AUTO RENEW\n");
        timeout = UpnpEventSubscribe_get_TimeOut(sub_struct);
        errCode = genaRenewSubscription(
            arg->handle, UpnpEventSubscribe_get_SID(sub_struct), &timeout);
        UpnpEventSubscribe_set_ErrCode(sub_struct, errCode);
        UpnpEventSubscribe_set_TimeOut(sub_struct, timeout);
        if (errCode != UPNP_E_SUCCESS && errCode != GENA_E_BAD_SID &&
            errCode != GENA_E_BAD_HANDLE) {
            send_callback = 1;
            eventType = UPNP_EVENT_AUTORENEWAL_FAILED;
        }
    }

    if (send_callback) {
        HandleReadLock(__FILE__, __LINE__);
        if (GetHandleInfo(arg->handle, &handle_info) != HND_CLIENT) {
            HandleUnlock(__FILE__, __LINE__);
            free_subscribe_arg(arg);
            goto end_function;
        }
        UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__, "HANDLE IS VALID\n");

        /* make callback */
        callback_fun = handle_info->Callback;
        cookie = handle_info->Cookie;
        HandleUnlock(__FILE__, __LINE__);
        callback_fun(eventType, arg->Event, cookie);
    }

    free_subscribe_arg(arg);

end_function:
    return;
}

/*!
 * \brief Schedules a job to renew the subscription just before time out.
 *
 * \return GENA_E_SUCCESS if successful, otherwise returns the appropriate
 *  error code.
 */
static int ScheduleGenaAutoRenew(
    /*! [in] Handle that also contains the subscription list. */
    int client_handle,
    /*! [in] The time out value of the subscription. */
    int TimeOut,
    /*! [in] Subscription being renewed. */
    GenlibClientSubscription* sub) {
    UpnpEventSubscribe* RenewEvent = NULL;
    job_arg* arg = NULL;
    int return_code = GENA_SUCCESS;
    ThreadPoolJob job;

    memset(&job, 0, sizeof(job));

    if (TimeOut == UPNP_INFINITE) {
        return_code = GENA_SUCCESS;
        goto end_function;
    }

    RenewEvent = UpnpEventSubscribe_new();
    if (RenewEvent == NULL) {
        return_code = UPNP_E_OUTOF_MEMORY;
        goto end_function;
    }

    arg = (job_arg*)malloc(sizeof(job_arg));
    if (arg == NULL) {
        free(RenewEvent);
        return_code = UPNP_E_OUTOF_MEMORY;
        goto end_function;
    }
    memset(arg, 0, sizeof(job_arg));

    /* schedule expire event */
    UpnpEventSubscribe_set_ErrCode(RenewEvent, UPNP_E_SUCCESS);
    UpnpEventSubscribe_set_TimeOut(RenewEvent, TimeOut);
    UpnpEventSubscribe_set_SID(RenewEvent,
                               GenlibClientSubscription_get_SID(sub));
    UpnpEventSubscribe_set_PublisherUrl(
        RenewEvent, GenlibClientSubscription_get_EventURL(sub));

    arg->handle = client_handle;
    arg->Event = RenewEvent;

    TPJobInit(&job, (start_routine)GenaAutoRenewSubscription, arg);
    TPJobSetFreeFunction(&job, (free_routine)free_subscribe_arg);
    TPJobSetPriority(&job, MED_PRIORITY);

    /* Schedule the job */
    return_code =
        TimerThreadSchedule(&gTimerThread, TimeOut - AUTO_RENEW_TIME, REL_SEC,
                            &job, SHORT_TERM, &(arg->eventId));
    if (return_code != UPNP_E_SUCCESS) {
        free_subscribe_arg(arg);
        goto end_function;
    }

    GenlibClientSubscription_set_RenewEventId(sub, arg->eventId);

    return_code = GENA_SUCCESS;

end_function:

    return return_code;
}

/*!
 * \brief Sends the UNSUBCRIBE gena request and recieves the response from the
 *  device and returns it as a parameter.
 *
 * \returns 0 if successful, otherwise returns the appropriate error code.
 */
static int gena_unsubscribe(
    /*! [in] Event URL of the service. */
    const UpnpString* url,
    /*! [in] The subcription ID. */
    const UpnpString* sid,
    /*! [out] The UNSUBCRIBE response from the device. */
    http_parser_t* response) {
    int return_code;
    uri_type dest_url;
    membuffer request;

    /* parse url */
    return_code = http_FixStrUrl(UpnpString_get_String(url),
                                 UpnpString_get_Length(url), &dest_url);
    if (return_code != 0) {
        return return_code;
    }

    /* make request msg */
    membuffer_init(&request);
    request.size_inc = 30;
    return_code = http_MakeMessage(&request, 1, 1,
                                   "q"
                                   "ssc"
                                   "Uc",
                                   HTTPMETHOD_UNSUBSCRIBE, &dest_url,
                                   "SID: ", UpnpString_get_String(sid));

    /* Not able to make the message so destroy the existing buffer */
    if (return_code != 0) {
        membuffer_destroy(&request);

        return return_code;
    }

    /* send request and get reply */
    return_code = http_RequestAndResponse(
        &dest_url, request.buf, request.length, HTTPMETHOD_UNSUBSCRIBE,
        HTTP_DEFAULT_TIMEOUT, response);
    membuffer_destroy(&request);
    if (return_code != 0) {
        httpmsg_destroy(&response->msg);
    }

    if (return_code == 0 && response->msg.status_code != HTTP_OK) {
        return_code = UPNP_E_UNSUBSCRIBE_UNACCEPTED;
        httpmsg_destroy(&response->msg);
    }

    return return_code;
}

/*!
 * \brief Subscribes or renew subscription.
 *
 * \return 0 if successful, otherwise returns the appropriate error code.
 */
static int gena_subscribe(
    /*! [in] URL of service to subscribe. */
    const UpnpString* url,
    /*! [in,out] Subscription time desired (in secs). */
    int* timeout,
    /*! [in] for renewal, this contains a currently held subscription SID.
     * For first time subscription, this must be NULL. */
    const UpnpString* renewal_sid,
    /*! [out] SID returned by the subscription or renew msg. */
    UpnpString* sid) {
    int return_code;
    parse_status_t parse_ret{};
    int local_timeout = CP_MINIMUM_SUBSCRIPTION_TIME;
    memptr sid_hdr;
    memptr timeout_hdr;
    char timeout_str[25];
    membuffer request;
    uri_type dest_url;
    http_parser_t response;
    int rc = 0;

    UpnpString_clear(sid);

    /* request timeout to string */
    if (timeout == NULL) {
        timeout = &local_timeout;
    }
    if (*timeout < 0) {
        memset(timeout_str, 0, sizeof(timeout_str));
        strncpy(timeout_str, "infinite", sizeof(timeout_str) - 1);
    } else if (*timeout < CP_MINIMUM_SUBSCRIPTION_TIME) {
        rc = snprintf(timeout_str, sizeof(timeout_str), "%d",
                      CP_MINIMUM_SUBSCRIPTION_TIME);
    } else {
        rc = snprintf(timeout_str, sizeof(timeout_str), "%d", *timeout);
    }
    if (rc < 0 || (unsigned int)rc >= sizeof(timeout_str))
        return UPNP_E_OUTOF_MEMORY;

    /* parse url */
    return_code = http_FixStrUrl(UpnpString_get_String(url),
                                 UpnpString_get_Length(url), &dest_url);
    if (return_code != 0) {
        return return_code;
    }

    /* make request msg */
    membuffer_init(&request);
    request.size_inc = 30;
    if (renewal_sid) {
        /* renew subscription */
        return_code =
            http_MakeMessage(&request, 1, 1,
                             "q"
                             "ssc"
                             "sscc",
                             HTTPMETHOD_SUBSCRIBE, &dest_url,
                             "SID: ", UpnpString_get_String(renewal_sid),
                             "TIMEOUT: Second-", timeout_str);
    } else {
        /* subscribe */
        if (dest_url.hostport.IPaddress.ss_family == AF_INET6) {
            struct sockaddr_in6* DestAddr6 =
                (struct sockaddr_in6*)&dest_url.hostport.IPaddress;
            return_code = http_MakeMessage(
                &request, 1, 1,
                "q"
                "sssdsc"
                "sc"
                "sscc",
                HTTPMETHOD_SUBSCRIBE, &dest_url, "CALLBACK: <http://[",
                (IN6_IS_ADDR_LINKLOCAL(&DestAddr6->sin6_addr) ||
                 strlen(gIF_IPV6_ULA_GUA) == 0)
                    ? gIF_IPV6
                    : gIF_IPV6_ULA_GUA,
                "]:",
                (IN6_IS_ADDR_LINKLOCAL(&DestAddr6->sin6_addr) ||
                 strlen(gIF_IPV6_ULA_GUA) == 0)
                    ? LOCAL_PORT_V6
                    : LOCAL_PORT_V6_ULA_GUA,
                "/>", "NT: upnp:event", "TIMEOUT: Second-", timeout_str);
        } else {
            return_code = http_MakeMessage(
                &request, 1, 1,
                "q"
                "sssdsc"
                "sc"
                "sscc",
                HTTPMETHOD_SUBSCRIBE, &dest_url, "CALLBACK: <http://", gIF_IPV4,
                ":", LOCAL_PORT_V4, "/>", "NT: upnp:event", "TIMEOUT: Second-",
                timeout_str);
        }
    }
    if (return_code != 0) {
        return return_code;
    }

    /* send request and get reply */
    return_code = http_RequestAndResponse(&dest_url, request.buf,
                                          request.length, HTTPMETHOD_SUBSCRIBE,
                                          HTTP_DEFAULT_TIMEOUT, &response);
    membuffer_destroy(&request);

    if (return_code != 0) {
        httpmsg_destroy(&response.msg);

        return return_code;
    }
    if (response.msg.status_code != HTTP_OK) {
        httpmsg_destroy(&response.msg);

        return UPNP_E_SUBSCRIBE_UNACCEPTED;
    }

    /* get SID and TIMEOUT */
    if (httpmsg_find_hdr(&response.msg, HDR_SID, &sid_hdr) == NULL ||
        sid_hdr.length == 0 ||
        httpmsg_find_hdr(&response.msg, HDR_TIMEOUT, &timeout_hdr) == NULL ||
        timeout_hdr.length == 0) {
        httpmsg_destroy(&response.msg);

        return UPNP_E_BAD_RESPONSE;
    }

    /* save timeout */
    parse_ret =
        matchstr(timeout_hdr.buf, timeout_hdr.length, "%iSecond-%d%0", timeout);
    if (parse_ret == PARSE_OK) {
        /* nothing to do */
    } else if (memptr_cmp_nocase(&timeout_hdr, "Second-infinite") == 0) {
        *timeout = -1;
    } else {
        httpmsg_destroy(&response.msg);

        return UPNP_E_BAD_RESPONSE;
    }

    /* save SID */
    UpnpString_set_StringN(sid, sid_hdr.buf, sid_hdr.length);
    if (UpnpString_get_String(sid) == NULL) {
        httpmsg_destroy(&response.msg);

        return UPNP_E_OUTOF_MEMORY;
    }
    httpmsg_destroy(&response.msg);

    return UPNP_E_SUCCESS;
}

int genaUnregisterClient(UpnpClient_Handle client_handle) {
    GenlibClientSubscription* sub_copy = GenlibClientSubscription_new();
    int return_code = UPNP_E_SUCCESS;
    struct Handle_Info* handle_info = NULL;
    http_parser_t response;

    while (1) {
        HandleLock(__FILE__, __LINE__);

        if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
            HandleUnlock(__FILE__, __LINE__);
            return_code = GENA_E_BAD_HANDLE;
            goto exit_function;
        }
        if (handle_info->ClientSubList == NULL) {
            return_code = UPNP_E_SUCCESS;
            break;
        }
        GenlibClientSubscription_assign(sub_copy, handle_info->ClientSubList);
        RemoveClientSubClientSID(&handle_info->ClientSubList,
                                 GenlibClientSubscription_get_SID(sub_copy));

        HandleUnlock(__FILE__, __LINE__);

        return_code = gena_unsubscribe(
            GenlibClientSubscription_get_EventURL(sub_copy),
            GenlibClientSubscription_get_ActualSID(sub_copy), &response);
        if (return_code == 0) {
            httpmsg_destroy(&response.msg);
        }
        free_client_subscription(sub_copy);
    }

    freeClientSubList(handle_info->ClientSubList);
    HandleUnlock(__FILE__, __LINE__);

exit_function:
    GenlibClientSubscription_delete(sub_copy);
    return return_code;
}

#ifdef INCLUDE_CLIENT_APIS
int genaUnSubscribe(UpnpClient_Handle client_handle, const UpnpString* in_sid) {
    GenlibClientSubscription* sub = NULL;
    int return_code = GENA_SUCCESS;
    struct Handle_Info* handle_info;
    GenlibClientSubscription* sub_copy = GenlibClientSubscription_new();
    http_parser_t response;

    /* validate handle and sid */
    HandleLock(__FILE__, __LINE__);
    if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
        HandleUnlock(__FILE__, __LINE__);
        return_code = GENA_E_BAD_HANDLE;
        goto exit_function;
    }
    sub = GetClientSubClientSID(handle_info->ClientSubList, in_sid);
    if (sub == NULL) {
        HandleUnlock(__FILE__, __LINE__);
        return_code = GENA_E_BAD_SID;
        goto exit_function;
    }
    GenlibClientSubscription_assign(sub_copy, sub);
    HandleUnlock(__FILE__, __LINE__);

    return_code = gena_unsubscribe(
        GenlibClientSubscription_get_EventURL(sub_copy),
        GenlibClientSubscription_get_ActualSID(sub_copy), &response);
    if (return_code == 0) {
        httpmsg_destroy(&response.msg);
    }
    free_client_subscription(sub_copy);

    HandleLock(__FILE__, __LINE__);
    if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
        HandleUnlock(__FILE__, __LINE__);
        return_code = GENA_E_BAD_HANDLE;
        goto exit_function;
    }
    RemoveClientSubClientSID(&handle_info->ClientSubList, in_sid);
    HandleUnlock(__FILE__, __LINE__);

exit_function:
    GenlibClientSubscription_delete(sub_copy);
    return return_code;
}
#endif /* INCLUDE_CLIENT_APIS */

#ifdef INCLUDE_CLIENT_APIS
int genaSubscribe(UpnpClient_Handle client_handle,
                  const UpnpString* PublisherURL, int* TimeOut,
                  UpnpString* out_sid) {
    int return_code = GENA_SUCCESS;
    GenlibClientSubscription* newSubscription = GenlibClientSubscription_new();
    uuid_upnp uid;
    Upnp_SID temp_sid;
    Upnp_SID temp_sid2;
    UpnpString* ActualSID = UpnpString_new();
    UpnpString* EventURL = UpnpString_new();
    struct Handle_Info* handle_info;
    int rc = 0;

    memset(temp_sid, 0, sizeof(temp_sid));
    memset(temp_sid2, 0, sizeof(temp_sid2));

    UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__, "GENA SUBSCRIBE BEGIN\n");

    UpnpString_clear(out_sid);

    HandleReadLock(__FILE__, __LINE__);
    /* validate handle */
    if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
        return_code = GENA_E_BAD_HANDLE;
        SubscribeLock();
        goto error_handler;
    }
    HandleUnlock(__FILE__, __LINE__);

    /* subscribe */
    SubscribeLock();
    return_code = gena_subscribe(PublisherURL, TimeOut, NULL, ActualSID);
    HandleLock(__FILE__, __LINE__);
    if (return_code != UPNP_E_SUCCESS) {
        UpnpPrintf(UPNP_CRITICAL, GENA, __FILE__, __LINE__,
                   "SUBSCRIBE FAILED in transfer error code: %d "
                   "returned\n",
                   return_code);
        goto error_handler;
    }

    if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
        return_code = GENA_E_BAD_HANDLE;
        goto error_handler;
    }

    /* generate client SID */
    uuid_create(&uid);
    upnp_uuid_unpack(&uid, temp_sid);
    rc = snprintf(temp_sid2, sizeof(temp_sid2), "uuid:%s", temp_sid);
    if (rc < 0 || (unsigned int)rc >= sizeof(temp_sid2)) {
        return_code = UPNP_E_OUTOF_MEMORY;
        goto error_handler;
    }
    UpnpString_set_String(out_sid, temp_sid2);

    /* create event url */
    UpnpString_assign(EventURL, PublisherURL);

    /* fill subscription */
    if (newSubscription == NULL) {
        return_code = UPNP_E_OUTOF_MEMORY;
        goto error_handler;
    }
    GenlibClientSubscription_set_RenewEventId(newSubscription, -1);
    GenlibClientSubscription_set_SID(newSubscription, out_sid);
    GenlibClientSubscription_set_ActualSID(newSubscription, ActualSID);
    GenlibClientSubscription_set_EventURL(newSubscription, EventURL);
    GenlibClientSubscription_set_Next(newSubscription,
                                      handle_info->ClientSubList);
    handle_info->ClientSubList = newSubscription;

    /* schedule expiration event */
    return_code =
        ScheduleGenaAutoRenew(client_handle, *TimeOut, newSubscription);

error_handler:
    UpnpString_delete(ActualSID);
    UpnpString_delete(EventURL);
    if (return_code != UPNP_E_SUCCESS)
        GenlibClientSubscription_delete(newSubscription);
    HandleUnlock(__FILE__, __LINE__);
    SubscribeUnlock();

    return return_code;
}
#endif /* INCLUDE_CLIENT_APIS */

int genaRenewSubscription(UpnpClient_Handle client_handle,
                          const UpnpString* in_sid, int* TimeOut) {
    int return_code = GENA_SUCCESS;
    GenlibClientSubscription* sub = NULL;
    GenlibClientSubscription* sub_copy = GenlibClientSubscription_new();
    struct Handle_Info* handle_info;
    UpnpString* ActualSID = UpnpString_new();
    ThreadPoolJob tempJob;

    HandleLock(__FILE__, __LINE__);

    /* validate handle and sid */
    if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
        HandleUnlock(__FILE__, __LINE__);

        return_code = GENA_E_BAD_HANDLE;
        goto exit_function;
    }

    sub = GetClientSubClientSID(handle_info->ClientSubList, in_sid);
    if (sub == NULL) {
        HandleUnlock(__FILE__, __LINE__);

        return_code = GENA_E_BAD_SID;
        goto exit_function;
    }

    /* remove old events */
    if (TimerThreadRemove(&gTimerThread,
                          GenlibClientSubscription_get_RenewEventId(sub),
                          &tempJob) == 0) {
        tempJob.free_func(tempJob.arg);
    }

    UpnpPrintf(UPNP_INFO, GENA, __FILE__, __LINE__,
               "REMOVED AUTO RENEW  EVENT\n");

    GenlibClientSubscription_set_RenewEventId(sub, -1);
    GenlibClientSubscription_assign(sub_copy, sub);

    HandleUnlock(__FILE__, __LINE__);

    return_code = gena_subscribe(
        GenlibClientSubscription_get_EventURL(sub_copy), TimeOut,
        GenlibClientSubscription_get_ActualSID(sub_copy), ActualSID);

    HandleLock(__FILE__, __LINE__);

    if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
        HandleUnlock(__FILE__, __LINE__);
        return_code = GENA_E_BAD_HANDLE;
        goto exit_function;
    }

    /* we just called GetHandleInfo, so we don't check for return value */
    /*GetHandleInfo(client_handle, &handle_info); */
    if (return_code != UPNP_E_SUCCESS) {
        /* network failure (remove client sub) */
        RemoveClientSubClientSID(&handle_info->ClientSubList, in_sid);
        free_client_subscription(sub_copy);
        HandleUnlock(__FILE__, __LINE__);
        goto exit_function;
    }

    /* get subscription */
    sub = GetClientSubClientSID(handle_info->ClientSubList, in_sid);
    if (sub == NULL) {
        free_client_subscription(sub_copy);
        HandleUnlock(__FILE__, __LINE__);
        return_code = GENA_E_BAD_SID;
        goto exit_function;
    }

    /* store actual sid */
    GenlibClientSubscription_set_ActualSID(sub, ActualSID);

    /* start renew subscription timer */
    return_code = ScheduleGenaAutoRenew(client_handle, *TimeOut, sub);
    if (return_code != GENA_SUCCESS) {
        RemoveClientSubClientSID(&handle_info->ClientSubList,
                                 GenlibClientSubscription_get_SID(sub));
    }
    free_client_subscription(sub_copy);
    HandleUnlock(__FILE__, __LINE__);

exit_function:
    UpnpString_delete(ActualSID);
    GenlibClientSubscription_delete(sub_copy);
    return return_code;
}

void gena_process_notification_event(SOCKINFO* info, http_message_t* event) {
    UpnpEvent* event_struct = UpnpEvent_new();
    IXML_Document* ChangedVars = NULL;
    int eventKey;
    token sid;
    GenlibClientSubscription* subscription = NULL;
    struct Handle_Info* handle_info;
    void* cookie;
    Upnp_FunPtr callback;
    UpnpClient_Handle client_handle;
    UpnpClient_Handle client_handle_start;
    int err_ret = HTTP_PRECONDITION_FAILED;

    memptr sid_hdr;
    memptr nt_hdr, nts_hdr;
    memptr seq_hdr;

    /* get SID */
    if (httpmsg_find_hdr(event, HDR_SID, &sid_hdr) == NULL) {
        error_respond(info, HTTP_PRECONDITION_FAILED, event);
        goto exit_function;
    }
    sid.buff = sid_hdr.buf;
    sid.size = sid_hdr.length;

    /* get event key */
    if (httpmsg_find_hdr(event, HDR_SEQ, &seq_hdr) == NULL ||
        matchstr(seq_hdr.buf, seq_hdr.length, "%d%0", &eventKey) != PARSE_OK) {
        error_respond(info, HTTP_BAD_REQUEST, event);
        goto exit_function;
    }

    /* get NT and NTS headers */
    if (httpmsg_find_hdr(event, HDR_NT, &nt_hdr) == NULL ||
        httpmsg_find_hdr(event, HDR_NTS, &nts_hdr) == NULL) {
        error_respond(info, HTTP_BAD_REQUEST, event);
        goto exit_function;
    }

    /* verify NT and NTS headers */
    if (memptr_cmp(&nt_hdr, "upnp:event") != 0 ||
        memptr_cmp(&nts_hdr, "upnp:propchange") != 0) {
        error_respond(info, HTTP_PRECONDITION_FAILED, event);
        goto exit_function;
    }

    /* parse the content (should be XML) */
    if (!has_xml_content_type(event) || event->msg.length == 0 ||
        ixmlParseBufferEx(event->entity.buf, &ChangedVars) != IXML_SUCCESS) {
        error_respond(info, HTTP_BAD_REQUEST, event);
        goto exit_function;
    }

    HandleLock(__FILE__, __LINE__);

    /* get client info */
    if (GetClientHandleInfo(&client_handle_start, &handle_info) != HND_CLIENT) {
        error_respond(info, HTTP_PRECONDITION_FAILED, event);
        HandleUnlock(__FILE__, __LINE__);
        goto exit_function;
    }

    HandleUnlock(__FILE__, __LINE__);

    for (client_handle = client_handle_start; client_handle < NUM_HANDLE;
         client_handle++) {
        HandleLock(__FILE__, __LINE__);

        /* get client info */
        if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
            HandleUnlock(__FILE__, __LINE__);
            continue;
        }

        /* get subscription based on SID */
        subscription = GetClientSubActualSID(handle_info->ClientSubList, &sid);
        if (subscription == NULL) {
            if (eventKey == 0) {
                /* wait until we've finished processing a
                 * subscription  */
                /*   (if we are in the middle) */
                /* this is to avoid mistakenly rejecting the
                 * first event if we  */
                /*   receive it before the subscription response
                 */
                HandleUnlock(__FILE__, __LINE__);

                /* try and get Subscription Lock  */
                /*   (in case we are in the process of
                 * subscribing) */
                SubscribeLock();

                /* get HandleLock again */
                HandleLock(__FILE__, __LINE__);

                if (GetHandleInfo(client_handle, &handle_info) != HND_CLIENT) {
                    SubscribeUnlock();
                    HandleUnlock(__FILE__, __LINE__);
                    continue;
                }

                subscription =
                    GetClientSubActualSID(handle_info->ClientSubList, &sid);
                if (subscription == NULL) {
                    SubscribeUnlock();
                    HandleUnlock(__FILE__, __LINE__);
                    continue;
                }

                SubscribeUnlock();
            } else {
                HandleUnlock(__FILE__, __LINE__);
                continue;
            }
        }

        /* success */
        err_ret = HTTP_OK;

        /* fill event struct */
        UpnpEvent_set_EventKey(event_struct, eventKey);
        UpnpEvent_set_ChangedVariables(event_struct, ChangedVars);
        UpnpEvent_set_SID(event_struct,
                          GenlibClientSubscription_get_SID(subscription));

        /* copy callback */
        callback = handle_info->Callback;
        cookie = handle_info->Cookie;

        HandleUnlock(__FILE__, __LINE__);

        /* make callback with event struct */
        /* In future, should find a way of mainting */
        /* that the handle is not unregistered in the middle of a */
        /* callback */
        callback(UPNP_EVENT_RECEIVED, event_struct, cookie);
    }

    error_respond(info, err_ret, event);

exit_function:
    ixmlDocument_free(ChangedVars);
    UpnpEvent_delete(event_struct);
}

#endif /* INCLUDE_CLIENT_APIS */
#endif /* EXCLUDE_GENA */
