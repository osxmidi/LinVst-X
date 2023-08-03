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

#include "remoteplugin.h"

#include "remotepluginclient.h"

#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "paths.h"

#define hidegui2 77775634

void *RemotePluginClient::AMThread() {
  int opcode;
  int val;
  int idx;
  float optval;
  int retval;
  int ok = 1;
  char retstr2[512];

  int timeout = 50;

  VstTimeInfo *timeInfo;
  VstTimeInfo *timeInfo2;
  VstTimeInfo timeinfo2;
  timeInfo2 = &timeinfo2;

  int els;
  int *ptr2;
  int sizeidx = 0;
  int size;
  VstEvents *evptr;

  struct amessage {
    int flags;
    int pcount;
    int parcount;
    int incount;
    int outcount;
    int delay;
  } am;

#ifdef EMBED
#ifdef EMBEDRESIZE
#define resizegui 123456789
#endif
#endif

#define disconnectserver 32143215

  ShmControl *m_shmControlptrth = m_shmControl;

  while (!m_threadbreak) {
    if (fwait2(m_shmControlptrth, &m_shmControlptrth->runServer, timeout)) {
      if (errno == ETIMEDOUT) {
        continue;
      } else {
        if (m_inexcept == 0)
          RemotePluginClosedException();
      }
    }

    RemotePluginOpcode opcode = RemotePluginNoOpcode;
    opcode = m_shmControlptrth->ropcode;

    if ((opcode != RemotePluginNoOpcode) && theEffect) {

      if (m_threadbreak)
        break;

      switch (opcode) {
/*		      
      case audioMasterSetTime:
        memcpy(timeInfo2, m_shmControlptrth->timeset, sizeof(VstTimeInfo));
        retval = 0;
        m_audioMaster(theEffect, audioMasterSetTime, 0, 0, timeInfo2, 0);
        m_shmControlptrth->retint = retval;
        break;
*/
      case audioMasterAutomate:
      {
        idx = m_shmControlptrth->value;
        optval = m_shmControlptrth->floatvalue;
        retval = 0;
#ifdef PCACHE
        ParamState* pstate = (ParamState*)m_shm6;        
        pstate[idx].value = optval;            
#endif                
        retval =
            m_audioMaster(theEffect, audioMasterAutomate, idx, 0, 0, optval);
        m_shmControlptrth->retint = retval;
        }
        break;

      case audioMasterGetAutomationState:
        retval = 0;
        retval =
            m_audioMaster(theEffect, audioMasterGetAutomationState, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterBeginEdit:
        idx = m_shmControlptrth->value;
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterBeginEdit, idx, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterEndEdit:
        idx = m_shmControlptrth->value;
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterEndEdit, idx, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterProcessEvents:
        ptr2 = (int *)m_shm4;
        els = *ptr2;
        sizeidx = sizeof(int);

      //  if (els > VSTSIZE)
      //    els = VSTSIZE;

        evptr = &vstev[0];
        evptr->numEvents = els;
        evptr->reserved = 0;

        for (int i = 0; i < els; i++) {
          VstEvent *bsize = (VstEvent *)&m_shm4[sizeidx];
          size = bsize->byteSize + (2 * sizeof(VstInt32));
          evptr->events[i] = bsize;
          sizeidx += size;
        }
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterProcessEvents, 0, 0, evptr, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterIOChanged:

        memcpy(&am, m_shmControlptrth->amptr, sizeof(am));
        if ((am.incount != m_numInputs) || (am.outcount != m_numOutputs)) {
          if ((am.incount + am.outcount) * m_bufferSize * sizeof(float) <
              (PROCESSSIZE)) {
            m_updateio = 1;
            m_updatein = am.incount;
            m_updateout = am.outcount;
            theEffect->numInputs = am.incount;
            theEffect->numOutputs = am.outcount;
          }
        }
        if (am.delay != m_delay) {
          m_delay = am.delay;
          theEffect->initialDelay = am.delay;
        }
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterIOChanged, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterUpdateDisplay:
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterUpdateDisplay, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

#ifdef EMBED
#ifdef EMBEDRESIZE
      case audioMasterSizeWindow: {
        width = m_shmControlptrth->value;
        height = m_shmControlptrth->value2;
 
        rp->bottom = height;
        rp->top = 0;
        rp->right = width;
        rp->left = 0;

        retval = m_audioMaster(theEffect, audioMasterSizeWindow, width, height, 0, 0);
      } 
      break;
#endif
#endif
		      
#ifdef MIDIEFF
      case audioMasterGetInputSpeakerArrangement:
        retval = 0;
        retval =
            m_audioMaster(theEffect, audioMasterGetInputSpeakerArrangement, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterGetSpeakerArrangement:
        retval = 0;
        retval =
            m_audioMaster(theEffect, audioMasterGetSpeakerArrangement, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;
#endif		      

      case audioMasterGetInputLatency:
        retval = 0;
        retval =
            m_audioMaster(theEffect, audioMasterGetInputLatency, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterGetOutputLatency:
        retval = 0;
        retval =
            m_audioMaster(theEffect, audioMasterGetOutputLatency, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterGetSampleRate:
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterGetSampleRate, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterGetBlockSize:
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterGetBlockSize, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterGetVendorString:
        retstr2[0] = '\0';
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterGetVendorString, 0, 0,
                               (char *)retstr2, 0);
        strcpy(m_shmControlptrth->retstr, retstr2);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterGetProductString:
        retstr2[0] = '\0';
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterGetProductString, 0, 0,
                               (char *)retstr2, 0);
        strcpy(m_shmControlptrth->retstr, retstr2);
        m_shmControlptrth->retint = retval;
        break;

      case audioMasterGetVendorVersion:
        retval = 0;
        retval =
            m_audioMaster(theEffect, audioMasterGetVendorVersion, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;

#ifdef CANDOEFF
      case audioMasterCanDo:
        retval = 0;
        retstr2[0] = '\0';
        strcpy(retstr2, m_shmControlptrth->retstr);
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterCanDo, 0, 0,
                               (char *)retstr2, 0);
        m_shmControlptrth->retint = retval;
        break;
#endif

#ifdef WAVES
      case audioMasterCurrentId:
        retval = 0;
        retval = m_audioMaster(theEffect, audioMasterCurrentId, 0, 0, 0, 0);
        m_shmControlptrth->retint = retval;
        break;
#endif
      case disconnectserver:
        m_inexcept = 1;
        usleep(100000);
        //   memset (theEffect, 0, sizeof(AEffect));
        //   theEffect = 0;
        waitForServer2exit();
        waitForServer3exit();
        waitForServer4exit();
        waitForServer5exit();
        waitForServer6exit();
        m_threadbreak = 1;

        //  m_threadbreakembed = 1;

        break;

      default:
        break;
      }
      m_shmControlptrth->ropcode = RemotePluginNoOpcode;
    }

    if (fpost2(m_shmControlptrth, &m_shmControlptrth->runClient)) {
      std::cerr << "Could not post to semaphore\n";
    }
  }
  m_threadbreakexit = 1;
  pthread_exit(0);
  return 0;
}

#ifdef EMBED
void RemotePluginClient::syncevents(Display *display) {
  int pending = XPending(display);

  for (int i = 0; i < pending; i++) {
    XEvent e;
    XNextEvent(display, &e);
  }
}
#endif

/*
void* RemotePluginClient::EMBEDThread()
{
     //struct sched_param param;
     //param.sched_priority = 0;
     //sched_setscheduler(0, SCHED_OTHER, &param);

      while (!m_threadbreakembed)
      {
      if(display && parent && child && (editopen > 0))
          {
      if(editopen == 2)
      {
          editopen = 0;
      continue;
      }

XEvent xevent;
XClientMessageEvent cm;
int accept = 0;
int x2 = 0;
int y2 = 0;
Atom XdndPosition = XInternAtom(display, "XdndPosition", False);
Atom XdndStatus = XInternAtom(display, "XdndStatus", False);
Atom XdndActionCopy = XInternAtom(display, "XdndActionCopy", False);
Atom XdndEnter = XInternAtom(display, "XdndEnter", False);
Atom XdndDrop = XInternAtom(display, "XdndDrop", False);
Atom XdndLeave = XInternAtom(display, "XdndLeave", False);
Atom XdndFinished = XInternAtom(display, "XdndFinished", False);

int x = 0;
int y = 0;
Window ignored = 0;
int mapped2 = 0;
#ifdef FOCUS
int x3 = 0;
int y3 = 0;
Window ignored3 = 0;
#endif
#ifdef XECLOSE
Atom xembedatom = XInternAtom(display, "_XEMBED_INFO", False);
#endif

      for (int loopidx=0; (loopidx < 10) && XPending(display); loopidx++)
      {
      XEvent e;

      XNextEvent(display, &e);

      switch (e.type)
      {
#ifdef XECLOSE
      case PropertyNotify:
      if (e.xproperty.atom == xembedatom)
      {
      xeclose = 1;
      }
      break;
#endif
      case MapNotify:
      if(e.xmap.window == child)
      mapped2 = 1;
      break;

      case UnmapNotify:
      if(e.xmap.window == child)
      mapped2 = 0;
      break;

#ifndef NOFOCUS
      case EnterNotify:
//      if(reaperid)
//        {
 //     if(mapped2)
  //    {
      if(e.xcrossing.focus == False)
      {
      XSetInputFocus(display, child, RevertToPointerRoot, CurrentTime);
//    XSetInputFocus(display, child, RevertToParent, e.xcrossing.time);
      }
 //     }
 //     }
      break;
#endif

#ifdef FOCUS
      case LeaveNotify:
      x3 = 0;
      y3 = 0;
      ignored3 = 0;
      XTranslateCoordinates(display, child, XDefaultRootWindow(display), 0, 0,
&x3, &y3, &ignored3);

      if(x3 < 0)
      {
      width += x3;
      x3 = 0;
      }

      if(y3 < 0)
      {
      height += y3;
      y3 = 0;
      }

      if(((e.xcrossing.x_root < x3) || (e.xcrossing.x_root > x3 + (width - 1)))
|| ((e.xcrossing.y_root < y3) || (e.xcrossing.y_root > y3 + (height - 1))))
      {
      if(mapped2)
      {
      if(parentok && reaperid)
      XSetInputFocus(display, pparent, RevertToPointerRoot, CurrentTime);
      else
      XSetInputFocus(display, PointerRoot, RevertToPointerRoot, CurrentTime);
      }
      }
      break;
#endif

      case ConfigureNotify:
//      if((e.xconfigure.event == parent) || (e.xconfigure.event == child) ||
((e.xconfigure.event == pparent) && (parentok)))
//      {

      x = 0;
      y = 0;
      ignored = 0;

      	XTranslateCoordinates(display, parent, XDefaultRootWindow(display), 0, 0, &x, &y, &ignored); 
	e.xconfigure.send_event = false; 
	e.xconfigure.type = ConfigureNotify; 
	e.xconfigure.event = child; 
	e.xconfigure.window = child;
#ifdef TRACKTIONWM  
        if(waveformid > 0)  
        {   
        e.xconfigure.x = x + waveformid2;
        e.xconfigure.y = y + waveformid;
        }
        else
        {
        e.xconfigure.x = x;
        e.xconfigure.y = y;
        }
#else
        e.xconfigure.x = x;
        e.xconfigure.y = y;
#endif    
      e.xconfigure.width = width;
      e.xconfigure.height = height;
      e.xconfigure.border_width = 0;
      e.xconfigure.above = None;
      e.xconfigure.override_redirect = False;
      XSendEvent (display, child, False, StructureNotifyMask |
SubstructureRedirectMask, &e);
//      }
      break;

#ifdef EMBEDDRAG
      case ClientMessage:
      if((e.xclient.message_type == XdndEnter) || (e.xclient.message_type ==
XdndPosition) || (e.xclient.message_type == XdndLeave) ||
(e.xclient.message_type == XdndDrop))
      {

      if(e.xclient.message_type == XdndPosition)
      {
      x = 0;
      y = 0;
      ignored = 0;

      e.xclient.window = child;
      XSendEvent (display, child, False, NoEventMask, &e);

      XTranslateCoordinates(display, child, XDefaultRootWindow(display), 0, 0,
&x, &y, &ignored);

      x2 = e.xclient.data.l[2]  >> 16;
      y2 = e.xclient.data.l[2] &0xffff;

      memset (&xevent, 0, sizeof(xevent));
      xevent.xany.type = ClientMessage;
      xevent.xany.display = display;
      xevent.xclient.window = e.xclient.data.l[0];
      xevent.xclient.message_type = XdndStatus;
      xevent.xclient.format = 32;
      xevent.xclient.data.l[0] = parent;
      if(((x2 >= x) && (x2 <= x + width)) && ((y2 >= y) && (y2 <= y + height)))
      {
      accept = 1;
      xevent.xclient.data.l[1] |= 1;
      }
      else
      {
      accept = 0;
      xevent.xclient.data.l[1] &= ~1;
      }
      xevent.xclient.data.l[4] = XdndActionCopy;

      XSendEvent (display, e.xclient.data.l[0], False, NoEventMask, &xevent);

      if(parentok && reaperid)
      {
      xevent.xclient.data.l[0] = pparent;
      XSendEvent (display, e.xclient.data.l[0], False, NoEventMask, &xevent);
      }
      }
      else if(e.xclient.message_type == XdndDrop)
      {
      e.xclient.window = child;
      XSendEvent (display, child, False, NoEventMask, &e);

      memset(&cm, 0, sizeof(cm));
      cm.type = ClientMessage;
      cm.display = display;
      cm.window = e.xclient.data.l[0];
      cm.message_type = XdndFinished;
      cm.format=32;
      cm.data.l[0] = parent;
      cm.data.l[1] = accept;
      if(accept)
      cm.data.l[2] = XdndActionCopy;
      else
      cm.data.l[2] = None;
      XSendEvent(display, e.xclient.data.l[0], False, NoEventMask,
(XEvent*)&cm); if(parentok && reaperid)
      {
      cm.data.l[0] = pparent;
      XSendEvent(display, e.xclient.data.l[0], False, NoEventMask,
(XEvent*)&cm);
      }
      }
      else
      {
      e.xclient.window = child;
      XSendEvent (display, child, False, NoEventMask, &e);
      }

      }
      break;
#endif

      default:
      break;
         }
        }
      }

      for(int idx=0;idx<100;idx++)
      {
      if(m_threadbreakembed == 1)
      break;
          usleep(100);
      }

     }
      m_threadbreakexitembed = 1;
      pthread_exit(0);
      return 0;
}
*/

#ifdef EMBED
#ifdef XECLOSE
#define XEMBED_EMBEDDED_NOTIFY 0
#define XEMBED_FOCUS_OUT 5

void RemotePluginClient::sendXembedMessage(Display *display, Window window,
                                           long message, long detail,
                                           long data1, long data2) {
  XEvent event;

  memset(&event, 0, sizeof(event));
  event.xclient.type = ClientMessage;
  event.xclient.window = window;
  event.xclient.message_type = XInternAtom(display, "_XEMBED", false);
  event.xclient.format = 32;
  event.xclient.data.l[0] = CurrentTime;
  event.xclient.data.l[1] = message;
  event.xclient.data.l[2] = detail;
  event.xclient.data.l[3] = data1;
  event.xclient.data.l[4] = data2;

  XSendEvent(display, window, false, NoEventMask, &event);
  XSync(display, false);
}
#endif
#endif

RemotePluginClient::RemotePluginClient(audioMasterCallback theMaster,
                                       Dl_info info)
    : m_audioMaster(theMaster), m_shmFd(-1), m_AMThread(0),
#ifdef EMBED
#ifdef XECLOSE
      xeclose(0),
#endif
/*
    m_EMBEDThread(0),
    m_threadbreakembed(0),
    m_threadbreakexitembed(0),
*/
#endif
      m_threadinit(0), m_threadbreak(0), m_threadbreakexit(0), editopen(0),
      m_shmFileName(0), m_shm(0), m_shmSize(0), m_shm2(0), m_shm3(0), m_shm4(0), m_shm5(0),
      m_shmControlFd(-1), m_shmControl(0), m_shmControlFileName(0),
      m_shmControl2(0), m_shmControl3(0), m_shmControl4(0), m_shmControl5(0), m_shmControl6(0),
      m_bufferSize(-1), m_numInputs(-1), m_numOutputs(-1), m_updateio(0),
      m_updatein(0), m_updateout(0), m_delay(0), timeInfo(0),
#ifdef CHUNKBUF
      chunk_ptr(0),
#endif
      m_inexcept(0),
#ifdef WAVES
      wavesthread(0),
#endif
      m_finishaudio(0), m_runok(0), m_syncok(0), m_386run(0), reaperid(0),
#ifdef EMBED
      child(0), parent(0), display(0), handle(0), width(0), height(0),
      displayerr(0), winm(0),
#ifdef EMBEDRESIZE
      resizedone(0),
#endif
#ifdef TRACKTIONWM
      waveformid(0), waveformid2(0), hosttracktion(0),
#endif
#ifdef EMBEDDRAG
      x11_win(0), pparent(0), root(0), children(0), numchildren(0), parentok(0),
#endif
      eventrun(0), eventstop(0), eventfinish(0),
#endif
#ifdef PCACHE
     m_shm6(0),
#endif    
      theEffect(0) {
  char tmpFileBase[60];

  srand(time(NULL));

  sprintf(tmpFileBase, "/tmp/rplugin_shm_XXXXXX");
  if (mkstemp(tmpFileBase) < 0) {
    cleanup();
	throw((std::string)"Failed to obtain temporary filename");
  }
  m_shmFileName = strdup(tmpFileBase);

  m_shmFd = open(m_shmFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (m_shmFd < 0) {
     cleanup();
	 throw((std::string)"Failed to open or create shared memory file");
  }

  if (sizeShm()) {
    cleanup();
    throw((std::string) "Failed to mmap shared memory file");
  }

  ServerConnect(info);
}

RemotePluginClient::~RemotePluginClient() {
	
    if (m_runok == 0) {
    m_threadbreak = 1;
    waitForClientexit();
    waitForServer2exit();
    waitForServer3exit();
    waitForServer4exit();
    waitForServer5exit();
    waitForServer6exit();

    cleanup();

    /*
    if (theEffect)
    delete theEffect;
    */
/*
#ifdef EMBED
    if (winm)
    delete winm;
#endif
*/
#ifdef CHUNKBUF
    if (chunk_ptr)
      free(chunk_ptr);
#endif
  }
}

VstIntPtr RemotePluginClient::dispatchproc(AEffect *effect, VstInt32 opcode,
                                           VstInt32 index, VstIntPtr value,
                                           void *ptr, float opt) {
  RemotePluginClient *plugin = (RemotePluginClient *)effect->object;
  VstIntPtr v = 0;

#ifdef EMBED
#ifdef EMBEDDRAG
  Atom XdndProxy;
  Atom XdndAware;
  Atom version;
  XSetWindowAttributes attr = {0};
#endif
#ifdef TRACKTIONWM
  char dawbuf[512];
#endif
#ifdef XECLOSE
  Atom xembedatom;
  unsigned long data[2];
#endif
#endif

  if (!effect)
    return 0;

  if (!plugin)
    return 0;

  if (plugin->m_inexcept == 1) {
    return 0;
  }

  if (plugin->m_threadbreak == 1) {
    return 0;
  }

  switch (opcode) {
  case effEditGetRect:
    if (plugin->editopen == 1)
    {
    *((struct ERect **)ptr) = plugin->rp;
    v = 1;
    }
    else
    v = 0;
    break;

  case effEditIdle:
    sched_yield();
    break;

  case effStartProcess:
    plugin->effVoidOp(effStartProcess);
    break;

  case effStopProcess:
    plugin->effVoidOp(effStopProcess);
    break;

  case effMainsChanged:
    v = plugin->getEffInt(effMainsChanged, value);
    break;

  case effGetVendorString:
    //    strncpy((char *) ptr, plugin->getMaker().c_str(),
    //    kVstMaxVendorStrLen);
    strcpy((char *)ptr, plugin->getMaker().c_str());
    v = 1;
    break;

  case effGetEffectName:
    //    strncpy((char *) ptr, plugin->getName().c_str(),
    //    kVstMaxEffectNameLen);
    strcpy((char *)ptr, plugin->getName().c_str());
    v = 1;
    break;

  case effGetParamName:
    //  strncpy((char *) ptr, plugin->getParameterName(index).c_str(),
    //  kVstMaxParamStrLen);
    // strncpy((char *) ptr, plugin->getParameterName(index).c_str(),
    // kVstMaxVendorStrLen);
    strcpy((char *)ptr, plugin->getParameterName(index).c_str());
    break;

  case effGetParamLabel:
    strcpy((char *)ptr, plugin->getParameterLabel(index).c_str());
    //   strcpy((char *) ptr, plugin->getEffString(effGetParamLabel,
    //   index).c_str());
    break;

  case effGetParamDisplay:
    strcpy((char *)ptr, plugin->getParameterDisplay(index).c_str());
    //   strcpy((char *) ptr, plugin->getEffString(effGetParamDisplay,
    //   index).c_str());
    break;

  case effGetProgramNameIndexed:
    v = plugin->getProgramNameIndexed(index, (char *)ptr);
    break;

  case effGetProgramName:
    //  strncpy((char *) ptr, plugin->getProgramName().c_str(),
    //  kVstMaxProgNameLen);
    strcpy((char *)ptr, plugin->getProgramName().c_str());
    break;

  case effSetSampleRate:
    plugin->setSampleRate(opt);
    break;

  case effSetBlockSize:
    plugin->setBufferSize((VstInt32)value);
    break;

#ifdef DOUBLEP
  case effSetProcessPrecision:
    v = plugin->getEffInt(effSetProcessPrecision, value);
    break;
#endif

  case effGetInputProperties:
    v = plugin->getEffInProp(index, (char *)ptr);
    break;

  case effGetOutputProperties:
    v = plugin->getEffOutProp(index, (char *)ptr);
    break;

#ifdef MIDIEFF
  case effGetMidiKeyName:
    v = plugin->getEffMidiKey(index, (char *)ptr);
    break;

  case effGetMidiProgramName:
    v = plugin->getEffMidiProgName(index, (char *)ptr);
    break;

  case effGetCurrentMidiProgram:
    v = plugin->getEffMidiCurProg(index, (char *)ptr);
    break;

  case effGetMidiProgramCategory:
    v = plugin->getEffMidiProgCat(index, (char *)ptr);
    break;

  case effHasMidiProgramsChanged:
    v = plugin->getEffMidiProgCh(index);
    break;

  case effSetSpeakerArrangement:
    v = plugin->setEffSpeaker(value, (char *)ptr);
    break;

  case effGetSpeakerArrangement:
    v = plugin->getEffSpeaker(value, (char *)ptr);
    break;
#endif

  case effGetVstVersion:
    v = kVstVersion;
    break;

  case effGetPlugCategory:
    v = plugin->getEffInt(effGetPlugCategory, 0);
    break;
#ifdef WAVES
  case effShellGetNextPlugin:
    v = plugin->getShellName((char *)ptr);
    break;
#endif
  case effSetProgram:
    plugin->setCurrentProgram((VstInt32)value);
    break;

  case effEditOpen:
#ifdef EMBED
    plugin->editopen = 0;

    plugin->winm->handle = (intptr_t)ptr;
    plugin->winm->winerror = 0;
    plugin->winm->width = 0;
    plugin->winm->height = 0;
    
    plugin->showGUI();
    
    if(plugin->winm->winerror == 0)
    {    
    plugin->rp->bottom = plugin->winm->height;
    plugin->rp->top = 0;
    plugin->rp->right = plugin->winm->width;
    plugin->rp->left = 0;
    }
    else
    break;
#else
    plugin->showGUI();
#endif
    plugin->editopen = 1;
    v = 1;
    break;

  case effEditClose:
    if((plugin->editopen == 1) && (plugin->winm->winerror == 0))
    {
    plugin->hideGUI();
    plugin->editopen = 0;
    v = 1;
    }
    break;

  case effCanDo:
    if (ptr && !strcmp((char *)ptr, "hasCockosExtensions")) {
      plugin->reaperid = 1;
      plugin->effVoidOp(78345432);
    }
#ifdef EMBED
#ifdef TRACKTIONWM    
        if(plugin->reaperid == 0)
        {
        if(plugin->theEffect && plugin->m_audioMaster)
        {
        plugin->m_audioMaster(plugin->theEffect, audioMasterGetProductString, 0, 0, dawbuf, 0);
        if((strcmp(dawbuf, "Tracktion") == 0) || (strcmp(dawbuf, "Waveform") == 0))
        {
	plugin->hosttracktion = 1;
        plugin->waveformid = plugin->effVoidOp2(67584930, index, value, opt);
	    }
        }
        }
#endif
#endif
#ifdef CANDOEFF
    v = plugin->getEffCanDo((char *)ptr);
#else
    v = 1;
#endif
    break;

  case effProcessEvents:
    v = plugin->processVstEvents((VstEvents *)ptr);
    break;

  case effGetChunk:
    v = plugin->getChunk((void **)ptr, index);
    break;

  case effSetChunk:
    v = plugin->setChunk(ptr, value, index);
    break;

  case effGetProgram:
    v = plugin->getProgram();
    break;

  case effCanBeAutomated:
    v = plugin->canBeAutomated(index);
    break;

  case effOpen:
    plugin->EffectOpen();
    break;

  case effClose:
   if((plugin->editopen == 1) && (plugin->winm->winerror == 0))
    {
    plugin->hideGUI();
    plugin->editopen = 0;
    v = 1;
    }
    
    plugin->effVoidOp(effClose);
		  
    delete plugin;
    break;

  default:
    break;
  }

  return v;
}

#ifdef DOUBLEP
void RemotePluginClient::prodproc(AEffect *effect, double **inputs,
                                  double **outputs, int sampleFrames) {
  RemotePluginClient *plugin = (RemotePluginClient *)effect->object;

  if (!plugin)
    return;

  if ((plugin->m_bufferSize > 0) && (plugin->m_numInputs >= 0) &&
      (plugin->m_numOutputs >= 0))
    plugin->processdouble(inputs, outputs, sampleFrames);
  return;
}
#endif

void RemotePluginClient::proproc(AEffect *effect, float **inputs,
                                 float **outputs, int sampleFrames) {
  RemotePluginClient *plugin = (RemotePluginClient *)effect->object;

  if (!plugin)
    return;

  if ((plugin->m_bufferSize > 0) && (plugin->m_numInputs >= 0) &&
      (plugin->m_numOutputs >= 0))
    plugin->process(inputs, outputs, sampleFrames);
  return;
}

void RemotePluginClient::setparproc(AEffect *effect, VstInt32 index,
                                    float parameter) {
  RemotePluginClient *plugin = (RemotePluginClient *)effect->object;

  if (!plugin)
    return;

  if ((plugin->m_bufferSize > 0) && (plugin->m_numInputs >= 0) &&
      (plugin->m_numOutputs >= 0))
    plugin->setParameter(index, parameter);
  return;
}

float RemotePluginClient::getparproc(AEffect *effect, VstInt32 index) {
  RemotePluginClient *plugin = (RemotePluginClient *)effect->object;
  float retval = -1;

  if (!plugin)
    return retval;

  if ((plugin->m_bufferSize > 0) && (plugin->m_numInputs >= 0) &&
      (plugin->m_numOutputs >= 0))
    retval = plugin->getParameter(index);
  return retval;
}

void RemotePluginClient::errwin(std::string dllname) {
  Window window = 0;
  Window ignored = 0;
  Display *display = 0;
  int screen = 0;
  Atom winstate;
  Atom winmodal;

  std::string filename;
  std::string filename2;

  size_t found2 = dllname.find_last_of("/");
  filename = dllname.substr(found2 + 1, strlen(dllname.c_str()) - (found2 + 1));
  filename2 = "VST dll file not found or timeout:  " + filename;

  XInitThreads();
  display = XOpenDisplay(NULL);
  if (!display)
    return;
  screen = DefaultScreen(display);
  window = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10,
                               480, 20, 0, BlackPixel(display, screen),
                               WhitePixel(display, screen));
  if (!window)
    return;
  winstate = XInternAtom(display, "_NET_WM_STATE", True);
  winmodal = XInternAtom(display, "_NET_WM_STATE_ABOVE", True);
  XChangeProperty(display, window, winstate, XA_ATOM, 32, PropModeReplace,
                  (unsigned char *)&winmodal, 1);
  XStoreName(display, window, filename2.c_str());
  XMapWindow(display, window);
  XSync(display, false);
  XFlush(display);
  sleep(10);
  XSync(display, false);
  XFlush(display);
  XDestroyWindow(display, window);
  XCloseDisplay(display);
}

void RemotePluginClient::ServerConnect(Dl_info info) {
  pid_t child;
  std::string dllName;
  std::string LinVstName;
  bool test;

  int dlltype;
  unsigned int offset;
  char buffer[256];

  char hit2[4096];

  int *sptr;
  int val5;
  char bufpipe[2048];
  int doexec;
  int memok;
  int memok2;
  int memok3;

  if (!info.dli_fname) {
    m_runok = 1;
    cleanup();
    return;
  }

  if (realpath(info.dli_fname, hit2) == 0) {
    m_runok = 1;
    cleanup();
    return;
  }

  dllName = hit2;
  dllName.replace(dllName.begin() + dllName.find(".so"), dllName.end(), ".dll");
  test = std::ifstream(dllName.c_str()).good();

  if (!test) {
    dllName = hit2;
    dllName.replace(dllName.begin() + dllName.find(".so"), dllName.end(),
                    ".Dll");
    test = std::ifstream(dllName.c_str()).good();

    if (!test) {
      dllName = hit2;
      dllName.replace(dllName.begin() + dllName.find(".so"), dllName.end(),
                      ".DLL");
      test = std::ifstream(dllName.c_str()).good();
    }

    if (!test) {
      dllName = hit2;
      dllName.replace(dllName.begin() + dllName.find(".so"), dllName.end(),
                      ".dll");
      errwin(dllName);
      m_runok = 1;
      cleanup();
      return;
    }
  }

#ifdef VST6432
  std::ifstream mfile(dllName.c_str(), std::ifstream::binary);

  if (!mfile) {
    m_runok = 1;
    cleanup();
    return;
  }

  mfile.read(&buffer[0], 2);
  short *ptr;
  ptr = (short *)&buffer[0];

  if (*ptr != 0x5a4d) {
    mfile.close();
    m_runok = 1;
    cleanup();
    return;
  }

  mfile.seekg(60, mfile.beg);
  mfile.read(&buffer[0], 4);

  int *ptr2;
  ptr2 = (int *)&buffer[0];
  offset = *ptr2;
  offset += 4;

  mfile.seekg(offset, mfile.beg);
  mfile.read(&buffer[0], 2);

  unsigned short *ptr3;
  ptr3 = (unsigned short *)&buffer[0];

  dlltype = 0;
  if (*ptr3 == 0x8664)
    dlltype = 1;
  else if (*ptr3 == 0x014c)
    dlltype = 2;
  else if (*ptr3 == 0x0200)
    dlltype = 3;

  if (dlltype == 0) {
    mfile.close();
    m_runok = 1;
    cleanup();
    return;
  }

  mfile.close();
#endif

#ifdef VST6432
  if (dlltype == 2) {
#ifdef EMBED
    LinVstName = "/usr/bin/lin-vst-server-x32.exe";
#else
    LinVstName = "/usr/bin/lin-vst-server-xst32.exe";
#endif
    test = std::ifstream(LinVstName.c_str()).good();
    if (!test) {
      m_runok = 1;
      cleanup();
      return;
    }
  } else {
#ifdef EMBED
    LinVstName = "/usr/bin/lin-vst-server-x.exe";
#else
    LinVstName = "/usr/bin/lin-vst-server-xst.exe";
#endif
    test = std::ifstream(LinVstName.c_str()).good();
    if (!test) {
      m_runok = 1;
      cleanup();
      return;
    }
  }
#else
#ifdef EMBED
  LinVstName = "/usr/bin/lin-vst-server-x.exe";
#else
  LinVstName = "/usr/bin/lin-vst-server-xst.exe";
#endif
  test = std::ifstream(LinVstName.c_str()).good();
  if (!test) {
    m_runok = 1;
    cleanup();
    return;
  }
#endif

  hit2[0] = '\0';

  std::string dllNamewin = dllName;
  std::size_t idx = dllNamewin.find("drive_c");

  if (idx != std::string::npos) {
    const char *hit = dllNamewin.c_str();
    strcpy(hit2, hit);
    hit2[idx - 1] = '\0';
    setenv("WINEPREFIX", hit2, 1);
  }

  std::string arg = dllName + "," + getFileIdentifiers();
  argStr = arg.c_str();

  doexec = 0;

#ifdef VST6432
#ifdef EMBED
  if (dlltype == 2)
    MyKey2 = ftok("/usr/bin/lin-vst-server-x32.exe", 'a');
  else
    MyKey2 = ftok("/usr/bin/lin-vst-server-x.exe", 't');
#else
  if (dlltype == 2)
    MyKey2 = ftok("/usr/bin/lin-vst-server-xst32.exe", 'a');
  else
    MyKey2 = ftok("/usr/bin/lin-vst-server-xst.exe", 't');
#endif
#else
#ifdef EMBED
  MyKey2 = ftok("/usr/bin/lin-vst-server-x.exe", 't');
#else
  MyKey2 = ftok("/usr/bin/lin-vst-server-xst.exe", 't');
#endif
#endif

  ShmID2 = shmget(MyKey2, 20000, 0666);

  if (ShmID2 == -1) {
    doexec = 1;
  }

#ifdef LVRT
  struct sched_param param;
  param.sched_priority = 1;

  int result = sched_setscheduler(0, SCHED_FIFO, &param);

  if (result < 0) {
    perror("Failed to set realtime priority");
  }
#endif

  //    signal(SIGCHLD, SIG_IGN);

  if (doexec) {
    if ((child = vfork()) < 0) {
      m_runok = 1;
      cleanup();
      return;
    } else if (child == 0) {
// for (int fd=3; fd<256; fd++) (void) close(fd);
/*
int maxfd=sysconf(_SC_OPEN_MAX);
for(int fd=3; fd<maxfd; fd++)
    close(fd);
*/
#ifdef EMBED
#ifdef VST6432
      if (dlltype == 2) {
        if (execlp("/usr/bin/lin-vst-server-x32.exe",
                   "/usr/bin/lin-vst-server-x32.exe", NULL, NULL)) {
          m_runok = 1;
          cleanup();
          return;
        }
      } else {
        if (execlp("/usr/bin/lin-vst-server-x.exe",
                   "/usr/bin/lin-vst-server-x.exe", NULL, NULL)) {
          m_runok = 1;
          cleanup();
          return;
        }
      }
#else
      if (execlp("/usr/bin/lin-vst-server-x.exe",
                 "/usr/bin/lin-vst-server-x.exe", NULL, NULL)) {
        m_runok = 1;
        cleanup();
        return;
      }
#endif
#else
#ifdef VST6432
      if (dlltype == 2) {
        if (execlp("/usr/bin/lin-vst-server-xst32.exe",
                   "/usr/bin/lin-vst-server-xst32.exe", NULL, NULL)) {
          m_runok = 1;
          cleanup();
          return;
        }
      } else {
        if (execlp("/usr/bin/lin-vst-server-xst.exe",
                   "/usr/bin/lin-vst-server-xst.exe", NULL, NULL)) {
          m_runok = 1;
          cleanup();
          return;
        }
      }
#else
      if (execlp("/usr/bin/lin-vst-server-xst.exe",
                 "/usr/bin/lin-vst-server-xst.exe", NULL, NULL)) {
        m_runok = 1;
        cleanup();
        return;
      }
#endif
#endif
    }
  }

  if (doexec) {
    for (int i9 = 0; i9 < 20000; i9++) {
      ShmID2 = shmget(MyKey2, 20000, 0666);
      if (ShmID2 != -1) {
        memok = 1;
        break;
      }
      usleep(1000);
    }

    ShmPTR2 = (char *)shmat(ShmID2, NULL, 0);
    sptr = (int *)ShmPTR2;

    memok2 = 0;

    if (memok && ShmPTR2) {
      for (int i12 = 0; i12 < 20000; i12++) {
        val5 = 0;
        val5 = *sptr;

#ifdef VST6432
        if (dlltype == 2) {
          if (val5 == 1001) {
            memok2 = 1;
            break;
          }
        } else {
          if (val5 == 1002) {
            memok2 = 1;
            break;
          }
        }
#else
        if (val5 == 1002) {
          memok2 = 1;
          break;
        }
#endif
        usleep(1000);
      }
    }

    if (!memok2) {
      m_runok = 1;
      cleanup();
      return;
    }
  }

  if (!doexec) {
    memok3 = 0;

#ifdef VST6432
#ifdef EMBED
    if (dlltype == 2)
      MyKey2 = ftok("/usr/bin/lin-vst-server-x32.exe", 'a');
    else
      MyKey2 = ftok("/usr/bin/lin-vst-server-x.exe", 't');
#else
    if (dlltype == 2)
      MyKey2 = ftok("/usr/bin/lin-vst-server-xst32.exe", 'a');
    else
      MyKey2 = ftok("/usr/bin/lin-vst-server-xst.exe", 't');
#endif
#else
#ifdef EMBED
    MyKey2 = ftok("/usr/bin/lin-vst-server-x.exe", 't');
#else
    MyKey2 = ftok("/usr/bin/lin-vst-server-xst.exe", 't');
#endif
#endif

    ShmID2 = shmget(MyKey2, 20000, 0666);

    memok3 = 1;

    if (ShmID2 == -1) {
      memok3 = 0;
      for (int i8 = 0; i8 < 5000; i8++) {
        ShmID2 = shmget(MyKey2, 20000, 0666);

        if (ShmID2 != -1) {
          memok3 = 1;
          break;
        }
        usleep(1000);
      }
    }

    if (memok3 == 0) {
      m_runok = 1;
      cleanup();
      return;
    }

    ShmPTR2 = (char *)shmat(ShmID2, NULL, 0);
  }

  if (ShmPTR2) {
    sptr = (int *)ShmPTR2;
    strcpy(ShmPTR2 + sizeof(int), argStr);

#ifdef VST6432
    if (dlltype == 2)
      *sptr = 645;
    else
      *sptr = 745;
#else
    *sptr = 745;
#endif

    for (int i3 = 0; i3 < 10000; i3++) {
      val5 = 0;
      val5 = *sptr;

#ifdef VST6432
      if (dlltype == 2) {
        if (val5 == 647) {
          m_runok = 1;
          cleanup();
          return;
        }
        if (val5 == 646)
          break;
      } else {
        if (val5 == 747) {
          m_runok = 1;
          cleanup();
          return;
        }
        if (val5 == 746)
          break;
      }
#else
      if (val5 == 747) {
        m_runok = 1;
        cleanup();
        return;
      }
      if (val5 == 746)
        break;
#endif
      usleep(1000);
    }
  } else {
    m_runok = 1;
    cleanup();
    return;
  }

  syncStartup();

  if (m_syncok == 0) {
    m_runok = 1;
    cleanup();
    return;
  }

  /*
      theEffect = new AEffect;

      if(!theEffect)
      {
      m_runok = 1;
      cleanup();
      return;
      }

  */

  //  initEffect();

  memset(theEffect, 0x0, sizeof(AEffect));

  theEffect->magic = kEffectMagic;
  theEffect->dispatcher = dispatchproc;
  theEffect->setParameter = setparproc;
  theEffect->getParameter = getparproc;
  theEffect->numInputs = getInputCount();
  theEffect->numOutputs = getOutputCount();
  theEffect->numPrograms = getProgramCount();
  theEffect->numParams = getParameterCount();
  theEffect->flags = getFlags();
#ifndef DOUBLEP
  theEffect->flags &= ~effFlagsCanDoubleReplacing;
  theEffect->flags |= effFlagsCanReplacing;
#endif
  theEffect->resvd1 = 0;
  theEffect->resvd2 = 0;
  theEffect->initialDelay = getinitialDelay();
  theEffect->object = (void *)this;
  theEffect->user = 0;
  theEffect->uniqueID = getUID();
  theEffect->version = getVersion();
  theEffect->processReplacing = proproc;
#ifdef DOUBLEP
  theEffect->processDoubleReplacing = prodproc;
#endif
}

void RemotePluginClient::syncStartup() {
  int startok;
  int *ptr;
  
  m_shmControlptr = m_shmControl6;  

  startok = 0;

  ptr = (int *)m_shm;

  for (int i = 0; i < 400000; i++) {
    if (*ptr == 478) {
      startok = 1;
      break;
    }
    usleep(100);
  }

  if (startok == 0) {
    *ptr = 4;
    m_runok = 1;
    cleanup();
    return;
  }

  if (m_386run == 1) {
    *ptr = 3;
  } else {
    *ptr = 2;
  }

  atomic_init(&m_shmControl->runServer, 0);
  atomic_init(&m_shmControl->runClient, 0);

  atomic_init(&m_shmControl2->runServer, 0);
  atomic_init(&m_shmControl2->runClient, 0);

  atomic_init(&m_shmControl3->runServer, 0);
  atomic_init(&m_shmControl3->runClient, 0);

  atomic_init(&m_shmControl4->runServer, 0);
  atomic_init(&m_shmControl4->runClient, 0);

  atomic_init(&m_shmControl5->runServer, 0);
  atomic_init(&m_shmControl5->runClient, 0);
  
  atomic_init(&m_shmControl6->runServer, 0);
  atomic_init(&m_shmControl6->runClient, 0);  

  atomic_init(&m_shmControl->nwaitersserver, 0);
  atomic_init(&m_shmControl2->nwaitersserver, 0);
  atomic_init(&m_shmControl3->nwaitersserver, 0);
  atomic_init(&m_shmControl4->nwaitersserver, 0);
  atomic_init(&m_shmControl5->nwaitersserver, 0);
  atomic_init(&m_shmControl6->nwaitersserver, 0);  

  atomic_init(&m_shmControl->nwaitersclient, 0);
  atomic_init(&m_shmControl2->nwaitersclient, 0);
  atomic_init(&m_shmControl3->nwaitersclient, 0);
  atomic_init(&m_shmControl4->nwaitersclient, 0);
  atomic_init(&m_shmControl5->nwaitersclient, 0);
  atomic_init(&m_shmControl6->nwaitersclient, 0);  
  
  theEffect = &theEffect2;

#ifdef EMBED
  winm = &winm2;

  winm->handle = 0;
  winm->winerror = 0;
  winm->width = 0;
  winm->height = 0;
	
  rp = &retRect;	
#endif

  if (pthread_create(&m_AMThread, NULL, RemotePluginClient::callAMThread,
                     this) != 0) {
    cleanup();
    throw((std::string) "Failed to initialize thread");
  }

  startok = 0;

  for (int i = 0; i < 400000; i++) { 
     if (*ptr == 2001) {
      break;
    }   
    if (*ptr == 2000) {
      startok = 1;
      break;
    }
    usleep(100);
  }

  *ptr = 6000;

  if (startok == 0) {
    m_runok = 1;
    cleanup();
    return;
  }

  /*
  #ifdef EMBED
      if(pthread_create(&m_EMBEDThread, NULL,
  RemotePluginClient::callEMBEDThread, this) != 0)
      {
      cleanup();
      throw((std::string)"Failed to initialize thread2");
      }
  #endif
  */
  m_syncok = 1;
}

void RemotePluginClient::cleanup() {
  if (m_threadbreak == 0)
    m_threadbreak = 1;
  /*
      if (m_shm)
          for (int i=0;i<100000;i++)
          {
              usleep(100);
              if (m_threadbreakexit)
              break;
          }
  */

  /*
  #ifdef EMBED
  if(m_threadbreakembed == 0)
  m_threadbreakembed = 1;

      if (m_shm)
          for (int i=0;i<100000;i++)
          {
              usleep(100);
              if (m_threadbreakexitembed)
              break;
          }
  #endif
  */

  if (m_AMThread)
    pthread_join(m_AMThread, NULL);
  /*
  #ifdef EMBED
             for(int i=0;i<5000;i++)
              {
                  if(editopen == 0)
                  break;
                  usleep(100);
              }
      if (m_EMBEDThread)
          pthread_join(m_EMBEDThread, NULL);
  #endif
  */
  if (m_shm) {
    munmap(m_shm, m_shmSize);
    m_shm = 0;
  }

  if (m_shmFd >= 0) {
    close(m_shmFd);
    m_shmFd = -1;
  }

  if (m_shmFileName) {
    unlink(m_shmFileName);
    free(m_shmFileName);
    m_shmFileName = 0;
  }
}

std::string RemotePluginClient::getFileIdentifiers() {
  std::string id;

  id += m_shmFileName + strlen(m_shmFileName) - 6;

  //  std::cerr << "Returning file identifiers: " << id << std::endl;
  return id;
}

int RemotePluginClient::sizeShm() {
  if (m_shm)
    return 0;

int pagesize = sysconf(_SC_PAGESIZE);
int chunksize;
int chunks;
int chunkrem;

    chunksize = PROCESSSIZE;
    chunks = chunksize / pagesize;
    chunkrem = chunksize % pagesize;

    if(chunkrem > 0)
    chunks += 1;

    int processsize = chunks * pagesize;

    chunksize = VSTEVENTS_PROCESS;
    chunks = chunksize / pagesize;
    chunkrem = chunksize % pagesize;

    if(chunkrem > 0)
    chunks += 1;

    int vsteventsprocess = chunks * pagesize;

    chunksize = CHUNKSIZEMAX;
    chunks = chunksize / pagesize;
    chunkrem = chunksize % pagesize;

    if(chunkrem > 0)
    chunks += 1;

    int chunksizemax = chunks * pagesize;

    chunksize = VSTEVENTS_SEND;
    chunks = chunksize / pagesize;
    chunkrem = chunksize % pagesize;

    if(chunkrem > 0)
    chunks += 1;

    int vsteventssend = chunks * pagesize;

    chunksize = sizeof(ShmControl);
    chunks = chunksize / pagesize;
    chunkrem = chunksize % pagesize;

    if(chunkrem > 0)
    chunks += 1;

    int chunksizecontrol = chunks * pagesize;

#ifdef PCACHE
    chunksize = PARCACHE;
    chunks = chunksize / pagesize;
    chunkrem = chunksize % pagesize;

    if(chunkrem > 0)
    chunks += 1;

    int parcachesize = chunks * pagesize;

  size_t sz = processsize + vsteventsprocess + chunksizemax + vsteventssend + (chunksizecontrol * 6) + parcachesize;
#else
  size_t sz = processsize + vsteventsprocess + chunksizemax + vsteventssend + (chunksizecontrol * 6);
#endif  

  ftruncate(m_shmFd, sz);
  m_shm = (char *)mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE,
                       m_shmFd, 0);
  if (!m_shm) {
    std::cerr
        << "RemotePluginClient::sizeShm: ERROR: mmap or mremap failed for "
        << sz << " bytes from fd " << m_shmFd << "!" << std::endl;
    m_shmSize = 0;
    return 1;
  } else {
    madvise(m_shm, sz, MADV_SEQUENTIAL | MADV_WILLNEED | MADV_DONTFORK);
    memset(m_shm, 0, sz);
    m_shmSize = sz;

    if (mlock(m_shm, sz) != 0)
      perror("mlock fail1");
  }

  m_shm2 = &m_shm[processsize];
  m_shm3 = &m_shm[processsize + vsteventsprocess];
  m_shm4 = &m_shm[processsize + vsteventsprocess + chunksizemax];
  m_shm5 = &m_shm[processsize + vsteventsprocess + chunksizemax + vsteventssend];

#ifdef PCACHE
  m_shm6 = &m_shm[processsize + vsteventsprocess + chunksizemax + vsteventssend + (chunksizecontrol * 6)];
#endif  

  m_shmControl = (ShmControl *)m_shm5;
//  memset(m_shmControl, 0, sizeof(ShmControl));
  m_shmControl2 = (ShmControl *)&m_shm5[chunksizecontrol];
//  memset(m_shmControl2, 0, sizeof(ShmControl));
  m_shmControl3 = (ShmControl *)&m_shm5[chunksizecontrol * 2];
//  memset(m_shmControl3, 0, sizeof(ShmControl));
  m_shmControl4 = (ShmControl *)&m_shm5[chunksizecontrol * 3];
//  memset(m_shmControl4, 0, sizeof(ShmControl));
  m_shmControl5 = (ShmControl *)&m_shm5[chunksizecontrol * 4];
//  memset(m_shmControl5, 0, sizeof(ShmControl));
  m_shmControl6 = (ShmControl *)&m_shm5[chunksizecontrol * 5];
//  memset(m_shmControl6, 0, sizeof(ShmControl));  

  m_threadbreak = 0;
  m_threadbreakexit = 0;

  /*
  #ifdef EMBED
      m_threadbreakembed = 0;
      m_threadbreakexitembed = 0;
  #endif
  */

  return 0;
}

int RemotePluginClient::getVersion() {
  int retval;

  m_shmControl3->ropcode = RemotePluginGetVersion;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->value;
  return retval;
}

std::string RemotePluginClient::getName() {
  char retval[512];

  m_shmControlptr->ropcode = RemotePluginGetName;
  waitForServer(m_shmControlptr);
  retval[0] = '\0';
  strcpy(retval, m_shmControlptr->retstr);
  return retval;
}

std::string RemotePluginClient::getMaker() {
  char retval[512];

  m_shmControlptr->ropcode = RemotePluginGetMaker;
  waitForServer(m_shmControlptr);
  retval[0] = '\0';
  strcpy(retval, m_shmControlptr->retstr);
  return retval;
}

void RemotePluginClient::setBufferSize(int s) {
  if (s <= 0)
    return;

  if (s == m_bufferSize)
    return;

  m_bufferSize = s;

  m_shmControlptr->ropcode = RemotePluginSetBufferSize;
  m_shmControlptr->value = s;
  waitForServer(m_shmControlptr);
}

void RemotePluginClient::setSampleRate(int s) {
  m_shmControlptr->ropcode = RemotePluginSetSampleRate;
  m_shmControlptr->value = s;
  waitForServer(m_shmControlptr);
}

void RemotePluginClient::reset() { return; }

void RemotePluginClient::terminate() { return; }

int RemotePluginClient::getEffInt(int opcode, int value) {
  int retval;

  m_shmControl3->ropcode = RemotePluginGetEffInt;
  m_shmControl3->opcode = opcode;
  m_shmControl3->value = value;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->retint;
  return retval;
}

std::string RemotePluginClient::getEffString(int opcode, int index) {
  char retval[512];

  m_shmControlptr->ropcode = RemotePluginGetEffString;
  m_shmControlptr->opcode = opcode;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retval[0] = '\0';
  strcpy(retval, m_shmControlptr->retstr);
  return retval;
}

std::string RemotePluginClient::getParameterName(int p) {
  char retval[512];

  m_shmControlptr->ropcode = RemotePluginGetParameterName;
  m_shmControlptr->value = p;
  waitForServer(m_shmControlptr);
  retval[0] = '\0';
  strcpy(retval, m_shmControlptr->retstr);
  return retval;
}

std::string RemotePluginClient::getParameterLabel(int p) {
  char retval[512];

  m_shmControlptr->ropcode = RemotePluginGetParameterLabel;
  m_shmControlptr->value = p;
  waitForServer(m_shmControlptr);
  retval[0] = '\0';
  strcpy(retval, m_shmControlptr->retstr);
  return retval;
}

std::string RemotePluginClient::getParameterDisplay(int p) {
  char retval[512];

  m_shmControlptr->ropcode = RemotePluginGetParameterDisplay;
  m_shmControlptr->value = p;
  waitForServer(m_shmControlptr);
  retval[0] = '\0';
  strcpy(retval, m_shmControlptr->retstr);
  return retval;
}

int RemotePluginClient::getParameterCount() {
  int retval;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return 0;
  }

  m_shmControl3->ropcode = RemotePluginGetParameterCount;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->retint;
  return retval;
}

#ifdef WAVES
int RemotePluginClient::getShellName(char *ptr) {
  int retval;

  m_shmControlptr->ropcode = RemotePluginGetShellName;
  waitForServer(m_shmControlptr);
  strcpy(ptr, m_shmControlptr->retstr);
  retval = m_shmControlptr->retint;
  return retval;
}
#endif

void RemotePluginClient::setParameter(int p, float v) {
  ShmControl *m_shmControlptr4;
  m_shmControlptr4 = m_shmControl4;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return;
  }

#ifdef PCACHE
  ParamState* pstate = (ParamState*)m_shm6; 

  if(p < 10000)
  { 
  if(pstate[p].value != v)
  {      
  pstate[p].changed = 1;
  pstate[p].valueupdate = v; 
  } 
  }
#else
  m_shmControlptr4->ropcode = RemotePluginSetParameter;
  m_shmControlptr4->value = p;
  m_shmControlptr4->floatvalue = v;
  waitForServer(m_shmControlptr4);
#endif  
}

