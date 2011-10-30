/*
 * WizardResources.java
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
package org.rstudio.core.client.widget;


import com.google.gwt.core.client.GWT;
import com.google.gwt.resources.client.ClientBundle;
import com.google.gwt.resources.client.CssResource;
import com.google.gwt.resources.client.ImageResource;
import com.google.gwt.resources.client.ImageResource.ImageOptions;
import com.google.gwt.resources.client.ImageResource.RepeatStyle;

public interface WizardResources extends ClientBundle
{
   interface Styles extends CssResource
   {
      String mainWidget();
      String headerLabel();
      String headerPanel();
      String subcaptionLabel();
      String wizardBodyPanel();
      String wizardPageSelector();
      String wizardPageSelectorItem();
      String wizardPageSelectorItemFirst();
      String wizardPageSelectorItemLast();
      String wizardPageBackground();
      String wizardBackButton();
   }
   
   @Source("Wizard.css")
   Styles styles(); 
   
   ImageResource wizardBackButton();
   ImageResource wizardDisclosureArrow();
   
   
   @ImageOptions(repeatStyle = RepeatStyle.Horizontal)
   ImageResource wizardPageSelectorBackground();
   
   @ImageOptions(repeatStyle = RepeatStyle.Horizontal)
   ImageResource wizardPageSelectorBackgroundFirst();
   
   @ImageOptions(repeatStyle = RepeatStyle.Horizontal)
   ImageResource wizardPageSelectorBackgroundLast();
   
   @ImageOptions(repeatStyle = RepeatStyle.Horizontal)
   ImageResource wizardPageBackground();
   
   static WizardResources INSTANCE = 
                        (WizardResources)GWT.create(WizardResources.class);
}