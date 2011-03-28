/*
 * Workspace.java
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
package org.rstudio.studio.client.workbench.views.workspace;


import com.google.gwt.user.client.Command;
import com.google.inject.Inject;
import org.rstudio.core.client.StringUtil;
import org.rstudio.core.client.command.CommandBinder;
import org.rstudio.core.client.command.Handler;
import org.rstudio.core.client.files.FileSystemItem;
import org.rstudio.core.client.files.FilenameTransform;
import org.rstudio.core.client.jsonrpc.RpcObjectList;
import org.rstudio.core.client.widget.*;
import org.rstudio.studio.client.application.events.EventBus;
import org.rstudio.studio.client.common.FileDialogs;
import org.rstudio.studio.client.common.GlobalDisplay;
import org.rstudio.studio.client.common.filetypes.events.OpenDataFileEvent;
import org.rstudio.studio.client.common.filetypes.events.OpenDataFileHandler;
import org.rstudio.studio.client.server.ServerError;
import org.rstudio.studio.client.server.ServerRequestCallback;
import org.rstudio.studio.client.server.Void;
import org.rstudio.studio.client.server.VoidServerRequestCallback;
import org.rstudio.studio.client.workbench.WorkbenchView;
import org.rstudio.studio.client.workbench.commands.Commands;
import org.rstudio.studio.client.workbench.model.RemoteFileSystemContext;
import org.rstudio.studio.client.workbench.views.BasePresenter;
import org.rstudio.studio.client.workbench.views.console.events.SendToConsoleEvent;
import org.rstudio.studio.client.workbench.views.workspace.dataimport.ImportFileSettings;
import org.rstudio.studio.client.workbench.views.workspace.dataimport.ImportFileSettingsDialog;
import org.rstudio.studio.client.workbench.views.workspace.dataimport.ImportGoogleSpreadsheetDialog;
import org.rstudio.studio.client.workbench.views.workspace.events.*;
import org.rstudio.studio.client.workbench.views.workspace.model.DownloadInfo;
import org.rstudio.studio.client.workbench.views.workspace.model.GoogleSpreadsheetImportSpec;
import org.rstudio.studio.client.workbench.views.workspace.model.WorkspaceObjectInfo;
import org.rstudio.studio.client.workbench.views.workspace.model.WorkspaceServerOperations;
import org.rstudio.studio.client.workbench.views.workspace.table.WorkspaceObjectTable;

import java.util.ArrayList;
import java.util.HashMap;

public class Workspace
      extends BasePresenter
   implements WorkspaceObjectAssignedHandler,
              WorkspaceObjectRemovedHandler,
              Widgetable,
              WorkspaceRefreshHandler,
              OpenDataFileHandler,
              WorkspaceObjectTable.Observer
{
   public interface Binder extends CommandBinder<Commands, Workspace> {}

   public interface Display extends WorkbenchView
   {
      WorkspaceObjectTable getWorkspaceObjectTable();
   }

   private class RequestCallback extends ServerRequestCallback<Void>
   {
      public RequestCallback(Command dismissProgress)
      {
         dismissProgress_ = dismissProgress;
      }

      @Override
      public void onError(ServerError error)
      {
         dismissProgress_.execute();
         globalDisplay_.showErrorMessage("Error", error.getUserMessage());
      }

      @Override
      public void onResponseReceived(Void response)
      {
         dismissProgress_.execute();
      }

      private final Command dismissProgress_;
   }

   @Inject
   public Workspace(Workspace.Display view,
                    EventBus eventBus,
                    WorkspaceServerOperations server,
                    final GlobalDisplay globalDisplay,
                    FileDialogs fileDialogs,
                    RemoteFileSystemContext fsContext,
                    Commands commands,
                    Binder binder)
   {
      super(view);
      binder.bind(commands, this);
      fileDialogs_ = fileDialogs;
      view_ = view ;
      eventBus_ = eventBus;
      server_ = server;
      globalDisplay_ = globalDisplay ;
      fsContext_ = fsContext;
      objects_ = view_.getWorkspaceObjectTable();

      objects_.setObserver(this);
      eventBus_.addHandler(WorkspaceRefreshEvent.TYPE, this);
      eventBus_.addHandler(WorkspaceObjectAssignedEvent.TYPE, this);
      eventBus_.addHandler(WorkspaceObjectRemovedEvent.TYPE, this);
   }
   
   @Override
   public void onBeforeSelected()
   {
      super.onBeforeSelected();
      synchronizeView();
   }

   public void onWorkspaceRefresh(WorkspaceRefreshEvent event)
   {
      synchronizeView();
   }

   public void onWorkspaceObjectAssigned(WorkspaceObjectAssignedEvent event)
   {
      WorkspaceObjectInfo objectInfo = event.getObjectInfo();
      if (!objectInfo.isHidden())
         objects_.updateObject(objectInfo);
   }

   public void onWorkspaceObjectRemoved(WorkspaceObjectRemovedEvent event)
   {
      objects_.removeObject(event.getObjectName());
   }

   public void editObject(String objectName)
   {
      executeFunctionForObject("fix", objectName);
   }

   public void viewObject(String objectName)
   {
      executeFunctionForObject("View", objectName);
   }

   public void removeObject(final String objectName)
   {
      if (objectName == null)
         return;
      // confirm the user wants to do the remove
      String message = "Are you sure you want to remove the object \"" +
                       objectName +
                       "\"? This operation cannot be undone.";
      globalDisplay_.showYesNoMessage(
         GlobalDisplay.MSG_QUESTION,
         "Confirm Remove",
         message,
         new Operation() {

            // perform the remove with progress
            public void execute()
            {
               executeFunctionForObject("remove", objectName);
            }
         },
       true);
   }
   
   private void executeFunctionForObject(String function, String objectName)
   {
      String editCode = function + "(" + StringUtil.toRSymbolName(objectName) + ")";
      SendToConsoleEvent event = new SendToConsoleEvent(editCode, true);
      eventBus_.fireEvent(event);
   }

   @Handler
   void onRefreshWorkspace()
   {
      refreshView();
   }

   void onClearWorkspace()
   {
      view_.bringToFront();
      globalDisplay_.showYesNoMessage(
         GlobalDisplay.MSG_QUESTION,
         "Confirm Clear Workspace",
         "Are you sure you want to remove all objects from the workspace?",
  
         new ProgressOperation() {
            public void execute(final ProgressIndicator indicator)
            {
               indicator.onProgress("Removing objects...");
               server_.removeAllObjects(
                     new VoidServerRequestCallback(indicator));
            }
         },
         
         true
      );
   }

   void onSaveWorkspace()
   {
      view_.bringToFront();
      fileDialogs_.saveFile(
            "Save Workspace",
            fsContext_,
            getLastWorkspaceFileItem(),
            new FilenameTransform()
            {
               public String transform(String filename)
               {
                  // auto-append .RData if that isn't the extension
                  String ext = FileSystemItem.getExtensionFromPath(filename);
                  return ext.equalsIgnoreCase(".rdata")
                        ? filename
                        : filename + ".RData";
               }
            },
            new ProgressOperationWithInput<FileSystemItem>()
            {
               public void execute(
                     FileSystemItem input,
                     ProgressIndicator indicator)
               {
                  if (input == null)
                     return;

                  indicator.onProgress("Saving workspace...");
                  
                  String workspaceFileName = input.getPath();
                  
                  lastWorkspaceFileName_ = workspaceFileName;

                  server_.saveWorkspace(workspaceFileName,
                                        new VoidServerRequestCallback(indicator));
               }
            });
   }
   
   void onSaveWorkspaceAs()
   {
      globalDisplay_.showErrorMessage("Workspace", "Save Workspace As");
      
   }

   private FileSystemItem getLastWorkspaceFileItem()
   {
      return lastWorkspaceFileName_ != null
                           ? FileSystemItem.createFile(lastWorkspaceFileName_)
                           : null;
   }

   void onOpenWorkspace()
   {
      view_.bringToFront();
      fileDialogs_.openFile(
            "Open Workspace",
            fsContext_,
            getLastWorkspaceFileItem(),
            new ProgressOperationWithInput<FileSystemItem>()
            {
               public void execute(FileSystemItem input, ProgressIndicator indicator)
               {
                  if (input == null)
                     return;

                  indicator.onProgress("Loading workspace...");
                  lastWorkspaceFileName_ = input.getPath();
                  server_.loadWorkspace(input.getPath(),
                                        new VoidServerRequestCallback(indicator));
               }
            });
   }
   
   public void onOpenDataFile(OpenDataFileEvent event)
   {
      final String dataFilePath = event.getFile().getPath();
      globalDisplay_.showYesNoMessage(GlobalDisplay.MSG_QUESTION, 
            
            "Confirm Load Workspace",
                                      
            "Do you want to load the R data file \"" + dataFilePath + "\" " +
            "into your workspace?", 
                                      
            new ProgressOperation() {
                public void execute(ProgressIndicator indicator)
                {
                   indicator.onProgress("Loading workspsace...");
                   server_.loadWorkspace(
                                   dataFilePath, 
                                   new VoidServerRequestCallback(indicator));
                                          
                }
           },
           
           true);   
   }
   
   
   void onImportDatasetFromFile()
   {
      view_.bringToFront();
      fileDialogs_.openFile(
            "Select File to Import",
            fsContext_,
            getLastWorkspaceFileItem(),
            new ProgressOperationWithInput<FileSystemItem>()
            {
               public void execute(
                     FileSystemItem input,
                     ProgressIndicator indicator)
               {
                  if (input == null)
                     return;

                  indicator.onCompleted();

                  showImportFileDialog(input, null);
               }
            });
   }
   
   private void showImportFileDialog(FileSystemItem input, String varname)
   {
      ImportFileSettingsDialog dialog = new ImportFileSettingsDialog(
            server_,
            input,
            varname,
            "Import Dataset",
            new OperationWithInput<ImportFileSettings>()
            {
               public void execute(
                     ImportFileSettings input)
               {
                  String var = StringUtil.toRSymbolName(input.getVarname());
                  String code =
                        var +
                        " <- " +
                        makeCommand(input) +
                        "\n  View(" + var + ")";
                  eventBus_.fireEvent(new SendToConsoleEvent(code, true));
               }
            },
            globalDisplay_);
      dialog.showModal();
   }

   private String makeCommand(ImportFileSettings input)
   {
      HashMap<String, ImportFileSettings> commandDefaults_ =
            new HashMap<String, ImportFileSettings>();

      commandDefaults_.put("read.table", new ImportFileSettings(
            null, null, false, "", "\"'"));
      commandDefaults_.put("read.csv", new ImportFileSettings(
            null, null, true, ",", "\""));
      commandDefaults_.put("read.delim", new ImportFileSettings(
            null, null, true, "\t", "\""));

      String command = "read.table";
      ImportFileSettings settings = commandDefaults_.get("read.table");
      int score = settings.calculateSimilarity(input);
      for (String cmd : new String[] {"read.csv", "read.delim"})
      {
         ImportFileSettings theseSettings = commandDefaults_.get(cmd);
         int thisScore = theseSettings.calculateSimilarity(input);
         if (thisScore > score)
         {
            score = thisScore;
            command = cmd;
            settings = theseSettings;
         }
      }

      StringBuilder code = new StringBuilder(command);
      code.append("(");
      code.append(StringUtil.textToRLiteral(input.getFile().getPath()));
      if (input.isHeader() != settings.isHeader())
         code.append(", header=" + (input.isHeader() ? "T" : "F"));
      if (!input.getSep().equals(settings.getSep()))
         code.append(", sep=" + StringUtil.textToRLiteral(input.getSep()));
      if (!input.getQuote().equals(settings.getQuote()))
         code.append(", quote=" + StringUtil.textToRLiteral(input.getQuote()));
      code.append(")");

      return code.toString();
   }
   
   void onImportDatasetFromURL()
   {
      view_.bringToFront();
      globalDisplay_.promptForText(
         "Import from Web URL" ,
         "Please enter the URL to import data from:", 
                                   "", 
         new ProgressOperationWithInput<String>(){
            public void execute(String input, final ProgressIndicator indicator)
            {
            
               
               indicator.onProgress("Downloading data...");
               server_.downloadDataFile(input.trim(), 
                                        new ServerRequestCallback<DownloadInfo>(){

                  @Override
                  public void onResponseReceived(DownloadInfo downloadInfo)
                  {
                     indicator.onCompleted();
                     showImportFileDialog(
                           FileSystemItem.createFile(downloadInfo.getPath()),
                           StringUtil.toRSymbolName(downloadInfo.getVarname()));
                  }
                  
                  @Override
                  public void onError(ServerError error)
                  {
                     indicator.onError(error.getUserMessage());
                  }
                  
               });
                                       
            }
      });
      
   }

   void onImportDatasetFromGoogleSpreadsheet()
   {
      view_.bringToFront();
      new ImportGoogleSpreadsheetDialog(
         server_,
         globalDisplay_,
         new ProgressOperationWithInput<GoogleSpreadsheetImportSpec>() {

            public void execute(final GoogleSpreadsheetImportSpec importSpec,
                                ProgressIndicator progress)
            {
               progress.onProgress("Importing Spreadsheet...");
               
               server_.importGoogleSpreadsheet(
                  importSpec, 
                  new VoidServerRequestCallback(progress) {
                     @Override
                     public void onSuccess()
                     {
                        String code = "View(" + StringUtil.toRSymbolName(importSpec.getObjectName()) +")"; 
                        eventBus_.fireEvent(new SendToConsoleEvent(code, true));
                     }
                  });
            }
            
         }).showModal();
   }

   private void synchronizeView()
   {
      refreshView(false);
   }
   
   private void refreshView()
   {
      refreshView(true);
   }
   
   private void refreshView(final boolean reset)
   {  
      // show progress if we are doing a full reset
      final boolean showProgress = reset;
      if (showProgress)
         view_.setProgress(true);
      
      // clean out existing if we doing a reseta 
      if (reset)
         objects_.clearObjects();
       
      server_.listObjects(new ServerRequestCallback<RpcObjectList<WorkspaceObjectInfo>>()
      {
         @Override
         public void onError(ServerError error)
         {
            globalDisplay_.showErrorMessage("Error Listing Objects",
                                           error.getUserMessage());
            
            
            if (showProgress)
               view_.setProgress(false);
         }

         @Override
         public void onResponseReceived(RpcObjectList<WorkspaceObjectInfo> response)
         {
            // if this is not a full reset then we need to perform the 
            // deletes manually because we never cleared the existing
            // object table. this state is here so we can implement "silent"
            // refreshes of the workspace that don't flash and reset the
            // user's scroll position
            if (!reset)
            {
               ArrayList<String> objectNames = objects_.getObjectNames();
               for (int i=0; i<objectNames.size(); i++)
               {
                  String objectName = objectNames.get(i);
                  if (!containsObject(response, objectName))
                     objects_.removeObject(objectName);
               }
            }
            
            // perform updates (will add or update as necessary)
            for (int i = 0; i < response.length(); i++)
            {
               WorkspaceObjectInfo objectInfo = response.get(i);
               if (!objectInfo.isHidden())
                  objects_.updateObject(objectInfo);
            } 
            
            if (showProgress)
               view_.setProgress(false);
         }
      });
   }
   
   public boolean containsObject(RpcObjectList<WorkspaceObjectInfo> objects,
                                 String name)
   {
      for (int i=0; i<objects.length(); i++)
      {
         if (objects.get(i).getName().equals(name))
            return true;
      }
      
      // didn't find it
      return false;
   }

   private final Workspace.Display view_ ;
   private final WorkspaceServerOperations server_;
   private final GlobalDisplay globalDisplay_ ;
   private final EventBus eventBus_;
   private final WorkspaceObjectTable objects_;
   private final RemoteFileSystemContext fsContext_;
   private String lastWorkspaceFileName_;
   private final FileDialogs fileDialogs_;
}
