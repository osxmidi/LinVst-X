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



#include "remotepluginclient.h"

#define hidegui2 77775634

#ifdef XEMBED
#define XEMBED_EMBEDDED_NOTIFY	0
#define XEMBED_FOCUS_OUT 5
#endif

void* RemotePluginClient::AMThread()
{
    int         opcode;
    int         val;
    int         idx;
    float       optval; 
    int         retval;
    int         ok = 1;
    char retstr2[512];

   int timeout = 50;

    VstTimeInfo *timeInfo;

    int         els;
    int         *ptr2;
    int         sizeidx = 0;
    int         size;
    VstEvents   *evptr;

    struct amessage
    {
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

    while (!m_threadbreak)
    {

#ifdef SEM

    if(m_386run == 0)
    {

    timespec ts_timeout;

    clock_gettime(CLOCK_REALTIME, &ts_timeout);
    time_t seconds = timeout / 1000;
    ts_timeout.tv_sec += seconds;
    ts_timeout.tv_nsec += (timeout - seconds * 1000) * 1000000;
    if (ts_timeout.tv_nsec >= 1000000000) {
        ts_timeout.tv_nsec -= 1000000000;
        ts_timeout.tv_sec++;
    }

    if (sem_timedwait(&m_shmControl->runServer, &ts_timeout)) {
        if (errno == ETIMEDOUT) {
          continue;
        } else {
        if(m_inexcept == 0)
            RemotePluginClosedException();
        }
    }
}
else
{
    if (fwait(&m_shmControl->runServer386, timeout)) {
        if (errno == ETIMEDOUT) {
            continue;
        } else {
        if(m_inexcept == 0)
            RemotePluginClosedException();
        }
    }
}

#else

    if (fwait(&m_shmControl->runServer, timeout)) {
        if (errno == ETIMEDOUT) {
            continue;
        } else {
        if(m_inexcept == 0)
            RemotePluginClosedException();
        }
    }

#endif

    while (dataAvailable(&m_shmControl->ringBuffer) && theEffect) {

     if(m_threadbreak)
    break;
   
                opcode = -1;

   tryReadring(&m_shmControl->ringBuffer, &opcode, sizeof(int));

                switch(opcode)
                {
                    case audioMasterGetTime:       
                    val = readIntring(&m_shmControl->ringBuffer);
                    timeInfo = (VstTimeInfo *) m_audioMaster(theEffect, audioMasterGetTime, 0, val, 0, 0);
                    if(timeInfo)
                    {
                    memcpy((VstTimeInfo*)&m_shm3[FIXED_SHM_SIZE3 - sizeof(VstTimeInfo)], timeInfo, sizeof(VstTimeInfo));
                    }
                    break;

                case audioMasterIOChanged:
                    retval = 0;
                    memcpy(&am, &m_shm3[FIXED_SHM_SIZE3], sizeof(am));
                    theEffect->flags = am.flags;
                    theEffect->numPrograms = am.pcount;
                    theEffect->numParams = am.parcount;
                    theEffect->numInputs = am.incount;
                    theEffect->numOutputs = am.outcount;
                    theEffect->initialDelay = am.delay;
                    // m_updateio = 1;
                    retval = m_audioMaster(theEffect, audioMasterIOChanged, 0, 0, 0, 0);
                    memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                    if((am.incount != m_numInputs) || (am.outcount != m_numOutputs))
                    {
                    if ((am.incount + am.outcount) * m_bufferSize * sizeof(float) < (PROCESSSIZE))
                    {
                    m_updateio = 1;
                    m_updatein = am.incount;
                    m_updateout = am.outcount;
                    }
                    }
                    break;

                case audioMasterProcessEvents:  
                    retval = 0;
                  //  val = readIntring(&m_shmControl->ringBuffer);

                    ptr2 = (int *)m_shm3;
                    els = *ptr2;
                    sizeidx = sizeof(int);

                    if (els > VSTSIZE)
                        els = VSTSIZE;

                    evptr = &vstev[0];
                    evptr->numEvents = els;
                    evptr->reserved = 0;

                    for (int i = 0; i < els; i++)
                    {
                        VstEvent* bsize = (VstEvent*) &m_shm3[sizeidx];
                        size = bsize->byteSize + (2*sizeof(VstInt32));
                        evptr->events[i] = bsize;
                        sizeidx += size;
                    }

                    retval = m_audioMaster(theEffect, audioMasterProcessEvents, 0, 0, evptr, 0);
                    memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));               
                    break;
				
                   case audioMasterAutomate:
                   retval = 0;
                   idx = readIntring(&m_shmControl->ringBuffer);
                   optval = readFloatring(&m_shmControl->ringBuffer);
                   retval = m_audioMaster(theEffect, audioMasterAutomate, idx, 0, 0, optval);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;

                   case audioMasterGetAutomationState:
                   retval = 0;
                   retval = m_audioMaster(theEffect, audioMasterGetAutomationState, 0, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;
				
                   case audioMasterBeginEdit:
                   retval = 0;
                   idx = readIntring(&m_shmControl->ringBuffer);
                   retval = m_audioMaster(theEffect, audioMasterBeginEdit, idx, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;

                   case audioMasterEndEdit:
                   retval = 0;
                   idx = readIntring(&m_shmControl->ringBuffer);
                   retval = m_audioMaster(theEffect, audioMasterEndEdit, idx, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;

                   case audioMasterGetInputLatency:
                   retval = 0;
                   retval = m_audioMaster(theEffect, audioMasterGetInputLatency, 0, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;

                   case audioMasterGetOutputLatency:
                   retval = 0;
                   retval = m_audioMaster(theEffect, audioMasterGetOutputLatency, 0, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;
				
		   case audioMasterGetSampleRate:
                   retval = 0;
                   retval = m_audioMaster(theEffect, audioMasterGetSampleRate, 0, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;

                   case audioMasterGetBlockSize:
                   retval = 0;
                   retval = m_audioMaster(theEffect, audioMasterGetBlockSize, 0, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;
#ifdef CANDOEFF                   
                   case audioMasterCanDo:    
                   retval = 0;
                   retstr2[0]='\0';
                   strcpy(retstr2, &m_shm[FIXED_SHM_SIZE3]);
                   retval = m_audioMaster(theEffect, audioMasterCanDo, 0, 0, (char *) retstr2, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;  
#endif				
                   case audioMasterGetVendorString:
                   retstr2[0]='\0';
                   retval = m_audioMaster(theEffect, audioMasterGetVendorString, 0, 0, (char *) retstr2, 0);                   
                   strcpy(&m_shm3[FIXED_SHM_SIZE3], retstr2);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3 + 512], &retval, sizeof(int));
                   break;

                   case audioMasterGetProductString:
                   retstr2[0]='\0';
                   retval = m_audioMaster(theEffect, audioMasterGetProductString, 0, 0, (char *) retstr2, 0);                   
                   strcpy(&m_shm3[FIXED_SHM_SIZE3], retstr2);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3 + 512], &retval, sizeof(int));
                   break;

                   case audioMasterGetVendorVersion:
                   retval = 0;
                   retval = m_audioMaster(theEffect, audioMasterGetVendorVersion, 0, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;
#ifdef EMBED			
#ifdef EMBEDRESIZE
                    case resizegui:
                    retRect.right = readIntring(&m_shmControl->ringBuffer);
                    retRect.bottom = readIntring(&m_shmControl->ringBuffer);
                    retRect.left = 0;
                    retRect.top = 0;
		    width = retRect.right;
                    height = retRect.bottom;
                    if(display && parent && child)
                    {
                    XUnmapWindow(display, child);
                    XResizeWindow(display, parent, width, height);
                    XResizeWindow(display, child, width, height);
                    XMapWindow(display, child);
                    XSync(display, false);
                    XFlush(display);
                    }
                    break;
#endif
#endif
#ifdef WAVES
                   case audioMasterCurrentId:
                   retval = 0;
                   retval = m_audioMaster(theEffect, audioMasterCurrentId, 0, 0, 0, 0);
                   memcpy(&m_shm3[FIXED_SHM_SIZE3], &retval, sizeof(int));
                   break;
#endif
                   case disconnectserver: 
                   m_inexcept = 1;
                   usleep(100000);
        //           memset (theEffect, 0, sizeof(AEffect));
        //           theEffect = 0;        
                   waitForServer2exit(); 
                   waitForServer3exit(); 
                   waitForServer4exit(); 
                   waitForServer5exit();  
  
                   m_threadbreak = 1;
#ifdef EMBED
#ifdef EMBEDTHREAD
                   m_threadbreakembed = 1;
#endif
#endif 
                 break;
                default:
                    break;
                }
              }

#ifdef SEM

     if(m_386run == 0)
     {
     if (sem_post(&m_shmControl->runClient)) {
        std::cerr << "Could not post to semaphore\n";
     }
     }
     else
     { 
     if (fpost(&m_shmControl->runClient386)) {
        std::cerr << "Could not post to semaphore\n";
     }
     }

#else

     if (fpost(&m_shmControl->runClient)) {
        std::cerr << "Could not post to semaphore\n";
     }

#endif
    }
    m_threadbreakexit = 1;
    // pthread_exit(0);
    return 0;
}

#ifdef EMBED
#ifdef XEMBED
#ifndef EMBEDTHREAD 

#define XEMBED_EMBEDDED_NOTIFY	0
#define XEMBED_FOCUS_OUT 5

void void RemotePluginClient::sendXembedMessage(Display* display, Window window, long message, long detail, long data1, long data2)
{
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
#endif

#ifdef EMBED
#ifdef EMBEDTHREAD
void* RemotePluginClient::EMBEDThread()
{
     int     embedcount = 0;

    while (!m_threadbreakembed)
    {

     usleep(10000);

     if(runembed == 1)
     {
     embedcount++;
     }

#ifdef XEMBED
     if((runembed == 1) && (embedcount == 40))
#else
     if((runembed == 1) && (embedcount == 20))
#endif
     {
#ifdef XEMBED
     XReparentWindow(display, child, parent, 0, 0);

  //    usleep(250000);
      sendXembedMessage(display, child, XEMBED_EMBEDDED_NOTIFY, 0, parent, 0);
    //  usleep(250000);

      XMapWindow(display, child);
      XSync(display, false);
      XFlush(display);
         
      usleep(250000);
       
      openGUI();
  
      runembed = 0;
      embedcount = 0;

      XCloseDisplay(display);
      display = 0;
#else
      XReparentWindow(display, child, parent, 0, 0);

      XMapWindow(display, child);
      XSync(display, false);
      XFlush(display);

 //     usleep(100000);

      openGUI();

      runembed = 0;
      embedcount = 0;
#endif
}

}
      m_threadbreakexitembed = 1;
      // pthread_exit(0);
return 0;
}
#endif
#endif

#ifdef EMBED
#ifdef XECLOSE
#define XEMBED_EMBEDDED_NOTIFY	0
#define XEMBED_FOCUS_OUT 5

void RemotePluginClient::sendXembedMessage(Display* display, Window window, long message, long detail, long data1, long data2)
{
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

RemotePluginClient::RemotePluginClient(audioMasterCallback theMaster, Dl_info     info) :
    m_audioMaster(theMaster),
    m_shmFd(-1),
    m_shmFd2(-1),
    m_shmFd3(-1),
    m_AMThread(0),
#ifdef EMBED
#ifdef XECLOSE
    xeclose(0),
#endif
#ifdef EMBEDTHREAD
    m_EMBEDThread(0),
    runembed(0),
    m_threadbreakembed(0),
    m_threadbreakexitembed(0),
#endif
#endif
    m_effeditclose(0),
    m_threadinit(0),
    m_threadbreak(0),
    m_threadbreakexit(0),
    m_shmFileName(0),
    m_shm(0),
    m_shmSize(0),
    m_shmFileName2(0),
    m_shm2(0),
    m_shmSize2(0),
    m_shmFileName3(0),
    m_shm3(0),
    m_shmSize3(0),
    m_shmControlFd(-1),
    m_shmControl(0),
    m_shmControlFileName(0),
    m_shmControl2Fd(-1),
    m_shmControl2(0),
    m_shmControl2FileName(0),
    m_shmControl3Fd(-1),
    m_shmControl3(0),
    m_shmControl3FileName(0),
    m_shmControl4Fd(-1),
    m_shmControl4(0),
    m_shmControl4FileName(0),
    m_shmControl5Fd(-1),
    m_shmControl5(0),
    m_shmControl5FileName(0),
    m_bufferSize(-1),
    m_numInputs(-1),
    m_numOutputs(-1),
    m_updateio(0),
    m_updatein(0),
    m_updateout(0),
#ifdef CHUNKBUF
    chunk_ptr(0),
#endif
    m_inexcept(0),
#ifdef WAVES
    wavesthread(0),
#endif
    m_finishaudio(0),
    m_runok(0),
    m_syncok(0),
    m_386run(0),
    reaperid(0),
#ifdef EMBED
    child(0),
    parent(0),
    display(0),
    handle(0),
    width(0),
    height(0),
    displayerr(0),
    winm(0),
#ifndef XEMBED
#ifdef EMBEDDRAG
    x11_win(0),
    pparent(0),
    root(0),
    children(0),
    numchildren(0),
    parentok(0),
#endif
    eventrun(0),
    eventfinish(0),
#endif
#endif
    theEffect(0)
{
    char tmpFileBase[60];

    srand(time(NULL));
    
    sprintf(tmpFileBase, "/vstrplugin_shm_XXXXXX");
    m_shmFd = shm_mkstemp(tmpFileBase);
    if (m_shmFd < 0)
    {
        cleanup();
        throw((std::string)"Failed to open or create shared memory file");
    }
    m_shmFileName = strdup(tmpFileBase);

    sprintf(tmpFileBase, "/vstrplugin_shn_XXXXXX");
    m_shmFd2 = shm_mkstemp(tmpFileBase);
    if (m_shmFd2 < 0)
    {
        cleanup();
        throw((std::string)"Failed to open or create shared memory file");
    }
    m_shmFileName2 = strdup(tmpFileBase);

    sprintf(tmpFileBase, "/vstrplugin_sho_XXXXXX");
    m_shmFd3 = shm_mkstemp(tmpFileBase);
    if (m_shmFd3 < 0)
    {
        cleanup();
        throw((std::string)"Failed to open or create shared memory file");
    }
    m_shmFileName3 = strdup(tmpFileBase);

    sprintf(tmpFileBase, "/vstrplugin_sha_XXXXXX");
    m_shmControlFd = shm_mkstemp(tmpFileBase);
    if (m_shmControlFd < 0) {
	cleanup();
	throw((std::string)"Failed to open or create shared memory file");
    }

    m_shmControlFileName = strdup(tmpFileBase);
    ftruncate(m_shmControlFd, sizeof(ShmControl));
    m_shmControl = (ShmControl *)mmap(0, sizeof(ShmControl), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_shmControlFd, 0);
    if (!m_shmControl) {
        cleanup();
        throw((std::string)"Failed to mmap shared memory file");
    }

    memset(m_shmControl, 0, sizeof(ShmControl));

    if(mlock(m_shmControl, sizeof(ShmControl)) != 0)
    perror("mlock fail4");

#ifdef SEM
    if(m_386run == 0)
    {
    if (sem_init(&m_shmControl->runServer, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    if (sem_init(&m_shmControl->runClient, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    }
#endif

    sprintf(tmpFileBase, "/vstrplugin_shb_XXXXXX");
    m_shmControl2Fd = shm_mkstemp(tmpFileBase);
    if (m_shmControl2Fd < 0) {
	cleanup();
	throw((std::string)"Failed to open or create shared memory file");
    }

    m_shmControl2FileName = strdup(tmpFileBase);
    ftruncate(m_shmControl2Fd, sizeof(ShmControl));
    m_shmControl2 = (ShmControl *)mmap(0, sizeof(ShmControl), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_shmControl2Fd, 0);
    if (!m_shmControl2) {
        cleanup();
        throw((std::string)"Failed to mmap shared memory file");
    }

    memset(m_shmControl2, 0, sizeof(ShmControl));

    if(mlock(m_shmControl2, sizeof(ShmControl)) != 0)
    perror("mlock fail4");

#ifdef SEM
    if(m_386run == 0)
    {
    if (sem_init(&m_shmControl2->runServer, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    if (sem_init(&m_shmControl2->runClient, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    }
#endif

    sprintf(tmpFileBase, "/vstrplugin_shc_XXXXXX");
    m_shmControl3Fd = shm_mkstemp(tmpFileBase);
    if (m_shmControl3Fd < 0) {
	cleanup();
	throw((std::string)"Failed to open or create shared memory file");
    }

    m_shmControl3FileName = strdup(tmpFileBase);
    ftruncate(m_shmControl3Fd, sizeof(ShmControl));
    m_shmControl3 = (ShmControl *)mmap(0, sizeof(ShmControl), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_shmControl3Fd, 0);
    if (!m_shmControl3) {
        cleanup();
        throw((std::string)"Failed to mmap shared memory file");
    }

    memset(m_shmControl3, 0, sizeof(ShmControl));

    if(mlock(m_shmControl3, sizeof(ShmControl)) != 0)
    perror("mlock fail4");

#ifdef SEM
    if(m_386run == 0)
    {
    if (sem_init(&m_shmControl3->runServer, 1, 0)) {
        cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    if (sem_init(&m_shmControl3->runClient, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    }
#endif

    sprintf(tmpFileBase, "/vstrplugin_shd_XXXXXX");
    m_shmControl4Fd = shm_mkstemp(tmpFileBase);
    if (m_shmControl4Fd < 0) {
	cleanup();
	throw((std::string)"Failed to open or create shared memory file");
    }

    m_shmControl4FileName = strdup(tmpFileBase);
    ftruncate(m_shmControl4Fd, sizeof(ShmControl));
    m_shmControl4 = (ShmControl *)mmap(0, sizeof(ShmControl), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_shmControl4Fd, 0);
    if (!m_shmControl4) {
        cleanup();
        throw((std::string)"Failed to mmap shared memory file");
    }

    memset(m_shmControl4, 0, sizeof(ShmControl));

    if(mlock(m_shmControl4, sizeof(ShmControl)) != 0)
    perror("mlock fail4");

#ifdef SEM
    if(m_386run == 0)
    {
    if (sem_init(&m_shmControl4->runServer, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    if (sem_init(&m_shmControl4->runClient, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    }
#endif

    sprintf(tmpFileBase, "/vstrplugin_she_XXXXXX");
    m_shmControl5Fd = shm_mkstemp(tmpFileBase);
    if (m_shmControl5Fd < 0) {
	cleanup();
	throw((std::string)"Failed to open or create shared memory file");
    }

    m_shmControl5FileName = strdup(tmpFileBase);
    ftruncate(m_shmControl5Fd, sizeof(ShmControl));
    m_shmControl5 = (ShmControl *)mmap(0, sizeof(ShmControl), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_shmControl5Fd, 0);
    if (!m_shmControl5) {
        cleanup();
        throw((std::string)"Failed to mmap shared memory file");
    }

    memset(m_shmControl5, 0, sizeof(ShmControl));

    if(mlock(m_shmControl5, sizeof(ShmControl)) != 0)
    perror("mlock fail4");

#ifdef SEM
    if(m_386run == 0)
    {
    if (sem_init(&m_shmControl5->runServer, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    if (sem_init(&m_shmControl5->runClient, 1, 0)) {
	cleanup();
        throw((std::string)"Failed to initialize shared memory semaphore");
    }
    }
#endif

    if(sizeShm())
    {
    cleanup();
    throw((std::string)"Failed to mmap shared memory file");
    }
 	    
    if(pthread_create(&m_AMThread, NULL, RemotePluginClient::callAMThread, this) != 0)
    {
    cleanup();
    throw((std::string)"Failed to initialize thread");
    }    
 #ifdef EMBED
 #ifdef EMBEDTHREAD
    if(pthread_create(&m_EMBEDThread, NULL, RemotePluginClient::callEMBEDThread, this) != 0)
    {    
    cleanup();
    throw((std::string)"Failed to initialize thread2");
    }
#endif
#endif

    ServerConnect(info);
}

RemotePluginClient::~RemotePluginClient()
{
 if(m_runok == 0)
 {	
    m_threadbreak = 1; 
    waitForClientexit();  
    waitForServer2exit(); 
    waitForServer3exit(); 
    waitForServer4exit(); 
    waitForServer5exit();          
		 
    cleanup();
	 
//    if (theEffect)
//    delete theEffect; 
  	
#ifdef CHUNKBUF
    if (chunk_ptr)
    free(chunk_ptr);
#endif
}
 //    wait(NULL);
}

#ifdef EMBED
#ifndef XEMBED
#ifdef EMBEDDRAG
void RemotePluginClient::eventloop(Display *display, Window pparent, Window parent, Window child, int width, int height, int eventrun2, int parentok, int reaperid)
#else
void RemotePluginClient::eventloop(Display *display, Window parent, Window child, int width, int height, int eventrun2, int reaperid)
#endif
{
   if(!display)
   return;	
	
#ifdef EMBEDDRAG
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
#endif
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

      if(eventrun2 == 1)
      {
    //  eventfinish = 0;	      
      if(parent && child)
      {

      int pending = XPending(display);

      for (int i=0; i<pending; i++)
      {
      XEvent e;

      XNextEvent(display, &e);

      switch (e.type)
      {
#ifdef XECLOSE   
      case PropertyNotify:
      if (e.xproperty.atom == xembedatom) 
      {
      xeclose = 2; 
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
		      
      case EnterNotify:
//      if(reaperid)
 //     if(mapped2)
  //    {     
      if(e.xcrossing.focus == False)
      {
      XSetInputFocus(display, child, RevertToPointerRoot, CurrentTime);
//    XSetInputFocus(display, child, RevertToParent, e.xcrossing.time);
      }
 //     }
      break;
		      
#ifdef FOCUS
      case LeaveNotify:
      x3 = 0;
      y3 = 0;
      ignored3 = 0;            
      XTranslateCoordinates(display, child, XDefaultRootWindow(display), 0, 0, &x3, &y3, &ignored3);
  
      if(x3 < 0)
      { 
      width += x3;
      x3 = 0;
      }
  
      if(y3 < 0)
      {
      height += y3;
      y3 = 0;;
      }
		      
      if(((e.xcrossing.x_root < x3) || (e.xcrossing.x_root > x3 + (width - 1))) || ((e.xcrossing.y_root < y3) || (e.xcrossing.y_root > y3 + (height - 1))))      
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
//      if((e.xconfigure.event == parent) || (e.xconfigure.event == child) || ((e.xconfigure.event == pparent) && (parentok)))
//      {
      x = 0;
      y = 0;
      ignored = 0;

      XTranslateCoordinates(display, parent, XDefaultRootWindow(display), 0, 0, &x, &y, &ignored);
      e.xconfigure.send_event = false;
      e.xconfigure.type = ConfigureNotify;
      e.xconfigure.event = child;
      e.xconfigure.window = child;
      e.xconfigure.x = x;
      e.xconfigure.y = y;
      e.xconfigure.width = width;
      e.xconfigure.height = height;
      e.xconfigure.border_width = 0;
      e.xconfigure.above = None;
      e.xconfigure.override_redirect = False;
      XSendEvent (display, child, False, StructureNotifyMask | SubstructureRedirectMask, &e);
//      }
      break;

#ifdef EMBEDDRAG
      case ClientMessage:
      if((e.xclient.message_type == XdndEnter) || (e.xclient.message_type == XdndPosition) || (e.xclient.message_type == XdndLeave) || (e.xclient.message_type == XdndDrop))
      {

      if(e.xclient.message_type == XdndPosition)
      {      
      x = 0;
      y = 0;
      ignored = 0;
            
      e.xclient.window = child;
      XSendEvent (display, child, False, NoEventMask, &e);
      
      XTranslateCoordinates(display, child, XDefaultRootWindow(display), 0, 0, &x, &y, &ignored);
      
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
      XSendEvent(display, e.xclient.data.l[0], False, NoEventMask, (XEvent*)&cm);
      if(parentok && reaperid)
      {
      cm.data.l[0] = pparent;
      XSendEvent(display, e.xclient.data.l[0], False, NoEventMask, (XEvent*)&cm);
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
    //  eventfinish = 1;	      
     }
    }
#endif
#endif

VstIntPtr RemotePluginClient::dispatchproc(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
    RemotePluginClient  *plugin = (RemotePluginClient *) effect->object;
    VstIntPtr           v = 0;
     ERect        *rp;

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
   #ifdef XEMBED
     Atom xembedatom;
       unsigned long data[2];
#endif
#ifdef XECLOSE
Atom xembedatom;
       unsigned long data[2];
#endif	
#endif
	
     int editopen = 0;
	
    if(!plugin)
    return 0;
	
    if(plugin->m_inexcept == 1)
    {
    return 0;
    }
	
    if(plugin->m_threadbreak == 1)
    {
    return 0;
    }    	

    switch (opcode)
    {
    case effEditGetRect:
    {
        rp = &plugin->retRect;
        *((struct ERect **)ptr) = rp;
    }
        break;

    case effEditIdle:
#ifdef EMBED
#ifdef XEMBED
#ifdef EMBEDDRAG
       if(plugin->eventrun == 1)
       plugin->eventloopx(plugin->display, plugin->parent, plugin->child, plugin->pparent, plugin->eventrun, plugin->width, plugin->height, plugin->parentok, plugin->reaperid, plugin);
#else
       if(plugin->eventrun == 1)
       plugin->eventloopx(plugin->display, plugin->parent, plugin->child, plugin->eventrun, plugin->width, plugin->height, plugin->reaperid, plugin);       
#endif
#else
#ifdef EMBEDDRAG
        if(plugin->eventrun == 1)
        plugin->eventloop(plugin->display, plugin->pparent, plugin->parent, plugin->child, plugin->width, plugin->height, plugin->eventrun, plugin->parentok, plugin->reaperid);
#else
        if(plugin->eventrun == 1)
        plugin->eventloop(plugin->display, plugin->parent, plugin->child, plugin->width, plugin->height, plugin->eventrun, plugin->reaperid);
#endif
#endif
#endif
 //       v = plugin->effVoidOp2(effEditIdle, index, value, opt);
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
    //    strncpy((char *) ptr, plugin->getMaker().c_str(), kVstMaxVendorStrLen);
	strcpy((char *) ptr, plugin->getMaker().c_str());	    
	v=1;	    
        break;

    case effGetEffectName:
    //    strncpy((char *) ptr, plugin->getName().c_str(), kVstMaxEffectNameLen);
	strcpy((char *) ptr, plugin->getName().c_str());	    
        v=1;		    
        break;

    case effGetParamName:
      //  strncpy((char *) ptr, plugin->getParameterName(index).c_str(), kVstMaxParamStrLen);
	// strncpy((char *) ptr, plugin->getParameterName(index).c_str(), kVstMaxVendorStrLen);
        strcpy((char *) ptr, plugin->getParameterName(index).c_str());	    
        break;

    case effGetParamLabel: 
     strcpy((char *) ptr, plugin->getParameterLabel(index).c_str());	         
  //   strcpy((char *) ptr, plugin->getEffString(effGetParamLabel, index).c_str());	
        break;

    case effGetParamDisplay:
            strcpy((char *) ptr, plugin->getParameterDisplay(index).c_str()); 
 //   strcpy((char *) ptr, plugin->getEffString(effGetParamDisplay, index).c_str());
        break;

    case effGetProgramNameIndexed:
        v = plugin->getProgramNameIndexed(index, (char *) ptr);
        break;

    case effGetProgramName:
      //  strncpy((char *) ptr, plugin->getProgramName().c_str(), kVstMaxProgNameLen);
	strcpy((char *) ptr, plugin->getProgramName().c_str());	    
        break;

    case effSetSampleRate:
        plugin->setSampleRate(opt);
        break;

    case effSetBlockSize:
        plugin->setBufferSize ((VstInt32)value);
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
        v = plugin->getEffMidiKey(index, (char *) ptr);
        break;   
        
    case effGetMidiProgramName:    
        v = plugin->getEffMidiProgName(index, (char *) ptr);
        break;   
        
    case effGetCurrentMidiProgram:    
        v = plugin->getEffMidiCurProg(index, (char *) ptr);
        break; 
        
    case effGetMidiProgramCategory:    
        v = plugin->getEffMidiProgCat(index, (char *) ptr);
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
        v = plugin->getShellName((char *) ptr);
        break;
#endif
    case effSetProgram:
        plugin->setCurrentProgram((VstInt32)value);
        break;

    case effEditOpen:
#ifdef EMBED
    {
        plugin->showGUI();
      //  usleep(50000);

       plugin->handle = plugin->winm->handle;
       plugin->width = plugin->winm->width;
       plugin->height = plugin->winm->height;
       plugin->parent = (Window) ptr;
       plugin->child = (Window) plugin->handle;

       rp = &plugin->retRect;
       rp->bottom = plugin->height;
       rp->top = 0;
       rp->right = plugin->width;
       rp->left = 0;
       
       if(!plugin->handle)
       {
	   plugin->displayerr = 1;
       plugin->eventrun = 0;
       break;
       }

       plugin->display = XOpenDisplay(0);

       if(!plugin->display)
       {
	   plugin->displayerr = 1;
       plugin->eventrun = 0;
       break;
       }
     
       plugin->eventrun = 1;  
            
#ifdef XECLOSE
       data[0] = 0;
       data[1] = 1;
       xembedatom = XInternAtom(plugin->display, "_XEMBED_INFO", False);
       XChangeProperty(plugin->display, plugin->child, xembedatom, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, 2); 
#endif       	       
#ifdef EMBEDDRAG
       plugin->root = 0;
       plugin->children = 0;
       plugin->numchildren = 0;

       attr = {0};
       attr.event_mask = NoEventMask;

       plugin->x11_win = XCreateWindow(plugin->display, DefaultRootWindow(plugin->display), 0, 0, 1, 1, 0, 0, InputOnly, CopyFromParent, CWEventMask, &attr);

       if(plugin->x11_win)
       {
       XdndProxy = XInternAtom(plugin->display, "XdndProxy", False);

       XdndAware = XInternAtom(plugin->display, "XdndAware", False);
       version = 5;
       XChangeProperty(plugin->display, plugin->x11_win, XdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char*)&version, 1);

       plugin->parentok = 0;

       if(XQueryTree(plugin->display, plugin->parent, &plugin->root, &plugin->pparent, &plugin->children, &plugin->numchildren) != 0)
       {
       if(plugin->children)
       XFree(plugin->children);
       if((plugin->pparent != plugin->root) && (plugin->pparent != 0))
       plugin->parentok = 1;
       }
	       
       if(plugin->parentok == 0)
       plugin->pparent = 0;
       
       if(plugin->parentok && plugin->reaperid)
       XChangeProperty(plugin->display, plugin->pparent, XdndProxy, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&plugin->x11_win, 1);
       else
       XChangeProperty(plugin->display, plugin->parent, XdndProxy, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&plugin->x11_win, 1);

       XChangeProperty(plugin->display, plugin->x11_win, XdndProxy, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&plugin->x11_win, 1);
       }
       #endif
	       
#ifdef FOCUS
      XSelectInput(plugin->display, plugin->parent, SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask);
      XSelectInput(plugin->display, plugin->child, SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask | EnterWindowMask | LeaveWindowMask | PropertyChangeMask); 
#else 
      XSelectInput(plugin->display, plugin->parent, SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask);
      XSelectInput(plugin->display, plugin->child, SubstructureRedirectMask | StructureNotifyMask | SubstructureNotifyMask | EnterWindowMask | PropertyChangeMask );	   
#endif

       XSync(plugin->display, false);

  //     XResizeWindow(plugin->display, plugin->parent, plugin->width, plugin->height);

#ifdef EMBEDTHREAD
        plugin->runembed = 1;  
#else
  //     usleep(100000);

       XReparentWindow(plugin->display, plugin->child, plugin->parent, 0, 0);

       XMapWindow(plugin->display, plugin->child);
       XSync(plugin->display, false);

 //     usleep(100000);

       plugin->openGUI();
#endif          
       plugin->displayerr = 0;   
     }
#else
        plugin->showGUI();
#endif
	   editopen = 1;	
	v=1;		    
        break;

        case effEditClose:
#ifdef EMBED                      
        if(plugin->displayerr == 1)
	    {
        if(plugin->display)
	    {
        XSync(plugin->display, true);
        XCloseDisplay(plugin->display);
	    }	
        break;
	    }			    
#ifdef XECLOSE 
        plugin->eventrun = 0;  
	XSync(plugin->display, true);	
		 
        plugin->xeclose = 1;
        plugin->sendXembedMessage(plugin->display, plugin->child, XEMBED_EMBEDDED_NOTIFY, 0, plugin->parent, 0);

        for(int i2=0;i2<5000;i2++)
        {
#ifdef EMBEDDRAG
        plugin->eventloop(plugin->display, plugin->pparent, plugin->parent, plugin->child, plugin->width, plugin->height, 1, plugin->parentok, plugin->reaperid);
#else
        plugin->eventloop(plugin->display, plugin->parent, plugin->child, plugin->width, plugin->height, 1, plugin->reaperid);
#endif

        if(plugin->xeclose == 2)
        break;
	usleep(1000);	
        }
	plugin->xeclose = 0;
	
	XSync(plugin->display, false);	  	    
#endif       
        plugin->hideGUI();	 
           
        if(plugin->display)
        {
#ifdef EMBEDDRAG
        if(plugin->x11_win)
        XDestroyWindow (plugin->display, plugin->x11_win);
        plugin->x11_win = 0;
#endif      	  	 
        XCloseDisplay(plugin->display);
        plugin->display = 0; 
        }  		    
#else            
        plugin->hideGUI();
#endif  
	editopen = 0;	 
	v=1;		    
        break;

    case effCanDo:
        if (ptr && !strcmp((char *)ptr,"hasCockosExtensions"))
	{
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
        plugin->effVoidOp(67584930);
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
        v = plugin->processVstEvents((VstEvents *) ptr);
        break;

    case effGetChunk:
        v = plugin->getChunk((void **) ptr, index);
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
     	if(editopen == 1)
        {
#ifdef EMBED		    
#ifdef XECLOSE
        plugin->eventrun = 0;  
        XSync(plugin->display, true);	
		 
        plugin->xeclose = 1;
        plugin->sendXembedMessage(plugin->display, plugin->child, XEMBED_EMBEDDED_NOTIFY, 0, plugin->parent, 0);

        for(int i2=0;i2<5000;i2++)
        {
#ifdef EMBEDDRAG
        plugin->eventloop(plugin->display, plugin->pparent, plugin->parent, plugin->child, plugin->width, plugin->height, 1, plugin->parentok, plugin->reaperid);
#else
        plugin->eventloop(plugin->display, plugin->parent, plugin->child, plugin->width, plugin->height, 1, plugin->reaperid);
#endif

        if(plugin->xeclose == 2)
        break;
	usleep(1000);	
        }
	plugin->xeclose = 0;
	
	XSync(plugin->display, false);	  	    
#endif  
#endif		
        plugin->hideGUI();
        }	
#ifdef EMBED		
        if(plugin->display)
        {
#ifdef EMBEDDRAG
        if(plugin->x11_win)
        XDestroyWindow (plugin->display, plugin->x11_win);
        plugin->x11_win = 0;
#endif      	  	 
        XCloseDisplay(plugin->display);
        plugin->display = 0; 
        }  		       
#endif            		    
	plugin->effVoidOp(effClose);	    

/*
#ifdef AMT
        if(plugin->m_shm3)
        {
            for(int i=0;i<5000;i++)
            {
                if(plugin->m_threadbreakexit)
                break;
		usleep(1000);
            }
        }
        else
        {
            usleep(500000);
        }
#else
        usleep(500000);
#endif
*/
     //   wait(NULL);	        	    
        delete plugin;				          
        break;

    default:
        break;
    }
    return v;
}

#ifdef DOUBLEP
void RemotePluginClient::prodproc(AEffect* effect, double** inputs, double** outputs, int sampleFrames)
{
    RemotePluginClient *plugin = (RemotePluginClient*)effect->object;

    if(!plugin)
    return;
    
    if((plugin->m_bufferSize > 0) && (plugin->m_numInputs >= 0) && (plugin->m_numOutputs >= 0))
        plugin->processdouble(inputs, outputs, sampleFrames);
    return;
}
#endif

void RemotePluginClient::proproc(AEffect* effect, float** inputs, float** outputs, int sampleFrames)
{
    RemotePluginClient *plugin = (RemotePluginClient*)effect->object;

    if(!plugin)
    return;
    
    if((plugin->m_bufferSize > 0) && (plugin->m_numInputs >= 0) && (plugin->m_numOutputs >= 0))
        plugin->process(inputs, outputs, sampleFrames);     
    return;
}

void RemotePluginClient::setparproc(AEffect* effect, VstInt32 index, float parameter)
{
    RemotePluginClient *plugin = (RemotePluginClient*)effect->object;

    if(!plugin)
    return;
    
    if((plugin->m_bufferSize > 0) && (plugin->m_numInputs >= 0) && (plugin->m_numOutputs >= 0))
        plugin->setParameter(index, parameter);
    return;
}

float RemotePluginClient::getparproc(AEffect* effect, VstInt32 index)
{
    RemotePluginClient  *plugin = (RemotePluginClient*)effect->object;
    float               retval = -1;

    if(!plugin)
    return retval;
    
    if((plugin->m_bufferSize > 0) && (plugin->m_numInputs >= 0) && (plugin->m_numOutputs >= 0))
        retval = plugin->getParameter(index);
    return retval;
}

void RemotePluginClient::errwin(std::string dllname)
{
 Window window = 0;
 Window ignored = 0;
 Display* display = 0;
 int screen = 0;
 Atom winstate;
 Atom winmodal;
    
std::string filename;
std::string filename2;

  size_t found2 = dllname.find_last_of("/");
  filename = dllname.substr(found2 + 1, strlen(dllname.c_str()) - (found2 +1));
  filename2 = "VST dll file not found or timeout:  " + filename;
      
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
  
void RemotePluginClient::ServerConnect(Dl_info info)
{
    pid_t       child;
    std::string dllName;
    std::string LinVstName;
    bool        test;

#ifdef VST6432
   int          dlltype;
   unsigned int offset;
   char         buffer[256];
#endif

    char        hit2[4096];

    int *sptr;
    int val5;
    char bufpipe[2048];
    int doexec;
    int memok;   
    int memok2;
    int memok3; 

    if (!info.dli_fname)
    {
        m_runok = 1;
        cleanup();
        return;
    }

    if (realpath(info.dli_fname, hit2) == 0)
    {
        m_runok = 1;
        cleanup();
        return;
    }

    dllName = hit2;
    dllName.replace(dllName.begin() + dllName.find(".so"), dllName.end(), ".dll");
    test = std::ifstream(dllName.c_str()).good();

    if (!test)
    {
        dllName = hit2;
        dllName.replace(dllName.begin() + dllName.find(".so"), dllName.end(), ".Dll");
        test = std::ifstream(dllName.c_str()).good();

        if (!test)
        {
            dllName = hit2;
            dllName.replace(dllName.begin() + dllName.find(".so"), dllName.end(), ".DLL");
            test = std::ifstream(dllName.c_str()).good();
        }

        if (!test)
        {
             dllName = hit2;
             dllName.replace(dllName.begin() + dllName.find(".so"), dllName.end(), ".dll");
             errwin(dllName);
             m_runok = 1;
             cleanup();
             return;
        }
    }

#ifdef VST6432
    std::ifstream mfile(dllName.c_str(), std::ifstream::binary);

    if (!mfile)
    {
        m_runok = 1;
        cleanup();
        return;
    }

    mfile.read(&buffer[0], 2);
    short *ptr;
    ptr = (short *) &buffer[0];

    if (*ptr != 0x5a4d)
    {
        mfile.close();
        m_runok = 1;
        cleanup();
        return;
    }

    mfile.seekg (60, mfile.beg);
    mfile.read (&buffer[0], 4);

    int *ptr2;
    ptr2 = (int *) &buffer[0];
    offset = *ptr2;
    offset += 4;

    mfile.seekg (offset, mfile.beg);
    mfile.read (&buffer[0], 2);

    unsigned short *ptr3;
    ptr3 = (unsigned short *) &buffer[0];

    dlltype = 0;
    if (*ptr3 == 0x8664)
        dlltype = 1;
    else if (*ptr3 == 0x014c)
        dlltype = 2;
    else if (*ptr3 == 0x0200)
        dlltype = 3;

    if (dlltype == 0)
    {
        mfile.close();
        m_runok = 1;
        cleanup();
        return;
    }

    mfile.close();
#endif
    
#ifdef VST6432
    if (dlltype == 2)
    {
#ifdef EMBED    
    LinVstName = "/usr/bin/lin-vst-server-x32.exe";
#else
    LinVstName = "/usr/bin/lin-vst-server-xst32.exe";
#endif    
    test = std::ifstream(LinVstName.c_str()).good();
    if (!test)
    {
    m_runok = 1;
    cleanup();
    return;
    }
    }
    else
    {
#ifdef EMBED    
    LinVstName = "/usr/bin/lin-vst-server-x.exe";
#else
    LinVstName = "/usr/bin/lin-vst-server-xst.exe";
#endif    
    test = std::ifstream(LinVstName.c_str()).good();
    if (!test)
    {
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
    if (!test)
    {
    m_runok = 1;
    cleanup();
    return;
    }
#endif      

    hit2[0] = '\0';

    std::string dllNamewin = dllName;
    std::size_t idx = dllNamewin.find("drive_c");

    if (idx != std::string::npos)
    {
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
                  
    if(ShmID2 == -1)
    {
    doexec = 1;
    }
    
//    signal(SIGCHLD, SIG_IGN);

    if(doexec)
    {   
    if ((child = fork()) < 0)
    {
    m_runok = 1;
    cleanup();
    return;
    }
    else if (child == 0)
    {
#ifdef EMBED
#ifdef VST6432
    if (dlltype == 2)
    {
    if(execlp("/usr/bin/lin-vst-server-x32.exe", "/usr/bin/lin-vst-server-x32.exe", NULL, NULL))
    {
    m_runok = 1;
    cleanup();
    return;
    }            
    }        
    else
    {
    if(execlp("/usr/bin/lin-vst-server-x.exe", "/usr/bin/lin-vst-server-x.exe", NULL, NULL))
    {
    m_runok = 1;
    cleanup();
    return;
    }            
    }      
#else
    if(execlp("/usr/bin/lin-vst-server-x.exe", "/usr/bin/lin-vst-server-x.exe", NULL, NULL))
    {
    m_runok = 1;
    cleanup();
    return;
    }          
#endif   
#else
#ifdef VST6432
    if (dlltype == 2)
    {
    if(execlp("/usr/bin/lin-vst-server-xst32.exe", "/usr/bin/lin-vst-server-xst32.exe", NULL, NULL))
    {
    m_runok = 1;
    cleanup();
    return;
    }            
    }        
    else
    {
    if(execlp("/usr/bin/lin-vst-server-xst.exe", "/usr/bin/lin-vst-server-xst.exe", NULL, NULL))
    {
    m_runok = 1;
    cleanup();
    return;
    }            
    }      
#else
    if(execlp("/usr/bin/lin-vst-server-xst.exe", "/usr/bin/lin-vst-server-xst.exe", NULL, NULL))
    {
    m_runok = 1;
    cleanup();
    return;
    }          
#endif   
#endif  
    }    
    }
        
    #ifdef LVRT
    struct sched_param param;
    param.sched_priority = 1;

    int result = sched_setscheduler(0, SCHED_FIFO, &param);

    if (result < 0)
    {
        perror("Failed to set realtime priority");
    }
    #endif
      
    if(doexec)
    {
    for(int i9=0;i9<20000;i9++)
    {
    ShmID2 = shmget(MyKey2, 20000, 0666);
    if(ShmID2 != -1)
    {
    memok = 1; 
    break;
    }
    usleep(1000);
    } 
    
    ShmPTR2  = (char *) shmat(ShmID2, NULL, 0);    
    sptr = (int *)ShmPTR2;
       
    memok2 = 0;
     
    if(memok && ShmPTR2)
    { 
    for(int i12=0;i12<20000;i12++) 
    {    
    val5 = 0;
    val5 = *sptr;

#ifdef VST6432 
    if(dlltype == 2)
    {
    if(val5 == 1001)
    {
    memok2 = 1;
    break;    
    }
    }
    else
    {    
    if(val5 == 1002)
    {
    memok2 = 1;
    break;
    }
    } 
#else
    if(val5 == 1002)
    {
    memok2 = 1;
    break;
    }
#endif     
    usleep(1000);
    }
    } 
          
    if(!memok2)
    {
    m_runok = 1;
    cleanup();
    return;        
    } 
    
    }
       
    if(!doexec)
    {
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
         
    if(ShmID2 == -1)
    {
    memok3 = 0;
    for(int i8=0;i8<5000;i8++)
    {
    ShmID2 = shmget(MyKey2, 20000, 0666);

    if(ShmID2 != -1)
    {
    memok3 = 1;
    break;
    }
    usleep(1000);
    }      
    }
         
    if(memok3 == 0)
    {
    m_runok = 1;
    cleanup();
    return;
    }  
                
    ShmPTR2 = (char *)shmat(ShmID2, NULL, 0);
    
    }
         
    if(ShmPTR2)
    {       
    sptr = (int *)ShmPTR2;
    strcpy(ShmPTR2 + sizeof(int), argStr);

#ifdef VST6432 
    if(dlltype == 2)
    *sptr = 645;   
    else             
    *sptr = 745;
#else
   *sptr = 745;
#endif    
          
    for(int i3=0;i3<10000;i3++)
    {
    val5 = 0;
    val5 = *sptr;

#ifdef VST6432  
    if(dlltype == 2)
    {
    if(val5 == 647)
    {
    m_runok = 1;
    cleanup();
    return;
    } 
    if(val5 == 646)
    break;    
    }
    else
    { 
    if(val5 == 747)
    {
    m_runok = 1;
    cleanup();
    return;
    }    
    if(val5 == 746)
    break;
    } 
#else
    if(val5 == 747)
    {
    m_runok = 1;
    cleanup();
    return;
    } 
    if(val5 == 746)
    break;
#endif               
    usleep(1000);
    }
    }
    else
    {
    m_runok = 1;
    cleanup();
    return;
    } 
    
    syncStartup();
    
    if(m_syncok == 0)
    {
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
    theEffect->version = 100;
    theEffect->processReplacing = proproc;
    #ifdef DOUBLEP
    theEffect->processDoubleReplacing = prodproc;
    #endif   
}

void RemotePluginClient::syncStartup()
{
int startok;
int *ptr;

startok = 0;

ptr = (int *)m_shm;

    for (int i=0;i<4000;i++)
    {
        usleep(10000);
        if (*ptr == 244)
         {
            startok = 1;
            break;
         }
    }  

   if(startok == 0)
   {
   *ptr = 4;
   m_runok = 1;
   cleanup();
   return;
   }

    if(m_386run == 1)
    {
    *ptr = 3;
    }
    else
    {
    *ptr = 2;
    }
    
#ifdef EMBED
#ifdef EMBEDTHREAD
    if(pthread_create(&m_EMBEDThread, NULL, RemotePluginClient::callEMBEDThread, this) != 0)
    {    
    *ptr = 4;
    m_runok = 1;   
    cleanup();
    }
#endif
#endif
    /*
    theEffect = new AEffect;	
    if(!theEffect)
    {
    *ptr = 4;
    m_runok = 1;   
    cleanup();	    
    }       
   */

   theEffect = &theEffect2;	

#ifdef EMBED
    /*	
    winm = new winmessage;
    if(!winm)
    {
    *ptr = 4;
    m_runok = 1;   
    cleanup();	    
    } 
    */
    winm = &winm2;
#endif	    

    m_syncok = 1;
}

void RemotePluginClient::cleanup()
{
if(m_threadbreak == 0) 
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
#ifdef EMBED
#ifdef EMBEDTHREAD
if(m_threadbreakembed == 0) 
m_threadbreakembed = 1;
/*
    if (m_shm)
        for (int i=0;i<100000;i++)
        {
            usleep(100);
            if (m_threadbreakexitembed)
            break;
        }
*/
#endif
#endif
    if (m_AMThread)
        pthread_join(m_AMThread, NULL);
#ifdef EMBED
#ifdef EMBEDTHREAD
    if (m_EMBEDThread)
        pthread_join(m_EMBEDThread, NULL);
#endif
#endif
        
    if (m_shm)
    {
        munmap(m_shm, m_shmSize);
        m_shm = 0;
    }
    if (m_shm2)
    {
        munmap(m_shm2, m_shmSize2);
        m_shm2 = 0;
    }
    if (m_shm3)
    {
        munmap(m_shm3, m_shmSize3);
        m_shm3 = 0;
    }

    if (m_shmFd >= 0)
    {
        close(m_shmFd);
        m_shmFd = -1;
    }
    if (m_shmFd2 >= 0)
    {
        close(m_shmFd2);
        m_shmFd2 = -1;
    }
    if (m_shmFd3 >= 0)
    {
        close(m_shmFd3);
        m_shmFd3 = -1;
    }

    if (m_shmFileName)
    {
	shm_unlink(m_shmFileName);
        free(m_shmFileName);
        m_shmFileName = 0;
    }
    if (m_shmFileName2)
    {
 	shm_unlink(m_shmFileName2);
        free(m_shmFileName2);
        m_shmFileName2 = 0;
    }
    if (m_shmFileName3)
    {
	shm_unlink(m_shmFileName3);
        free(m_shmFileName3);
        m_shmFileName3 = 0;
    }

    if (m_shmControl) {
        munmap(m_shmControl, sizeof(ShmControl));
        m_shmControl = 0;
    }
    if (m_shmControlFd >= 0) {
        close(m_shmControlFd);
        m_shmControlFd = -1;
    }
    if (m_shmControlFileName) {
        shm_unlink(m_shmControlFileName);
        free(m_shmControlFileName);
        m_shmControlFileName = 0;
    }

    if (m_shmControl2) {
        munmap(m_shmControl2, sizeof(ShmControl));
        m_shmControl2 = 0;
    }
    if (m_shmControl2Fd >= 0) {
        close(m_shmControl2Fd);
        m_shmControl2Fd = -1;
    }
    if (m_shmControl2FileName) {
        shm_unlink(m_shmControl2FileName);
        free(m_shmControl2FileName);
        m_shmControl2FileName = 0;
    }

    if (m_shmControl3) {
        munmap(m_shmControl3, sizeof(ShmControl));
        m_shmControl3 = 0;
    }
    if (m_shmControl3Fd >= 0) {
        close(m_shmControl3Fd);
        m_shmControl3Fd = -1;
    }
    if (m_shmControl3FileName) {
        shm_unlink(m_shmControl3FileName);
        free(m_shmControl3FileName);
        m_shmControl3FileName = 0;
    }

    if (m_shmControl4) {
        munmap(m_shmControl4, sizeof(ShmControl));
        m_shmControl4 = 0;
    }
    if (m_shmControl4Fd >= 0) {
        close(m_shmControl4Fd);
        m_shmControl4Fd = -1;
    }
    if (m_shmControl4FileName) {
        shm_unlink(m_shmControl4FileName);
        free(m_shmControl4FileName);
        m_shmControl4FileName = 0;
    }

    if (m_shmControl5) {
        munmap(m_shmControl5, sizeof(ShmControl));
        m_shmControl5 = 0;
    }
    if (m_shmControl5Fd >= 0) {
        close(m_shmControl5Fd);
        m_shmControl5Fd = -1;
    }
    if (m_shmControl5FileName) {
        shm_unlink(m_shmControl5FileName);
        free(m_shmControl5FileName);
        m_shmControl5FileName = 0;
    }
}

std::string RemotePluginClient::getFileIdentifiers()
{
    std::string id;

    id += m_shmFileName + strlen(m_shmFileName) - 6;
    id += m_shmFileName2 + strlen(m_shmFileName2) - 6;
    id += m_shmFileName3 + strlen(m_shmFileName3) - 6;
    id += m_shmControlFileName + strlen(m_shmControlFileName) - 6;
    id += m_shmControl2FileName + strlen(m_shmControl2FileName) - 6;
    id += m_shmControl3FileName + strlen(m_shmControl3FileName) - 6;
    id += m_shmControl4FileName + strlen(m_shmControl4FileName) - 6;
    id += m_shmControl5FileName + strlen(m_shmControl5FileName) - 6;

    //  std::cerr << "Returning file identifiers: " << id << std::endl;
    return id;
}

int RemotePluginClient::sizeShm()
{
    if (m_shm)
        return 0;

    size_t sz = FIXED_SHM_SIZE + 1024;
    size_t sz2 = FIXED_SHM_SIZE2 + 1024 + (2 * sizeof(float));
    size_t sz3 = FIXED_SHM_SIZE3 + 1024;

    ftruncate(m_shmFd, sz);
    m_shm = (char *)mmap(0, sz, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_shmFd, 0);
    if (!m_shm)
    {
        std::cerr << "RemotePluginClient::sizeShm: ERROR: mmap or mremap failed for " << sz
                    << " bytes from fd " << m_shmFd << "!" << std::endl;
        m_shmSize = 0;
        return 1;	    
    }
    else
    {
	madvise(m_shm, sz, MADV_SEQUENTIAL | MADV_WILLNEED | MADV_DONTFORK);    
        memset(m_shm, 0, sz);
        m_shmSize = sz;
        
        if(mlock(m_shm, sz) != 0)
        perror("mlock fail1");
    }

    ftruncate(m_shmFd2, sz2);
    m_shm2 = (char *)mmap(0, sz2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_shmFd2, 0);
    if (!m_shm2)
    {
        std::cerr << "RemotePluginClient::sizeShm: ERROR: mmap or mremap failed for " << sz2
                    << " bytes from fd " << m_shmFd2 << "!" << std::endl;
        m_shmSize2 = 0;
        return 1;	    
    }
    else
    {
	    
        madvise(m_shm2, sz2, MADV_SEQUENTIAL | MADV_WILLNEED | MADV_DONTFORK);memset(m_shm2, 0, sz2);
        m_shmSize2 = sz2;

        if(mlock(m_shm2, sz2) != 0)
        perror("mlock fail2");

    }

    ftruncate(m_shmFd3, sz3);
    m_shm3 = (char *)mmap(0, sz3, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, m_shmFd3, 0);
    if (!m_shm3)
    {
        std::cerr << "RemotePluginClient::sizeShm: ERROR: mmap or mremap failed for " << sz3
                    << " bytes from fd " << m_shmFd3 << "!" << std::endl;
        m_shmSize3 = 0;
        return 1;	    
    }
    else
    {
	madvise(m_shm3, sz3, MADV_SEQUENTIAL | MADV_WILLNEED | MADV_DONTFORK);    
        memset(m_shm3, 0, sz3);
        m_shmSize3 = sz3;

        if(mlock(m_shm3, sz3) != 0)
        perror("mlock fail3");

    }

    m_threadbreak = 0;
    m_threadbreakexit = 0;

#ifdef EMBED
#ifdef EMBEDTHREAD    
    m_threadbreakembed = 0;
    m_threadbreakexitembed = 0;   
#endif
#endif
	
    return 0;	
}

float RemotePluginClient::getVersion()
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginGetVersion);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    return readFloat(&m_shm[FIXED_SHM_SIZE]);
}

std::string RemotePluginClient::getName()
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginGetName);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    return &m_shm[FIXED_SHM_SIZE];
}

std::string RemotePluginClient::getMaker()
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginGetMaker);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    return &m_shm[FIXED_SHM_SIZE];
}

void RemotePluginClient::setBufferSize(int s)
{
    if (s <= 0)
        return;

    if (s == m_bufferSize)
        return;
       
    m_bufferSize = s;
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginSetBufferSize);
    writeIntring(&m_shmControl5->ringBuffer, s);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
}

void RemotePluginClient::setSampleRate(int s)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginSetSampleRate);
    writeIntring(&m_shmControl5->ringBuffer, s);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
}

void RemotePluginClient::reset()
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginReset);
    if (m_shmSize > 0)
    {
        memset(m_shm, 0, m_shmSize);
        memset(m_shm2, 0, m_shmSize2);
        memset(m_shm3, 0, m_shmSize3);
    }
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
}

void RemotePluginClient::terminate()
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginTerminate);
}

int RemotePluginClient::getEffInt(int opcode, int value)
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetEffInt);
    writeIntring(&m_shmControl3->ringBuffer, opcode);
    writeIntring(&m_shmControl3->ringBuffer, value);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  

    return readInt(&m_shm[FIXED_SHM_SIZE]);
}

std::string RemotePluginClient::getEffString(int opcode, int index)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginGetEffString);
    writeIntring(&m_shmControl5->ringBuffer, opcode);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    return &m_shm[FIXED_SHM_SIZE];
}

std::string RemotePluginClient::getParameterName(int p)
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetParameterName);
    writeIntring(&m_shmControl3->ringBuffer, p);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return &m_shm[FIXED_SHM_SIZE];
}

std::string RemotePluginClient::getParameterLabel(int p)
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetParameterLabel);
    writeIntring(&m_shmControl3->ringBuffer, p);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return &m_shm[FIXED_SHM_SIZE];
}

std::string RemotePluginClient::getParameterDisplay(int p)
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetParameterDisplay);
    writeIntring(&m_shmControl3->ringBuffer, p);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return &m_shm[FIXED_SHM_SIZE];
}

#ifdef WAVES
int RemotePluginClient::getShellName(char *ptr)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginGetShellName);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    strcpy(ptr, &m_shm[FIXED_SHM_SIZE]);
    return readInt(&m_shm[FIXED_SHM_SIZE + 512]);
}
#endif

void
RemotePluginClient::setParameter(int p, float v)
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return;
    }

    writeOpcodering(&m_shmControl4->ringBuffer, RemotePluginSetParameter);
    writeIntring(&m_shmControl4->ringBuffer, p);
    writeFloatring(&m_shmControl4->ringBuffer, v);
    commitWrite(&m_shmControl4->ringBuffer);
    waitForServer4();
}

float
RemotePluginClient::getParameter(int p)
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return 0;
    }

    writeOpcodering(&m_shmControl4->ringBuffer, RemotePluginGetParameter);
    writeIntring(&m_shmControl4->ringBuffer, p);
    commitWrite(&m_shmControl4->ringBuffer);
    waitForServer4();
    return readFloat(&m_shm2[FIXED_SHM_SIZE2 + 1024]);
}

float RemotePluginClient::getParameterDefault(int p)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginGetParameterDefault);
    writeIntring(&m_shmControl5->ringBuffer, p);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    return readFloat(&m_shm[FIXED_SHM_SIZE]);
}

void RemotePluginClient::getParameters(int p0, int pn, float *v)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginGetParameters);
    writeIntring(&m_shmControl5->ringBuffer, p0);
    writeIntring(&m_shmControl5->ringBuffer, pn);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    tryRead(&m_shm[FIXED_SHM_SIZE], v, (pn - p0 + 1) * sizeof(float));
}


#ifdef SEM

void
RemotePluginClient::waitForServer2()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl2->runServer);

    timespec ts_timeout;
    clock_gettime(CLOCK_REALTIME, &ts_timeout);
    ts_timeout.tv_sec += 60;
    if (sem_timedwait(&m_shmControl2->runClient, &ts_timeout) != 0) {
        if(m_inexcept == 0)
	RemotePluginClosedException();
    }
    }
    else
    {
    fpost(&m_shmControl2->runServer386);

    if (fwait(&m_shmControl2->runClient386, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
    }
}

void
RemotePluginClient::waitForServer3()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl3->runServer);

    timespec ts_timeout;
    clock_gettime(CLOCK_REALTIME, &ts_timeout);
    ts_timeout.tv_sec += 60;
    if (sem_timedwait(&m_shmControl3->runClient, &ts_timeout) != 0) {
        if(m_inexcept == 0)
	RemotePluginClosedException();
    }
    }
    else
    {
    fpost(&m_shmControl3->runServer386);

    if (fwait(&m_shmControl3->runClient386, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
    }
}

void
RemotePluginClient::waitForServer4()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl4->runServer);

    timespec ts_timeout;
    clock_gettime(CLOCK_REALTIME, &ts_timeout);
    ts_timeout.tv_sec += 60;
    if (sem_timedwait(&m_shmControl4->runClient, &ts_timeout) != 0) {
        if(m_inexcept == 0)
	RemotePluginClosedException();
    }
    }
    else
    {
    fpost(&m_shmControl4->runServer386);

    if (fwait(&m_shmControl4->runClient386, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
    }
}

void
RemotePluginClient::waitForServer5()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl5->runServer);

    timespec ts_timeout;
    clock_gettime(CLOCK_REALTIME, &ts_timeout);
    ts_timeout.tv_sec += 60;
    if (sem_timedwait(&m_shmControl5->runClient, &ts_timeout) != 0) {
        if(m_inexcept == 0)
	RemotePluginClosedException();
    }
    }
    else
    {
    fpost(&m_shmControl5->runServer386);

    if (fwait(&m_shmControl5->runClient386, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
    }
}

void
RemotePluginClient::waitForServer2exit()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl2->runServer);
    }
    else
    {
    fpost(&m_shmControl2->runServer386);
    }
}