float RemotePluginClient::getParameter(int p) {
  ShmControl *m_shmControlptr5;
  m_shmControlptr5 = m_shmControl5;

  float retval;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return 0;
  }

#ifdef PCACHE
  if(p < 10000)
  {    
  ParamState* pstate = (ParamState*)m_shm6;        
  return pstate[p].value;
  }
#else
  m_shmControlptr5->ropcode = RemotePluginGetParameter;
  m_shmControlptr5->value = p;
  waitForServer(m_shmControlptr5);
  retval = m_shmControlptr5->retfloat;
  return retval;
#endif  
}

float RemotePluginClient::getParameterDefault(int p) { return 0.0; }

void RemotePluginClient::getParameters(int p0, int pn, float *v) { return; }

void RemotePluginClient::waitForServer(ShmControl *m_shmControlptr3) {
  fpost2(m_shmControlptr3, &m_shmControlptr3->runServer);

  if (fwait2(m_shmControlptr3, &m_shmControlptr3->runClient, 60000)) {
    if (m_inexcept == 0)
      RemotePluginClosedException();
  }

  m_shmControlptr3->ropcode = RemotePluginNoOpcode;
}

void RemotePluginClient::waitForServer2exit() {
  fpost(m_shmControl2, &m_shmControl2->runServer);
  fpost(m_shmControl2, &m_shmControl2->runClient);
}

