/*
 * Pam.hpp
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

#include <security/pam_appl.h>

#include <string>

#include <boost/utility.hpp>

namespace core {
   class ErrorLocation;
}

namespace server {
namespace pam {

// NOTE: Mac OS X supports PAM but ships with it in a locked-down config
// which will cause all passwords to be rejected. To make it work run:
//
//   sudo cp /etc/pam.d/ftpd /etc/pam.d/rstudio
//
// That configures PAM to send rstudio through the same authentication
// stack as ftpd uses, which is similar to us.


class PAM : boost::noncopyable
{
public:
   PAM(bool silent);
   virtual ~PAM();

   std::pair<int, const std::string> lastError();


   int login(const std::string& username,
             const std::string& password);

private:
    int defaultFlags_;
    pam_handle_t* pamh_;
    int status_;
};

int inappropriateUsage(const std::string& utility,
                       const core::ErrorLocation& location);

std::string readPassword(const std::string& username);


} // namespace pam
} // namespace server