void
RemotePluginClient::waitForServer3exit()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl3->runServer);
    }
    else
    {
    fpost(&m_shmControl3->runServer386);
    }
}

void
RemotePluginClient::waitForServer4exit()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl4->runServer);
    }
    else
    {
    fpost(&m_shmControl4->runServer386);
    }
}

void
RemotePluginClient::waitForServer5exit()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl5->runServer);
    }
    else
    {
    fpost(&m_shmControl5->runServer386);
    }
}

void
RemotePluginClient::waitForClientexit()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl->runClient);
    }
    else
    {
    fpost(&m_shmControl->runClient386);
    }
}

#else

void
RemotePluginClient::waitForServer2()
{
    fpost(&m_shmControl2->runServer);

    if (fwait(&m_shmControl2->runClient, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
}

void
RemotePluginClient::waitForServer3()
{
    fpost(&m_shmControl3->runServer);

    if (fwait(&m_shmControl3->runClient, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
}

void
RemotePluginClient::waitForServer4()
{
    fpost(&m_shmControl4->runServer);

    if (fwait(&m_shmControl4->runClient, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
}

void
RemotePluginClient::waitForServer5()
{
    fpost(&m_shmControl5->runServer);

    if (fwait(&m_shmControl5->runClient, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
}

void
RemotePluginClient::waitForServer2exit()
{
    fpost(&m_shmControl2->runServer);
}

void
RemotePluginClient::waitForServer3exit()
{
    fpost(&m_shmControl3->runServer);
}

void
RemotePluginClient::waitForServer4exit()
{
    fpost(&m_shmControl4->runServer);
}

void
RemotePluginClient::waitForServer5exit()
{
    fpost(&m_shmControl5->runServer);
}

void
RemotePluginClient::waitForClientexit()
{
    fpost(&m_shmControl->runClient);
}

#endif

void RemotePluginClient::process(float **inputs, float **outputs, int sampleFrames)
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return;
    } 
    if ((m_bufferSize <= 0) || (sampleFrames <= 0))
    {
        return;
    }
    if (m_numInputs < 0)
    {
        return;
    }
    if (m_numOutputs < 0)
    {
        return;
    }

    if(m_updateio == 1)
    {
    m_numInputs = m_updatein;
    m_numOutputs = m_updateout;	    
    writeOpcodering(&m_shmControl2->ringBuffer, RemotePluginProcess);
    writeIntring(&m_shmControl2->ringBuffer, -1);
    commitWrite(&m_shmControl2->ringBuffer);
    waitForServer2();  
    m_updateio = 0;
    }
	
    if (((m_numInputs + m_numOutputs) * m_bufferSize * sizeof(float)) >= (PROCESSSIZE))
    return;
	
#ifdef NEWTIME	
    if(m_audioMaster && theEffect && m_shm3)
    {
    timeInfo = 0;

//    timeInfo = (VstTimeInfo *)m_audioMaster(theEffect, audioMasterGetTime, 0, 0, 0, 0);
	    
    timeInfo = (VstTimeInfo *)m_audioMaster(theEffect, audioMasterGetTime, 0, kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstCyclePosValid | kVstTimeSigValid, 0, 0);
	    
    if(timeInfo)
    {    
 //   printf("%f\n", timeInfo->sampleRate);
    
    memcpy((VstTimeInfo*)&m_shm3[FIXED_SHM_SIZE3 - sizeof(VstTimeInfo)], timeInfo, sizeof(VstTimeInfo));
    }
    } 
#endif                	 	

    size_t blocksz = sampleFrames * sizeof(float);

    if(m_numInputs > 0)
    {
    for (int i = 0; i < m_numInputs; ++i)
    memcpy(m_shm + i * blocksz, inputs[i], blocksz);
    }

    writeOpcodering(&m_shmControl2->ringBuffer, RemotePluginProcess);
    writeIntring(&m_shmControl2->ringBuffer, sampleFrames);

    commitWrite(&m_shmControl2->ringBuffer);

    waitForServer2();  

    if(m_numOutputs > 0)
    {
    for (int i = 0; i < m_numOutputs; ++i)
    memcpy(outputs[i], m_shm + i * blocksz, blocksz);
    }
    return;
}

#ifdef DOUBLEP
void RemotePluginClient::processdouble(double **inputs, double **outputs, int sampleFrames)
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return;
    } 
    if ((m_bufferSize <= 0) || (sampleFrames <= 0))
    {
        return;
    }
    if (m_numInputs < 0)
    {
        return;
    }
    if (m_numOutputs < 0)
    {
        return;
    }

    if(m_updateio == 1)
    {
    m_numInputs = m_updatein;
    m_numOutputs = m_updateout;	    
    writeOpcodering(&m_shmControl2->ringBuffer, RemotePluginProcessDouble);
    writeIntring(&m_shmControl2->ringBuffer, -1);
    commitWrite(&m_shmControl2->ringBuffer);
    waitForServer2();  
    m_updateio = 0;
    }
	
    if (((m_numInputs + m_numOutputs) * m_bufferSize * sizeof(double)) >= (PROCESSSIZE))
    return;
	
#ifdef NEWTIME	
    if(m_audioMaster && theEffect && m_shm3)
    {
    timeInfo = 0;

//    timeInfo = (VstTimeInfo *)m_audioMaster(theEffect, audioMasterGetTime, 0, 0, 0, 0);
	    
    timeInfo = (VstTimeInfo *)m_audioMaster(theEffect, audioMasterGetTime, 0, kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstCyclePosValid | kVstTimeSigValid, 0, 0);
	    
    if(timeInfo)
    {    
 //   printf("%f\n", timeInfo->sampleRate);
    
    memcpy((VstTimeInfo*)&m_shm3[FIXED_SHM_SIZE3 - sizeof(VstTimeInfo)], timeInfo, sizeof(VstTimeInfo));
    }
    } 
#endif                	

    size_t blocksz = sampleFrames * sizeof(double);

    if(m_numInputs > 0)
    {
    for (int i = 0; i < m_numInputs; ++i)
    memcpy(m_shm + i * blocksz, inputs[i], blocksz);
    }

    writeOpcodering(&m_shmControl2->ringBuffer, RemotePluginProcessDouble);
    writeIntring(&m_shmControl2->ringBuffer, sampleFrames);

    commitWrite(&m_shmControl2->ringBuffer);

    waitForServer2();  

    if(m_numOutputs > 0)
    {
    for (int i = 0; i < m_numOutputs; ++i)
    memcpy(outputs[i], m_shm + i * blocksz, blocksz);
    }
    return;
}

bool RemotePluginClient::setPrecision(int value)
{
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteSetPrecision);
    writeIntring(&m_shmControl5->ringBuffer, value);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
 
    tryRead(&m_shm[FIXED_SHM_SIZE], &b, sizeof(bool));
    return b;
}
#endif

int RemotePluginClient::processVstEvents(VstEvents *evnts)
{
    int ret;
    int eventnum;
    int *ptr;
    int sizeidx = 0;
	
    if(!m_shm)
    return 0;    

    if (!evnts)
    return 0;

    if ((evnts->numEvents <= 0) || (m_inexcept == 1) || (m_finishaudio == 1))
    return 0;        

    ptr = (int *)m_shm2;
    eventnum = evnts->numEvents;
    sizeidx = sizeof(int);

    if (eventnum > VSTSIZE)
    eventnum = VSTSIZE;            

    for (int i = 0; i < eventnum; i++)
    {
        VstEvent* pEvent = evnts->events[i];

        if (pEvent->type == kVstSysExType)
        {
            eventnum--;
        }
        else
        {
            unsigned int size = (2*sizeof(VstInt32)) + evnts->events[i]->byteSize;
            memcpy(&m_shm2[sizeidx], evnts->events[i] , size);
            sizeidx += size;
        }
    }

    *ptr = eventnum;
    ret = evnts->numEvents;

#ifdef OLDMIDI
    writeOpcodering(&m_shmControl2->ringBuffer, RemotePluginProcessEvents);
    commitWrite(&m_shmControl2->ringBuffer);
    waitForServer2();  
#endif
	
    return ret;
}

#ifdef VESTIGE
bool RemotePluginClient::getEffInProp(int index, void *ptr)
{
char ptr2[256];
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteInProp);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
 
    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - 256], &ptr2, 256);
    memcpy(ptr, &ptr2, sizeof(vinfo));

    return b;
}

