/*
 * DesktopHooks.java
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
package org.rstudio.studio.client.application;

import com.google.gwt.core.client.GWT;
import com.google.inject.Inject;
import org.rstudio.core.client.command.AppCommand;
import org.rstudio.core.client.files.FileSystemItem;
import org.rstudio.core.client.js.BaseExpression;
import org.rstudio.core.client.js.JsObjectInjector;
import org.rstudio.studio.client.application.events.EventBus;
import org.rstudio.studio.client.application.events.SaveActionChangedEvent;
import org.rstudio.studio.client.application.events.SaveActionChangedHandler;
import org.rstudio.studio.client.application.model.SaveAction;
import org.rstudio.studio.client.common.GlobalDisplay;
import org.rstudio.studio.client.common.filetypes.FileTypeRegistry;
import org.rstudio.studio.client.server.Server;
import org.rstudio.studio.client.workbench.WorkbenchContext;
import org.rstudio.studio.client.workbench.commands.Commands;
import org.rstudio.studio.client.workbench.views.source.SourceShim;

/**
 * Any methods on this class are automatically made available to the
 * Qt frame code.
 */
public class DesktopHooks
{
   @BaseExpression("$wnd.desktopHooks")
   interface DesktopHooksInjector extends JsObjectInjector<DesktopHooks> {}
   private static final DesktopHooksInjector injector =
         GWT.create(DesktopHooksInjector.class);

   @Inject
   public DesktopHooks(Commands commands,
                       EventBus events,
                       GlobalDisplay globalDisplay,
                       Server server,
                       FileTypeRegistry fileTypeRegistry,
                       WorkbenchContext workbenchContext,
                       SourceShim sourceShim)
   {
      commands_ = commands;
      events_ = events;
      globalDisplay_ = globalDisplay;
      server_ = server;
      fileTypeRegistry_ = fileTypeRegistry;
      workbenchContext_ = workbenchContext;
      sourceShim_ = sourceShim;
      
      events_.addHandler(SaveActionChangedEvent.TYPE, 
                         new SaveActionChangedHandler() 
      {
         public void onSaveActionChanged(SaveActionChangedEvent event)
         {
            saveAction_ = event.getAction();  
         }
      });
      
      injector.injectObject(this);

      addCopyHook();
   }

   private native void addCopyHook() /*-{
      var clean = function() {
         setTimeout(function() {
            $wnd.desktop.cleanClipboard(false);
         }, 100)
      };
      $wnd.addEventListener("copy", clean, true);
      $wnd.addEventListener("cut", clean, true);
   }-*/;

   
   String getActiveProjectDir()
   {
      if (workbenchContext_.getActiveProjectDir() != null)
         return workbenchContext_.getActiveProjectDir().getPath();
      else
         return "";
   }
   
   String getActiveProjectFile()
   {
      if (workbenchContext_.getActiveProjectFileFullPath() != null)
         return workbenchContext_.getActiveProjectFileFullPath();
      else
         return "";
   }

   void invokeCommand(String cmdId)
   {
      commands_.getCommandById(cmdId).execute();
   }

   boolean isCommandVisible(String commandId)
   {
      AppCommand command = commands_.getCommandById(commandId);
      return command != null && command.isVisible();
   }

   boolean isCommandEnabled(String commandId)
   {
      AppCommand command = commands_.getCommandById(commandId);
      return command != null && command.isEnabled();
   }

   String getCommandLabel(String commandId)
   {
      AppCommand command = commands_.getCommandById(commandId);
      return command != null ? command.getMenuLabel(true) : "";
   }

   void openFile(String filePath)
   {
      // get the file system item
      FileSystemItem file = FileSystemItem.createFile(filePath);
      
      if (file.isDirectory())
         return;
      
      // you can't open a project on top of another project
      // via a file-association (rather, a new instance of 
      // RStudio is run for the associated project)
      if (file.getExtension().equalsIgnoreCase(".rproj"))
      {
         return;
      }
      else
      {
         // open the file. pass false for second param to prevent
         // the default handler (the browser) from taking it
         fileTypeRegistry_.openFile(file, false);
      }
   }
   
   void quitR()
   {
      commands_.quitSession().execute();
   }
  
   
   int getSaveAction()
   {
      return saveAction_.getAction();
   }
   
   String getREnvironmentPath()
   {
      return workbenchContext_.getREnvironmentPath();
   }

   private final Commands commands_;
   private final EventBus events_;
   private final GlobalDisplay globalDisplay_;
   private final Server server_;
   private final FileTypeRegistry fileTypeRegistry_;
   private final WorkbenchContext workbenchContext_;
   private final SourceShim sourceShim_;
   
   private SaveAction saveAction_ = SaveAction.saveAsk();
}