void RemotePluginClient::waitForServer3exit() {
  fpost(m_shmControl3, &m_shmControl3->runServer);
  fpost(m_shmControl3, &m_shmControl3->runClient);
}

void RemotePluginClient::waitForServer4exit() {
  fpost(m_shmControl4, &m_shmControl4->runServer);
  fpost(m_shmControl4, &m_shmControl4->runClient);
}

void RemotePluginClient::waitForServer5exit() {
  fpost(m_shmControl5, &m_shmControl5->runServer);
  fpost(m_shmControl5, &m_shmControl5->runClient);
}

void RemotePluginClient::waitForServer6exit() {
  fpost(m_shmControl6, &m_shmControl6->runServer);
  fpost(m_shmControl6, &m_shmControl6->runClient);
}

void RemotePluginClient::waitForClientexit() {
  fpost(m_shmControl, &m_shmControl->runClient);
  fpost(m_shmControl, &m_shmControl->runServer);
}

void RemotePluginClient::process(float **inputs, float **outputs,
                                 int sampleFrames) {
  ShmControl *m_shmControlptr2;

  m_shmControlptr2 = m_shmControl2;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return;
  }
  if ((m_bufferSize <= 0) || (sampleFrames <= 0)) {
    return;
  }
  if (m_numInputs < 0) {
    return;
  }
  if (m_numOutputs < 0) {
    return;
  }

  if (m_updateio == 1) {
    m_numInputs = m_updatein;
    m_numOutputs = m_updateout;
    m_shmControlptr2->ropcode = RemotePluginProcess;
    m_shmControlptr2->value2 = -1;
    waitForServer(m_shmControlptr2);
    m_updateio = 0;
    return;
  }

  if (((m_numInputs + m_numOutputs) * m_bufferSize * sizeof(float)) >=
      (PROCESSSIZE))
    return;