bool RemotePluginClient::getEffOutProp(int index, void *ptr)
{
char ptr2[256];
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteOutProp);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
 
    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - 256], &ptr2, 256);
    memcpy(ptr, &ptr2, sizeof(vinfo));

    return b;
}
#else
bool RemotePluginClient::getEffInProp(int index, void *ptr)
{
VstPinProperties ptr2;
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteInProp);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
 
    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - sizeof(VstPinProperties)], &ptr2, sizeof(VstPinProperties));
    memcpy(ptr, &ptr2, sizeof(VstPinProperties));

    return b;
}

bool RemotePluginClient::getEffOutProp(int index, void *ptr)
{
VstPinProperties ptr2;
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteOutProp);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
 
    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - sizeof(VstPinProperties)], &ptr2, sizeof(VstPinProperties));
    memcpy(ptr, &ptr2, sizeof(VstPinProperties));

    return b;
}
#endif

#ifdef MIDIEFF
bool RemotePluginClient::getEffMidiKey(int index, void *ptr)
{
MidiKeyName ptr2;
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteMidiKey);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  

    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - sizeof(MidiKeyName)], &ptr2, sizeof(MidiKeyName));
    memcpy(ptr, &ptr2, sizeof(MidiKeyName));

    return b;
}

bool RemotePluginClient::getEffMidiProgName(int index, void *ptr)
{
MidiProgramName ptr2;
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteMidiProgName);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
  
    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - sizeof(MidiProgramName)], &ptr2, sizeof(MidiProgramName));
    memcpy(ptr, &ptr2, sizeof(MidiProgramName));
    
    return b;
}

