#ifdef COMPA_HAVE_CTRLPT_SSDP

#ifndef COMPA_SSDPRESULTDATA_HPP
#define COMPA_SSDPRESULTDATA_HPP
// Copyright (C) 2022+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-06-12
/*!
 * \file
 * \brief Header file for SSDPResultData methods.
 * \ingroup compa-Discovery
 *
 * \author Marcelo Roberto Jimenez
 */

#include <UpnpDiscovery.hpp>
#include <Callback.hpp>

/// \cond
#include <cstdlib> /* for size_t */
/// \endcond

/*! \brief SSDPResultData
 * \ingroup SSDP-Data */
typedef struct s_SSDPResultData SSDPResultData;

/*! @{
 * \ingroup SSDP-common_functions */

/*! \brief Constructor */
SSDPResultData* SSDPResultData_new();
/*! \brief Destructor */
void SSDPResultData_delete(SSDPResultData* p);
/*! \brief Copy Constructor */
SSDPResultData* SSDPResultData_dup(const SSDPResultData* p);
/*! \brief Assignment operator */
int SSDPResultData_assign(SSDPResultData* p, const SSDPResultData* q);

/*! \brief SSDPResultData_get_Param */
const UpnpDiscovery* SSDPResultData_get_Param(const SSDPResultData* p);
/*! \brief SSDPResultData_set_Param */
int SSDPResultData_set_Param(SSDPResultData* p, const UpnpDiscovery* n);

/*! \brief SSDPResultData_get_Cookie */
void* SSDPResultData_get_Cookie(const SSDPResultData* p);
/*! \brief SSDPResultData_set_Cookie */
int SSDPResultData_set_Cookie(SSDPResultData* p, void* n);

/*! \brief SSDPResultData_get_CtrlptCallback */
Upnp_FunPtr SSDPResultData_get_CtrlptCallback(const SSDPResultData* p);
/*! \brief SSDPResultData_set_CtrlptCallback */
int SSDPResultData_set_CtrlptCallback(SSDPResultData* p, Upnp_FunPtr n);

/// @} SSDP Common Functions

#endif // COMPA_HAVE_CTRLPT_SSDP
#endif /* COMPA_SSDPRESULTDATA_HPP */