#ifdef NEWTIME
  if (m_audioMaster && theEffect && m_shm) {
    timeInfo = 0;

    timeInfo = (VstTimeInfo *)m_audioMaster(
        theEffect, audioMasterGetTime, 0,
        kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstCyclePosValid |
            kVstTimeSigValid,
        0, 0);

    if (timeInfo) {
      //  memcpy(m_shmControl->timeget, timeInfo, sizeof(VstTimeInfo));

      //	 printf("servercount %d\n", serverthreadcount);
      ShmControl *m_shmControlptrtime = m_shmControl;
      m_shmControlptrtime->timeinit = 1;
      memcpy(m_shmControlptrtime->timeget, timeInfo, sizeof(VstTimeInfo));
    }
  }
#endif

  size_t blocksz = sampleFrames * sizeof(float);

  if (m_numInputs > 0) {
    for (int i = 0; i < m_numInputs; ++i)
      memcpy(m_shm + i * blocksz, inputs[i], blocksz);
  }

  m_shmControlptr2->ropcode = RemotePluginProcess;
  m_shmControlptr2->value2 = sampleFrames;

  waitForServer(m_shmControlptr2);

  if (m_numOutputs > 0) {
    for (int i = 0; i < m_numOutputs; ++i)
      memcpy(outputs[i], m_shm + i * blocksz, blocksz);
  }
  return;
}