bool RemotePluginClient::getEffMidiCurProg(int index, void *ptr)
{
MidiProgramName ptr2;
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteMidiCurProg);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
 
    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - sizeof(MidiProgramName)], &ptr2, sizeof(MidiProgramName));
    memcpy(ptr, &ptr2, sizeof(MidiProgramName));

    return b;
}

bool RemotePluginClient::getEffMidiProgCat(int index, void *ptr)
{
MidiProgramCategory ptr2;
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteMidiProgCat);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
   
    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - sizeof(MidiProgramCategory)], &ptr2, sizeof(MidiProgramCategory));
    memcpy(ptr, &ptr2, sizeof(MidiProgramCategory));

    return b;
}

bool RemotePluginClient::getEffMidiProgCh(int index)
{
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteMidiProgCh);
    writeIntring(&m_shmControl5->ringBuffer, index);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
 
    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));
    return b;
}

bool RemotePluginClient::setEffSpeaker(VstIntPtr value, void *ptr)
{
bool b;

    tryWrite(&m_shm2[FIXED_SHM_SIZE2 - (sizeof(VstSpeakerArrangement)*2)], ptr, sizeof(VstSpeakerArrangement));
    tryWrite(&m_shm2[FIXED_SHM_SIZE2 - sizeof(VstSpeakerArrangement)], (VstIntPtr *)value, sizeof(VstSpeakerArrangement));

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteSetSpeaker);
    commitWrite(&m_shmControl5->ringBuffer);

    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));

    return b;
}

