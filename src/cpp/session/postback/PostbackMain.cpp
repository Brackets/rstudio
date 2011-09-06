/*
 * PostbackMain.cpp
 *
 * Copyright (C) 2009-11 by RStudio, Inc.
 *
 * This program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include <iostream>

#include <core/Error.hpp>
#include <core/FilePath.hpp>
#include <core/Log.hpp>
#include <core/ProgramStatus.hpp>
#include <core/SafeConvert.hpp>

#include <core/system/System.hpp>

#include <core/http/Request.hpp>
#include <core/http/Response.hpp>
#include <core/http/LocalStreamBlockingClient.hpp>
#include <core/http/TcpIpBlockingClient.hpp>

#include <session/SessionConstants.hpp>
#include <session/SessionLocalStreams.hpp>

#include "PostbackOptions.hpp"

using namespace core ;
using namespace session::postback ;

int exitFailure(const Error& error)
{
   LOG_ERROR(error);
   return EXIT_FAILURE;
}

Error sendRequest(http::Request* pRequest, http::Response* pResponse)
{
   std::string portNum = system::getenv(kRSessionPortNumber);
   if (!portNum.empty())
   {
      pRequest->setHeader("X-Shared-Secret", system::getenv("RS_SHARED_SECRET"));
      return http::sendRequest("127.0.0.1", portNum, *pRequest, pResponse);
   }
   else
   {
      // determine stream path
      std::string userIdentity = system::getenv(kRStudioUserIdentity);
      FilePath streamPath = session::local_streams::streamPath(userIdentity);

      return http::sendRequest(streamPath, *pRequest, pResponse);
   }
}

int main(int argc, char * const argv[]) 
{
   try
   { 
      // initialize log
      initializeSystemLog("rpostback", core::system::kLogLevelWarning);

      // read program options 
      Options& options = session::postback::options();
      ProgramStatus status = options.read(argc, argv); 
      if ( status.exit() )
         return status.exitCode() ;
      
       // determine postback uri
      std::string uri = std::string(kLocalUriLocationPrefix kPostbackUriScope) + 
                        options.command();
      
      // build postback request
      http::Request request;
      request.setMethod("POST");
      request.setUri(uri);
      request.setHeader("Accept", "*/*");
      request.setHeader("Connection", "close");
      request.setBody(options.argument());

      // send it
      http::Response response;
      Error error = sendRequest(&request, &response);
      if (error)
         return exitFailure(error);

      std::string exitCode = response.headerValue(kPostbackExitCodeHeader);
      std::cout << response.body();
      return safe_convert::stringTo<int>(exitCode, EXIT_FAILURE);
   }
   CATCH_UNEXPECTED_EXCEPTION
   
   // if we got this far we had an unexpected exception
   return EXIT_FAILURE ;
}

