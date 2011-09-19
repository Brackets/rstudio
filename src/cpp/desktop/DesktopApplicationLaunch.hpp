/*
 * DesktopApplicationLaunch.hpp
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

#ifndef DESKTOPAPPLICATIONLAUNCH_HPP
#define DESKTOPAPPLICATIONLAUNCH_HPP

#include <QObject>
#include <QWidget>
#include <QApplication>

#include <boost/scoped_ptr.hpp>

namespace core {
   class FilePath;
}

namespace desktop {

class MainWindow;

class ApplicationLaunch : public QWidget
{
    Q_OBJECT
public:
   static void init(QString appname,
                    int& argc,
                    char* argv[],
                    boost::scoped_ptr<QApplication>* ppApp,
                    boost::scoped_ptr<ApplicationLaunch>* ppAppLaunch);

   void initInstanceTracking(const QString &appId = QString());

   void setActivationWindow(MainWindow* pWindow);

   void activateWindow();

   QString startupOpenFileRequest() const;

public:
   static QString instanceIdFromFilename(const QString& filename);

protected:
    ApplicationLaunch();
#ifdef _WIN32
    bool winEvent(MSG *message, long *result);
#endif

signals:
    void openFileRequest(QString filename);

public slots:
    bool sendMessage(QString filename);
    void onWorkbenchInitialized(QString projectFile);

private:
    MainWindow* pMainWindow_;
};

} // namespace desktop

#endif // DESKTOPAPPLICATIONLAUNCH_HPP
