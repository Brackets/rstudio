/*
 * ConnectionRetryProfile.hpp
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

#ifndef CORE_HTTP_CONNECTION_RETRY_PROFILE_HPP
#define CORE_HTTP_CONNECTION_RETRY_PROFILE_HPP

#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace core {
namespace http {

struct ConnectionRetryProfile
{
   ConnectionRetryProfile()
      : maxWait(boost::posix_time::not_a_date_time),
        retryInterval(boost::posix_time::not_a_date_time)
   {
   }

   ConnectionRetryProfile(
         const boost::posix_time::time_duration& maxWait,
         const boost::posix_time::time_duration& retryInterval,
         const boost::function<void()>& recoveryFunction =
                                             boost::function<void()>())
      : maxWait(maxWait),
        retryInterval(retryInterval),
        recoveryFunction(recoveryFunction)
   {
   }

   bool empty() const { return maxWait.is_not_a_date_time(); }

   boost::posix_time::time_duration maxWait;
   boost::posix_time::time_duration retryInterval;
   boost::function<void()> recoveryFunction;
};


} // namespace http
} // namespace core

#endif // CORE_HTTP_CONNECTION_RETRY_PROFILE_HPP