#ifdef DOUBLEP
void RemotePluginClient::processdouble(double **inputs, double **outputs,
                                       int sampleFrames) {
  ShmControl *m_shmControlptr2;

  m_shmControlptr2 = m_shmControl2;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return;
  }
  if ((m_bufferSize <= 0) || (sampleFrames <= 0)) {
    return;
  }
  if (m_numInputs < 0) {
    return;
  }
  if (m_numOutputs < 0) {
    return;
  }

  if (m_updateio == 1) {
    m_numInputs = m_updatein;
    m_numOutputs = m_updateout;
    m_shmControlptr2->ropcode = RemotePluginProcessDouble;
    m_shmControlptr2->value2 = -1;
    waitForServer(m_shmControlptr2);
    m_updateio = 0;
    return;
  }

  if (((m_numInputs + m_numOutputs) * m_bufferSize * sizeof(double)) >=
      (PROCESSSIZE))
    return;

#ifdef NEWTIME
  if (m_audioMaster && theEffect && m_shm) {
    timeInfo = 0;

    timeInfo = (VstTimeInfo *)m_audioMaster(
        theEffect, audioMasterGetTime, 0,
        kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstCyclePosValid |
            kVstTimeSigValid,
        0, 0);

    if (timeInfo) {
      //  memcpy(m_shmControl->timeget, timeInfo, sizeof(VstTimeInfo));

      //	 printf("servercount %d\n", serverthreadcount);
      ShmControl *m_shmControlptrtime = m_shmControl;
      m_shmControlptrtime->timeinit = 1;
      memcpy(m_shmControlptrtime->timeget, timeInfo, sizeof(VstTimeInfo));
    }
  }
