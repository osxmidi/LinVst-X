/*  dssi-vst: a DSSI plugin wrapper for VST effects and instruments
    Copyright 2004-2007 Chris Cannam

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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef REMOTE_PLUGIN_CLIENT_H
#define REMOTE_PLUGIN_CLIENT_H

#define __cdecl

#include <stdint.h>

#ifdef VESTIGE
typedef int16_t VstInt16;
typedef int32_t VstInt32;
typedef int64_t VstInt64;
typedef intptr_t VstIntPtr;
#define VESTIGECALLBACK __cdecl
#include "vestige.h"
#else
#include "pluginterfaces/vst2.x/aeffectx.h"
#endif

#include "rdwrops.h"
#include "remoteplugin.h"
#include <dlfcn.h>
#include <fstream>
#include <string>
#include <sys/shm.h>
#include <vector>

#ifdef EMBED
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#include <atomic>

// Any of the methods in this file, including constructors, should be
// considered capable of throwing RemotePluginClosedException.  Do not
// call anything here without catching it.

class RemotePluginClient {
public:
  RemotePluginClient(audioMasterCallback theMaster, Dl_info info);
  ~RemotePluginClient();

  std::string getFileIdentifiers();

  int getVersion();
  int getUID();

  std::string getName();
  std::string getMaker();

  void setBufferSize(int);
  void setSampleRate(int);

  void reset();
  void terminate();

  int getInputCount();
  int getOutputCount();
  int getFlags();
  int getinitialDelay();

  int getParameterCount();
  std::string getParameterName(int);
  std::string getParameterLabel(int);
  std::string getParameterDisplay(int);
#ifdef WAVES
  int getShellName(char *ptr);
#endif
  void setParameter(int, float);
  float getParameter(int);
  float getParameterDefault(int);
  void getParameters(int, int, float *);

  int getProgramCount();
  int getProgramNameIndexed(int, char *ptr);
  std::string getProgramName();
  void setCurrentProgram(int);

  int processVstEvents(VstEvents *);
#ifdef DOUBLEP
  void processdouble(double **inputs, double **outputs, int sampleFrames);
  bool setPrecision(int value);
#endif
  bool getEffInProp(int index, void *ptr);
  bool getEffOutProp(int index, void *ptr);
#ifdef MIDIEFF
  //   bool                getEffInProp(int index, void *ptr);
  //   bool                getEffOutProp(int index, void *ptr);
  bool getEffMidiKey(int index, void *ptr);
  bool getEffMidiProgName(int index, void *ptr);
  bool getEffMidiCurProg(int index, void *ptr);
  bool getEffMidiProgCat(int index, void *ptr);
  bool getEffMidiProgCh(int index);
  bool setEffSpeaker(VstIntPtr value, void *ptr);
  bool getEffSpeaker(VstIntPtr value, void *ptr);
#endif
#ifdef CANDOEFF
  bool getEffCanDo(char *ptr);
#endif
  int getChunk(void **ptr, int bank_prog);
  int setChunk(void *ptr, int sz, int bank_prog);
  int canBeAutomated(int param);
  int getProgram();
  int EffectOpen();

#ifdef EMBED
#ifdef XECLOSE
  void sendXembedMessage(Display *display, Window window, long message,
                         long detail, long data1, long data2);
#endif
#endif

  // void                effMainsChanged(int s);
  // int                 getUniqueID();

  // Either inputs or outputs may be NULL if (and only if) there are none
  void process(float **inputs, float **outputs, int sampleFrames);

  void waitForServer(ShmControl *m_shmControlptr);

  void waitForServer2exit();
  void waitForServer3exit();
  void waitForServer4exit();
  void waitForServer5exit();
  void waitForServer6exit();

  void waitForClientexit();

  void setDebugLevel(RemotePluginDebugLevel);
  bool warn(std::string);

  void showGUI();
  void hideGUI();

#ifdef EMBED
  void openGUI();
  ERect *rp;    
#endif

  int getEffInt(int opcode, int value);
  std::string getEffString(int opcode, int index);
  void effVoidOp(int opcode);
  int effVoidOp2(int opcode, int index, int value, float opt);

  void errwin(std::string dllname);

  // static const char * selfname();

  void ServerConnect(Dl_info info);

  static VstIntPtr dispatchproc(AEffect *effect, VstInt32 opcode,
                                VstInt32 index, VstIntPtr value, void *ptr,
                                float opt);

#ifdef DOUBLEP
  static void prodproc(AEffect *effect, double **inputs, double **outputs,
                       int sampleFrames);
#endif

  static void proproc(AEffect *effect, float **inputs, float **outputs,
                      int sampleFrames);

  static void setparproc(AEffect *effect, int index, float parameter);

  static float getparproc(AEffect *effect, int index);

  int m_effeditclose;
  int m_bufferSize;
  int m_numInputs;
  int m_numOutputs;
  int m_finishaudio;
  int m_runok;
  int m_syncok;
  int m_386run;
  AEffect *theEffect;
  AEffect theEffect2;
  audioMasterCallback m_audioMaster;

  int m_threadbreak;
  int m_threadbreakexit;
  int editopen;
#ifdef EMBED
#ifdef XECLOSE
  int xeclose;
#endif
/*
    int                 m_threadbreakembed;
    int                 m_threadbreakexitembed;
*/
#endif
  VstEvents vstev[VSTSIZE];
  ERect retRect = {0, 0, 200, 500};
  int reaperid;
  int m_updateio;
  int m_updatein;
  int m_updateout;
  int m_delay;