bool RemotePluginClient::getEffSpeaker(VstIntPtr value, void *ptr)
{
bool b;

    writeOpcodering(&m_shmControl5->ringBuffer, RemoteGetSpeaker);
    commitWrite(&m_shmControl5->ringBuffer);
    
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - ( sizeof(VstSpeakerArrangement)*2)], ptr, sizeof(VstSpeakerArrangement));
    tryRead(&m_shm2[FIXED_SHM_SIZE2 - sizeof(VstSpeakerArrangement)], (VstIntPtr *)value, sizeof(VstSpeakerArrangement));

    tryRead(&m_shm2[FIXED_SHM_SIZE2], &b, sizeof(bool));

    return b;
}
#endif

#ifdef CANDOEFF
bool RemotePluginClient::getEffCanDo(char *ptr)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginEffCanDo);
    writeStringring(&m_shmControl5->ringBuffer, ptr);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    bool b;
    tryRead(&m_shm[FIXED_SHM_SIZE], &b, sizeof(bool));
    return b;
}
#endif	

void RemotePluginClient::setDebugLevel(RemotePluginDebugLevel level)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginSetDebugLevel);
    tryWritering(&m_shmControl5->ringBuffer, &level, sizeof(RemotePluginDebugLevel));
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  

}

bool RemotePluginClient::warn(std::string str)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginWarn);
    writeStringring(&m_shmControl5->ringBuffer, str);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    bool b;
    tryRead(&m_shm[FIXED_SHM_SIZE], &b, sizeof(bool));
    return b;
}

