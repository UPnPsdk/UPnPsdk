#ifndef UPnPsdk_NETADAPTER2_HPP
#define UPnPsdk_NETADAPTER2_HPP
// Copyright (C) 2025+ GPL 3 and higher by Ingo Höft, <Ingo@Hoeft-online.de>
// Redistribution only with this Copyright remark. Last modified: 2025-03-15
/*!
 * \file
 * \brief Manage information about network adapters.
 */

#include <UPnPsdk/visibility.hpp>
#include <string>
#include <memory>

namespace UPnPsdk {

/*!
 * \brief Portable catch **one** network socket error from the operating
 * system, and provide information about it.
 * \ingroup upnplibAPI-socket
 * \ingroup upnplib-socket
 *
 * This is a C++ interface for dependency injection of different
 * \glos{depinj,di-services}, e.g. for production or Unit Tests (mocking).
 */
class UPnPsdk_API ISocketErr {
  public:
    ISocketErr();
    virtual ~ISocketErr();
    /// Get error number.
    virtual operator const int&() = 0;
    /// Catch error for later use.
    virtual void catch_error() = 0;
    /// Get human readable error description of the catched error.
    virtual std::string get_error_str() const = 0;
};

/*!
 * \brief Smart pointer to \glos{depinj,di-service} objects that handle network
 * socket errors and used to inject the objects.
 * \ingroup upnplib-socket
 */
using PSocketErr = std::shared_ptr<ISocketErr>;

/*!
 * \brief \glos{depinj,di-service} for portable handling of network socket
 * errors.
 * \ingroup upnplibAPI-socket
 * \ingroup upnplib-socket
 *
 * There is a compatibility problem with Winsock2 on the Microsoft Windows
 * platform that does not support detailed error information given in the global
 * variable 'errno' that is used by POSIX. Instead it returns them with calling
 * 'WSAGetLastError()'. This class encapsulates differences so there is no need
 * to always check the platform to get the error information.
 */
class UPnPsdk_API CSocketErrService : public ISocketErr {
  public:
    CSocketErrService();
    virtual ~CSocketErrService() override;
    /// Get error number.
    virtual operator const int&() override;
    /// Catch error for later use.
    virtual void catch_error() override;
    /// Get human readable error description of the catched error.
    virtual std::string get_error_str() const override;

  private:
    int m_errno{}; // Cached error number
};

/*!
 * \brief \glos{depinj,di-client} for portable handling of network socket
 * errors with injected \glos{depinj,di-service}.
 * \ingroup upnplibAPI-socket
 * \ingroup upnplib-socket
 *
 * Usage of the class:
 * \code
 * CSocketErr2 serrObj;
 * int ret = some_function_1();
 * if (ret != 0) {
 *     serrObj.catch_error();
 *     int errid = serrObj;
 *     std::cout << "Error " << errid << ": "
 *               << serrObj.get_error_str() << "\n";
 * }
 * ret = some_function_2();
 * if (ret != 0) {
 *     serrObj.catch_error();
 *     std::cout << "Error " << static_cast<int>(serrObj) << ": "
 *               << serrObj.get_error_str() << "\n";
 * }
 * \endcode
 */
class CSocketErr2 {
    // Due to warning C4251 "'type' : class 'type1' needs to have dll-interface
    // to be used by clients of class 'type2'" on Microsoft Windows each member
    // function needs to be decorated with UPnPsdk_API instead of just only the
    // class. The reason is 'm_socket_errObj'.
  public:
    /*! \brief Constructor */
    UPnPsdk_API CSocketErr2(
        /*! [in] Inject the used \glos{depinj,di-service} object that is by
         * default the productive one but may also be a mocked object for Unit
         * Tests. */
        PSocketErr a_socket_errObj = std::make_shared<CSocketErrService>());

    /* \brief Destructor */
    UPnPsdk_API virtual ~CSocketErr2();

    /*! \brief Get the catched error number.
     * \details The error number is that from the operating system, for example
     * **errno** on Posix platforms or from WSAGetLastError() on Microroft
     * Windows. */
    UPnPsdk_API virtual operator const int&();

    /*! \brief Catches detailed error information.
     *  \details Because error information from the operating system is very
     * volatile this method should be called as early as possible after the
     * error was detected. */
    UPnPsdk_API virtual void catch_error();

    /*! \brief Gets a human readable error description from the operating
     * system that explains the catched error. */
    UPnPsdk_API virtual std::string get_error_str() const;

  private:
    PSocketErr m_socket_errObj;
};

} // namespace UPnPsdk

#endif // UPnPsdk_NETADAPTER2_HPP