#ifdef CHUNKBUF
  char *chunk_ptr;
#endif

#ifdef EMBED
  Window child;
  Window parent;
  Display *display;
  int handle;
  int width;
  int height;
  struct alignas(64) winmessage {
    int handle;
    int width;
    int height;
    int winerror;
  } winm2;
  winmessage *winm;
  int displayerr;
#ifdef EMBEDRESIZE
  int resizedone;
#endif
  void syncevents(Display *display);

  /*
     pthread_t           m_EMBEDThread;
     static void         *callEMBEDThread(void *arg) { return
     ((RemotePluginClient*)arg)->EMBEDThread(); } void *EMBEDThread();
  */

  Window pparent;
  Window windowreturn;
  Window root;
  Window *children;
  unsigned int numchildren;
  int parentok;
#ifdef EMBEDDRAG
  Window x11_win;
  Window dragwin;
  int dragwinok;    
#endif
  int eventrun;
  int eventstop;
  int eventfinish;
#endif

  key_t MyKey2;
  int ShmID2;
  char *ShmPTR2;

  const char *argStr;

  char *m_shm3;
  char *m_shm4;
  char *m_shm5;
#ifdef PCACHE
  char *m_shm6;

  struct alignas(64) ParamState {
  float value;
  float valueupdate;
  char changed;
  };
#endif

  int m_inexcept;

  /*
  struct alignas(64) vinfo
  {
  char a[64 + 8 + (sizeof(int32_t) * 2) + 48];
  // char a[96];
  };
  */

  ShmControl *m_shmControlptr;

  VstTimeInfo *timeInfo;

#ifdef EMBED
#ifdef TRACKTIONWM
  int waveformid;
  int waveformid2;
  int hosttracktion;
#endif
#endif

#ifdef WAVES
  int wavesthread;
#endif

protected:
  void cleanup();
  void syncStartup();

private:
  int m_shmFd;
  int m_shmControlFd;
  char *m_shmControlFileName;
  ShmControl *m_shmControl;
  ShmControl *m_shmControl2;
  ShmControl *m_shmControl3;
  ShmControl *m_shmControl4;
  ShmControl *m_shmControl5;
  ShmControl *m_shmControl6;
  int m_AMRequestFd;
  int m_AMResponseFd;
  char *m_AMRequestFileName;
  char *m_AMResponseFileName;
  char *m_shmFileName;
  char *m_shm;
  size_t m_shmSize;
  char *m_shm2;

  int sizeShm();

  pthread_t m_AMThread;
  static void *callAMThread(void *arg) {
    return ((RemotePluginClient *)arg)->AMThread();
  }
  void *AMThread();
  int m_threadinit;

  void RemotePluginClosedException();

  bool fwait(ShmControl *m_shmControlptr, std::atomic_int *fcount, int ms);
  bool fpost(ShmControl *m_shmControlptr, std::atomic_int *fcount);

  bool fwait2(ShmControl *m_shmControlptr, std::atomic_int *fcount, int ms);
  bool fpost2(ShmControl *m_shmControlptr, std::atomic_int *fcount);
};
#endif