void RemotePluginClient::showGUI()
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginShowGUI);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  

#ifdef EMBED
    tryRead(&m_shm[FIXED_SHM_SIZE], winm, sizeof(winmessage));
#endif
}

void RemotePluginClient::hideGUI()
{
 //     effVoidOp(hidegui2);	
	
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginHideGUI);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
}

#ifdef EMBED
void RemotePluginClient::openGUI()
{	
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginOpenGUI);    
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
}
#endif

#define errorexit 9999

void RemotePluginClient::effVoidOp(int opcode)
{
    if (opcode == effClose || opcode == errorexit)
    {

      if(opcode == errorexit)
      {  
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
#ifdef EMBED
#ifdef EMBEDTHREAD
        m_threadbreakembed = 1;
/*
    if (m_shm)
        for (int i=0;i<100000;i++)
        {
            usleep(100);
            if (m_threadbreakexitembed)
            break;
        }
*/
#endif
#endif 
        m_finishaudio = 1;
        writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginDoVoid);
        writeIntring(&m_shmControl5->ringBuffer, effClose);
        commitWrite(&m_shmControl5->ringBuffer);
      }
      else
      {
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
#ifdef EMBED  
#ifdef EMBEDTHREAD
        m_threadbreakembed = 1;
/*
    if (m_shm)
        for (int i=0;i<100000;i++)
        {
            usleep(100);
            if (m_threadbreakexitembed)
            break;
        }
*/
#endif
#endif 
        m_finishaudio = 1;
        writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginDoVoid);
        writeIntring(&m_shmControl5->ringBuffer, opcode);
        commitWrite(&m_shmControl5->ringBuffer);

        waitForServer2exit(); 
        waitForServer3exit(); 
        waitForServer4exit(); 
        waitForServer5exit();  
      }
    }
    else
    {
        writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginDoVoid);
        writeIntring(&m_shmControl5->ringBuffer, opcode);
        commitWrite(&m_shmControl5->ringBuffer);
        waitForServer5();  
    }
}

