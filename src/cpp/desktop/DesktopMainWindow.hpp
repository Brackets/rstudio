/*
 * DesktopMainWindow.hpp
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

#ifndef DESKTOP_MAIN_WINDOW_HPP
#define DESKTOP_MAIN_WINDOW_HPP

#include <QProcess>
#include <QtGui>
#include "DesktopGwtCallback.hpp"
#include "DesktopMenuCallback.hpp"
#include "DesktopBrowserWindow.hpp"
#include "DesktopUpdateChecker.hpp"

namespace desktop {

class MainWindow : public BrowserWindow
{
   Q_OBJECT

public:
   MainWindow(QUrl url=QUrl());

public slots:
   void quit();
   void loadUrl(const QUrl& url);
   void setMenuBar(QMenuBar *pMenuBar);
   void invokeCommand(QString commandId);
   void manageCommand(QString cmdId, QAction* pAction);
   void openFileInRStudio(QString path);
   void checkForUpdates();
signals:
   void firstWorkbenchInitialized();
   void workbenchInitialized(QString projectFile);

protected slots:
   void onJavaScriptWindowObjectCleared();
   void onWorkbenchInitialized();
   void resetMargins();
   void fireWorkbenchInitialized();

protected:
   virtual void closeEvent(QCloseEvent*);

// private interface for SessionLauncher
private:
   friend class SessionLauncher;

   // allow SessionLauncher to give us a reference to the currently
   // active rsession process so that we can use it in closeEvent handling
   void setSessionProcess(QProcess* pSessionProcess);

   // allow SessionLauncher to collect switch requests from GwtCallback
   bool collectPendingSwitchToProjectRequest();

private:
   bool quitConfirmed_;
   MenuCallback menuCallback_;
   GwtCallback gwtCallback_;
   UpdateChecker updateChecker_;
   QProcess* pCurrentSessionProcess_;
   QString currentProjectFile_;
};

} // namespace desktop

#endif // DESKTOP_MAIN_WINDOW_HPP
