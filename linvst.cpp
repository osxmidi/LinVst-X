/*  linvst is based on wacvst Copyright 2009 retroware. All rights reserved. and dssi-vst Copyright 2004-2007 Chris Cannam

    linvst Mark White 2017

    This file is part of linvst.

    linvst is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. *
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "remotepluginclient.h"

extern "C" {

#define VST_EXPORT   __attribute__ ((visibility ("default")))

    extern VST_EXPORT AEffect * VSTPluginMain(audioMasterCallback audioMaster);

    AEffect * main_plugin (audioMasterCallback audioMaster) asm ("main");

#define main main_plugin

    VST_EXPORT AEffect * main(audioMasterCallback audioMaster)
    {
        return VSTPluginMain(audioMaster);
    }
}

const char * selfname()
{
    int i = 5;
}

void errwin2()
{
 Window window = 0;
 Window ignored = 0;
 Display* display = 0;
 int screen = 0;
 Atom winstate;
 Atom winmodal;
    
std::string filename2;

  filename2 = "lin-vst-server/vst not found or LinVst version mismatch";
      
  XInitThreads();
  display = XOpenDisplay(NULL);  
  if (!display) 
  return;  
  screen = DefaultScreen(display);
  window = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10, 480, 20, 0, BlackPixel(display, screen), WhitePixel(display, screen));
  if (!window) 
  return;
  winstate = XInternAtom(display, "_NET_WM_STATE", True);
  winmodal = XInternAtom(display, "_NET_WM_STATE_ABOVE", True);
  XChangeProperty(display, window, winstate, XA_ATOM, 32, PropModeReplace, (unsigned char*)&winmodal, 1);
  XStoreName(display, window, filename2.c_str()); 
  XMapWindow(display, window);
  XSync (display, false);
  XFlush(display);
  sleep(10);
  XSync (display, false);
  XFlush(display);
  XDestroyWindow(display, window);
  XCloseDisplay(display);  
  }

VST_EXPORT AEffect* VSTPluginMain (audioMasterCallback audioMaster)
{
    RemotePluginClient *plugin;
    
       Dl_info     info;

    if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
        return 0;
        
    if (!dladdr((const char*) selfname, &info))
    {
        std::cerr << "dl name error" << std::endl;
        return 0;
    }

    try
    {
    plugin = new RemotePluginClient(audioMaster, info);
    }
    catch (std::string e)
    {
        std::cerr << "Could not connect to Server" << std::endl;
	    errwin2();    
        if(plugin)
        {
        plugin->m_runok = 1;
        delete plugin;
        }
        return 0;
    }
    
 //   plugin->initEffect(plugin);
    
 //   plugin->ServerConnect();
		
    if(plugin->m_runok == 1)
    {
        std::cerr << "LinVst Error: lin-vst-server not found or vst dll load timeout or LinVst version mismatch" << std::endl;
 //       errwin2();
	    if(plugin)
        delete plugin;
        return 0;
    }

#ifdef EMBED
        XInitThreads();
#endif

    return plugin->theEffect;
}