int RemotePluginClient::effVoidOp2(int opcode, int index, int value, float opt)
{
        writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginDoVoid2);
        writeIntring(&m_shmControl5->ringBuffer, opcode);
        writeIntring(&m_shmControl5->ringBuffer, index);
        writeIntring(&m_shmControl5->ringBuffer, value);
        writeFloatring(&m_shmControl5->ringBuffer, opt);
        commitWrite(&m_shmControl5->ringBuffer);
        waitForServer5();  
        return readInt(&m_shm[FIXED_SHM_SIZE]);
}

int RemotePluginClient::canBeAutomated(int param)
{
    writeOpcodering(&m_shmControl5->ringBuffer, RemotePluginCanBeAutomated);
    writeIntring(&m_shmControl5->ringBuffer, param);
    commitWrite(&m_shmControl5->ringBuffer);
    waitForServer5();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
}

int RemotePluginClient::EffectOpen()
{
    if(m_threadinit == 1)
    return 0;
	
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginEffectOpen);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3(); 
	
#ifdef WAVES
    wavesthread = readInt(&m_shm[FIXED_SHM_SIZE]);
   if(wavesthread == 1)
   theEffect->flags |= effFlagsHasEditor; 
#endif
	
    m_threadinit = 1;
	
    return 1;
}

int RemotePluginClient::getFlags()
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return 0;
    }
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetFlags);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
}

int RemotePluginClient::getinitialDelay()
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return 0;
    }
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetinitialDelay);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
}

int RemotePluginClient::getInputCount()
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return 0;
    }

    // writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetInputCount);
    // m_numInputs = readInt(m_processResponseFd);

    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetInputCount);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    m_numInputs = readInt(&m_shm[FIXED_SHM_SIZE]);

    return m_numInputs;
}

int RemotePluginClient::getOutputCount()
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return 0;
    }

    // writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetOutputCount);
    // m_numOutputs = readInt(m_processResponseFd);

    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetOutputCount);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    m_numOutputs = readInt(&m_shm[FIXED_SHM_SIZE]);

    return m_numOutputs;
}

int RemotePluginClient::getParameterCount()
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return 0;
    }

    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetParameterCount);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
}

int RemotePluginClient::getProgramCount()
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return 0;
    }
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetProgramCount);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
}

int RemotePluginClient::getUID()
{
    if (m_inexcept == 1 || m_finishaudio == 1)
    {
        return 0;
    }
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginUniqueID);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
}

int RemotePluginClient::getProgramNameIndexed(int n, char *ptr)
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetProgramNameIndexed);
    writeIntring(&m_shmControl3->ringBuffer, n);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();    
    strcpy(ptr, &m_shm[FIXED_SHM_SIZE]);
    return readInt(&m_shm[FIXED_SHM_SIZE + 512]);
}

std::string RemotePluginClient::getProgramName()
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetProgramName);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return &m_shm[FIXED_SHM_SIZE];
}

void RemotePluginClient::setCurrentProgram(int n)
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginSetCurrentProgram);
    writeIntring(&m_shmControl3->ringBuffer, n);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
}

int RemotePluginClient::getProgram()
{
    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetProgram);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
}

int RemotePluginClient::getChunk(void **ptr, int bank_prg)
{
#ifdef CHUNKBUF
int chunksize;
int chunks;
int chunkrem;
int sz;

    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetChunk);
    writeIntring(&m_shmControl3->ringBuffer, bank_prg);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  

    sz = readInt(&m_shm[FIXED_SHM_SIZE]);

    if(sz <= 0)
    {
    *ptr = &m_shm[FIXED_SHM_SIZECHUNKSTART];	 
    return 0;
    }

    if(sz >= CHUNKSIZEMAX)
    {
        if(chunk_ptr)
        free(chunk_ptr);

        chunk_ptr = (char *) malloc(sz);

        if(!chunk_ptr)
        return 0;

        chunksize = CHUNKSIZE;
        chunks = sz / chunksize;
        chunkrem = sz % chunksize;

        for(int i=0;i<chunks;++i)
        {
        writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetBuf);
        writeIntring(&m_shmControl3->ringBuffer, chunksize);
        writeIntring(&m_shmControl3->ringBuffer, i * chunksize);
        commitWrite(&m_shmControl3->ringBuffer);
        waitForServer3();

        tryRead(&m_shm[FIXED_SHM_SIZECHUNKSTART], &chunk_ptr[i * chunksize], chunksize);
        }

        if(chunkrem)
        {
        writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetBuf);
        writeIntring(&m_shmControl3->ringBuffer, chunkrem);
        writeIntring(&m_shmControl3->ringBuffer, chunks * chunksize);
        commitWrite(&m_shmControl3->ringBuffer);
        waitForServer3();

        tryRead(&m_shm[FIXED_SHM_SIZECHUNKSTART], &chunk_ptr[chunks * chunksize], chunkrem);
        }

        *ptr = chunk_ptr;

    return sz;
    }
    else
    {
    if(sz >= (CHUNKSIZEMAX) || sz <= 0)
    {
    *ptr = &m_shm[FIXED_SHM_SIZECHUNKSTART];	    
    return 0;
    }

    *ptr = &m_shm[FIXED_SHM_SIZECHUNKSTART];
    return sz;
    } 
