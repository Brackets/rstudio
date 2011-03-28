/*
 * Commands.java
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
package org.rstudio.studio.client.workbench.commands;

import org.rstudio.core.client.command.AppCommand;
import org.rstudio.core.client.command.CommandBundle;
import org.rstudio.core.client.command.MenuCallback;

public abstract class Commands extends CommandBundle
{
   public abstract void mainMenu(MenuCallback callback);

   // Workbench
   public abstract AppCommand setWorkingDir();
   
   // Source
   public abstract AppCommand newSourceDoc();
   public abstract AppCommand openSourceDoc();
   public abstract AppCommand reopenSourceDocWithEncoding();
   public abstract AppCommand saveSourceDoc();
   public abstract AppCommand saveSourceDocAs();
   public abstract AppCommand closeSourceDoc();
   public abstract AppCommand closeAllSourceDocs();
   public abstract AppCommand executeAllCode();
   public abstract AppCommand executeCode();
   public abstract AppCommand compilePDF();
   public abstract AppCommand publishPDF();
   public abstract AppCommand activateSource();
   public abstract AppCommand printSourceDoc();
   public abstract AppCommand popoutDoc();
   public abstract AppCommand findReplace();
   public abstract AppCommand extractFunction();
   public abstract AppCommand commentUncomment();

   // Console
   public abstract AppCommand consoleClear();
   public abstract AppCommand interruptR();
   public abstract AppCommand activateConsole();

   // Files
   public abstract AppCommand newTextFile();
   public abstract AppCommand newRSourceFile();
   public abstract AppCommand newFolder();
   public abstract AppCommand uploadFile();
   public abstract AppCommand copyFile();
   public abstract AppCommand moveFiles();
   public abstract AppCommand exportFiles();
   public abstract AppCommand renameFile();
   public abstract AppCommand deleteFiles();
   public abstract AppCommand refreshFiles();
   public abstract AppCommand quickOpen();
   public abstract AppCommand activateFiles();
   public abstract AppCommand syncWorkingDir();
   public abstract AppCommand showFolder();

   // View
   public abstract AppCommand switchToTab();
   public abstract AppCommand previousTab();
   public abstract AppCommand nextTab();
   public abstract AppCommand firstTab();
   public abstract AppCommand lastTab();

   // History
   public abstract AppCommand historySendToSource();
   public abstract AppCommand historySendToConsole();
   public abstract AppCommand historyDismissResults();
   public abstract AppCommand historyShowContext();
   public abstract AppCommand historyDismissContext();
   public abstract AppCommand activateHistory();

   // Workspace
   public abstract AppCommand clearWorkspace();
   public abstract AppCommand refreshWorkspace();
   public abstract AppCommand saveWorkspace();
   public abstract AppCommand saveWorkspaceAs();
   public abstract AppCommand openWorkspace();
   public abstract AppCommand importDatasetFromFile();
   public abstract AppCommand importDatasetFromURL();
   public abstract AppCommand importDatasetFromGoogleSpreadsheet();
   public abstract AppCommand activateWorkspace();
  
   // Plots
   public abstract AppCommand nextPlot();
   public abstract AppCommand previousPlot();
   public abstract AppCommand exportPlotAsImage();
   public abstract AppCommand printPlot();
   public abstract AppCommand zoomPlot();
   public abstract AppCommand clearPlots();
   public abstract AppCommand refreshPlot();
   public abstract AppCommand activatePlots();
   public abstract AppCommand showManipulator();

   // Packages
   public abstract AppCommand installPackage();
   public abstract AppCommand refreshPackages();
   public abstract AppCommand activatePackages();

   // Tools
   public abstract AppCommand showOptions();

   // Help
   public abstract AppCommand helpBack();
   public abstract AppCommand helpForward();
   public abstract AppCommand helpHome();
   public abstract AppCommand printHelp();
   public abstract AppCommand addToHelpFavorites();
   public abstract AppCommand clearHelpHistory();
   public abstract AppCommand helpPopout();
   public abstract AppCommand refreshHelp();
   public abstract AppCommand raiseException();
   public abstract AppCommand raiseException2();
   public abstract AppCommand activateHelp();
   public abstract AppCommand showAboutDialog();
   public abstract AppCommand checkForUpdates();
   public abstract AppCommand helpUsingRStudio();
   public abstract AppCommand helpKeyboardShortcuts();
   public abstract AppCommand showRequestLog();
   public abstract AppCommand logFocusedElement();

   // Application
   public abstract AppCommand quitSession();
   public abstract AppCommand updateCredentials();
   public abstract AppCommand showLogFiles();
   public abstract AppCommand rstudioSupport();
   public abstract AppCommand rstudioAgreement();
   public abstract AppCommand rstudioLicense();

   public abstract AppCommand showWarningBar();

   // Clipboard placeholders
   public abstract AppCommand undoDummy();
   public abstract AppCommand redoDummy();
   public abstract AppCommand cutDummy();
   public abstract AppCommand copyDummy();
   public abstract AppCommand pasteDummy();

   public abstract AppCommand mru0();
   public abstract AppCommand mru1();
   public abstract AppCommand mru2();
   public abstract AppCommand mru3();
   public abstract AppCommand mru4();
   public abstract AppCommand mru5();
   public abstract AppCommand mru6();
   public abstract AppCommand mru7();
   public abstract AppCommand mru8();
   public abstract AppCommand mru9();
   public abstract AppCommand clearRecentFiles();
   
   public abstract AppCommand workspaceMru0();
   public abstract AppCommand workspaceMru1();
   public abstract AppCommand workspaceMru2();
   public abstract AppCommand workspaceMru3();
   public abstract AppCommand workspaceMru4();
   public abstract AppCommand workspaceMru5();
   public abstract AppCommand workspaceMru6();
   public abstract AppCommand workspaceMru7();
   public abstract AppCommand workspaceMru8();
   public abstract AppCommand workspaceMru9();
   public abstract AppCommand clearRecentWorkspaceFiles();
}
