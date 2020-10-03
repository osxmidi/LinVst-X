# TestVst

TestVst can roughly test how a vst dll plugin might run under Wine.

It tries to open the vst dll and display the vst window for roughly 8 seconds and then closes.

Usage is ./testvst.exe "path to vstfile.dll"

paths and vst filenames that contain spaces need to be enclosed in quotes.

for example (cd into the testvst folder using the terminal)

./testvst.exe "/home/your-user-name/.wine/drive_c/Program Files/Steinberg/VstPlugins/delay.dll"

./testvst32.exe "/home/your-user-name/.wine/drive_c/Program Files (x86)/Steinberg/VstPlugins/delay32.dll"

testvst32.exe is for 32bit vst plugins.

Use testvst.exe from a folder that is not in a daw search path.

If testvst.exe.so and/or testvst32.exe.so are in any daw search paths then they can cause problems if the daw tries to load them.

-----

Batch Testing

For testing multiple vst dll files at once, unzip the testvst folder, then (using the terminal) cd into the unzipped testvst folder.

then enter

chmod +x testvst-batch
(chmod +x testvst32-batch for 32 bit plugins)

Usage is ./testvst-batch "path to the vst folder containing the vst dll files"

paths that contain spaces need to be enclosed in quotes.

pathnames must end with a /

Same usage procedure applies to testvst32-batch which tests 32 bit vst dll files.

for example

./testvst-batch "/home/your-user-name/.wine/drive_c/Program Files/Steinberg/VstPlugins/"

./testvst32-batch "/home/your-user-name/.wine/drive_c/Program Files (x86)/Steinberg/VstPlugins/"

After that, testvst.exe (testvst32.exe for 32 bit plugins) will attempt to run the plugins one after another, any plugin dialogs that popup should be dismissed as soon as possible.

If a Wine plugin problem is encountered, then that plugin can be identified by the terminal output from testvst.exe (testvst32.exe for 32 bit plugins).

Use testvst.exe from a folder that is not in a daw search path.

If testvst.exe.so and/or testvst32.exe.so are in any daw search paths then they can cause problems if the daw tries to load them.

-----

