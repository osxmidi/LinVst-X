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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>

#include <sched.h>

//#define WIN32_LEAN_AND_MEAN

//#include <windows.h>

#include "remotepluginserver.h"

#include "paths.h"

#define APPLICATION_CLASS_NAME "dssi_vst"
#ifdef TRACKTIONWM
#define APPLICATION_CLASS_NAME2 "dssi_vst2"
#endif
#define OLD_PLUGIN_ENTRY_POINT "main"
#define NEW_PLUGIN_ENTRY_POINT "VSTPluginMain"

#if VST_FORCE_DEPRECATED
#define DEPRECATED_VST_SYMBOL(x) __##x##Deprecated
#else
#define DEPRECATED_VST_SYMBOL(x) x
#endif

#ifdef VESTIGE
typedef AEffect *(VESTIGECALLBACK *VstEntry)(audioMasterCallback audioMaster);
#else
typedef AEffect *(VSTCALLBACK *VstEntry)(audioMasterCallback audioMaster);
#endif

#define disconnectserver 32143215

#define hidegui2 77775634

#define WM_SYNC (WM_USER + 1)
#define WM_SYNC2 (WM_USER + 2)
#define WM_SYNC3 (WM_USER + 3)
#define WM_SYNC4 (WM_USER + 4)
#define WM_SYNC5 (WM_USER + 5)
#define WM_SYNC6 (WM_USER + 6)
#define WM_SYNC7 (WM_USER + 7)

char libnamesync[4096];

int hcidx = 512000;

using namespace std;

class RemoteVSTServer : public RemotePluginServer {
  static LRESULT CALLBACK MainProc(HWND hWnd, UINT msg, WPARAM wParam,
                                   LPARAM lParam);
  static LRESULT CALLBACK MainProc2(HWND hWnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam);
  LRESULT CALLBACK realWndProc(HWND hWnd, UINT msg, WPARAM wParam,
                               LPARAM lParam);
  LRESULT CALLBACK realWndProc2(HWND hWnd, UINT msg, WPARAM wParam,
                                LPARAM lParam);

public:
  RemoteVSTServer(std::string fileIdentifiers, std::string fallbackName);
  virtual ~RemoteVSTServer();

  // virtual std::string getName() { return m_name; }
  // virtual std::string getMaker() { return m_maker; }
  virtual std::string getName();
  virtual std::string getMaker();

  virtual void setBufferSize(int);
  virtual void setSampleRate(int);
  virtual void reset();
  virtual void terminate();

  virtual int getInputCount() {
    if (m_plugin)
      return m_plugin->numInputs;
  }
  virtual int getOutputCount() {
    if (m_plugin)
      return m_plugin->numOutputs;
  }
  virtual int getFlags() {
    if (m_plugin)
      return m_plugin->flags;
  }
  virtual int getinitialDelay() {
    if (m_plugin)
      return m_plugin->initialDelay;
  }
  virtual int getUID() {
    if (m_plugin)
      return m_plugin->uniqueID;
  }
  virtual int getParameterCount() {
    if (m_plugin)
      return m_plugin->numParams;
  }
  virtual std::string getParameterName(int);
  virtual std::string getParameterLabel(int);
  virtual std::string getParameterDisplay(int);
  virtual void setParameter(int, float);
  virtual float getParameter(int);
  virtual void getParameters(int, int, float *);

  virtual int getProgramCount() {
    if (m_plugin)
      return m_plugin->numPrograms;
  }
  virtual int getProgramNameIndexed(int, char *name);
  virtual std::string getProgramName();
#ifdef WAVES
  virtual int getShellName(char *name);
#endif
  virtual void setCurrentProgram(int);

  virtual void showGUI(ShmControl *m_shmControlptr);
  virtual void hideGUI();
  virtual void hideGUI2();
#ifdef EMBED
  virtual void openGUI();
#endif
  virtual void guiUpdate();
  virtual void finisherror();

  virtual int getEffInt(int opcode, int value);
  virtual std::string getEffString(int opcode, int index);
  virtual void effDoVoid(int opcode);
  virtual int effDoVoid2(int opcode, int index, int value, float opt);

  //    virtual int         getInitialDelay() {return m_plugin->initialDelay;}
  //    virtual int         getUniqueID() { return m_plugin->uniqueID;}
  //    virtual int         getVersion() { return m_plugin->version;}

  virtual int processVstEvents();
  virtual void getChunk(ShmControl *m_shmControlptr);
  virtual void setChunk(ShmControl *m_shmControlptr);
  virtual void canBeAutomated(ShmControl *m_shmControlptr);
  virtual void getProgram(ShmControl *m_shmControlptr);
  virtual void EffectOpen(ShmControl *m_shmControlptr);
  //    virtual void        eff_mainsChanged(int v);

  virtual void process(float **inputs, float **outputs, int sampleFrames);
#ifdef DOUBLEP
  virtual void processdouble(double **inputs, double **outputs,
                             int sampleFrames);
  virtual bool setPrecision(int);
#endif

  virtual bool getOutProp(int, ShmControl *m_shmControlptr);
  virtual bool getInProp(int, ShmControl *m_shmControlptr);

#ifdef MIDIEFF
  virtual bool getMidiKey(int, ShmControl *m_shmControlptr);
  virtual bool getMidiProgName(int, ShmControl *m_shmControlptr);
  virtual bool getMidiCurProg(int, ShmControl *m_shmControlptr);
  virtual bool getMidiProgCat(int, ShmControl *m_shmControlptr);
  virtual bool getMidiProgCh(int, ShmControl *m_shmControlptr);
  virtual bool setSpeaker(ShmControl *m_shmControlptr);
  virtual bool getSpeaker(ShmControl *m_shmControlptr);
#endif
#ifdef CANDOEFF
  virtual bool getEffCanDo(std::string);
#endif

  virtual void setDebugLevel(RemotePluginDebugLevel level) {
    debugLevel = level;
  }

  virtual bool warn(std::string);

  virtual void waitForServer(ShmControl *m_shmControlptr);
  virtual void waitForServerexit();

  static VstIntPtr VESTIGECALLBACK hostCallback(AEffect *plugin,
                                                VstInt32 opcode, VstInt32 index,
                                                VstIntPtr value, void *ptr,
                                                float opt) {
    RemoteVSTServer *remote5;
    VstIntPtr rv = 0;

    switch (opcode) {
    case audioMasterVersion:
      rv = 2400;
      break;

    default: {
      if (plugin) {
        remote5 = (RemoteVSTServer *)plugin->resvd2;
        if (remote5) {
          rv = remote5->hostCallback2(plugin, opcode, index, value, ptr, opt);
        }
      }
    } break;
    }
    remote5 = nullptr;
    return rv;
  }

  VstIntPtr hostCallback2(AEffect *plugin, VstInt32 opcode, VstInt32 index,
                          VstIntPtr value, void *ptr, float opt);

  static DWORD WINAPI AudioThreadMain(void *parameter) {
    RemoteVSTServer *remote1 = (RemoteVSTServer *)parameter;

    while (!remote1->exiting) {
      remote1->dispatchProcess(50);
    }
    // param.sched_priority = 0;
    // (void)sched_setscheduler(0, SCHED_OTHER, &param);
    remote1->audfin = 1;
    remote1 = nullptr;
    //   ExitThread(0);
    return 0;
  }

  static DWORD WINAPI GetSetThreadMain(void *parameter) {
    RemoteVSTServer *remote2 = (RemoteVSTServer *)parameter;

    while (!remote2->exiting) {
      remote2->dispatchGetSet(50);
    }
    // param.sched_priority = 0;
    // (void)sched_setscheduler(0, SCHED_OTHER, &param);
    remote2->getfin = 1;
    remote2 = nullptr;
    //    ExitThread(0);
    return 0;
  }

  static DWORD WINAPI ParThreadMain(void *parameter) {
    RemoteVSTServer *remote3 = (RemoteVSTServer *)parameter;

    while (!remote3->exiting) {
      remote3->dispatchPar(50);
    }
    // param.sched_priority = 0;
    // (void)sched_setscheduler(0, SCHED_OTHER, &param);
    remote3->parfin = 1;
    remote3 = nullptr;
    //    ExitThread(0);
    return 0;
  }
  
  static DWORD WINAPI ControlThreadMain(void *parameter) {
    RemoteVSTServer *remote3 = (RemoteVSTServer *)parameter;

    while (!remote3->exiting) {
      remote3->dispatchControl2(50);
    }
    // param.sched_priority = 0;
    // (void)sched_setscheduler(0, SCHED_OTHER, &param);
    remote3->confin = 1;
    remote3 = nullptr;
    //    ExitThread(0);
    return 0;
  }  

  void StartThreadFunc() {
    DWORD ThreadID;
    ThreadHandle[0] = CreateThread(0, 0, AudioThreadMain, (LPVOID)this,
                                   CREATE_SUSPENDED, &ThreadID);
  }

  void StartThreadFunc2() {
    DWORD ThreadID;
    ThreadHandle[1] = CreateThread(0, 0, GetSetThreadMain, (void *)this,
                                   CREATE_SUSPENDED, &ThreadID);
  }

  void StartThreadFunc3() {
    DWORD ThreadID;
    ThreadHandle[2] = CreateThread(0, 0, ParThreadMain, (void *)this,
                                   CREATE_SUSPENDED, &ThreadID);
  }
  
  void StartThreadFunc4() {
    DWORD ThreadID;
    ThreadHandle[3] = CreateThread(0, 0, ControlThreadMain, (void *)this,
                                   CREATE_SUSPENDED, &ThreadID);
  }  

  HWND hWnd;
  WNDCLASSEX wclass;
#ifdef TRACKTIONWM
  WNDCLASSEX wclass2;
  POINT offset;
#endif
  HANDLE ThreadHandle[4];
  UINT_PTR timerval;
  bool haveGui;
#ifdef EMBED
  HANDLE handlewin;
  struct winmessage {
    int handle;
    int width;
    int height;
    int winerror;
  } winm2;
  winmessage *winm;
#endif
  int guiupdate;
  int guiupdatecount;
  int guiresizewidth;
  int guiresizeheight;
  ERect *rect;
  int setprogrammiss;
  int hostreaper;
  int melda;
  int wavesthread;
#ifdef EMBED
#ifdef TRACKTIONWM
  int hosttracktion;
#endif
#endif

  VstEvents vstev[VSTSIZE];
  bool exiting;
  bool effectrun;
  bool inProcessThread;
  bool guiVisible;
  int parfin;
  int audfin;
  int getfin;
  int confin;  
  int hidegui;

  std::string deviceName2;
  std::string bufferwaves;

  MSG msg;

  char wname[4096];
  char wname2[4096];

  int pidx;

  int plugerr;

  RemotePluginDebugLevel debugLevel;