#else
int sz;

    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginGetChunk);
    writeIntring(&m_shmControl3->ringBuffer, bank_prg);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  

    sz = readInt(&m_shm[FIXED_SHM_SIZE]);

    if(sz >= (CHUNKSIZEMAX) || sz <= 0)
    {
    *ptr = &m_shm[FIXED_SHM_SIZECHUNKSTART];    
    return 0;
    }

    *ptr = &m_shm[FIXED_SHM_SIZECHUNKSTART];
    return sz;
#endif
}

int RemotePluginClient::setChunk(void *ptr, int sz, int bank_prg)
{
#ifdef CHUNKBUF
char *ptridx;
int sz2;
int chunksize;
int chunks;
int chunkrem;

    if(sz <= 0)
    return 0;

    if(sz >= CHUNKSIZEMAX)
    {
        ptridx = (char *)ptr;
        sz2 = sz;

        chunksize = CHUNKSIZE;
        chunks = sz / chunksize;
        chunkrem = sz % chunksize;

        for(int i=0;i<chunks;++i)
        {
        tryWrite(&m_shm[FIXED_SHM_SIZECHUNKSTART], &ptridx[i * chunksize], chunksize);

        writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginSetBuf);
        writeIntring(&m_shmControl3->ringBuffer, chunksize);
        writeIntring(&m_shmControl3->ringBuffer, i * chunksize);
        writeIntring(&m_shmControl3->ringBuffer, sz2);
        commitWrite(&m_shmControl3->ringBuffer);
        waitForServer3();

        sz2 = -1;
        }

        if(chunkrem)
        {
        if(!chunks)
        sz2 = chunkrem;
        else
        sz2 = -1;

        tryWrite(&m_shm[FIXED_SHM_SIZECHUNKSTART], &ptridx[chunks * chunksize], chunkrem);

        writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginSetBuf);
        writeIntring(&m_shmControl3->ringBuffer, chunkrem);
        writeIntring(&m_shmControl3->ringBuffer, chunks * chunksize);
        writeIntring(&m_shmControl3->ringBuffer, sz2);
        commitWrite(&m_shmControl3->ringBuffer);
        waitForServer3();
        }

        writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginSetChunk);
        writeIntring(&m_shmControl3->ringBuffer, sz);
        writeIntring(&m_shmControl3->ringBuffer, bank_prg);
        commitWrite(&m_shmControl3->ringBuffer);
        waitForServer3();  
        return readInt(&m_shm[FIXED_SHM_SIZE]);
    }
    else
    {
    if(sz >= (CHUNKSIZEMAX) || sz <= 0)
    return 0;

    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginSetChunk);
    writeIntring(&m_shmControl3->ringBuffer, sz);
    writeIntring(&m_shmControl3->ringBuffer, bank_prg);
    tryWrite(&m_shm[FIXED_SHM_SIZECHUNKSTART], ptr, sz);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
    }
#else
    if(sz >= (CHUNKSIZEMAX) || sz <= 0)
    return 0;

    writeOpcodering(&m_shmControl3->ringBuffer, RemotePluginSetChunk);
    writeIntring(&m_shmControl3->ringBuffer, sz);
    writeIntring(&m_shmControl3->ringBuffer, bank_prg);
    tryWrite(&m_shm[FIXED_SHM_SIZECHUNKSTART], ptr, sz);
    commitWrite(&m_shmControl3->ringBuffer);
    waitForServer3();  
    return readInt(&m_shm[FIXED_SHM_SIZE]);
#endif
}

void RemotePluginClient::rdwr_tryReadring(RingBuffer *ringbuf, void *buf, size_t count, const char *file, int line)
{
    char *charbuf = (char *)buf;
    size_t tail = ringbuf->tail;
    size_t head = ringbuf->head;
    size_t wrap = 0;

   // if(m_runok == 1)
   // return;

    if (head <= tail) {
        wrap = SHM_RING_BUFFER_SIZE;
    }
    if (head - tail + wrap < count) {
        if(m_inexcept == 0)     
        RemotePluginClosedException();
    }
    size_t readto = tail + count;
    if (readto >= SHM_RING_BUFFER_SIZE) {
        readto -= SHM_RING_BUFFER_SIZE;
        size_t firstpart = SHM_RING_BUFFER_SIZE - tail;
        memcpy(charbuf, ringbuf->buf + tail, firstpart);
        memcpy(charbuf + firstpart, ringbuf->buf, readto);
    } else {
        memcpy(charbuf, ringbuf->buf + tail, count);
    }
    ringbuf->tail = readto;
}

void RemotePluginClient::rdwr_tryWritering(RingBuffer *ringbuf, const void *buf, size_t count, const char *file, int line)
{
    const char *charbuf = (const char *)buf;
    size_t written = ringbuf->written;
    size_t tail = ringbuf->tail;
    size_t wrap = 0;

   // if(m_runok == 1)
   // return;

    if (tail <= written) {
        wrap = SHM_RING_BUFFER_SIZE;
    }
    if (tail - written + wrap < count) {
        std::cerr << "Operation ring buffer full! Dropping events." << std::endl;
        ringbuf->invalidateCommit = true;
        return;
    }

    size_t writeto = written + count;
    if (writeto >= SHM_RING_BUFFER_SIZE) {
        writeto -= SHM_RING_BUFFER_SIZE;
        size_t firstpart = SHM_RING_BUFFER_SIZE - written;
        memcpy(ringbuf->buf + written, charbuf, firstpart);
        memcpy(ringbuf->buf, charbuf + firstpart, writeto);
    } else {
        memcpy(ringbuf->buf + written, charbuf, count);
    }
    ringbuf->written = writeto;
   }
   
void RemotePluginClient::rdwr_writeOpcodering(RingBuffer *ringbuf, RemotePluginOpcode opcode, const char *file, int line)
{
rdwr_tryWritering(ringbuf, &opcode, sizeof(RemotePluginOpcode), file, line);
}

int RemotePluginClient::rdwr_readIntring(RingBuffer *ringbuf, const char *file, int line)
{
    int i = 0;
    rdwr_tryReadring(ringbuf, &i, sizeof(int), file, line);
    return i;
}

void RemotePluginClient::rdwr_writeIntring(RingBuffer *ringbuf, int i, const char *file, int line)
{
   rdwr_tryWritering(ringbuf, &i, sizeof(int), file, line);
}

void RemotePluginClient::rdwr_writeFloatring(RingBuffer *ringbuf, float f, const char *file, int line)
{
   rdwr_tryWritering(ringbuf, &f, sizeof(float), file, line);
}

float RemotePluginClient::rdwr_readFloatring(RingBuffer *ringbuf, const char *file, int line)
{
    float f = 0;
    rdwr_tryReadring(ringbuf, &f, sizeof(float), file, line);
    return f;
}

void RemotePluginClient::rdwr_writeStringring(RingBuffer *ringbuf, const std::string &str, const char *file, int line)
{
    int len = str.length();
    rdwr_tryWritering(ringbuf, &len, sizeof(int), file, line);
    rdwr_tryWritering(ringbuf, str.c_str(), len, file, line);
}

std::string RemotePluginClient::rdwr_readStringring(RingBuffer *ringbuf, const char *file, int line)
{
    int len;
     char *buf = 0;
     int bufLen = 0;
    rdwr_tryReadring(ringbuf, &len, sizeof(int), file, line);
    if (len + 1 > bufLen) {
	delete buf;
	buf = new char[len + 1];
	bufLen = len + 1;
    }
    rdwr_tryReadring(ringbuf, buf, len, file, line);
    buf[len] = '\0';
    return std::string(buf);
}

void RemotePluginClient::rdwr_commitWrite(RingBuffer *ringbuf, const char *file, int line)
{
    if (ringbuf->invalidateCommit) {
        ringbuf->written = ringbuf->head;
        ringbuf->invalidateCommit = false;
    } else {
        ringbuf->head = ringbuf->written;
    }
}

bool RemotePluginClient::dataAvailable(RingBuffer *ringbuf)
{
    return ringbuf->tail != ringbuf->head;
}

void RemotePluginClient::rdwr_tryRead(char *ptr, void *buf, size_t count, const char *file, int line)
{
  memcpy(buf, ptr, count);
}

void RemotePluginClient::rdwr_tryWrite(char *ptr, const void *buf, size_t count, const char *file, int line)
{
  memcpy(ptr, buf, count);
}

void RemotePluginClient::rdwr_writeOpcode(char *ptr, RemotePluginOpcode opcode, const char *file, int line)
{
memcpy(ptr, &opcode, sizeof(RemotePluginOpcode));
}    

void RemotePluginClient::rdwr_writeString(char *ptr, const std::string &str, const char *file, int line)
{
strcpy(ptr, str.c_str());
}

std::string RemotePluginClient::rdwr_readString(char *ptr, const char *file, int line)
{
 char buf[534];
strcpy(buf, ptr);   
return std::string(buf);
}
   
void RemotePluginClient::rdwr_writeInt(char *ptr, int i, const char *file, int line)
{
int *ptr2;
ptr2 = (int *)ptr;
*ptr2  = i;

// memcpy(ptr, &i, sizeof(int));
}

int RemotePluginClient::rdwr_readInt(char *ptr, const char *file, int line)
{
 int i = 0;
int *ptr2;
ptr2 = (int *)ptr;
i = *ptr2;

//    memcpy(&i, ptr, sizeof(int));

    return i;
}

void RemotePluginClient::rdwr_writeFloat(char *ptr, float f, const char *file, int line)
{
float *ptr2;
ptr2 = (float *)ptr;
*ptr2 = f;

// memcpy(ptr, &f, sizeof(float));
}

float RemotePluginClient::rdwr_readFloat(char *ptr, const char *file, int line)
{
 float f = 0;
float *ptr2;
ptr2 = (float *)ptr;
f = *ptr2;

//    memcpy(&f, ptr, sizeof(float));

    return f;
}

void RemotePluginClient::RemotePluginClosedException()
{
m_inexcept = 1;

 //   m_runok = 1;
	
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
#ifdef EMBED
#ifdef EMBEDTHREAD
m_threadbreakembed = 1;
/*
    if (m_shm)
        for (int i=0;i<100000;i++)
        {
            usleep(100);
            if (m_threadbreakexitembed)
            break;
        }
*/
#endif
#endif   
    m_finishaudio = 1;

    sleep(5);

    if (m_AMThread)
        pthread_join(m_AMThread, NULL);

#ifdef EMBED
#ifdef EMBEDTHREAD
    if (m_EMBEDThread)
        pthread_join(m_EMBEDThread, NULL);
#endif
#endif

   // effVoidOp(errorexit);
	
 //   effVoidOp(effClose);
	
/*
#ifdef EMBED
#ifndef XEMBED
    if(display)
    XCloseDisplay(display);   
#endif
#endif
*/

sleep(5);

                   memset (theEffect, 0, sizeof(AEffect));
                   theEffect = 0;          
                   waitForServer2exit(); 
                   waitForServer3exit(); 
                   waitForServer4exit(); 
                   waitForServer5exit();  

}

bool RemotePluginClient::fwait(int *futexp, int ms)
       {
       timespec timeval;
       int retval;

       if(ms > 0) {
		timeval.tv_sec  = ms / 1000;
		timeval.tv_nsec = (ms %= 1000) * 1000000;
	}

       for (;;) {                  
          retval = syscall(SYS_futex, futexp, FUTEX_WAIT, 0, &timeval, NULL, 0);
          if (retval == -1 && errno != EAGAIN)
          return true;

          if((*futexp != 0) && (__sync_val_compare_and_swap(futexp, *futexp, *futexp - 1) > 0))
          break;
          }
                               
          return false;          
       }

bool RemotePluginClient::fpost(int *futexp)
       {
       int retval;

	__sync_fetch_and_add(futexp, 1);

        retval = syscall(SYS_futex, futexp, FUTEX_WAKE, 1, NULL, NULL, 0);
/*
        if (retval  == -1)
        return true;
*/  
        return false;          
       }