#endif

  size_t blocksz = sampleFrames * sizeof(double);

  if (m_numInputs > 0) {
    for (int i = 0; i < m_numInputs; ++i)
      memcpy(m_shm + i * blocksz, inputs[i], blocksz);
  }

  m_shmControlptr2->ropcode = RemotePluginProcessDouble;
  m_shmControlptr2->value2 = sampleFrames;

  waitForServer(m_shmControlptr2);

  if (m_numOutputs > 0) {
    for (int i = 0; i < m_numOutputs; ++i)
      memcpy(outputs[i], m_shm + i * blocksz, blocksz);
  }
  return;
}

bool RemotePluginClient::setPrecision(int value) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteSetPrecision;
  m_shmControlptr->value = value;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  return retbool;
}
#endif

int RemotePluginClient::processVstEvents(VstEvents *evnts) {
  int ret;
  int eventnum;
  int eventnum2;  
  int *ptr;
  int sizeidx = 0;

  if (!m_shm)
    return 0;

  if (!evnts)
    return 0;

  if ((evnts->numEvents <= 0) || (m_inexcept == 1) || (m_finishaudio == 1))
    return 0;

  ptr = (int *)m_shm2;
  eventnum = evnts->numEvents;
  eventnum2 = 0;  
  sizeidx = sizeof(int);

 // if (eventnum > VSTSIZE)
 //   eventnum = VSTSIZE;

  for (int i = 0; i < eventnum; i++) {
    VstEvent *pEvent = evnts->events[i];

    if (pEvent->type == kVstSysExType) {
    continue;
    } else {
      unsigned int size = (2 * sizeof(VstInt32)) + evnts->events[i]->byteSize;
      memcpy(&m_shm2[sizeidx], evnts->events[i], size);
      sizeidx += size;
      if((sizeidx) >= VSTEVENTS_PROCESS)
      break;   
      eventnum2++;   
    }
  }

  *ptr = eventnum2;
  ret = evnts->numEvents;
 // ret = eventnum2;

#ifdef OLDMIDI
  if(eventnum2 > 0)
  {
  ShmControl *m_shmControlptr2;

  m_shmControlptr2 = m_shmControl2;

  m_shmControlptr2->ropcode = RemotePluginProcessEvents;
  waitForServer2(m_shmControlptr2);
  ret = m_shmControlptr2->retint;  
  }
#endif

  return ret;
}

