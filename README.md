# LinVst-X

LinVst-X (the X stands for Extra) adds support for Windows vst plugins to be used in Linux vst capable DAW's.

LinVst-X can coexist with LinVst, but it's best to move or copy the plugin dlls to a separate folder (which should be possible for most vst plugins) and then use linvstxconvert on the vst dlls in the folder and then set that folder as a vst search path in the Daw.

LinVst-X runs vst plugins in a single Wine process so plugins that communicate with each other or plugins that can use shared samples between instances will be able to communicate with their other instances.

Plugin instances can communicate with each other (which is not possible with LinVst), for example, Voxengo GlissEQ instances on different tracks simultaneously displaying the track spectrums and Kontakt instances on different tracks that use the same library not having to load samples for each new instance etc.

Plugins running in one process are not sandboxed, so if one plugin crashes then the whole lot might crash.

It's best to use plugins that already run with LinVst and/or use TestVst to test how a plugin might run under Wine.

For Waveform, disable sandbox option for plugins.

LinVst-X is also probably the version to use for best results when using Daw Automation.

LinVst-X usage is basically the same as LinVst except that the file to be renamed to the vst dll name is linvstx.so (rather than linvst.so for LinVst).

LinVst-X binaries are at https://github.com/osxmidi/LinVst-X/releases

See https://github.com/osxmidi/LinVst/tree/master/Make-Guide for setup info and make options

See the convert folder Notes on how to make the linvstx.so renaming conversion utilities.

LinVst-X server details.

The LinVst-X server is first started (after a boot) when a plugin is first loaded and after that the LinVst-X server continues to run and further plugin loading can occur faster than with LinVst.

The LinVst-X server can be prestarted (before the Daw) by entering into Terminal /usr/bin/lin-vst-server-x.exe or wine "/usr/bin/lin-vst-server-x.exe.so"

The LinVst-X server can be killed when no plugins are currently loaded.

To kill the server (when no plugins are loaded) get the pid of the lin-vst-server

pgrep lin-vst

or

ps -ef | grep lin-vst

and then use the pid to kill the lin-vst-server

kill -15 pid

## Quick Start Guide

(See the Wiki page for a visual guide https://github.com/osxmidi/LinVst/wiki)

Copy all of the lin-vst-serverxxxx files (files with lin-vst-server in their names) to /usr/bin.

Install the vst's.

The vst's will probably be installed by default to a Wine folder, something like ~/.wine/drive_c/Program Files/Steinberg/VSTPlugins (which is similar to where they are installed on Windows).

It's also possible with most plugins to make a folder and install the windows vst's into it.

Start linvstconvert (in the convert folder) and then select the linvst.so file.

Point linvstconvert to the folder containing the windows vst's and hit the Start (Convert) button.

Start up the linux DAW and point it to scan the folder containing the windows vst's.

If new vst plugins are added to a folder, then just run linvstconvert again on that folder.

Binary LinVst-X releases are available at https://github.com/osxmidi/LinVst-X/releases

## Common Problems/Possible Fixes

If window resizing does not work, then after a resize the UI needs to be closed and then reopened for the new window size to take effect.

If a LinVst version error pops up then LinVst probably needs to be reinstalled to /usr/bin and the older (renamed) linvst.so files in the vst dll folder need to be overwritten (using linvstconvert).

LinVst looks for wine in /usr/bin and if there isn't a /usr/bin/wine then that will probably cause problems.
/usr/bin/wine can be a symbolic link to /opt/wine-staging/bin/wine (for wine staging) for example.

Quite a few plugins need winetricks corefonts installed for fonts.

A large number of vst plugin crashes/problems can be basically narrowed down to the following dll's and then worked around by overriding/disabling them.

Quite a few vst plugins rely on the Visual C++ Redistributable dlls msvcr120.dll msvcr140.dll msvcp120.dll msvcp140.dll etc

Some vst plugins might crash if the Visual C++ Redistributable dlls are not present in /home/username/.wine/drive_c/windows/system32 for 64 bit vst's and /home/username/.wine/drive_c/windows/syswow64 for 32 bit vst's, and then overridden in the winecfg Libraries tab

Some vst plugins might not work due to Wines current capabilities or for some other reason.

Use TestVst for testing how a vst plugin might run under Wine.

Some vst plugins rely on the d2d1 dll which is not totally implemented in current Wine.

If a plugin has trouble with it's display then disabling d2d1 in the winecfg Libraries tab can be tried.

Some plugins need Windows fonts (~/.wine/drive_c/windows/Fonts) ./winetricks corefonts

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

For details about overriding dll's, see the Wine Config section in the Detailed Guide https://github.com/osxmidi/LinVst/tree/master/Detailed-Guide

To enable 32 bit vst's on a 64 bit system, a distro's multilib needs to be installed (on Ubuntu it would be sudo apt-get install gcc-multilib g++-multilib)

Also, see the Tested VST's folder at https://github.com/osxmidi/LinVst/tree/master/Tested-VST-Plugins for some vst plugin setups and possible tips.

**Hyperthreading**

For Reaper, in Options/Preferences/Buffering uncheck Auto-detect the number of needed audio processing threads and set 
Audio reading/processing threads to the amount of physical cores of the cpu (not virtual cores such as hyperthreading cores).

This can help with stutters and rough audio response.

Other Daws might have similar settings.

**Renoise**

Sometimes a synth vst might not declare itself as a synth and Renoise might not enable it.

A workaround is to install sqlitebrowser

sudo add-apt-repository ppa:linuxgndu/sqlitebrowser-testing

sudo apt-get update && sudo apt-get install sqlitebrowser

Add the synth vst's path to VST_PATH and start Renoise to scan it.

Then exit renoise and edit the database file /home/user/.renoise/V3.1.0/ CachedVSTs_x64.db (enable hidden folders with right click in the file browser).

Go to the "Browse Data" tab in SQLite browser and choose the CachedPlugins table and then locate the entry for the synth vst and enable the "IsSynth" flag from "0" (false) to "1" (true) and save.

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
