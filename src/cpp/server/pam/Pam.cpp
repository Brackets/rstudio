/*
 * Pam.cpp
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
 *
 * This program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include "Pam.hpp"

#include <iostream>

#include <boost/utility.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>

#include <core/Log.hpp>
#include <core/Error.hpp>
#include <core/system/System.hpp>
#include <core/system/PosixUser.hpp>

using namespace core ;

namespace server {
namespace pam {

namespace {

class MemoryPool : boost::noncopyable {

   typedef boost::function<void*(size_t)> Alloc;
   typedef boost::function<void(void*)> Free;

public:
   MemoryPool(Alloc allocFunc = ::malloc, Free freeFunc = ::free) :
      alloc_(allocFunc),
      free_(freeFunc)
   {}

   ~MemoryPool()
   {
      try
      {
         for (size_t i = 0; i < buffers_.size(); i++)
         {
            free_(buffers_.at(i));
         }
      }
      catch(...)
      {
      }
   }

   void* alloc(size_t size)
   {
      void* p = alloc_(size);
      if (p)
         buffers_.push_back(p);
      return p;
   }

   void relinquishOwnership()
   {
      buffers_.clear();
   }

private:
   std::vector<void*> buffers_;
   Alloc alloc_;
   Free free_;
};

int conv(int num_msg,
         const struct pam_message** msg,
         struct pam_response** resp,
         void * appdata_ptr)
{
   try
   {
      MemoryPool pool;

      // resp will be freed by the caller
      *resp = static_cast<pam_response*>(pool.alloc(sizeof(pam_response) * num_msg));
      if (!*resp)
         return PAM_BUF_ERR;

      ::memset(*resp, 0, sizeof(pam_response) * num_msg);

      for (int i = 0; i < num_msg; i++)
      {
         const pam_message* input = msg[i];
         std::string msgText = input->msg;

         switch (input->msg_style)
         {
         case PAM_PROMPT_ECHO_OFF:
         {
            boost::regex passwordRegex("\\bpassword:\\s*$",
                                       boost::regex_constants::icase);
            boost::smatch match;
            if (regex_search(msgText, match, passwordRegex))
            {
               resp[i]->resp_retcode = 0;
               char* password = static_cast<char*>(appdata_ptr);
               // respBuf will be freed by the caller
               char* respBuf = static_cast<char*>(pool.alloc(strlen(password) + 1));
               resp[i]->resp = ::strcpy(respBuf, password);
            }
            else
               return PAM_CONV_ERR;
            break;
         }
         case PAM_TEXT_INFO:
         {
            resp[i]->resp_retcode = 0;
            char* respBuf = static_cast<char*>(pool.alloc(1));
            respBuf[0] = '\0';
            resp[i]->resp = respBuf;
            break;
         }
         case PAM_PROMPT_ECHO_ON:
         case PAM_ERROR_MSG:
         default:
            return PAM_CONV_ERR;
         }
      }

      // The caller will free all the memory we allocated
      pool.relinquishOwnership();

      return PAM_SUCCESS;
   }
   CATCH_UNEXPECTED_EXCEPTION
   return PAM_CONV_ERR;
}

} // anonymous namespace


PAM::PAM(bool silent) :
      defaultFlags_(silent ? PAM_SILENT : 0),
      pamh_(NULL),
      status_(PAM_SUCCESS)
{
}

PAM::~PAM()
{
   if (pamh_)
   {
      ::pam_end(pamh_, status_ | (defaultFlags_ & PAM_SILENT));
   }
}

std::pair<int, const std::string> PAM::lastError()
{
   return std::pair<int, const std::string>(
         status_,
         std::string(::pam_strerror(pamh_, status_)));
}

int PAM::login(const std::string& username,
                   const std::string& password)
{
   struct pam_conv myConv;
   myConv.conv = conv;
   myConv.appdata_ptr = const_cast<void*>(static_cast<const void*>(password.c_str()));
   status_ = ::pam_start("rstudio",
                         username.c_str(),
                         &myConv,
                         &pamh_);
   if (status_ != PAM_SUCCESS)
   {
      LOG_ERROR_MESSAGE("pam_start failed: " + lastError().second);
      return status_;
   }

   status_ = ::pam_authenticate(pamh_, defaultFlags_);
   if (status_ != PAM_SUCCESS)
   {
      if (status_ != PAM_AUTH_ERR)
         LOG_ERROR_MESSAGE("pam_authenticate failed: " + lastError().second);
      return status_;
   }

   status_ = ::pam_acct_mgmt(pamh_, defaultFlags_);
   if (status_ != PAM_SUCCESS)
   {
      LOG_ERROR_MESSAGE("pam_acct_mgmt failed: " + lastError().second);
      return status_;
   }

   return PAM_SUCCESS;
}

int inappropriateUsage(const std::string& utility,
                       const core::ErrorLocation& location)
{
   // log warning
   boost::format fmt("Inappropriate use of %1% helper binary (user=%2%)");
   std::string msg = boost::str(
               fmt % utility
                   % core::system::user::currentUserIdentity().userId);
   core::log::logWarningMessage(msg, location);

   // additional notification to the user
   std::cerr << "\nThis binary is not designed for running this way\n"
                "-- the system administrator has been informed\n\n";

   // cause further annoyance
   ::sleep(10);

   return EXIT_FAILURE;
}

std::string readPassword(const std::string& username)
{
    // read password (up to 200 chars in length)
    std::string password;
    const int MAXPASS = 200;
    int ch = 0;
    int count = 0;
    while((ch = ::fgetc(stdin)) != EOF)
    {
       if (++count <= MAXPASS)
       {
          password.push_back(static_cast<char>(ch));
       }
       else
       {
          LOG_WARNING_MESSAGE("Password exceeded maximum length for "
                              "user " + username);
          return std::string();
       }
    }

    return password;
}

} // namespace pam
} // namespace server
