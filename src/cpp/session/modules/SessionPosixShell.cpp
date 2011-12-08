/*
 * SessionPosixShell.cpp
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

#include "SessionPosixShell.hpp"

#include <boost/shared_ptr.hpp>

#include <core/Log.hpp>
#include <core/Error.hpp>
#include <core/Exec.hpp>
#include <core/system/Process.hpp>
#include <core/system/Environment.hpp>
#include <core/system/Crypto.hpp>
#include <core/system/ShellUtils.hpp>

#include <session/SessionModuleContext.hpp>

#include "SessionCrypto.hpp"
#include "SessionVCS.hpp"

#include "SessionSVN.hpp"
#include "SessionGit.hpp"

using namespace core ;

namespace session {
namespace modules { 
namespace posix_shell {

namespace {

std::string aliasedCurrentPath()
{
   return module_context::createAliasedPath(module_context::safeCurrentPath());
}

class PosixShell;
boost::shared_ptr<PosixShell> s_pActiveShell;

class PosixShell : boost::noncopyable
{
public:
   static Error create(int width,
                       int maxLines,
                       boost::shared_ptr<PosixShell>* ppPosixShell)
   {
      // create posix shell and bind callbacks
      boost::shared_ptr<PosixShell> pShell(new PosixShell(maxLines));
      core::system::ProcessCallbacks cb;
      cb.onContinue = boost::bind(&PosixShell::onContinue, pShell, _1);
      cb.onStdout = boost::bind(&PosixShell::onStdout, pShell, _2);
      cb.onExit = boost::bind(&PosixShell::onExit, pShell, _1);

      // configure environment for shell
      core::system::Options shellEnv;
      core::system::environment(&shellEnv);

      // terminal and prompt
      core::system::setenv(&shellEnv, "TERM", "dumb");
      core::system::setenv(
            &shellEnv,
            "PS1",
             (aliasedCurrentPath().length() > 30) ? "\\W$ " : "\\w$ ");


      // add custom git path if necessary
      std::string gitBinDir = git::nonPathGitBinDir();
      if (!gitBinDir.empty())
         core::system::addToPath(&shellEnv, gitBinDir);

      // add custom svn path if necessary
      std::string svnBinDir = svn::nonPathSvnBinDir();
      if (!svnBinDir.empty())
         core::system::addToPath(&shellEnv, svnBinDir);

      // configure bash command
#ifdef _WIN32
      core::shell_utils::ShellCommand bashCommand("cmd.exe");
      bashCommand << "/Q";
#else
      core::shell_utils::ShellCommand bashCommand("/bin/bash");
      bashCommand << "--norc";
#endif

      // set options and run process
      core::system::ProcessOptions options;
      options.workingDir = module_context::shellWorkingDirectory();
#ifndef _WIN32
      options.pseudoterminal = core::system::Pseudoterminal(width, 1);
#endif
      options.environment = shellEnv;
      Error error = module_context::processSupervisor().runCommand(bashCommand,
                                                                   options,
                                                                   cb);
      if (!error)
      {
         *ppPosixShell = pShell;
         return Success();
      }
      else
      {
         return error;
      }

   }

private:
   PosixShell(int maxLines)
      : maxLines_(maxLines), interrupt_(false), terminate_(false)
   {
   }

public:
   virtual ~PosixShell() {}

   void enqueueInput(const std::string &input)
   {
      inputQueue_.append(input);
   }

   void interrupt()
   {
      interrupt_ = true;
   }

   void terminate()
   {
      terminate_ = true;
   }


private:
   bool onContinue(core::system::ProcessOperations& ops)
   {
      if (terminate_)
         return false;

      if (!inputQueue_.empty())
      {
         Error error = ops.writeToStdin(inputQueue_, false);
         if (error)
            LOG_ERROR(error);

         inputQueue_.clear();
      }

      if (interrupt_)
      {
         Error error = ops.ptyInterrupt();
         if (error)
            LOG_ERROR(error);

         interrupt_ = false;
      }

      return true;
   }

   void onStdout(const std::string& output)
   {
      // if we are the active shell then emit output event
      if (s_pActiveShell.get() == this)
      {
         // If there's more output than the client can even show, then
         // truncate it to the amount that the client can show. Too much
         // output can overwhelm the client, making it unresponsive.
         std::string trimmedOutput = output;
         string_utils::trimLeadingLines(maxLines_, &trimmedOutput);

         // fire the event
         ClientEvent event(client_events::kPosixShellOutput, trimmedOutput);
         module_context::enqueClientEvent(event);
      }
   }

   void onExit(int exitCode)
   {
      // if we are the active shell then notify the client that we
      // are being terminated (if we aren't the active shell then
      // the client isn't (logically) listenting for exit
      if (s_pActiveShell.get() == this)
      {
         s_pActiveShell.reset();

         // notify client
         json::Object eventData;
         eventData["exit_code"] = exitCode;
         ClientEvent event(client_events::kPosixShellExit, eventData);
         module_context::enqueClientEvent(event);
      }

      // always enque a vcs refresh event after the posix shell exits
      // (in case manipulations inside the shell caused underlying
      // vcs state changes)
      source_control::enqueueRefreshEvent();
   }

private:
   int maxLines_;
   std::string inputQueue_;
   bool interrupt_;
   bool terminate_;
};


Error startPosixShell(const json::JsonRpcRequest& request,
                      json::JsonRpcResponse* pResponse)
{
   // get parameters
   int width, maxLines;
   Error error = json::readParams(request.params, &width, &maxLines);
   if (error)
      return error;

   // terminate any existing shell
   if (s_pActiveShell)
   {
      s_pActiveShell->terminate();
      s_pActiveShell.reset();
   }

#ifndef _WIN32
   // set public key info
   pResponse->setResult(crypto::publicKeyInfoJson());
#endif

   // start a new shell
   return PosixShell::create(width, maxLines, &s_pActiveShell);
}

Error interruptPosixShell(const json::JsonRpcRequest& request,
                          json::JsonRpcResponse* pResponse)
{
   // validate we have an active shell
   if (!s_pActiveShell)
      return Error(json::errc::MethodUnexpected, ERROR_LOCATION);

   s_pActiveShell->interrupt();

   return Success();
}

Error sendInputToPosixShell(const json::JsonRpcRequest& request,
                            json::JsonRpcResponse* pResponse)
{
   // read param
   std::string input;
   Error error = json::readParam(request.params, 0, &input);
   if (error)
      return error;

   // validate we have an active shell
   if (!s_pActiveShell)
      return Error(json::errc::MethodUnexpected, ERROR_LOCATION);

   // decrypt input
   std::string decryptedInput;
#ifndef _WIN32
   error = core::system::crypto::rsaPrivateDecrypt(input, &decryptedInput);
   if (error)
      return error;
#else
   decryptedInput = input;
#endif

   // send input
   s_pActiveShell->enqueueInput(decryptedInput);

   return Success();
}

Error terminatePosixShell(const json::JsonRpcRequest& request,
                          json::JsonRpcResponse* pResponse)
{
   // validate we have an active shell
   if (!s_pActiveShell)
      return Error(json::errc::MethodUnexpected, ERROR_LOCATION);

   s_pActiveShell->terminate();

   return Success();
}


void onClientInit()
{
   // terminate any active shell we have on client init
   if (s_pActiveShell)
   {
      s_pActiveShell->terminate();
      s_pActiveShell.reset();
   }
}


} // anonymous namespace


Error initialize()
{
   using namespace module_context;
   module_context::events().onClientInit.connect(boost::bind(onClientInit));

   ExecBlock initBlock ;
   initBlock.addFunctions()
      (bind(registerRpcMethod, "start_posix_shell", startPosixShell))
      (bind(registerRpcMethod, "interrupt_posix_shell", interruptPosixShell))
      (bind(registerRpcMethod, "send_input_to_posix_shell", sendInputToPosixShell))
      (bind(registerRpcMethod, "terminate_posix_shell", terminatePosixShell));
   return initBlock.execute();

}
   
   
} // namespace posix_shell
} // namespace modules
} // namesapce session