#ifdef VESTIGE
bool RemotePluginClient::getEffInProp(int index, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteInProp;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  memcpy(ptr, m_shmControlptr->vret, sizeof(vinfo));
  return retbool;
}

bool RemotePluginClient::getEffOutProp(int index, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteOutProp;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  memcpy(ptr, m_shmControlptr->vret, sizeof(vinfo));
  return retbool;
}
#else
bool RemotePluginClient::getEffInProp(int index, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteInProp;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  memcpy(ptr, m_shmControlptr->vpin, sizeof(VstPinProperties));
  return retbool;
}

bool RemotePluginClient::getEffOutProp(int index, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteOutProp;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  memcpy(ptr, m_shmControlptr->vpin, sizeof(VstPinProperties));
  return retbool;
}
#endif

#ifdef MIDIEFF
bool RemotePluginClient::getEffMidiKey(int index, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteMidiKey;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  memcpy(ptr, m_shmControlptr->midikey, sizeof(MidiKeyName));
  return retbool;
}

bool RemotePluginClient::getEffMidiProgName(int index, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteMidiProgName;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  memcpy(ptr, m_shmControlptr->midiprogram, sizeof(MidiProgramName));
  return retbool;
}

bool RemotePluginClient::getEffMidiCurProg(int index, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteMidiCurProg;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  memcpy(ptr, m_shmControlptr->midiprogram, sizeof(MidiProgramName));
  return retbool;
}

bool RemotePluginClient::getEffMidiProgCat(int index, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteMidiProgCat;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  memcpy(ptr, m_shmControlptr->midiprogramcat, sizeof(MidiProgramCategory));
  return retbool;
}

bool RemotePluginClient::getEffMidiProgCh(int index) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteMidiProgCh;
  m_shmControlptr->value = index;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  return retbool;
}

bool RemotePluginClient::setEffSpeaker(VstIntPtr value, void *ptr) {
  bool retbool;

  memcpy(m_shmControlptr->vstspeaker2, ptr, sizeof(VstSpeakerArrangement));
  memcpy(m_shmControlptr->vstspeaker, (VstIntPtr *)value,
         sizeof(VstSpeakerArrangement));
  m_shmControlptr->ropcode = RemoteSetSpeaker;
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  return retbool;
}

bool RemotePluginClient::getEffSpeaker(VstIntPtr value, void *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemoteGetSpeaker;
  waitForServer(m_shmControlptr);
  memcpy(ptr, m_shmControlptr->vstspeaker2, sizeof(VstSpeakerArrangement));
  memcpy((VstIntPtr *)value, m_shmControlptr->vstspeaker,
         sizeof(VstSpeakerArrangement));
  retbool = m_shmControlptr->retbool;
  return retbool;
}
#endif

#ifdef CANDOEFF
bool RemotePluginClient::getEffCanDo(char *ptr) {
  bool retbool;

  m_shmControlptr->ropcode = RemotePluginEffCanDo;
  strcpy(m_shmControlptr->sendstr, ptr);
  waitForServer(m_shmControlptr);
  retbool = m_shmControlptr->retbool;
  return retbool;
}
#endif

void RemotePluginClient::setDebugLevel(RemotePluginDebugLevel level) { return; }

bool RemotePluginClient::warn(std::string str) { return false; }

void RemotePluginClient::showGUI() {
#ifdef EMBED
  memcpy(m_shmControl3->wret, winm, sizeof(winmessage));
#endif
  m_shmControl3->ropcode = RemotePluginShowGUI;
  waitForServer(m_shmControl3);
#ifdef EMBED
  memcpy(winm, m_shmControl3->wret, sizeof(winmessage));
#endif
}

void RemotePluginClient::hideGUI() {
  m_shmControl3->ropcode = RemotePluginHideGUI;
  waitForServer(m_shmControl3);
}

#ifdef EMBED
void RemotePluginClient::openGUI() {
  m_shmControl3->ropcode = RemotePluginOpenGUI;
  waitForServer(m_shmControl3);
}
#endif

#define errorexit 9999

void RemotePluginClient::effVoidOp(int opcode) {

  if (opcode == errorexit) {
    m_threadbreak = 1;
    /*
        if (m_shm)
            for (int i=0;i<100000;i++)
            {
                usleep(100);
                if (m_threadbreakexit)
                break;
            }
    */

    /*
    #ifdef EMBED
            m_threadbreakembed = 1;

        if (m_shm)
            for (int i=0;i<100000;i++)
            {
                if(m_threadbreakexitembed)
                break;
                usleep(100);
            }
    #endif
    */

    m_finishaudio = 1;
    m_shmControl3->ropcode = RemotePluginDoVoid;
    m_shmControl3->opcode = opcode;
  } else if (opcode == effClose) {
    waitForClientexit();

    m_threadbreak = 1;
    /*
        if (m_shm)
            for (int i=0;i<100000;i++)
            {
                usleep(100);
                if (m_threadbreakexit)
                break;
            }
    */

    /*
    #ifdef EMBED
            m_threadbreakembed = 1;

        if (m_shm)
            for (int i=0;i<100000;i++)
            {
                if (m_threadbreakexitembed)
                break;
                usleep(100);
            }
    #endif
    */

    m_finishaudio = 1;
    m_shmControl3->ropcode = RemotePluginDoVoid;
    m_shmControl3->opcode = opcode;

    waitForServer(m_shmControl3);

    waitForServer2exit();
    waitForServer3exit();
    waitForServer4exit();
    waitForServer5exit();
    waitForServer6exit();
  } else {
    m_shmControlptr->ropcode = RemotePluginDoVoid;
    m_shmControlptr->opcode = opcode;
    waitForServer(m_shmControlptr);
  }
}

int RemotePluginClient::effVoidOp2(int opcode, int index, int value,
                                   float opt) {
  int retval;

  m_shmControlptr->ropcode = RemotePluginDoVoid2;
  m_shmControlptr->opcode = opcode;
  m_shmControlptr->value = index;
  m_shmControlptr->value2 = value;
  m_shmControlptr->value3 = opt;
  waitForServer(m_shmControlptr);
  retval = m_shmControlptr->retint;
  return retval;
}

int RemotePluginClient::canBeAutomated(int param) {
  int retval;

  m_shmControlptr->ropcode = RemotePluginCanBeAutomated;
  m_shmControlptr->value = param;
  waitForServer(m_shmControlptr);
  retval = m_shmControlptr->retint;
  return retval;
}

int RemotePluginClient::EffectOpen() {
  if (m_threadinit == 1)
    return 0;

  m_shmControl3->ropcode = RemotePluginEffectOpen;
  waitForServer(m_shmControl3);

#ifdef WAVES
  wavesthread = m_shmControl3->retint;
  if (wavesthread == 1)
    theEffect->flags |= effFlagsHasEditor;
#endif

  m_threadinit = 1;

  return 1;
}

int RemotePluginClient::getFlags() {
  int retval;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return 0;
  }

  m_shmControl3->ropcode = RemotePluginGetFlags;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->retint;
  return retval;
}

int RemotePluginClient::getinitialDelay() {
  int retval;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return 0;
  }

  m_shmControl3->ropcode = RemotePluginGetinitialDelay;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->retint;
  m_delay = retval;
  return retval;
}

int RemotePluginClient::getInputCount() {
  int retval;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return 0;
  }

  m_shmControl3->ropcode = RemotePluginGetInputCount;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->retint;
  m_numInputs = retval;
  return retval;
}

int RemotePluginClient::getOutputCount() {
  int retval;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return 0;
  }

  m_shmControl3->ropcode = RemotePluginGetOutputCount;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->retint;
  m_numOutputs = retval;
  return retval;
}

int RemotePluginClient::getProgramCount() {
  int retval;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return 0;
  }

  m_shmControl3->ropcode = RemotePluginGetProgramCount;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->retint;
  return retval;
}