  std::string m_name;
  std::string m_maker;

  HANDLE ghWriteEvent;
  HANDLE ghWriteEvent2;
  HANDLE ghWriteEvent3;
  HANDLE ghWriteEvent4;
  HANDLE ghWriteEvent5;
  HANDLE ghWriteEvent7;

public:
  HINSTANCE libHandle;
  AEffect *m_plugin;
};

struct threadargs {
  char fileInfo[4096];
  char libname[4096];
  int idx;
} threadargs2;

int finishloop = 1;
int plugincount = 0;
int realplugincount = 0;

DWORD threadIdvst[128000];

HANDLE ThreadHandlevst[128000];

RemoteVSTServer *remoteVSTServerInstance2[128000];

HWND hWndvst[128000];

UINT_PTR timerval[128000];

DWORD mainThreadId;

WNDCLASSEX wclass[128000];

RemoteVSTServer::RemoteVSTServer(std::string fileIdentifiers,
                                 std::string fallbackName)
    : RemotePluginServer(fileIdentifiers), m_name(fallbackName), m_maker(""),
      setprogrammiss(0), hostreaper(0),
#ifdef WAVES
      wavesthread(0),
#endif
#ifdef EMBED
#ifdef TRACKTIONWM
      hosttracktion(0),
#endif
#endif
      haveGui(true), timerval(0), exiting(false), effectrun(false),
      inProcessThread(false), guiVisible(false), parfin(0), audfin(0),
      getfin(0), confin(0), guiupdate(0), guiupdatecount(0), guiresizewidth(500),
      guiresizeheight(200), melda(0), winm(0), hWnd(0), hidegui(0),
      ghWriteEvent(0), ghWriteEvent2(0), ghWriteEvent3(0), ghWriteEvent4(0),
      ghWriteEvent5(0), ghWriteEvent7(0), libHandle(0), m_plugin(0), pidx(0),
      plugerr(0), debugLevel(RemotePluginDebugNone) {
  ThreadHandle[0] = 0;
  ThreadHandle[1] = 0;
  ThreadHandle[2] = 0;
  ThreadHandle[3] = 0;  

#ifdef EMBED
  /*
  winm = new winmessage;
  if(!winm)
  starterror = 1;
  */
  winm = &winm2;
#endif
}

LRESULT CALLBACK RemoteVSTServer::MainProc(HWND hWnd, UINT msg, WPARAM wParam,
                                           LPARAM lParam) {
  RemoteVSTServer *remote7;

  if (msg == WM_NCCREATE) {
    remote7 = (RemoteVSTServer *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)remote7);
    if (remote7)
      return remote7->realWndProc(hWnd, msg, wParam, lParam);
    else
      return DefWindowProc(hWnd, msg, wParam, lParam);
  } else {
    remote7 = (RemoteVSTServer *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (remote7)
      return remote7->realWndProc(hWnd, msg, wParam, lParam);
    else
      return DefWindowProc(hWnd, msg, wParam, lParam);
  }
}

LRESULT CALLBACK RemoteVSTServer::MainProc2(HWND hWnd, UINT msg, WPARAM wParam,
                                            LPARAM lParam) {
  RemoteVSTServer *remote8;

  if (msg == WM_NCCREATE) {
    remote8 = (RemoteVSTServer *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)remote8);
    if (remote8)
      return remote8->realWndProc2(hWnd, msg, wParam, lParam);
    else
      return DefWindowProc(hWnd, msg, wParam, lParam);
  } else {
    remote8 = (RemoteVSTServer *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (remote8)
      return remote8->realWndProc2(hWnd, msg, wParam, lParam);
    else
      return DefWindowProc(hWnd, msg, wParam, lParam);
  }
}

