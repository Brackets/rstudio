/*
 * SessionSourceDatabaseSupervisor.cpp
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

#include "SessionSourceDatabaseSupervisor.hpp"

#include <vector>

#include <boost/foreach.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include <core/Error.hpp>
#include <core/FilePath.hpp>
#include <core/FileSerializer.hpp>
#include <core/BoostErrors.hpp>

#include <core/system/System.hpp>

#include <session/SessionModuleContext.hpp>
#include "session/SessionSourceDatabase.hpp"

using namespace core;

namespace session {
namespace source_database {
namespace supervisor {

namespace {

const char * const kSessionDirPrefix = "session-";

FilePath oldSourceDatabaseRoot()
{
   return
      module_context::scopedScratchPath().complete("source_database");
}

FilePath sourceDatabaseRoot()
{
   return
      module_context::scopedScratchPath().complete("source_database_v2");
}

FilePath persistentTitledDir()
{
   return sourceDatabaseRoot().complete("persistent/titled");
}


FilePath persistentUntitledDir()
{
   return sourceDatabaseRoot().complete("persistent/untitled");
}

FilePath sessionLockFilePath(const FilePath& sessionDir)
{
   return sessionDir.complete("lock_file");
}

// session dir name and lock (initialized by attachToSourceDatabase)
FilePath s_sessionDir;
boost::interprocess::file_lock s_sessionDirLock;


bool isLocked(const FilePath& sessionDir)
{
   using namespace boost::interprocess;

   // if the lock file doesn't exist then it's not locked
   FilePath lockFilePath = sessionLockFilePath(sessionDir);
   if (!lockFilePath.exists())
      return false;

   // check if it is locked
   try
   {
      file_lock lock(lockFilePath.absolutePath().c_str());

      if (lock.try_lock())
      {
         lock.unlock();
         return false;
      }
      else
      {
         return true;
      }
   }
   catch(boost::interprocess::interprocess_exception& e)
   {
      Error error(boost::interprocess::ec_from_exception(e), ERROR_LOCATION);
      error.addProperty("session-dir", sessionDir);
      LOG_ERROR(error);
      return false;
   }
}

Error acquireLock(const FilePath& sessionDir)
{
   using namespace boost::interprocess;

   // make sure the lock file exists
   FilePath lockFilePath = sessionLockFilePath(sessionDir);
   if (!lockFilePath.exists())
   {
      Error error = core::writeStringToFile(lockFilePath, "");
      if (error)
         return error;
   }

   // try to acquire the session dir lock
   try
   {
      file_lock lock(lockFilePath.absolutePath().c_str());

      if (lock.try_lock())
      {
         // set module level statics
         s_sessionDir = sessionDir;
         s_sessionDirLock.swap(lock);

         return Success();
      }
      else
      {
         return systemError(boost::system::errc::no_lock_available,
                            ERROR_LOCATION);
      }
   }
   catch(interprocess_exception& e)
   {
      Error error(ec_from_exception(e), ERROR_LOCATION);
      error.addProperty("session-dir", sessionDir);
      return error;
   }

   return Success();
}

Error releaseLock()
{
   using namespace boost::interprocess;

   // make sure the lock file exists
   FilePath lockFilePath = sessionLockFilePath(s_sessionDir);
   if (!lockFilePath.exists())
   {
      return systemError(boost::system::errc::no_lock_available,
                         ERROR_LOCATION);
   }

   // always cleanup the lock file on exit
   BOOST_SCOPE_EXIT( (&lockFilePath) )
   {
      Error error = lockFilePath.remove();
      if (error)
         LOG_ERROR(error);
   }
   BOOST_SCOPE_EXIT_END

   // try to unlock it
   try
   {
      s_sessionDirLock.unlock();
      return Success();
   }
   catch(interprocess_exception& e)
   {
      Error error(ec_from_exception(e), ERROR_LOCATION);
      error.addProperty("session-dir", s_sessionDir);
      return error;
   }

   return Success();
}

FilePath generateSessionDirPath()
{
   return sourceDatabaseRoot().complete(kSessionDirPrefix +
                                        core::system::generateUuid());
}

bool isNotSessionDir(const FilePath& filePath)
{
   return !filePath.isDirectory() || !boost::algorithm::starts_with(
                                                filePath.absolutePath(),
                                                kSessionDirPrefix);
}

Error enumerateSessionDirs(std::vector<FilePath>* pSessionDirs)
{
   // get the directories
   Error error = sourceDatabaseRoot().children(pSessionDirs);
   if (error)
      return error;

   // clean out non session dirs
   pSessionDirs->erase(std::remove_if(pSessionDirs->begin(),
                                      pSessionDirs->end(),
                                      isNotSessionDir));

   // return success
   return Success();
}

void attemptToMoveSourceDbFiles(const FilePath& fromPath,
                                const FilePath& toPath)
{
   // enumerate the from path
   std::vector<FilePath> children;
   Error error = fromPath.children(&children);
   if (error)
      LOG_ERROR(error);

   // move the files
   BOOST_FOREACH(const FilePath& filePath, children)
   {
      Error error = filePath.move(toPath.complete(filePath.filename()));
      if (error)
         LOG_ERROR(error);
   }
}

// NOTE: the supervisor needs to return a session dir in order for the process
// to start. therefore, in the createSessionDir family of functions below
// once we successfully create and lock the session dir other errors (such
// as trying to move files into the session dir) are simply logged

Error createSessionDir(FilePath* pSessionDir)
{
   *pSessionDir = generateSessionDirPath();

   Error error = pSessionDir->ensureDirectory();
   if (error)
      return error;

   return acquireLock(*pSessionDir);
}

Error createSessionDirFromOldSourceDatabase(FilePath* pSessionDir)
{
   // move the old source database into a new dir
   *pSessionDir = generateSessionDirPath();
   Error error = oldSourceDatabaseRoot().move(*pSessionDir);
   if (error)
      LOG_ERROR(error);

   // if that failed we might still need to call ensureDirectory
   error = pSessionDir->ensureDirectory();
   if (error)
      return error;

   // acquire the lock
   return acquireLock(*pSessionDir);
}

Error createSessionDirFromPersistent(FilePath* pSessionDir)
{
   // create new session dir
   Error error = createSessionDir(pSessionDir);
   if (error)
      return error;

   // move persistent titled files
   if (persistentTitledDir().exists())
      attemptToMoveSourceDbFiles(persistentTitledDir(), *pSessionDir);

   // move persistent untitled files
   if (persistentUntitledDir().exists())
      attemptToMoveSourceDbFiles(persistentUntitledDir(), *pSessionDir);

   // return success
   return Success();
}

bool reclaimOrphanedSession(const std::vector<FilePath>& sessionDirs,
                            FilePath* pSessionDir)
{
   BOOST_FOREACH(const FilePath& sessionDir, sessionDirs)
   {
      if (!isLocked(sessionDir))
      {
         Error error = acquireLock(sessionDir);
         if (!error)
         {
            *pSessionDir = sessionDir;
            return true;
         }
         else
         {
            LOG_ERROR(error);
         }
      }
   }

   return false;
}

bool noLockedSessionsExist(const std::vector<FilePath>& sessionDirs)
{
   BOOST_FOREACH(const FilePath& sessionDir, sessionDirs)
   {
      if (isLocked(sessionDir))
         return false;
   }

   return true;
}

} // anonymous namespace


Error attachToSourceDatabase(FilePath* pSessionDir)
{  
   // check whether we will need to migrate -- ensure we do this only
   // one time so that if for whatever reason we can't migrate the
   // old source database we don't get stuck trying to do it every
   // time we start up
   bool needToMigrate = !sourceDatabaseRoot().exists() &&
                        oldSourceDatabaseRoot().exists();

   // ensure the root path exists
   Error error = sourceDatabaseRoot().ensureDirectory();
   if (error)
      return error;

   // check for existing sessions (use this to decide how to startup below)
   std::vector<FilePath> sessionDirs;
   error = enumerateSessionDirs(&sessionDirs);
   if (error)
      LOG_ERROR(error);

   // attempt to migrate if necessary
   if (needToMigrate)
      return createSessionDirFromOldSourceDatabase(pSessionDir);

   // if there is an orphan (crash) then reclaim it
   else if (reclaimOrphanedSession(sessionDirs, pSessionDir))
      return Success();

   // if there are no existing locked sessions then create from persistent
   else if (noLockedSessionsExist(sessionDirs))
      return createSessionDirFromPersistent(pSessionDir);

   // otherwise startup with a brand new session dir
   else
      return createSessionDir(pSessionDir);
}

Error detachFromSourceDatabase()
{
   // list all current source docs
   std::vector<boost::shared_ptr<SourceDocument> > sourceDocs;
   Error error = source_database::list(&sourceDocs);
   if (error)
      return error;

   // get references to persistent subdirs
   FilePath titledDir = persistentTitledDir();
   FilePath untitledDir = persistentUntitledDir();

   // first blow away the existing persistent titled dir
   error = titledDir.removeIfExists();
   if (error)
      LOG_ERROR(error);

   // ensure both directories exist -- if they don't it is a fatal error
   error = titledDir.ensureDirectory();
   if (error)
      return error;
   error = untitledDir.ensureDirectory();
   if (error)
      return error;

   // now write the source database entries to the appropriate places
   BOOST_FOREACH(boost::shared_ptr<SourceDocument> pDoc, sourceDocs)
   {
      if (pDoc->isUntitled())
      {
         error = pDoc->writeToFile(untitledDir.complete(pDoc->id()));
         if (error)
            LOG_ERROR(error);
      }
      else
      {
         error = pDoc->writeToFile(titledDir.complete(pDoc->id()));
         if (error)
            LOG_ERROR(error);
      }
   }

   // give up our lock
   error = releaseLock();
   if (error)
      LOG_ERROR(error);

   // remove the session directory
   return s_sessionDir.remove();
}


} // namespace supervisor
} // namespace source_database
} // namespace session


