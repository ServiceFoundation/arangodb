////////////////////////////////////////////////////////////////////////////////
/// @brief handler factory
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2013 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2009-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_HTTP_SERVER_HTTP_HANDLER_FACTORY_H
#define TRIAGENS_HTTP_SERVER_HTTP_HANDLER_FACTORY_H 1

#include "Basics/Common.h"

#include "Basics/Mutex.h"
#include "Basics/ReadWriteLock.h"

// -----------------------------------------------------------------------------
// --SECTION--                                              forward declarations
// -----------------------------------------------------------------------------

namespace triagens {
  namespace rest {
    struct ConnectionInfo;
    class HttpHandler;
    class HttpRequest;
    class HttpResponse;
    class MaintenanceCallback;

// -----------------------------------------------------------------------------
// --SECTION--                                          class HttpHandlerFactory
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup HttpServer
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief handler factory
////////////////////////////////////////////////////////////////////////////////

    class HttpHandlerFactory {

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                      public types
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup HttpServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief handler
////////////////////////////////////////////////////////////////////////////////

        typedef HttpHandler GeneralHandler;

////////////////////////////////////////////////////////////////////////////////
/// @brief request
////////////////////////////////////////////////////////////////////////////////

        typedef HttpRequest GeneralRequest;

////////////////////////////////////////////////////////////////////////////////
/// @brief response
////////////////////////////////////////////////////////////////////////////////

        typedef HttpResponse GeneralResponse;

////////////////////////////////////////////////////////////////////////////////
/// @brief handler creator
////////////////////////////////////////////////////////////////////////////////

        typedef HttpHandler* (*create_fptr) (HttpRequest*, void*);

////////////////////////////////////////////////////////////////////////////////
/// @brief authentication handler
////////////////////////////////////////////////////////////////////////////////

        typedef bool (*auth_fptr) (char const*, char const*);

////////////////////////////////////////////////////////////////////////////////
/// @brief context handler
////////////////////////////////////////////////////////////////////////////////

        typedef bool (*context_fptr) (HttpRequest*);

////////////////////////////////////////////////////////////////////////////////
/// @brief authentication cache invalidation handler
////////////////////////////////////////////////////////////////////////////////
        
        typedef bool (*flush_fptr) ();

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup HttpServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructs a new handler factory
////////////////////////////////////////////////////////////////////////////////

        HttpHandlerFactory (std::string const&, 
                            auth_fptr, 
                            flush_fptr, 
                            context_fptr);

////////////////////////////////////////////////////////////////////////////////
/// @brief clones a handler factory
////////////////////////////////////////////////////////////////////////////////

        HttpHandlerFactory (HttpHandlerFactory const&);

////////////////////////////////////////////////////////////////////////////////
/// @brief copies a handler factory
////////////////////////////////////////////////////////////////////////////////

        HttpHandlerFactory& operator= (HttpHandlerFactory const&);

////////////////////////////////////////////////////////////////////////////////
/// @brief destructs a handler factory
////////////////////////////////////////////////////////////////////////////////

        virtual ~HttpHandlerFactory ();

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup HttpServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief require authentication
////////////////////////////////////////////////////////////////////////////////

        void setRequireAuthentication (bool);

////////////////////////////////////////////////////////////////////////////////
/// @brief flush the authentication cache
////////////////////////////////////////////////////////////////////////////////

        void flushAuthentication ();

////////////////////////////////////////////////////////////////////////////////
/// @brief returns header and body size restrictions
////////////////////////////////////////////////////////////////////////////////

        virtual pair<size_t, size_t> sizeRestrictions () const;

////////////////////////////////////////////////////////////////////////////////
/// @brief authenticates a new request, wrapper method
////////////////////////////////////////////////////////////////////////////////

        virtual bool authenticateRequest (HttpRequest * request);
        
////////////////////////////////////////////////////////////////////////////////
/// @brief set request context, wrapper method
////////////////////////////////////////////////////////////////////////////////

        virtual bool setRequestContext (HttpRequest * request);


////////////////////////////////////////////////////////////////////////////////
/// @brief returns the authentication realm
////////////////////////////////////////////////////////////////////////////////

        virtual std::string const& authenticationRealm (HttpRequest * request) const;

        
////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new request
////////////////////////////////////////////////////////////////////////////////

        virtual HttpRequest* createRequest (ConnectionInfo const& info, 
                                      char const* ptr, size_t length);

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a new handler
////////////////////////////////////////////////////////////////////////////////

        virtual HttpHandler* createHandler (HttpRequest*);

////////////////////////////////////////////////////////////////////////////////
/// @brief adds a maintenance handler
///
/// Note the maintenance callback is deleted, after it is fired.
////////////////////////////////////////////////////////////////////////////////

        void addMaintenanceCallback (MaintenanceCallback*);

////////////////////////////////////////////////////////////////////////////////
/// @brief adds a path and constructor to the factory
////////////////////////////////////////////////////////////////////////////////

        void addHandler (string const& path, create_fptr, void* data = 0);

////////////////////////////////////////////////////////////////////////////////
/// @brief adds a prefix path and constructor to the factory
////////////////////////////////////////////////////////////////////////////////

        void addPrefixHandler (string const& path, create_fptr, void* data = 0);

////////////////////////////////////////////////////////////////////////////////
/// @brief adds a path and constructor to the factory
////////////////////////////////////////////////////////////////////////////////

        void addNotFoundHandler (create_fptr);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup HttpServer
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief authenticates a new request, worker method
////////////////////////////////////////////////////////////////////////////////

        virtual bool authenticate (HttpRequest * request);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup HttpServer
/// @{
////////////////////////////////////////////////////////////////////////////////

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief authentication realm
////////////////////////////////////////////////////////////////////////////////

        string _authenticationRealm;

////////////////////////////////////////////////////////////////////////////////
/// @brief authentication callback
////////////////////////////////////////////////////////////////////////////////

        auth_fptr _checkAuthentication;

////////////////////////////////////////////////////////////////////////////////
/// @brief set context callback
////////////////////////////////////////////////////////////////////////////////

        context_fptr _setContext;

////////////////////////////////////////////////////////////////////////////////
/// @brief authentication cache flush callback
////////////////////////////////////////////////////////////////////////////////

        flush_fptr _flushAuthentication;

////////////////////////////////////////////////////////////////////////////////
/// @brief require authentication
////////////////////////////////////////////////////////////////////////////////

        bool _requireAuthentication;

////////////////////////////////////////////////////////////////////////////////
/// @brief authentication lock for cache
////////////////////////////////////////////////////////////////////////////////

        basics::ReadWriteLock _authLock;

////////////////////////////////////////////////////////////////////////////////
/// @brief authentication cache
////////////////////////////////////////////////////////////////////////////////

        std::map<std::string, std::string> _authCache;

////////////////////////////////////////////////////////////////////////////////
/// @brief list of constructors
////////////////////////////////////////////////////////////////////////////////

        map<string, create_fptr> _constructors;

////////////////////////////////////////////////////////////////////////////////
/// @brief list of data pointers for constructors
////////////////////////////////////////////////////////////////////////////////

        map<string, void*> _datas;

////////////////////////////////////////////////////////////////////////////////
/// @brief list of prefix handlers
////////////////////////////////////////////////////////////////////////////////

        vector<string> _prefixes;

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor for a not-found handler
////////////////////////////////////////////////////////////////////////////////

        create_fptr _notFound;

////////////////////////////////////////////////////////////////////////////////
/// @brief list of maintenance callbacks
////////////////////////////////////////////////////////////////////////////////

        vector<MaintenanceCallback*> _maintenanceCallbacks;
    };
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