LRESULT CALLBACK RemoteVSTServer::realWndProc(HWND hWnd, UINT msg,
                                              WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_CLOSE:
#ifndef EMBED
    if (!exiting && guiVisible) {
      hidegui = 1;
      //         hideGUI();
      return 0;
    }
#endif
    break;

  case WM_TIMER:
    /*
             m_plugin->dispatcher ( m_plugin, effEditIdle, 0, 0, NULL, 0);
            //    printf("timer %d\n", pidx);
    */
    break;

  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

LRESULT CALLBACK RemoteVSTServer::realWndProc2(HWND hWnd, UINT msg,
                                               WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_CLOSE:
    break;

  case WM_TIMER:
    break;

  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

static VstIntPtr VESTIGECALLBACK hostCallback3(AEffect *plugin, VstInt32 opcode,
                                               VstInt32 index, VstIntPtr value,
                                               void *ptr, float opt) {
  RemoteVSTServer *remote5;
  VstIntPtr rv = 0;

  if (plugin) {
    if (!plugin->resvd2) {
      if (hcidx != 512000) {
        if (remoteVSTServerInstance2[hcidx])
          plugin->resvd2 = remoteVSTServerInstance2[hcidx];
      }
    }
  }

  switch (opcode) {
  case audioMasterVersion:
    rv = 2400;
    break;

  default: {
    if (plugin) {
      remote5 = (RemoteVSTServer *)plugin->resvd2;
      if (remote5) {
        rv = remote5->hostCallback2(plugin, opcode, index, value, ptr, opt);
      }
    }
  } break;
  }
  remote5 = nullptr;
  return rv;
}

std::string RemoteVSTServer::getName() {
  char buffer[512];
  memset(buffer, 0, sizeof(buffer));
  m_plugin->dispatcher(m_plugin, effGetEffectName, 0, 0, buffer, 0);
  if (buffer[0])
    m_name = buffer;
  return m_name;
}

std::string RemoteVSTServer::getMaker() {
  char buffer[512];
  memset(buffer, 0, sizeof(buffer));
  m_plugin->dispatcher(m_plugin, effGetVendorString, 0, 0, buffer, 0);
  if (buffer[0])
    m_maker = buffer;
  return m_maker;
}

void RemoteVSTServer::EffectOpen(ShmControl *m_shmControlptr) {
  DWORD dwWaitResult;

  if (debugLevel > 0)
    cerr << "dssi-vst-server[1]: opening plugin" << endl;

  sched_yield();
  ghWriteEvent4 = 0;
  ghWriteEvent4 = CreateEvent(NULL, TRUE, FALSE, "wm_syncevent4");
  while (0 ==
         PostThreadMessage(mainThreadId, WM_SYNC4, (WPARAM)pidx, (LPARAM)wname))
    sched_yield();
  dwWaitResult = WaitForSingleObject(ghWriteEvent4, 20000);
  CloseHandle(ghWriteEvent4);
  sched_yield();

  //    m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 0, NULL, 0);

  m_plugin->dispatcher(m_plugin, effSetBlockSize, 0, bufferSize, NULL, 0);
  m_plugin->dispatcher(m_plugin, effSetSampleRate, 0, 0, NULL,
                       (float)sampleRate);

  char buffer[512];
  memset(buffer, 0, sizeof(buffer));

  string buffer2 = getMaker();
  strcpy(buffer, buffer2.c_str());

  /*
      if (strncmp(buffer, "Guitar Rig 5", 12) == 0)
          setprogrammiss = 1;
      if (strncmp(buffer, "T-Rack", 6) == 0)
          setprogrammiss = 1;
  */

  /*

      if(strcmp("MeldaProduction", buffer) == 0)
      {
      melda = 1;
      wavesthread = 1;
      }

  */

#ifdef WAVES
  if (strcmp("Waves", buffer) == 0) {
    m_plugin->flags |= effFlagsHasEditor;
    haveGui = true;
    wavesthread = 1;
    bufferwaves = buffer;
  }

  m_shmControlptr->retint = wavesthread;
#endif
  /*
      if (strncmp(buffer, "IK", 2) == 0)
          setprogrammiss = 1;
  */

#ifdef TRACKTIONWM
  if (haveGui == true) {

    offset.x = 0;
    offset.y = 0;

    memset(&wclass2, 0, sizeof(WNDCLASSEX));
    wclass2.cbSize = sizeof(WNDCLASSEX);
    wclass2.style = 0;
    // CS_HREDRAW | CS_VREDRAW;
    wclass2.lpfnWndProc = DefWindowProc;
    wclass2.cbClsExtra = 0;
    wclass2.cbWndExtra = 0;
    wclass2.hInstance = GetModuleHandle(0);
    wclass2.hIcon = LoadIcon(GetModuleHandle(0), APPLICATION_CLASS_NAME2);
    wclass2.hCursor = LoadCursor(0, IDI_APPLICATION);
    // wclass2.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wclass2.lpszMenuName = "MENU_DSSI_VST2";
    wclass2.lpszClassName = APPLICATION_CLASS_NAME2;
    wclass2.hIconSm = 0;

    if (!RegisterClassEx(&wclass2)) {
      UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
      guiVisible = false;
      haveGui = false;
      return;
    }

    RECT offsetcl, offsetwin;

    HWND hWnd2 = CreateWindow(APPLICATION_CLASS_NAME2, "LinVst", WS_CAPTION, 0,
                              0, 200, 200, 0, 0, GetModuleHandle(0), 0);
    GetClientRect(hWnd2, &offsetcl);
    GetWindowRect(hWnd2, &offsetwin);
    DestroyWindow(hWnd2);

    offset.x = (offsetwin.right - offsetwin.left) - offsetcl.right;
    offset.y = (offsetwin.bottom - offsetwin.top) - offsetcl.bottom;

    UnregisterClassA(APPLICATION_CLASS_NAME2, GetModuleHandle(0));
  }
#endif

  struct amessage am;

  //      am.pcount = m_plugin->numPrograms;
  //     am.parcount = m_plugin->numParams;
  am.incount = m_plugin->numInputs;
  am.outcount = m_plugin->numOutputs;
  am.delay = m_plugin->initialDelay;
  /*
  #ifndef DOUBLEP
          am.flags = m_plugin->flags;
          am.flags &= ~effFlagsCanDoubleReplacing;
  #else
          am.flags = m_plugin->flags;
  #endif
  */

  if ((am.incount != m_numInputs) || (am.outcount != m_numOutputs) ||
      (am.delay != m_delay)) {
    memcpy(m_shmControl->amptr, &am, sizeof(am));
    m_shmControl->ropcode = (RemotePluginOpcode)audioMasterIOChanged;
    waitForServer(m_shmControl);
  }

  // m_plugin->dispatcher(m_plugin, effMainsChanged, 0, 1, NULL, 0);

  effectrun = true;
}

RemoteVSTServer::~RemoteVSTServer() {
  DWORD dwWaitResult;

  sched_yield();
  ghWriteEvent5 = 0;
  ghWriteEvent5 = CreateEvent(NULL, TRUE, FALSE, "wm_syncevent5");
  while (0 ==
         PostThreadMessage(mainThreadId, WM_SYNC5, (WPARAM)pidx, (LPARAM)wname))
    sched_yield();
  dwWaitResult = WaitForSingleObject(ghWriteEvent5, 20000);
  CloseHandle(ghWriteEvent5);
  sched_yield();

  //waitForServerexit();

  waitForClient2exit();
  waitForClient3exit();
  waitForClient4exit();
  waitForClient5exit();
  waitForClient6exit();

  sched_yield();

  /*
  #ifdef EMBED
      if (winm)
      delete winm;
  #endif
  */
}

void RemoteVSTServer::process(float **inputs, float **outputs,
                              int sampleFrames) {
  inProcessThread = true;
  m_plugin->processReplacing(m_plugin, inputs, outputs, sampleFrames);
  inProcessThread = false;
}

#ifdef DOUBLEP
void RemoteVSTServer::processdouble(double **inputs, double **outputs,
                                    int sampleFrames) {
  inProcessThread = true;
  m_plugin->processDoubleReplacing(m_plugin, inputs, outputs, sampleFrames);
  inProcessThread = false;
}

bool RemoteVSTServer::setPrecision(int value) {
  bool retval;

  retval =
      m_plugin->dispatcher(m_plugin, effSetProcessPrecision, 0, value, 0, 0);

  return retval;
}
#endif

#ifdef VESTIGE
bool RemoteVSTServer::getOutProp(int index, ShmControl *m_shmControlptr) {
  char ptr[sizeof(vinfo)];
  bool retval;

  retval =
      m_plugin->dispatcher(m_plugin, effGetOutputProperties, index, 0, &ptr, 0);
  memcpy(m_shmControlptr->vret, ptr, sizeof(vinfo));
  return retval;
}

bool RemoteVSTServer::getInProp(int index, ShmControl *m_shmControlptr) {
  char ptr[sizeof(vinfo)];
  bool retval;

  retval =
      m_plugin->dispatcher(m_plugin, effGetInputProperties, index, 0, &ptr, 0);
  memcpy(m_shmControlptr->vret, ptr, sizeof(vinfo));
  return retval;
}
#else
bool RemoteVSTServer::getInProp(int index, ShmControl *m_shmControlptr) {
  VstPinProperties ptr;
  bool retval;

  retval =
      m_plugin->dispatcher(m_plugin, effGetInputProperties, index, 0, &ptr, 0);
  memcpy(m_shmControlptr->vpin, &ptr, sizeof(VstPinProperties));
  return retval;
}

bool RemoteVSTServer::getOutProp(int index, ShmControl *m_shmControlptr) {
  VstPinProperties ptr;
  bool retval;

  retval =
      m_plugin->dispatcher(m_plugin, effGetOutputProperties, index, 0, &ptr, 0);
  memcpy(m_shmControlptr->vpin, &ptr, sizeof(VstPinProperties));
  return retval;
}
#endif

#ifdef MIDIEFF
bool RemoteVSTServer::getMidiKey(int index, ShmControl *m_shmControlptr) {
  MidiKeyName ptr;
  bool retval;

  retval = m_plugin->dispatcher(m_plugin, effGetMidiKeyName, index, 0, &ptr, 0);
  memcpy(m_shmControlptr->midikey, &ptr, sizeof(MidiKeyName));
  return retval;
}

bool RemoteVSTServer::getMidiProgName(int index, ShmControl *m_shmControlptr) {
  MidiProgramName ptr;
  bool retval;

  retval =
      m_plugin->dispatcher(m_plugin, effGetMidiProgramName, index, 0, &ptr, 0);
  memcpy(m_shmControlptr->midiprogram, &ptr, sizeof(MidiProgramName));
  return retval;
}

bool RemoteVSTServer::getMidiCurProg(int index, ShmControl *m_shmControlptr) {
  MidiProgramName ptr;
  bool retval;

  retval = m_plugin->dispatcher(m_plugin, effGetCurrentMidiProgram, index, 0,
                                &ptr, 0);
  memcpy(m_shmControlptr->midiprogram, &ptr, sizeof(MidiProgramName));
  return retval;
}

bool RemoteVSTServer::getMidiProgCat(int index, ShmControl *m_shmControlptr) {
  MidiProgramCategory ptr;
  bool retval;

  retval = m_plugin->dispatcher(m_plugin, effGetMidiProgramCategory, index, 0,
                                &ptr, 0);
  memcpy(m_shmControlptr->midiprogramcat, &ptr, sizeof(MidiProgramCategory));
  return retval;
}

bool RemoteVSTServer::getMidiProgCh(int index, ShmControl *m_shmControlptr) {
  bool retval;

  retval =
      m_plugin->dispatcher(m_plugin, effHasMidiProgramsChanged, index, 0, 0, 0);
  return retval;
}

bool RemoteVSTServer::setSpeaker(ShmControl *m_shmControlptr) {
  VstSpeakerArrangement ptr;
  VstSpeakerArrangement value;
  bool retval;

  memcpy(&ptr, m_shmControlptr->vstspeaker2, sizeof(VstSpeakerArrangement));
  memcpy(&value, m_shmControlptr->vstspeaker, sizeof(VstSpeakerArrangement));
  retval = m_plugin->dispatcher(m_plugin, effSetSpeakerArrangement, 0,
                                (VstIntPtr)&value, &ptr, 0);
  return retval;
}

bool RemoteVSTServer::getSpeaker(ShmControl *m_shmControlptr) {
  VstSpeakerArrangement ptr;
  VstSpeakerArrangement value;
  bool retval;

  retval = m_plugin->dispatcher(m_plugin, effSetSpeakerArrangement, 0,
                                (VstIntPtr)&value, &ptr, 0);
  memcpy(m_shmControlptr->vstspeaker2, &ptr, sizeof(VstSpeakerArrangement));
  memcpy(m_shmControlptr->vstspeaker, &value, sizeof(VstSpeakerArrangement));
  return retval;
}
#endif

#ifdef CANDOEFF
bool RemoteVSTServer::getEffCanDo(std::string ptr) {
  if (m_plugin->dispatcher(m_plugin, effCanDo, 0, 0, (char *)ptr.c_str(), 0))
    return true;
  else
    return false;
}
#endif

int RemoteVSTServer::getEffInt(int opcode, int value) {
  return m_plugin->dispatcher(m_plugin, opcode, 0, value, NULL, 0);
}

void RemoteVSTServer::effDoVoid(int opcode) {
  if (opcode == 78345432) {
    //        hostreaper = 1;
    return;
  }

  if (opcode == effClose) {
    waitForServerexit();

    // usleep(500000);
    terminate();
    return;
  }

  m_plugin->dispatcher(m_plugin, opcode, 0, 0, NULL, 0);
}

int RemoteVSTServer::effDoVoid2(int opcode, int index, int value, float opt) {
  int ret;

  ret = 0;
  /*
      if(opcode == hidegui2)
      {
      hidegui = 1;
  #ifdef XECLOSE
      while(hidegui == 1)
      {
      sched_yield();
      }
  #endif
      }
      else
   */

#ifdef EMBED
#ifdef TRACKTIONWM
  if (opcode == 67584930) {
    hosttracktion = 1;
    return offset.y;
  }
  if (opcode == 67584931) {
    return offset.x;
  }  
#endif
#endif

  ret = m_plugin->dispatcher(m_plugin, opcode, index, value, NULL, opt);
  return ret;
}

std::string RemoteVSTServer::getEffString(int opcode, int index) {
  char name[512];
  memset(name, 0, sizeof(name));

  m_plugin->dispatcher(m_plugin, opcode, index, 0, name, 0);
  return name;
}

void RemoteVSTServer::setBufferSize(int sz) {
  if (bufferSize != sz) {
    //    m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 0, NULL, 0);
    m_plugin->dispatcher(m_plugin, effSetBlockSize, 0, sz, NULL, 0);
    //   m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 1, NULL, 0);
    bufferSize = sz;
  }

  if (debugLevel > 0)
    cerr << "dssi-vst-server[1]: set buffer size to " << sz << endl;
}

void RemoteVSTServer::setSampleRate(int sr) {
  if (sampleRate != sr) {
    //    m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 0, NULL, 0);
    m_plugin->dispatcher(m_plugin, effSetSampleRate, 0, 0, NULL, (float)sr);
    //    m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 1, NULL, 0);
    sampleRate = sr;
  }

  if (debugLevel > 0)
    cerr << "dssi-vst-server[1]: set sample rate to " << sr << endl;
}

void RemoteVSTServer::reset() {
  cerr << "dssi-vst-server[1]: reset" << endl;

  //  m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 0, NULL, 0);
  //  m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 1, NULL, 0);
}

void RemoteVSTServer::terminate() {
  exiting = true;

  //  cerr << "RemoteVSTServer::terminate: setting exiting flag" << endl;
}

std::string RemoteVSTServer::getParameterName(int p) {
  char name[512];
  memset(name, 0, sizeof(name));

  m_plugin->dispatcher(m_plugin, effGetParamName, p, 0, name, 0);
  return name;
}

std::string RemoteVSTServer::getParameterLabel(int p) {
  char name[512];
  memset(name, 0, sizeof(name));

  m_plugin->dispatcher(m_plugin, effGetParamLabel, p, 0, name, 0);
  return name;
}

std::string RemoteVSTServer::getParameterDisplay(int p) {
  char name[512];
  memset(name, 0, sizeof(name));

  m_plugin->dispatcher(m_plugin, effGetParamDisplay, p, 0, name, 0);
  return name;
}

void RemoteVSTServer::setParameter(int p, float v) {
  m_plugin->setParameter(m_plugin, p, v);
}

float RemoteVSTServer::getParameter(int p) {
  return m_plugin->getParameter(m_plugin, p);
}

void RemoteVSTServer::getParameters(int p0, int pn, float *v) {
  for (int i = p0; i <= pn; ++i)
    v[i - p0] = m_plugin->getParameter(m_plugin, i);
}

int RemoteVSTServer::getProgramNameIndexed(int p, char *name) {
  if (debugLevel > 1)
    cerr << "dssi-vst-server[2]: getProgramName(" << p << ")" << endl;

  int retval = 0;
  char nameret[512];
  memset(nameret, 0, sizeof(nameret));

  retval = m_plugin->dispatcher(m_plugin, effGetProgramNameIndexed, p, 0,
                                nameret, 0);
  strcpy(name, nameret);
  return retval;
}

std::string RemoteVSTServer::getProgramName() {
  if (debugLevel > 1)
    cerr << "dssi-vst-server[2]: getProgramName()" << endl;

  char name[512];
  memset(name, 0, sizeof(name));

  m_plugin->dispatcher(m_plugin, effGetProgramName, 0, 0, name, 0);
  return name;
}

#ifdef WAVES
int RemoteVSTServer::getShellName(char *name) {
  int retval = 0;
  char nameret[512];

  if (debugLevel > 1)
    cerr << "dssi-vst-server[2]: getProgramName()" << endl;

  memset(nameret, 0, sizeof(nameret));
  retval =
      m_plugin->dispatcher(m_plugin, effShellGetNextPlugin, 0, 0, nameret, 0);
  strcpy(name, nameret);
  return retval;
}
#endif

void RemoteVSTServer::setCurrentProgram(int p) {
  if (debugLevel > 1)
    cerr << "dssi-vst-server[2]: setCurrentProgram(" << p << ")" << endl;

  /*
      if ((hostreaper == 1) && (setprogrammiss == 1))
      {
          writeIntring(&m_shmControl5->ringBuffer, 1);
          return;
      }
  */

  if (p < m_plugin->numPrograms)
    m_plugin->dispatcher(m_plugin, effSetProgram, 0, p, 0, 0);
}

bool RemoteVSTServer::warn(std::string warning) {
  if (hWnd)
    MessageBox(hWnd, warning.c_str(), "Error", 0);
  return true;
}

void RemoteVSTServer::showGUI(ShmControl *m_shmControlptr) {
  DWORD dwWaitResult;

  winm->winerror = 0;

  if ((haveGui == false) || (guiVisible == true)) {
    winm->handle = 0;
    winm->width = 0;
    winm->height = 0;
    memcpy(m_shmControlptr->wret, winm, sizeof(winmessage));
    return;
  }

  memset(wname, 0, 4096);
  memset(wname2, 0, 4096);

  sprintf(wname2, "%d", pidx);
  strcpy(wname, m_name.c_str());
  strcat(wname, wname2);

  ghWriteEvent = 0;

  ghWriteEvent = CreateEvent(NULL, TRUE, FALSE, "wm_syncevent");

  while (0 ==
         PostThreadMessage(mainThreadId, WM_SYNC, (WPARAM)pidx, (LPARAM)wname))
    sched_yield();

  dwWaitResult = WaitForSingleObject(ghWriteEvent, 20000);
  //     dwWaitResult = MsgWaitForMultipleObjects(1, &ghWriteEvent, TRUE, 20000,
  //     QS_ALLEVENTS);

  sched_yield();

  CloseHandle(ghWriteEvent);
    
  if (dwWaitResult != WAIT_OBJECT_0) {
  cerr << "dssi-vst-server: ERROR: Failed to create window!\n" << endl;
  guiVisible = false;
  winm->handle = 0;
  winm->width = 0;
  winm->height = 0;
  memcpy(m_shmControlptr->wret, winm, sizeof(winmessage));
  }    
   
  if(winm->winerror == 1) {
  cerr << "dssi-vst-server: ERROR: Failed to create window2!\n" << endl;      
  guiVisible = false;     
  winm->handle = 0;
  winm->width = 0;
  winm->height = 0;
  memcpy(m_shmControlptr->wret, winm, sizeof(winmessage));
  return;
  }  

  sched_yield();
}

void RemoteVSTServer::hideGUI2() {
  hidegui = 1;
  //#ifdef XECLOSE
  while (hidegui == 1) {
    sched_yield();
  }
  //#endif
}

void RemoteVSTServer::hideGUI() {
  DWORD dwWaitResult;

  if ((haveGui == false) || (guiVisible == false))
    return;

  sched_yield();
  ghWriteEvent2 = 0;
  ghWriteEvent2 = CreateEvent(NULL, TRUE, FALSE, "wm_syncevent2");

  while (0 == PostThreadMessage(mainThreadId, WM_SYNC2, (WPARAM)pidx, 0))
    sched_yield();

  dwWaitResult = WaitForSingleObject(ghWriteEvent2, 20000);
  //     dwWaitResult = MsgWaitForMultipleObjects(1, &ghWriteEvent2, TRUE,
  //     20000, QS_ALLEVENTS);

  sched_yield();
  CloseHandle(ghWriteEvent2);

  guiVisible = false;
  hidegui = 0;

  sched_yield();
}

#ifdef EMBED
void RemoteVSTServer::openGUI() {
  DWORD dwWaitResult;

  sched_yield();
  ghWriteEvent3 = 0;
  ghWriteEvent3 = CreateEvent(NULL, TRUE, FALSE, "wm_syncevent3");

  while (0 ==
         PostThreadMessage(mainThreadId, WM_SYNC3, (WPARAM)pidx, (LPARAM)wname))
    sched_yield();

  dwWaitResult = WaitForSingleObject(ghWriteEvent3, 20000);
  //    dwWaitResult = MsgWaitForMultipleObjects(1, &ghWriteEvent3, TRUE, 20000,
  //    QS_ALLEVENTS);

  sched_yield();
  CloseHandle(ghWriteEvent3);

  guiVisible = true;
  sched_yield();
}
#endif

int RemoteVSTServer::processVstEvents() {
  int els;
  int *ptr;
  int sizeidx = 0;
  int size;
  VstEvents *evptr;

  ptr = (int *)m_shm2;
  els = *ptr;
  sizeidx = sizeof(int);

  if (els > VSTSIZE)
    els = VSTSIZE;

  evptr = &vstev[0];
  evptr->numEvents = els;
  evptr->reserved = 0;

  for (int i = 0; i < els; i++) {
    VstEvent *bsize = (VstEvent *)&m_shm2[sizeidx];
    size = bsize->byteSize + (2 * sizeof(VstInt32));
    evptr->events[i] = bsize;
    sizeidx += size;
  }

  m_plugin->dispatcher(m_plugin, effProcessEvents, 0, 0, evptr, 0);

  return 1;
}

void RemoteVSTServer::getChunk(ShmControl *m_shmControlptr) {
#ifdef CHUNKBUF
  int bnk_prg = m_shmControlptr->value;
  int sz =
      m_plugin->dispatcher(m_plugin, effGetChunk, bnk_prg, 0, &chunkptr, 0);

  if (sz >= CHUNKSIZEMAX) {
    m_shmControlptr->retint = sz;
    return;
  } else {
    if (sz < CHUNKSIZEMAX)
      memcpy(m_shm3, chunkptr, sz);
    m_shmControlptr->retint = sz;
    return;
  }
#else
  void *ptr;
  int bnk_prg = m_shmControlptr->value;
  int sz = m_plugin->dispatcher(m_plugin, effGetChunk, bnk_prg, 0, &ptr, 0);
  if (sz < CHUNKSIZEMAX)
    memcpy(m_shm3, ptr, sz);
  m_shmControlptr->retint = sz;
  return;
#endif
}

void RemoteVSTServer::setChunk(ShmControl *m_shmControlptr) {
#ifdef CHUNKBUF
  int sz = m_shmControlptr->value;
  if (sz >= CHUNKSIZEMAX) {
    int bnk_prg = m_shmControlptr->value2;
    void *ptr = chunkptr2;
    int r = m_plugin->dispatcher(m_plugin, effSetChunk, bnk_prg, sz, ptr, 0);
    free(chunkptr2);
    m_shmControlptr->retint = r;
    return;
  } else {
    int bnk_prg = m_shmControlptr->value2;
    void *ptr = m_shm3;
    int r = m_plugin->dispatcher(m_plugin, effSetChunk, bnk_prg, sz, ptr, 0);
    m_shmControlptr->retint = r;
    return;
  }
#else
  int sz = m_shmControlptr->value;
  int bnk_prg = m_shmControlptr->value2;
  void *ptr = m_shm3;
  int r = m_plugin->dispatcher(m_plugin, effSetChunk, bnk_prg, sz, ptr, 0);
  m_shmControlptr->retint = r;
  return;
#endif
}

void RemoteVSTServer::canBeAutomated(ShmControl *m_shmControlptr) {
  int param = m_shmControlptr->value;
  int r = m_plugin->dispatcher(m_plugin, effCanBeAutomated, param, 0, 0, 0);
  m_shmControlptr->retint = r;
}

void RemoteVSTServer::getProgram(ShmControl *m_shmControlptr) {
  int r = m_plugin->dispatcher(m_plugin, effGetProgram, 0, 0, 0, 0);
  m_shmControlptr->retint = r;
}

void RemoteVSTServer::waitForServer(ShmControl *m_shmControlptr) {
  fpost2(m_shmControlptr, &m_shmControlptr->runServer);

  if (fwait2(m_shmControlptr, &m_shmControlptr->runClient, 60000)) {
    if (m_inexcept == 0)
      RemotePluginClosedException();
  }
}

void RemoteVSTServer::waitForServerexit() {
  fpost(m_shmControl, &m_shmControl->runServer);
  fpost(m_shmControl, &m_shmControl->runClient);
}

#ifdef VESTIGE
VstIntPtr RemoteVSTServer::hostCallback2(AEffect *plugin, VstInt32 opcode,
                                         VstInt32 index, VstIntPtr value,
                                         void *ptr, float opt)
#else
VstIntPtr RemoteVSTServer::hostCallback2(AEffect *plugin, VstInt32 opcode,
                                         VstInt32 index, VstIntPtr value,
                                         void *ptr, float opt)
#endif
{
  VstIntPtr rv = 0;
  int retval = 0;

  m_shmControlptr = m_shmControl;

  switch (opcode) {
  case audioMasterVersion:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterVersion requested" << endl;
    rv = 2400;
    break;

  case audioMasterAutomate:
    //     plugin->setParameter(plugin, index, opt);
    if (!exiting && effectrun) {
      m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
      m_shmControlptr->value = index;
      m_shmControlptr->floatvalue = opt;
      waitForServer(m_shmControlptr);
      retval = 0;
      retval = m_shmControlptr->retint;
      rv = retval;
    }
    break;

  case audioMasterGetAutomationState:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetAutomationState requested"
           << endl;

    if (!exiting && effectrun) {
      m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
      waitForServer(m_shmControlptr);
      retval = 0;
      retval = m_shmControlptr->retint;
      rv = retval;
    }
    //     rv = 4; // read/write
    break;

  case audioMasterBeginEdit:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterBeginEdit requested" << endl;

    if (!exiting && effectrun) {
      m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
      m_shmControlptr->value = index;
      waitForServer(m_shmControlptr);
      retval = 0;
      retval = m_shmControlptr->retint;
      rv = retval;
    }
    break;

  case audioMasterEndEdit:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterEndEdit requested" << endl;

    if (!exiting && effectrun) {
      m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
      m_shmControlptr->value = index;
      waitForServer(m_shmControlptr);
      retval = 0;
      retval = m_shmControlptr->retint;
      rv = retval;
    }
    break;

  case audioMasterCurrentId:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterCurrentId requested" << endl;
#ifdef WAVES
    if (!exiting) {
      m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
      waitForServer(m_shmControlptr);
      retval = 0;
      retval = m_shmControlptr->retint;
      rv = retval;
    }
#endif
    break;

  case audioMasterIdle:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterIdle requested " << endl;
    // plugin->dispatcher(plugin, effEditIdle, 0, 0, 0, 0);
    break;

  case audioMasterSetTime:
    if (!exiting && effectrun) {
      memcpy(m_shmControlptr->timeset, ptr, sizeof(VstTimeInfo));

      m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
      waitForServer(m_shmControlptr);
      retval = 0;
      retval = m_shmControlptr->retint;
      rv = retval;
    }
    break;

  case audioMasterGetTime:
    if (!exiting && effectrun) {
      memcpy(timeinfo, m_shmControlptr->timeget, sizeof(VstTimeInfo));

      // printf("%f\n", timeinfo->sampleRate);

      if (m_shmControlptr->timeinit)
        rv = (VstIntPtr)timeinfo;
    }
    break;

  case audioMasterProcessEvents:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterProcessEvents requested" << endl;
    {
      VstEvents *evnts;
      int eventnum;
      int *ptr2;
      int sizeidx = 0;
      int ok;

      if (!exiting && effectrun) {
        evnts = (VstEvents *)ptr;

        if (!evnts) {
          break;
        }

        if (evnts->numEvents <= 0) {
          break;
        }

        eventnum = evnts->numEvents;

        ptr2 = (int *)&m_shm2[FIXED_SHM_SIZE2SEND];

        sizeidx = sizeof(int);

        if (eventnum > VSTSIZE)
          eventnum = VSTSIZE;

        for (int i = 0; i < eventnum; i++) {
          VstEvent *pEvent = evnts->events[i];
          if (pEvent->type == kVstSysExType)
            eventnum--;
          else {
            unsigned int size =
                (2 * sizeof(VstInt32)) + evnts->events[i]->byteSize;
            memcpy(&m_shm3[FIXED_SHM_SIZE2SEND + sizeidx], evnts->events[i],
                   size);
            sizeidx += size;
          }
        }
        *ptr2 = eventnum;

        m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
        waitForServer(m_shmControlptr);
        retval = 0;
        retval = m_shmControlptr->retint;
        rv = retval;
      }
    }
    break;

  case audioMasterIOChanged: {
    struct amessage am;

    if (!exiting && effectrun) {
      //   am.pcount = plugin->numPrograms;
      //   am.parcount = plugin->numParams;
      am.incount = plugin->numInputs;
      am.outcount = plugin->numOutputs;
      am.delay = plugin->initialDelay;
      /*
      #ifndef DOUBLEP
                      am.flags = plugin->flags;
                      am.flags &= ~effFlagsCanDoubleReplacing;
      #else
                      am.flags = plugin->flags;
      #endif
      */

      if ((am.incount != m_numInputs) || (am.outcount != m_numOutputs) ||
          (am.delay != m_delay)) {
        /*
 if((am.incount != m_numInputs) || (am.outcount != m_numOutputs))
 {
 if ((am.incount + am.outcount) * m_bufferSize * sizeof(float) < (PROCESSSIZE))
        }
*/

        if (am.delay != m_delay)
          m_delay = am.delay;

        memcpy(m_shmControlptr->amptr, &am, sizeof(am));

        m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
        waitForServer(m_shmControlptr);
        retval = 0;
        retval = m_shmControlptr->retint;
        rv = retval;
        // }
      }
      /*
                      if((am.incount != m_numInputs) || (am.outcount !=
         m_numOutputs))
                      {
                      if ((am.incount + am.outcount) * m_bufferSize *
         sizeof(float) < (PROCESSSIZE))
                      {
                      m_updateio = 1;
                      m_updatein = am.incount;
                      m_updateout = am.outcount;
                      }
                      }
          */
      /*
              AEffect* update = m_plugin;
              update->flags = am.flags;
              update->numPrograms = am.pcount;
              update->numParams = am.parcount;
              update->numInputs = am.incount;
              update->numOutputs = am.outcount;
              update->initialDelay = am.delay;
      */
    }
  } break;

  case audioMasterUpdateDisplay:
    /*
           if (debugLevel > 1)
               cerr << "dssi-vst-server[2]: audioMasterUpdateDisplay requested"
       << endl;

       if (!exiting && effectrun)
       {
       m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
       waitForServer(m_shmControlptr);
       retval = 0;
       retval = m_shmControlptr->retint;
       rv = retval;
        }
    */
    break;

  case audioMasterSizeWindow:
  
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterSizeWindow requested" << endl;

    break;

  case audioMasterGetProductString:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetProductString requested"
           << endl;
    {
      char retstr[512];

      if (!exiting) {
        m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
        waitForServer(m_shmControlptr);
        strcpy(retstr, m_shmControlptr->retstr);
        strcpy((char *)ptr, retstr);
        retval = 0;
        retval = m_shmControlptr->retint;
        rv = retval;
      }
    }
    break;

  case audioMasterGetVendorVersion:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetVendorVersion requested"
           << endl;

    if (!exiting) {
      m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
      waitForServer(m_shmControlptr);
      retval = 0;
      retval = m_shmControlptr->retint;
      rv = retval;
    }
    break;

  case audioMasterCanDo:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterCanDo(" << (char *)ptr
           << ") requested" << endl;
#ifdef CANDOEFF
    {
      int retval;

      if (remoteVSTServerInstance && !exiting) {
        m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
        strcpy(m_shmControlptr->retstr, (char *)ptr);
        waitForServer(m_shmControlptr);

        memcpy(&retval, m_shmControlptr->retint, sizeof(int));
        rv = retval;
      }
    }
#else
    if (!strcmp((char *)ptr, "sendVstEvents") ||
        !strcmp((char *)ptr, "sendVstMidiEvent") ||
        !strcmp((char *)ptr, "receiveVstEvents") ||
        !strcmp((char *)ptr, "receiveVstMidiEvents") ||
        !strcmp((char *)ptr, "receiveVstTimeInfo")
#ifdef WAVES
        || !strcmp((char *)ptr, "shellCategory") ||
        !strcmp((char *)ptr, "supportShell")
#endif
        || !strcmp((char *)ptr, "acceptIOChanges") ||
        !strcmp((char *)ptr, "startStopProcess")
#ifdef EMBED
#ifdef EMBEDRESIZE
        || !strcmp((char *)ptr, "sizeWindow")
#endif
#else
        || !strcmp((char *)ptr, "sizeWindow")
#endif
        // || !strcmp((char*)ptr, "supplyIdle")
    )
      rv = 1;
#endif
    break;

  case audioMasterGetSampleRate:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetSampleRate requested" << endl;
    /*
            if (!exiting && effectrun)
            {
            if (!sampleRate)
            {
                //  cerr << "WARNING: Sample rate requested but not yet set" <<
       endl; break;
            }
            plugin->dispatcher(plugin, effSetSampleRate, 0, 0, NULL,
       (float)sampleRate);
            }
    */
    /*
        if (!exiting && effectrun)
        {
        m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
        waitForServer(m_shmControlptr);
        retval = 0;
        retval = m_shmControlptr->retint;
        rv = retval;
          }
        */

    if (!exiting && effectrun) {
      rv = sampleRate;
    }
    break;

  case audioMasterGetBlockSize:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetBlockSize requested" << endl;
    /*
            if (!exiting && effectrun)
            {
            if (!bufferSize)
            {
                // cerr << "WARNING: Buffer size requested but not yet set" <<
       endl; break;
            }
            plugin->dispatcher(plugin, effSetBlockSize, 0, bufferSize, NULL, 0);
            }
    */
    /*
        if (!exiting && effectrun)
        {
        m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
        waitForServer(m_shmControlptr);
        retval = 0;
        retval = m_shmControlptr->retint;
        rv = retval;
          }
     */

    if (!exiting && effectrun) {
      rv = bufferSize;
    }
    break;

  case audioMasterGetInputLatency:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetInputLatency requested"
           << endl;
    /*

        {
        if (!exiting && effectrun)
        {
        m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
        waitForServer(m_shmControlptr);
        retval = 0;
        retval = m_shmControlptr->retint;
        rv = retval;
          }
         }
    */
    break;

  case audioMasterGetOutputLatency:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetOutputLatency requested"
           << endl;
    /*
        if (!exiting && effectrun)
        {
        m_shmControlptr->ropcode = (RemotePluginOpcode)opcode;
        waitForServer(m_shmControlptr);
        retval = 0;
        retval = m_shmControlptr->retint;
        rv = retval;
          }
    */
    break;

  case audioMasterGetCurrentProcessLevel:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetCurrentProcessLevel requested"
           << endl;
    // 0 -> unsupported, 1 -> gui, 2 -> process, 3 -> midi/timer, 4 -> offline

    if (!exiting && effectrun) {
      if (inProcessThread)
        rv = 2;
      else
        rv = 1;
    }
    break;

  case audioMasterGetLanguage:
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetLanguage requested" << endl;
    rv = kVstLangEnglish;
    break;

  case DEPRECATED_VST_SYMBOL(audioMasterWillReplaceOrAccumulate):
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterWillReplaceOrAccumulate requested"
           << endl;
    // 0 -> unsupported, 1 -> replace, 2 -> accumulate
    rv = 1;
    break;

  case DEPRECATED_VST_SYMBOL(audioMasterWantMidi):
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterWantMidi requested" << endl;
    // happy to oblige
    rv = 1;
    break;

  case DEPRECATED_VST_SYMBOL(audioMasterTempoAt):
    // if (debugLevel > 1)
    // cerr << "dssi-vst-server[2]: audioMasterTempoAt requested" << endl;
    // can't support this, return 120bpm
    rv = 120 * 10000;
    break;

  case DEPRECATED_VST_SYMBOL(audioMasterGetNumAutomatableParameters):
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterGetNumAutomatableParameters "
              "requested"
           << endl;
    rv = 5000;
    break;

  case DEPRECATED_VST_SYMBOL(audioMasterGetParameterQuantization):
    if (debugLevel > 1)
      cerr
          << "dssi-vst-server[2]: audioMasterGetParameterQuantization requested"
          << endl;
    rv = 1;
    break;

  case DEPRECATED_VST_SYMBOL(audioMasterNeedIdle):
    if (debugLevel > 1)
      cerr << "dssi-vst-server[2]: audioMasterNeedIdle requested" << endl;
    // might be nice to handle this better
    rv = 1;
    break;

  default:
    if (debugLevel > 0)
      cerr << "dssi-vst-server[0]: unsupported audioMaster callback opcode "
           << opcode << endl;
    break;
  }

  return rv;
}

VOID CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime) {
  HWND hwnderr = FindWindow(NULL, "LinVst Error");
  SendMessage(hwnderr, WM_COMMAND, IDCANCEL, 0);
}

void RemoteVSTServer::guiUpdate() {
#ifdef EMBED
#ifdef EMBEDRESIZE
  guiupdatecount += 1;

  if (guiupdatecount == 2) {
    ShowWindow(hWndvst[pidx], SW_SHOWNORMAL);
    UpdateWindow(hWndvst[pidx]);
    guiupdate = 0;
    guiupdatecount = 0;
  }
#endif
#endif
#ifndef EMBED
  //      SetWindowPos(hWndvst[pidx], 0, 0, 0, guiresizewidth + 6,
  //      guiresizeheight + 25, SWP_NOMOVE | SWP_HIDEWINDOW);
  SetWindowPos(hWndvst[pidx], 0, 0, 0, guiresizewidth + 6, guiresizeheight + 25,
               SWP_NOMOVE);
  ShowWindow(hWndvst[pidx], SW_SHOWNORMAL);
  UpdateWindow(hWndvst[pidx]);
  guiupdate = 0;
#endif
}

/*
      remoteVSTServerInstance2[idx]->m_shmControl->ropcode =
   (RemotePluginOpcode)disconnectserver;
      remoteVSTServerInstance2[idx]->waitForServer(remoteVSTServerInstance2[idx]->m_shmControl);
      remoteVSTServerInstance2[idx]->waitForClient2exit();
      remoteVSTServerInstance2[idx]->waitForClient3exit();
      remoteVSTServerInstance2[idx]->waitForClient4exit();
      remoteVSTServerInstance2[idx]->waitForClient5exit();
      remoteVSTServerInstance2[idx]->waitForClient6exit();
*/

void RemoteVSTServer::finisherror() {
  exiting = 1;
  sleep(1);

  if (ThreadHandle[0]) {
    WaitForSingleObject(ThreadHandle[0], 5000);
    CloseHandle(ThreadHandle[0]);
  }
  if (ThreadHandle[1]) {
    WaitForSingleObject(ThreadHandle[1], 5000);
    CloseHandle(ThreadHandle[1]);
  }
  if (ThreadHandle[2]) {
    WaitForSingleObject(ThreadHandle[2], 5000);
    CloseHandle(ThreadHandle[2]);
  }
  if (ThreadHandle[3]) {
    WaitForSingleObject(ThreadHandle[3], 5000);
    CloseHandle(ThreadHandle[3]);
  }  

  if (m_shmControl) {
    m_shmControl->ropcode = (RemotePluginOpcode)disconnectserver;
    waitForServer(m_shmControl);
    waitForClient2exit();
    waitForClient3exit();
    waitForClient4exit();
    waitForClient5exit();
    waitForClient6exit();
  }
  usleep(5000000);
}

DWORD WINAPI VstThreadMain(LPVOID parameter) {
  char fileInfo[4096];
  char libname[4096];
  int idx;
  int loaderr;
  DWORD dwWaitResult;
  int *ptr;

  threadargs *args;
  args = (threadargs *)parameter;

  strcpy(fileInfo, args->fileInfo);
  strcpy(libname, args->libname);

  idx = args->idx;

  loaderr = 0;

  string deviceName = libname;
  size_t foundext = deviceName.find_last_of(".");
  deviceName = deviceName.substr(0, foundext);
  remoteVSTServerInstance2[idx] = new RemoteVSTServer(fileInfo, deviceName);

  if (!remoteVSTServerInstance2[idx]) {
    cerr << "ERROR: Remote VST startup failed" << endl;
    usleep(5000000);
    sched_yield();
    remoteVSTServerInstance2[idx] = 0;
    sched_yield();
    if (ThreadHandlevst[idx])
      CloseHandle(ThreadHandlevst[idx]);
    sched_yield();
    realplugincount--;
    //     ExitThread(0);
    return 0;
  }

  if (remoteVSTServerInstance2[idx]->starterror == 1) {
    cerr << "ERROR: Remote VST start error" << endl;
    sched_yield();
    if (remoteVSTServerInstance2[idx]) {
      remoteVSTServerInstance2[idx]->finisherror();
      delete remoteVSTServerInstance2[idx];
    }
    sched_yield();
    remoteVSTServerInstance2[idx] = 0;
    sched_yield();
    if (ThreadHandlevst[idx])
      CloseHandle(ThreadHandlevst[idx]);
    sched_yield();
    realplugincount--;
    ///      ExitThread(0);
    return 0;
  }

  remoteVSTServerInstance2[idx]->pidx = idx;

  int startok;

  startok = 0;

  ptr = (int *)remoteVSTServerInstance2[idx]->m_shm;

  *ptr = 400;

  for (int i = 0; i < 400000; i++) {
    if ((*ptr == 2) || (*ptr == 3)) {
      startok = 1;
      break;
    }

    if (*ptr == 4) {
      startok = 0;
      break;
    }
    usleep(100);
  }

  if (startok == 0) {
    cerr << "ERROR: Remote VST start error" << endl;
    sched_yield();
    remoteVSTServerInstance2[idx]->finisherror();
    delete remoteVSTServerInstance2[idx];
    sched_yield();
    remoteVSTServerInstance2[idx] = 0;
    sched_yield();
    if (ThreadHandlevst[idx])
      CloseHandle(ThreadHandlevst[idx]);
    sched_yield();
    realplugincount--;
    ///      ExitThread(0);
    return 0;
  }

  if (*ptr == 3)
    remoteVSTServerInstance2[idx]->m_386run = 1;

  sched_yield();

  remoteVSTServerInstance2[idx]->ThreadHandle[0] = 0;
  remoteVSTServerInstance2[idx]->ThreadHandle[1] = 0;
  remoteVSTServerInstance2[idx]->ThreadHandle[2] = 0;
  remoteVSTServerInstance2[idx]->ThreadHandle[3] = 0;  

  remoteVSTServerInstance2[idx]->StartThreadFunc();
  remoteVSTServerInstance2[idx]->StartThreadFunc2();
  remoteVSTServerInstance2[idx]->StartThreadFunc3();
  remoteVSTServerInstance2[idx]->StartThreadFunc4();  

  if ((!remoteVSTServerInstance2[idx]->ThreadHandle[0]) ||
      (!remoteVSTServerInstance2[idx]->ThreadHandle[1]) ||
      (!remoteVSTServerInstance2[idx]->ThreadHandle[2]) ||
      (!remoteVSTServerInstance2[idx]->ThreadHandle[3])) {
    cerr << "Load Error" << endl;
    sched_yield();
    remoteVSTServerInstance2[idx]->finisherror();
    delete remoteVSTServerInstance2[idx];
    sched_yield();
    remoteVSTServerInstance2[idx] = 0;
    sched_yield();
    if (ThreadHandlevst[idx])
      CloseHandle(ThreadHandlevst[idx]);
    sched_yield();
    realplugincount--;
    ///      ExitThread(0);
    return 0;
  }

  sched_yield();

  *ptr = 2000;

  startok = 0;

  for (int i = 0; i < 400000; i++) {
    if (*ptr == 6000) {
      startok = 1;
      break;
    }
    usleep(100);
  }

  if (startok == 0) {
    cerr << "Failed to connect" << endl;
    sched_yield();
    remoteVSTServerInstance2[idx]->finisherror();
    delete remoteVSTServerInstance2[idx];
    sched_yield();
    remoteVSTServerInstance2[idx] = 0;
    sched_yield();
    if (ThreadHandlevst[idx])
      CloseHandle(ThreadHandlevst[idx]);
    sched_yield();
    realplugincount--;
    ///      ExitThread(0);
    return 0;
  }

  sched_yield();

  remoteVSTServerInstance2[idx]->ghWriteEvent7 = 0;

  remoteVSTServerInstance2[idx]->ghWriteEvent7 =
      CreateEvent(NULL, TRUE, FALSE, "wm_syncevent");

  while (0 == PostThreadMessage(mainThreadId, WM_SYNC7, (WPARAM)idx,
                                (LPARAM)libname))
    sched_yield();

  dwWaitResult =
      WaitForSingleObject(remoteVSTServerInstance2[idx]->ghWriteEvent7, 20000);

  CloseHandle(remoteVSTServerInstance2[idx]->ghWriteEvent7);

  if (remoteVSTServerInstance2[idx]->plugerr == 1) {
    cerr << "Load Error" << endl;
    sched_yield();
    remoteVSTServerInstance2[idx]->finisherror();
    delete remoteVSTServerInstance2[idx];
    sched_yield();
    remoteVSTServerInstance2[idx] = 0;
    sched_yield();
    if (ThreadHandlevst[idx])
      CloseHandle(ThreadHandlevst[idx]);
    sched_yield();
    realplugincount--;
    ///      ExitThread(0);
    return 0;
  }

  sched_yield();

  if (remoteVSTServerInstance2[idx]->m_plugin->flags & effFlagsHasEditor)
    remoteVSTServerInstance2[idx]->haveGui = true;
  else
    remoteVSTServerInstance2[idx]->haveGui = false;

  sched_yield();

  ResumeThread(remoteVSTServerInstance2[idx]->ThreadHandle[0]);
  ResumeThread(remoteVSTServerInstance2[idx]->ThreadHandle[1]);
  ResumeThread(remoteVSTServerInstance2[idx]->ThreadHandle[2]);
  ResumeThread(remoteVSTServerInstance2[idx]->ThreadHandle[3]);  

  sched_yield();

  while (!remoteVSTServerInstance2[idx]->exiting) {
    /*
if(remoteVSTServerInstance2[idx]->hidegui == 1)
{
remoteVSTServerInstance2[idx]->hideGUI();
}

if(remoteVSTServerInstance2[idx]->exiting)
break;
*/

    remoteVSTServerInstance2[idx]->dispatchControl(50);
  }

  // remoteVSTServerInstance2[pidx]->m_plugin->resvd2 = 0;

  sched_yield();

  //remoteVSTServerInstance2[idx]->waitForServerexit();
  remoteVSTServerInstance2[idx]->waitForClient2exit();
  remoteVSTServerInstance2[idx]->waitForClient3exit();
  remoteVSTServerInstance2[idx]->waitForClient4exit();
  remoteVSTServerInstance2[idx]->waitForClient5exit();
  remoteVSTServerInstance2[idx]->waitForClient6exit();

  // WaitForMultipleObjects(4, remoteVSTServerInstance2[idx]->ThreadHandle,
  // TRUE, 5000);
  MsgWaitForMultipleObjects(4, remoteVSTServerInstance2[idx]->ThreadHandle, TRUE, 5000, QS_ALLEVENTS);

  for (int idx50 = 0; idx50 < 100000; idx50++) {
    if (remoteVSTServerInstance2[idx]->parfin &&
        remoteVSTServerInstance2[idx]->audfin &&
        remoteVSTServerInstance2[idx]->getfin &&
        remoteVSTServerInstance2[idx]->confin)
      break;
    usleep(100);
  }

  if (remoteVSTServerInstance2[idx]->ThreadHandle[0])
    CloseHandle(remoteVSTServerInstance2[idx]->ThreadHandle[0]);

  if (remoteVSTServerInstance2[idx]->ThreadHandle[1])
    CloseHandle(remoteVSTServerInstance2[idx]->ThreadHandle[1]);

  if (remoteVSTServerInstance2[idx]->ThreadHandle[2])
    CloseHandle(remoteVSTServerInstance2[idx]->ThreadHandle[2]);
    
  if (remoteVSTServerInstance2[idx]->ThreadHandle[3])
    CloseHandle(remoteVSTServerInstance2[idx]->ThreadHandle[3]);    

  sched_yield();

  if (remoteVSTServerInstance2[idx]) {
    delete remoteVSTServerInstance2[idx];
  }

  sched_yield();

  remoteVSTServerInstance2[idx] = 0;

  if (ThreadHandlevst[idx])
    CloseHandle(ThreadHandlevst[idx]);

  sched_yield();

  realplugincount--;

  //    ExitThread(0);

  return 0;
}

void SIGTERM_handler(int signal) {
  if (realplugincount == 0)
    finishloop = 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdlinexxx,
                   int cmdshow) {
  string pathName;
  string fileName;
  char cdpath[4096];
  char libname[4096];
  char libname2[4096];
  char fileInfo[4096];
  char cmdline[4096];

  key_t MyKey2;
  int ShmID2;
  char *ShmPTR2;
  int *sptr;
  int val5;
  MSG msg;
  VstEntry getinstance;
  DWORD dwWaitResult10;

  cerr << "DSSI VST plugin server v" << RemotePluginVersion << endl;
  cerr << "Copyright (c) 2012-2013 Filipe Coelho" << endl;
  cerr << "Copyright (c) 2010-2011 Kristian Amlie" << endl;
  cerr << "Copyright (c) 2004-2006 Chris Cannam" << endl;
#ifdef EMBED
#ifdef VST32SERVER
  cerr << "LinVst-X version 4.0-32bit" << endl;
#else
  cerr << "LinVst-X version 4.0-64bit" << endl;
#endif
#else
#ifdef VST32SERVER
  cerr << "LinVst-X version 4.0st-32bit" << endl;
#else
  cerr << "LinVst-X version 4.0st-64bit" << endl;
#endif
#endif

  if (signal(SIGTERM, SIGTERM_handler) == SIG_ERR) {
    cerr << "SIGTERM handler error\n" << endl;
  }

#ifdef EMBED
#ifdef VST32SERVER
  MyKey2 = ftok("/usr/bin/lin-vst-server-x32.exe", 'a');
  ShmID2 = shmget(MyKey2, 20000, IPC_CREAT | 0666);
  ShmPTR2 = (char *)shmat(ShmID2, NULL, 0);
#else
  MyKey2 = ftok("/usr/bin/lin-vst-server-x.exe", 't');
  ShmID2 = shmget(MyKey2, 20000, IPC_CREAT | 0666);
  ShmPTR2 = (char *)shmat(ShmID2, NULL, 0);
#endif
#else
#ifdef VST32SERVER
  MyKey2 = ftok("/usr/bin/lin-vst-server-xst32.exe", 'a');
  ShmID2 = shmget(MyKey2, 20000, IPC_CREAT | 0666);
  ShmPTR2 = (char *)shmat(ShmID2, NULL, 0);
#else
  MyKey2 = ftok("/usr/bin/lin-vst-server-xst.exe", 't');
  ShmID2 = shmget(MyKey2, 20000, IPC_CREAT | 0666);
  ShmPTR2 = (char *)shmat(ShmID2, NULL, 0);
#endif
#endif

  if (ShmID2 == -1) {
    cerr << "ComMemCreateError\n" << endl;
    exit(0);
  }

  struct sched_param param;
  param.sched_priority = 1;

  int result = sched_setscheduler(0, SCHED_FIFO, &param);

  if (result < 0) {
    perror("Failed to set realtime priority");
  }

  threadargs args2;
  threadargs *args;
  args = &args2;

  if (ShmPTR2)
    sptr = (int *)ShmPTR2;
  else {
    cerr << "ComMemCreateError2\n" << endl;
    exit(0);
  }

#ifdef VST32SERVER
  *sptr = 1001;
#else
  *sptr = 1002;
#endif

  PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

  mainThreadId = GetCurrentThreadId();

  for (int cidx = 0; cidx < 128000; cidx++) {
    threadIdvst[cidx] = 0;
    ThreadHandlevst[cidx] = 0;
    remoteVSTServerInstance2[cidx] = 0;
    hWndvst[cidx] = 0;
    timerval[cidx] = 0;
  }

  while (finishloop == 1) {
    val5 = *sptr;

    //   while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    //    for (int loopidx = 0; (loopidx < 10) && PeekMessage(&msg, 0, 0, 0,
    //    PM_REMOVE); loopidx++)
    //    {
    // Raymond Chen
    DWORD dwTimeout = 100;
    DWORD dwStart = GetTickCount();
    DWORD dwElapsed;

    while ((dwElapsed = GetTickCount() - dwStart) < dwTimeout) {
      DWORD dwStatus = MsgWaitForMultipleObjectsEx(
          0, NULL, dwTimeout - dwElapsed, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

      if (dwStatus == WAIT_OBJECT_0) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
          switch (msg.message) {
          case WM_SYNC: {
            sched_yield();
            int pidx = (int)msg.wParam;
            if (remoteVSTServerInstance2[pidx]) {
              memset(&wclass[pidx], 0, sizeof(WNDCLASSEX));
              wclass[pidx].cbSize = sizeof(WNDCLASSEX);
              wclass[pidx].style = 0;
              // CS_HREDRAW | CS_VREDRAW;
              wclass[pidx].lpfnWndProc = DefWindowProc;
              wclass[pidx].cbClsExtra = 0;
              wclass[pidx].cbWndExtra = 0;
              wclass[pidx].hInstance = GetModuleHandle(0);
              wclass[pidx].hIcon = LoadIcon(
                  GetModuleHandle(0), remoteVSTServerInstance2[pidx]->wname);
              wclass[pidx].hCursor = LoadCursor(0, IDI_APPLICATION);
              // wclass[pidx].hbrBackground =
              // (HBRUSH)GetStockObject(BLACK_BRUSH);
              wclass[pidx].lpszMenuName = "MENU_DSSI_VST";
              wclass[pidx].lpszClassName =
                  remoteVSTServerInstance2[pidx]->wname;
              wclass[pidx].hIconSm = 0;

              if (!RegisterClassEx(&wclass[pidx])) {
                cerr << "RegClassErr\n" << endl;
                remoteVSTServerInstance2[pidx]->winm->winerror = 1;
                sched_yield();
                SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent);
                break;
              }

              hWndvst[pidx] = 0;

#ifdef EMBEDDRAG
              hWndvst[pidx] = CreateWindowEx(
                  WS_EX_TOOLWINDOW | WS_EX_ACCEPTFILES,
                  remoteVSTServerInstance2[pidx]->wname, "LinVst", WS_POPUP, 0,
                  0, 200, 200, 0, 0, GetModuleHandle(0), 0);
#else
              hWndvst[pidx] = CreateWindowEx(
                  WS_EX_TOOLWINDOW, remoteVSTServerInstance2[pidx]->wname,
                  "LinVst", WS_POPUP, 0, 0, 200, 200, 0, 0, GetModuleHandle(0),
                  0);
#endif

              if (!hWndvst[pidx]) {
                cerr << "WindowCreateErr\n" << endl;
                remoteVSTServerInstance2[pidx]->winm->winerror = 1;
                sched_yield();
                SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent);
                break;
              }

              sched_yield();
              remoteVSTServerInstance2[pidx]->rect = 0;
              remoteVSTServerInstance2[pidx]->m_plugin->dispatcher(
                  remoteVSTServerInstance2[pidx]->m_plugin, effEditGetRect, 0,
                  0, &remoteVSTServerInstance2[pidx]->rect, 0);
              sched_yield();
              remoteVSTServerInstance2[pidx]->m_plugin->dispatcher(
                  remoteVSTServerInstance2[pidx]->m_plugin, effEditOpen, 0, 0,
                  hWndvst[pidx], 0);
              sched_yield();
              remoteVSTServerInstance2[pidx]->m_plugin->dispatcher(
                  remoteVSTServerInstance2[pidx]->m_plugin, effEditGetRect, 0,
                  0, &remoteVSTServerInstance2[pidx]->rect, 0);
              sched_yield();

              if (!remoteVSTServerInstance2[pidx]->rect) {
                cerr << "RectErr\n" << endl;
                remoteVSTServerInstance2[pidx]->winm->winerror = 1;
                sched_yield();
                SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent);
                break;
              }
              
#ifdef TRACKTIONWM
 			 if (remoteVSTServerInstance2[pidx]->hosttracktion == 1) {
 				 SetWindowPos(hWndvst[pidx], HWND_TOP,
                 GetSystemMetrics(SM_XVIRTUALSCREEN) + remoteVSTServerInstance2[pidx]->offset.x,
                 GetSystemMetrics(SM_YVIRTUALSCREEN) + remoteVSTServerInstance2[pidx]->offset.y,
                 remoteVSTServerInstance2[pidx]->rect->right - remoteVSTServerInstance2[pidx]->rect->left, remoteVSTServerInstance2[pidx]->rect->bottom - 			remoteVSTServerInstance2[pidx]->rect->top, 0);
 			 } else {
  				 SetWindowPos(hWndvst[pidx], HWND_TOP, GetSystemMetrics(SM_XVIRTUALSCREEN),
                 GetSystemMetrics(SM_YVIRTUALSCREEN), remoteVSTServerInstance2[pidx]->rect->right - remoteVSTServerInstance2[pidx]->rect->left,
                 remoteVSTServerInstance2[pidx]->rect->bottom - remoteVSTServerInstance2[pidx]->rect->top, 0);
  }
#else
 				 SetWindowPos(hWndvst[pidx], HWND_TOP, GetSystemMetrics(SM_XVIRTUALSCREEN),
           		 GetSystemMetrics(SM_YVIRTUALSCREEN), remoteVSTServerInstance2[pidx]->rect->right - remoteVSTServerInstance2[pidx]->rect->left,
                 remoteVSTServerInstance2[pidx]->rect->bottom - remoteVSTServerInstance2[pidx]->rect->top, 0);
#endif

 				 remoteVSTServerInstance2[pidx]->winm->width = remoteVSTServerInstance2[pidx]->rect->right - remoteVSTServerInstance2[pidx]->rect->left;
  				 remoteVSTServerInstance2[pidx]->winm->height = remoteVSTServerInstance2[pidx]->rect->bottom - remoteVSTServerInstance2[pidx]->rect->top;
                 remoteVSTServerInstance2[pidx]->winm->handle = (long int)GetPropA(hWndvst[pidx], "__wine_x11_whole_window");

  				 memcpy(remoteVSTServerInstance2[pidx]->m_shmControl3->wret, remoteVSTServerInstance2[pidx]->winm, sizeof(winmessage));

  				 remoteVSTServerInstance2[pidx]->guiresizewidth = remoteVSTServerInstance2[pidx]->rect->right - remoteVSTServerInstance2[pidx]->rect->left;
 				 remoteVSTServerInstance2[pidx]->guiresizeheight = remoteVSTServerInstance2[pidx]->rect->bottom - remoteVSTServerInstance2[pidx]->rect->top;              
              

                 timerval[pidx] = 6788888 + pidx;
                 timerval[pidx] = SetTimer(hWndvst[pidx], timerval[pidx], 80, 0);

                 sched_yield();
                 SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent);
            }
          } break;

          case WM_SYNC2: {
            sched_yield();
            int pidx = (int)msg.wParam;

            if (remoteVSTServerInstance2[pidx]) {
              remoteVSTServerInstance2[pidx]->m_plugin->dispatcher(
                  remoteVSTServerInstance2[pidx]->m_plugin, effEditClose, 0, 0,
                  0, 0);
              sched_yield();
              if (timerval[pidx])
                KillTimer(hWndvst[pidx], timerval[pidx]);
              timerval[pidx] = 0;
              //#ifdef XECLOSE
              sched_yield();
              if (hWndvst[pidx])
                DestroyWindow(hWndvst[pidx]);
              sched_yield();
              //#endif

              UnregisterClassA(remoteVSTServerInstance2[pidx]->wname,
                               GetModuleHandle(0));

              sched_yield();

              hWndvst[pidx] = 0;
              sched_yield();
              SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent2);
            }
          } break;

          case WM_SYNC3: {
            sched_yield();
            int pidx = (int)msg.wParam;

            if (remoteVSTServerInstance2[pidx]) {
              ShowWindow(hWndvst[pidx], SW_SHOWNORMAL);
              sched_yield();
              // ShowWindow(hWnd, SW_SHOW);
              UpdateWindow(hWndvst[pidx]);
              sched_yield();
              SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent3);
            }
          } break;

          case WM_SYNC4: {
            sched_yield();
            int pidx = (int)msg.wParam;

            if (remoteVSTServerInstance2[pidx]) {
              remoteVSTServerInstance2[pidx]->m_plugin->dispatcher(
                  remoteVSTServerInstance2[pidx]->m_plugin, effOpen, 0, 0, NULL,
                  0);
              sched_yield();
              SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent4);
            }
          } break;

          case WM_SYNC5: {
            sched_yield();
            int pidx = (int)msg.wParam;

            if (remoteVSTServerInstance2[pidx]) {
              if (remoteVSTServerInstance2[pidx]->effectrun == true)
                remoteVSTServerInstance2[pidx]->m_plugin->dispatcher(
                    remoteVSTServerInstance2[pidx]->m_plugin, effClose, 0, 0,
                    NULL, 0);
              sched_yield();
              if (remoteVSTServerInstance2[pidx]->libHandle)
                FreeLibrary(remoteVSTServerInstance2[pidx]->libHandle);
              sched_yield();
              SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent5);
            }
          } break;

            /*
               case WM_SYNC6:
               {
                   sched_yield();
                   int pidx = (int) msg.wParam;

                   if(remoteVSTServerInstance2[pidx])
               {
               if(libHandle)
               FreeLibrary(libHandle);
               sched_yield();
               SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent6);
               }
               }
               break;
           */

          case WM_SYNC7: {
            sched_yield();

            memset(libnamesync, 0, 4096);
            strcpy(libnamesync, (char *)msg.lParam);
            int pidx = (int)msg.wParam;

            hcidx = pidx;

            if (remoteVSTServerInstance2[pidx]) {
              sched_yield();

              remoteVSTServerInstance2[pidx]->libHandle =
                  LoadLibrary(libnamesync);
              if (!remoteVSTServerInstance2[pidx]->libHandle) {
                cerr << "MplugLibraryErr\n" << endl;
                remoteVSTServerInstance2[pidx]->plugerr = 1;
                hcidx = 512000;
                sched_yield();
                SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent7);
                break;
              }

              getinstance = 0;

              getinstance = (VstEntry)GetProcAddress(
                  remoteVSTServerInstance2[pidx]->libHandle,
                  NEW_PLUGIN_ENTRY_POINT);

              if (!getinstance) {
                getinstance = (VstEntry)GetProcAddress(
                    remoteVSTServerInstance2[pidx]->libHandle,
                    OLD_PLUGIN_ENTRY_POINT);
                if (!getinstance) {
                  cerr << "dssi-vst-server: ERROR: VST entrypoints \""
                       << NEW_PLUGIN_ENTRY_POINT << "\" or \""
                       << OLD_PLUGIN_ENTRY_POINT << "\" not found in DLL \""
                       << libnamesync << "\"" << endl;
                  remoteVSTServerInstance2[pidx]->plugerr = 1;
                  hcidx = 512000;
                  sched_yield();
                  SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent7);
                  break;
                }
              }

              remoteVSTServerInstance2[pidx]->m_plugin =
                  getinstance(hostCallback3);
              if (!remoteVSTServerInstance2[pidx]->m_plugin) {
                cerr << "MplugInstanceErr\n" << endl;
                remoteVSTServerInstance2[pidx]->plugerr = 1;
                hcidx = 512000;
                sched_yield();
                SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent7);
                break;
              }

              if (remoteVSTServerInstance2[pidx]->m_plugin->magic !=
                  kEffectMagic) {
                cerr << "dssi-vst-server: ERROR: Not a VST plugin in DLL \""
                     << libnamesync << "\"" << endl;
                remoteVSTServerInstance2[pidx]->plugerr = 1;
                hcidx = 512000;
                sched_yield();
                SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent7);
                break;
              }

              if (!(remoteVSTServerInstance2[pidx]->m_plugin->flags &
                    effFlagsCanReplacing)) {
                cerr << "dssi-vst-server: ERROR: Not a VST plugin in DLL \""
                     << libnamesync << "\"" << endl;
                remoteVSTServerInstance2[pidx]->plugerr = 1;
                hcidx = 512000;
                sched_yield();
                SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent7);
                ;
                break;
              }

              remoteVSTServerInstance2[pidx]->m_plugin->resvd2 =
                  (RemoteVSTServer *)remoteVSTServerInstance2[pidx];
              hcidx = 512000;
              sched_yield();
              SetEvent(remoteVSTServerInstance2[pidx]->ghWriteEvent7);
            }
          } break;

          case WM_TIMER: {
            if (msg.wParam >= 6788888) {
              int valt = msg.wParam - 6788888;

              if (valt < plugincount)
                remoteVSTServerInstance2[valt]->m_plugin->dispatcher(
                    remoteVSTServerInstance2[valt]->m_plugin, effEditIdle, 0, 0,
                    NULL, 0);
            }
          } break;

          default:
            break;
          }
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }
    }

    sched_yield();
    //    usleep(1000);

