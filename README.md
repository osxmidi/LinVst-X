# LinVst-X

LinVst-X adds support for Windows vst's to be used in Linux vst capable DAW's.

LinVst-X runs a single Wine process so plugins that communicate with each other or plugins that can use shared samples between instances will be able to communicate with their other instances.

Usage is basically the same as LinVst except that linvst.so is linvstx.so

## Quick Start Guide

(See the Wiki page for a visual guide https://github.com/osxmidi/LinVst/wiki)

Decide on what version to run, either the embedded window version or the standalone window version (the embedded version would probably be the default choice)

Copy all of the lin-vst-server files (files with lin-vst-server in their names) from the version that was chosen to /usr/bin (either the embedded or standalone version folder)

Make a folder and place the windows vst's in it.

Start linvstxconvert (in the convert folder) and then select the linvstx.so from the chosen embedded or standalone window version folder.

Point linvstxconvert to the folder containing the windows vst's and hit the Start (Convert) button.

Start up the linux DAW and point it to scan the folder containing the windows vst's.

Binary LinVst-X releases are available at https://github.com/osxmidi/LinVst-X/releases

## Symlinks

Symlinks can be used for convenience if wanted.
One reason to use symlinks would be to be able to group plugins together outside of their Wine install paths and reference them from a common folder via symlinks.

For a quick simple example, say that the plugin dll file is in ~/.wine/drive_c/Program Files/Vstplugins and is called plugin.dll.
Then just open that folder using the file manager and drag linvstx.so to it and rename linvstx.so to whatever the dll name is (plugin.so for plugin.dll).
Then create a symlink to plugin.so using a right click and selecting create symlink from the option menu, the make sure the symlink ends in a .so extension (might need to edit the symlinks name) and then drag that symlink to anywhere the DAW searches (say ~/vst for example) and then plugin.so should load (via the symlink) within the DAW.

If the dll plugin files are in a sudo permission folder (or any permission folder) such as /usr/lib/vst, then make a user permission folder such as /home/user/vst and then make symbolic links to /usr/lib/vst in the /home/user/vst folder by changing into /home/user/vst and running&nbsp;&nbsp;ln -s /usr/lib/vst/&lowast;&nbsp;&nbsp;.&nbsp;&nbsp;and then run linvstxconvert on the /home/user/vst folder and then set the DAW to search the /home/user/vst folder.

linvstxconvert can also be run with sudo permission for folders/directories that need sudo permission.

Another way to use symlinks is, if the vst dll files and correspondingly named linvstx .so files (made by using linvstxconvert) are in say for example /home/user/.wine/drive_c/"Program Files"/VSTPlugins

then setting up links in say for example /home/user/vst

by creating the /home/user/vst directory and changing into the /home/user/vst directory

and then running

```
ln -s /home/user/.wine/drive_c/"Program Files"/VSTPlugins/*.so /home/user/vst
```

will create symbolic links in /home/user/vst to the linvstx .so files in /home/user/.wine/drive_c/"Program Files"/VSTPlugins and then the DAW can be pointed to scan /home/user/vst

## More Detailed Guide

To use LinVst-X, the linvstx.so file simply needs to be renamed to match the windows vst dll's filename.

(the .so and .dll extensions are left as they are and are not part of the renaming)

Both the windows vst dll file and the renamed linvstx.so file are then a matched pair and need to be kept together in the same directory/folder.

**For Example**

copy lin-vst-server.exe and lin-vst-server.so to /usr/bin

copy linvstx.so to wherever the windows vst dll file is that someone wants to run in a Linux DAW, 
and then rename linvstx.so to the windows vst dll filename 

linvstx.so gets renamed to test.so for test.dll

Use the renamed file (test.so) in a Linux VST capable DAW 

Load test.so within the Linux DAW and then test.so will load and run the (name associated) test.dll windows vst 

linvstxconvert (GUI or CLI) and linvstxconverttree can automatically batch name convert linvstx.so to mutiple windows vst dll names that are located in a folder/directory (the linvstxconvert CLI version needs to be run from within the dll's folder/directory).
linvstxconverttree can automatically name convert folders and sub folders (directories and sub directories) of vst dll plugins.

linvstxconvert needs to be run with sudo permission for folders/directories that need sudo permission.

After the naming conversion, the newly created files (.so files) are ready to be used in Linux vst DAW's from the folder that was used for the naming conversion.

Copying/moving plugins (and in some cases their associated presets etc) to a folder/directory with user permissions (if possible) is generally a good idea unless the vst plugin requires a fixed path.

### Using a folder of windows dll files as an example.

Say the folder contains 

- Delay.dll
- Compressor.dll
- Chorus.dll
- Synth.dll

then after the renaming (using the renaming utility) the folder will look like

- Delay.dll
- Delay.so



- Compressor.dll
- Compressor.so



- Chorus.dll
- Chorus.so



- Synth.dll
- Synth.so

The files ending with .so can be used inside Linux Vst DAWS to load and manage the associated dll files (ie Delay.so loads and manages Delay.dll).

Say for instance that a windows vst installation has installed a windows vst named Delay.dll into the VstPlugins directory inside of a WINEPREFIX ie (~/.wine/drive_c/Program Files/VstPlugins), then the renaming utility needs to be run on the VstPlugins directory.

After the renaming there is Delay.dll and Delay.so in the ~/.wine/drive_c/Program Files/VstPlugins directory.

The Linux DAW plugin search path is then appended to include ~/.wine/drive_c/Program Files/VstPlugins and then the plugin(s) can be loaded.

Another way is to make a symbolic link to ~/.wine/drive_c/Program Files/VstPlugins/Delay.so or to the whole ~/.wine/drive_c/Program Files/VstPlugins directory from a more convenient folder such as /home/user/vst and then append /home/user/vst to the Linux DAW's plugin search path.

There can be multiple WINEPREFIXES (by default there is one ~/.wine) containing various vst dll plugins and each WINEPREFIX can have a different wine setup, including dll overrides etc.

Different windows vst plugins that might require different wine setups can be installed into different WINEPREFIXES by creating a new WINEPREFIX (export WINEPREFIX=&sim;/.wine-new winecfg) and then run the windows vst installation program.
Or by setting the WINEPREFIX environmental variable to a particular pre existing WINEPREFIX and then installing the vst into it ie export WINEPREFIX=&sim;/.wine-preexisting and then run the vst install.

A particular WINEPREFIX can be configured by using winecfg with the WINEPREFIX environmental variable set to that particular WINEPREFIX ie export WINEPREFIX=~/.wine-new winecfg.

When a plugin is loaded from within a WINEPREFIX, it picks up on that WINEPREFIXES individual setup (also works for symbolic links as discussed above).

------

linvstx.so needs to be renamed to the windows vst name (the .so and .dll extensions are left as they are and are not part of the renaming).

Both the windows vst dll file and the renamed linvstx.so file need to be in the same folder/directory.

linvstx.so is a Linux vst template that loads and runs any windows vst that linvstx.so is renamed to.

linvstx.so can be copied and renamed multiple times to load multiple windows vst's.

Basically, linvstx.so becomes the windows vst once it's renamed to the windows vst's name and can then be used in Linux vst hosts.

Individual plugins can have their own WINEPREFIX environment.

If a windows vst dll file and it's associated renamed linvstx.so file are located within a WINEPREFIX then the plugin will use that WINEPREFIX.

Symlinks can point to renamed linvstx.so files located within a WINEPREFIX.

## Common Problems/Possible Fixes

LinVst-X looks for wine in /usr/bin and if there isn't a /usr/bin/wine then that will probably cause problems.
/usr/bin/wine can be a symbolic link to /opt/wine-staging/bin/wine (for wine staging) for example.

Quite a few plugins need winetricks corefonts installed for fonts.

A large number of vst plugin crashes/problems can be basically narrowed down to the following dll's and then worked around by overriding/disabling them.

Quite a few vst plugins rely on the Visual C++ Redistributable dlls msvcr120.dll msvcr140.dll msvcp120.dll msvcp140.dll etc

Some vst plugins might crash if the Visual C++ Redistributable dlls are not present in /home/username/.wine/drive_c/windows/system32 for 64 bit vst's and /home/username/.wine/drive_c/windows/syswow64 for 32 bit vst's, and then overridden in the winecfg Libraries tab

Quite a few plugins rely on the d2d1 dll 

Some vst's might crash if Wines inbuilt d2d1 is active (which it is by default).

d2d1 can be disabled in the winecfg Libraries tab.

Setting HKEY_CURRENT_USER Software Wine Direct3D MaxVersionGL to 30002 (MaxVersionGL is a DWORD hex value) might help with some plugins and d2d1 (can also depend on hardware and drivers).

wininet is used by some vst's for net access including registration and online help etc and sometimes wines inbuilt wininet might cause a crash or have unimplemented functions.

wininet can be overridden with wininet.dll and iertutil.dll and nsi.dll

The winbind and libntlm0 and gnutls packages might need to be installed for net access (sudo apt-get install winbind sudo apt-get install gnutls-bin sudo apt-get install libntlm0)

Occasionally other dlls might need to be overridden such as gdiplus.dll etc

Winetricks https://github.com/Winetricks/winetricks can make overriding dll's easier.

**For the above dll overrides**

```
winetricks vcrun2013
winetricks vcrun2015
winetricks wininet
winetricks gdiplus
```

Winetricks also has a force flag --force ie winetricks --force vcrun2013

cabextract needs to be installed (sudo apt-get install cabextract, yum install cabextract etc)

To enable 32 bit vst's on a 64 bit system, a distro's multilib needs to be installed (on Ubuntu it would be sudo apt-get install gcc-multilib g++-multilib)

For details about overriding dll's, see the next section (Wine Config).

Drag and Drop is enabled for the embedded LinVst-X version used with Reaper/Tracktion/Waveforn/Bitwig but it's only for items dragged and dropped into the vst window and not for items dragged and dropped from the vst window to the DAW/host window.

**Renoise**

Sometimes a synth vst doesn't declare itself as a synth and Renoise might not enable it.

A workaround is to install sqlitebrowser

sudo add-apt-repository ppa:linuxgndu/sqlitebrowser-testing

sudo apt-get update && sudo apt-get install sqlitebrowser

Add the synth vst's path to VST_PATH and start Renoise to scan it.

Then exit renoise and edit the database file /home/user/.renoise/V3.1.0/ CachedVSTs_x64.db (enable hidden folders with right click in the file browser).

Go to the "Browse Data" tab in SQLite browser and choose the CachedPlugins table and then locate the entry for the synth vst and enable the "IsSynth" flag from "0" (false) to "1" (true) and save.

## Wine Config

LinVst-X expects wine to be found in /usr/bin.

Setting WINELOADER etc to a new wine path might possibly be used for different wine paths.

Keyboard input etc can be enabled for the standalone window LinVst-X version only, by creating a UseTakeFocus string and setting it to a value of N, in HKEY_CURRENT_USER/Software/Wine/X11 Driver (regedit).

More keyboard control (not recommended) can be enabled for the standalone window LinVst-X version only, by unchecking the winecfg option "Allow the window manager to control the windows".

Keyboard input should be ok for the embedded window version.

The embedded window version needs to be run with the winecfg option "Allow the window manager to control the windows" checked (which is usually the default).

Sometimes usernames and passwords might need to be copied and pasted into the window because manual entry might not work in all cases.

Sometimes a windows vst needs a Wine dll override.

Finding out what dll's to possibly override can be done by running "strings vstname.dll | grep -i dll", which will display a list of dll's from the plugins dll file.

For instance, if the dll list contains d2d1.dll and there are problems running the plugin, then d2d1 might possibly be a candidate to override or disable.

If the Wine debugger displays "unimplemented function in XXXX.dll" somewhere in it's output, then that dll usually needs to be overriden with a windows dll.

Overriding a dll involves copying the windows dll to a wine windows directory and then running winecfg to configure wine to override the dll.

64 bit .dlls are copied to /home/username/.wine/drive_c/windows/system32 

32 bit .dlls are copied to /home/username/.wine/drive_c/windows/syswow64

Run winecfg and select the Libraries tab and then select the dll to override from the list or type the name.

After adding the dll, check with the edit option that the dll's settings are native first and then builtin.

https://www.winehq.org/docs/wineusr-guide/config-wine-main

Sometimes required dll's might be missing and additional windows redistributable packages might need to be installed.

Some windows vst's use D3D, and Wine uses Linux OpenGL to implement D3D, so a capable Linux OpenGL driver/setup might be required for some windows vst's.

Disabling d2d1 in the Libraries section of winecfg might help with some windows vst's.

Some D3D dll overrides might be needed for some windows vst's.

D3D/OpenGL Wine config advice can be found at gaming forums and other forums.

Additional dll's (dll overrides) might have to be added to Wine for some Windows vst's to work.

Winetricks might help with some plugins https://github.com/Winetricks/winetricks

Some plugins might use wininet for internet connections (online registration, online help, etc) which might cause problems depending on Wines current implementation.

Running winetricks wininet and/or installing winbind and libntlm0 for a distro (sudo apt-get install winbind, sudo apt-get install libntlm0) might help (wininet and it's associated dll's can also be manually installed as dll overrides).

Turning off the vst's multiprocessor support and/or GPU acceleration might help in some cases, due to what features Wine currently supports (Wine version dependent).

On some slower systems Wine can initially take a long time to load properly when Wine is first used, which might cause a LinVst-X crash.
The solution is to initialise Wine first by running winecfg or any other Wine based program, so that Wine has been initialised before LinVst-X is used.

Upgrading to the latest wine version is recommended.