int RemotePluginClient::getUID() {
  int retval;

  if (m_inexcept == 1 || m_finishaudio == 1) {
    return 0;
  }

  m_shmControl3->ropcode = RemotePluginUniqueID;
  waitForServer(m_shmControl3);
  retval = m_shmControl3->retint;
  return retval;
}

int RemotePluginClient::getProgramNameIndexed(int n, char *ptr) {
  char retvalstr[512];
  int retval;

  m_shmControlptr->ropcode = RemotePluginGetProgramNameIndexed;
  m_shmControlptr->value = n;
  waitForServer(m_shmControlptr);
  //    retvalstr[0]='\0';
  //   strcpy(retvalstr, m_shmControlptr->retstr);
  //   strcpy(ptr, retvalstr);
  strcpy(ptr, m_shmControlptr->retstr);
  retval = m_shmControlptr->retint;
  return retval;
}

std::string RemotePluginClient::getProgramName() {
  char retval[512];

  m_shmControlptr->ropcode = RemotePluginGetProgramName;
  waitForServer(m_shmControlptr);
  retval[0] = '\0';
  strcpy(retval, m_shmControlptr->retstr);
  return retval;
}

void RemotePluginClient::setCurrentProgram(int n) {
  m_shmControlptr->ropcode = RemotePluginSetCurrentProgram;
  m_shmControlptr->value = n;
  waitForServer(m_shmControlptr);
}

int RemotePluginClient::getProgram() {
  int retval;

  m_shmControlptr->ropcode = RemotePluginGetProgram;
  waitForServer(m_shmControlptr);
  retval = m_shmControlptr->retint;
  return retval;
}

int RemotePluginClient::getChunk(void **ptr, int bank_prg) {
#ifdef CHUNKBUF
  int chunksize;
  int chunks;
  int chunkrem;
  int sz;
	
  ShmControl *m_shmControlptr = m_shmControl3;  	

  m_shmControlptr->ropcode = RemotePluginGetChunk;
  m_shmControlptr->value = bank_prg;
  waitForServer(m_shmControlptr);

  sz = m_shmControlptr->retint;

  if (sz <= 0) {
    *ptr = m_shm3;
    return 0;
  }

  if (sz >= CHUNKSIZEMAX) {
    if (chunk_ptr)
      free(chunk_ptr);

    chunk_ptr = (char *)malloc(sz);

    if (!chunk_ptr)
      return 0;

    chunksize = CHUNKSIZE;
    chunks = sz / chunksize;
    chunkrem = sz % chunksize;

    for (int i = 0; i < chunks; ++i) {
      m_shmControlptr->ropcode = RemotePluginGetBuf;
      m_shmControlptr->value = chunksize;
      m_shmControlptr->value2 = i * chunksize;
      waitForServer(m_shmControlptr);

      memcpy(&chunk_ptr[i * chunksize], m_shm3, chunksize);
    }

    if (chunkrem) {
      m_shmControlptr->ropcode = RemotePluginGetBuf;
      m_shmControlptr->value = chunkrem;
      m_shmControlptr->value2 = chunks * chunksize;
      waitForServer(m_shmControlptr);

      memcpy(&chunk_ptr[chunks * chunksize], m_shm3, chunkrem);
    }

    *ptr = chunk_ptr;

    return sz;
  } else {
    if (sz >= (CHUNKSIZEMAX) || sz <= 0) {
      *ptr = m_shm3;
      return 0;
    }

    *ptr = m_shm3;
    return sz;
  }
#else
  int sz;

  m_shmControlptr->ropcode = RemotePluginGetChunk;
  m_shmControlptr->value = bank_prg;
  waitForServer(m_shmControlptr);

  sz = m_shmControlptr->retint;

  if (sz >= (CHUNKSIZEMAX) || sz <= 0) {
    *ptr = m_shm3;
    return 0;
  }

  *ptr = m_shm3;
  return sz;
#endif
}

int RemotePluginClient::setChunk(void *ptr, int sz, int bank_prg) {
  int retval;
#ifdef CHUNKBUF
  char *ptridx;
  int sz2;
  int chunksize;
  int chunks;
  int chunkrem;

  if (sz <= 0)
    return 0;
	
   ShmControl *m_shmControlptr = m_shmControl3;  	

   if (sz >= CHUNKSIZEMAX) {
    ptridx = (char *)ptr;
    sz2 = sz;

    chunksize = CHUNKSIZE;
    chunks = sz / chunksize;
    chunkrem = sz % chunksize;

    for (int i = 0; i < chunks; ++i) {
      memcpy(m_shm3, &ptridx[i * chunksize], chunksize);

      m_shmControlptr->ropcode = RemotePluginSetBuf;
      m_shmControlptr->value = chunksize;
      m_shmControlptr->value2 = i * chunksize;
      m_shmControlptr->value3 = sz2;
      waitForServer(m_shmControlptr);

      sz2 = -1;
    }

    if (chunkrem) {
      if (!chunks)
        sz2 = chunkrem;
      else
        sz2 = -1;

      memcpy(m_shm3, &ptridx[chunks * chunksize], chunkrem);

      m_shmControlptr->ropcode = RemotePluginSetBuf;
      m_shmControlptr->value = chunkrem;
      m_shmControlptr->value2 = chunks * chunksize;
      m_shmControlptr->value3 = sz2;
      waitForServer(m_shmControlptr);
    }

    m_shmControlptr->ropcode = RemotePluginSetChunk;
    m_shmControlptr->value = sz;
    m_shmControlptr->value2 = bank_prg;
    waitForServer(m_shmControlptr);

    retval = m_shmControlptr->retint;
    return retval;
  } else {
    if (sz >= (CHUNKSIZEMAX) || sz <= 0)
      return 0;

    m_shmControlptr->ropcode = RemotePluginSetChunk;
    m_shmControlptr->value = sz;
    m_shmControlptr->value2 = bank_prg;

    memcpy(m_shm3, ptr, sz);

    waitForServer(m_shmControlptr);

    retval = m_shmControlptr->retint;
    return retval;
  }
#else
  if (sz >= (CHUNKSIZEMAX) || sz <= 0)
    return 0;

  m_shmControlptr->ropcode = RemotePluginSetChunk;
  m_shmControlptr->value = sz;
  m_shmControlptr->value2 = bank_prg;

  memcpy(m_shm3, ptr, sz);

  waitForServer(m_shmControlptr);

  retval = m_shmControlptr->retint;
  return retval;
#endif
}

void RemotePluginClient::RemotePluginClosedException() {
  m_inexcept = 1;

  waitForClientexit();

  m_threadbreak = 1;

  /*
  #ifdef EMBED
  m_threadbreakembed = 1;

      if (m_shm)
          for (int i=0;i<100000;i++)
          {
              usleep(100);
              if (m_threadbreakexitembed)
              break;
          }
  #endif
  */

  m_finishaudio = 1;

  sleep(5);

  if (m_AMThread)
    pthread_join(m_AMThread, NULL);
  /*
  #ifdef EMBED
      if (m_EMBEDThread)
          pthread_join(m_EMBEDThread, NULL);
  #endif
  */
  // effVoidOp(errorexit);

  effVoidOp(effClose);

  sleep(5);

  memset(theEffect, 0, sizeof(AEffect));
  theEffect = 0;
  waitForServer2exit();
  waitForServer3exit();
  waitForServer4exit();
  waitForServer5exit();
  waitForServer6exit();
}

using namespace std;

bool RemotePluginClient::fwait2(ShmControl *m_shmControlptr,
                                std::atomic_int *futexp, int ms) {
  timespec timeval;
  int retval;

  if (ms > 0) {
    timeval.tv_sec = ms / 1000;
    timeval.tv_nsec = (ms %= 1000) * 1000000;
  }

  for (;;) {
    int value = atomic_load_explicit(futexp, std::memory_order_seq_cst);
    if ((*futexp != 0) &&
        (std::atomic_compare_exchange_strong(futexp, &value, value - 1) > 0))
      break;
     std::atomic_fetch_add_explicit((std::atomic_int *)&m_shmControlptr->nwaitersclient, 1, std::memory_order_seq_cst);
    retval = syscall(SYS_futex, futexp, FUTEX_WAIT, 0, &timeval, NULL, 0);
     std::atomic_fetch_sub_explicit((std::atomic_int *)&m_shmControlptr->nwaitersclient, 1, std::memory_order_seq_cst);
    if (retval == -1 && errno != EAGAIN)
      return true;
  }
  return false;
}

bool RemotePluginClient::fpost2(ShmControl *m_shmControlptr,
                                std::atomic_int *futexp) {
  int retval;
  int nval =
      std::atomic_fetch_add_explicit(futexp, 1, std::memory_order_seq_cst);
   int value = atomic_load_explicit((std::atomic_int *)&m_shmControlptr->nwaitersserver, std::memory_order_seq_cst); 
   if(value > 0)
  //{
  retval = syscall(SYS_futex, futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
  //}
  /*
          if (retval  == -1)
          return true;
  */
  return false;
}

bool RemotePluginClient::fwait(ShmControl *m_shmControlptr,
                               std::atomic_int *futexp, int ms) {
  timespec timeval;
  int retval;

  if (ms > 0) {
    timeval.tv_sec = ms / 1000;
    timeval.tv_nsec = (ms %= 1000) * 1000000;
  }

  for (;;) {
    int value = atomic_load_explicit(futexp, std::memory_order_seq_cst);
    if ((*futexp != 0) &&
        (std::atomic_compare_exchange_strong(futexp, &value, value - 1) > 0))
      break;
    retval = syscall(SYS_futex, futexp, FUTEX_WAIT, 0, &timeval, NULL, 0);
    if (retval == -1 && errno != EAGAIN)
      return true;
  }
  return false;
}

bool RemotePluginClient::fpost(ShmControl *m_shmControlptr,
                               std::atomic_int *futexp) {
  int retval;

  std::atomic_fetch_add_explicit(futexp, 1, std::memory_order_seq_cst);
  retval = syscall(SYS_futex, futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
  /*
          if (retval  == -1)
          return true;
  */
  return false;
}