#ifdef VST32SERVER
    if (val5 == 645)
#else
    if (val5 == 745)
#endif
    {
      if (plugincount > 128000)
        continue;

      memset(args->fileInfo, 0, 4096);
      memset(args->libname, 0, 4096);
      memset(cmdline, 0, 4096);
      memset(fileInfo, 0, 4096);
      memset(libname, 0, 4096);
      memset(libname2, 0, 4096);

      strcpy(cmdline, ShmPTR2 + sizeof(int));

      if (cmdline[0] != '\0') {
        int offset = 0;
        if (cmdline[0] == '"' || cmdline[0] == '\'')
          offset = 1;
        for (int ci = offset; cmdline[ci]; ++ci) {
          if (cmdline[ci] == ',') {
            strncpy(libname2, cmdline + offset, ci - offset);
            ++ci;
            if (cmdline[ci]) {
              strcpy(fileInfo, cmdline + ci);
              int l = strlen(fileInfo);
              if (fileInfo[l - 1] == '"' || fileInfo[l - 1] == '\'')
                fileInfo[l - 1] = '\0';
            }
          }
        }
      } else {
        cerr << "CmdlineErr\n" << endl;
        continue;
      }

      if (libname2 != NULL) {
        if ((libname2[0] == '/') && (libname2[1] == '/'))
          strcpy(libname, &libname2[1]);
        else
          strcpy(libname, libname2);
      } else {
        cerr << "Usage: dssi-vst-server <vstname.dll>,<tmpfilebase>" << endl;
        cerr << "(Command line was: " << cmdline << ")" << endl;
        continue;
      }

      if (!libname || !libname[0] || !fileInfo || !fileInfo[0]) {
        cerr << "Usage: dssi-vst-server <vstname.dll>,<tmpfilebase>" << endl;
        cerr << "(Command line was: " << cmdline << ")" << endl;
        continue;
      }
      /*
      strcpy(cdpath, libname);
      pathName = cdpath;
      fileName = cdpath;
      size_t found = pathName.find_last_of("/");
      pathName = pathName.substr(0, found);
      size_t found2 = fileName.find_last_of("/");
      fileName = fileName.substr(found2 + 1, strlen(libname) - (found2 +1));
      SetCurrentDirectory(pathName.c_str());
      */

      cerr << "Loading  " << libname << endl;

      strcpy(args->fileInfo, fileInfo);
      strcpy(args->libname, libname);

      args->idx = plugincount;

      threadIdvst[plugincount] = 0;
      ThreadHandlevst[plugincount] =
          CreateThread(0, 0, VstThreadMain, args, CREATE_SUSPENDED,
                       &threadIdvst[plugincount]);

      if (!ThreadHandlevst[plugincount]) {
        cerr << "ThreadErr\n" << endl;

        sched_yield();
#ifdef VST32SERVER
        *sptr = 647;
#else
        *sptr = 747;
#endif
        sched_yield();
        continue;
      }

      sched_yield();

      ResumeThread(ThreadHandlevst[plugincount]);

      sched_yield();
#ifdef VST32SERVER
      *sptr = 646;
#else
      *sptr = 746;
#endif
      sched_yield();

      plugincount++;
      realplugincount++;
    }
    //   if((plugincount > 0) && (realplugincount == 0))
    //   break;
  }

  if (ShmPTR2)
    shmdt(ShmPTR2);

  shmctl(ShmID2, IPC_RMID, NULL);

  cerr << "dssi-vst-server[1]: exiting" << endl;

  exit(0);
}
