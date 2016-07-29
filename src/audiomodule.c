#include <Python.h>
#include "structmember.h"
#include <libkern/OSAtomic.h>

#include <AudioUnit/AudioUnit.h>
#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioToolbox.h>

#define checkStatus( err) \
if(err) {\
  printf("Error: %s ->  %s: %d\n", (char *)&err,__FILE__, __LINE__);\
    fflush(stdout);\
      return (PyObject *)-1;\
} 
///////////////////////
//
//Object Declarations
//
///////////////////////
static PyTypeObject audio_OutputType;
typedef struct{
	PyObject_HEAD
	
} audio_UnbufferdAudioFile;
typedef struct {
  PyObject_HEAD
  UInt64 totalPacketCount;
  UInt64 fileByteCount;
  UInt32 maxPacketSize;
  
  AudioStreamBasicDescription fileASBD;
  //AudioChannelLayout channelLayout;
  AudioFileID fileID;
  char *fileName;
  FSRef fileRef;
  void *fileBuffer;
  //void *sourceBuffer;
  OSSpinLock *tellLock;
  UInt64  tell;            //position in file for last read in bytes
  UInt32  packetTell;            //position in file for last read in packets
  
} audio_AudioFileObject;


typedef struct {
  PyObject_HEAD
  AudioUnit theOutputUnit;
  AudioStreamBasicDescription theDesc;
  AURenderCallbackStruct renderCallback;
  PyObject *currentFile;
  PyObject *converter;
  void * CABuffer; 
  Boolean isPlaying;
} audio_OutputObject;

typedef struct {
  PyObject_HEAD
  AudioConverterRef converter;
} audio_AudioConverterObject;

///////////////////////
//
//Some Functions
//
///////////////////////
static PyObject * hello_world(PyObject *self, PyObject *args){
  if(!PyArg_ParseTuple(args, ""))
    return NULL;
  return Py_BuildValue("s","Hello World");
}

static PyMethodDef AudioMethods[]={
  {"HelloWorld",hello_world,METH_VARARGS,"Returns a string containing \"Hellp World\""},
  {NULL, NULL, 0, NULL}
};

///////////////////////
//
//The AudioFile Object
//
///////////////////////


///////////////////////
//
//C help functions AudioFile
//
///////////////////////
OSStatus bufferFile(audio_AudioFileObject *self){
  OSStatus err = noErr;
  
  //total bytes read from audio file
  UInt32  bytesReturned = 0;
  
  //total amount of packets in audio file
  UInt32 packets =self->totalPacketCount;
  
  //alloc a buffer of memory to hold the data read from disk.
  self->fileBuffer = malloc(self->fileByteCount);
  memset(self->fileBuffer, 0, self->fileByteCount);
  
  //Read in the ENTIRE file into a memory buffer
  err = AudioFileReadPackets (self->fileID,
                              true,
                              &bytesReturned,
                              NULL,
                              0,
                              &packets,
                              self->fileBuffer);
  
  
  return err;

}

OSStatus GetFileInfo(audio_AudioFileObject *self){
  //fixing compatebility whith py object audio_AudioFileObject
  FSRef *fileRef=&(self->fileRef);
  AudioFileID *fileID=&(self->fileID);
  AudioStreamBasicDescription *fileASBD=&(self->fileASBD);
  const char *fileName=self->fileName;
  OSStatus err= noErr;
  UInt32 size;
  UInt64 gTotalPacketCount;
  UInt64 gFileByteCount;
  UInt32 gMaxPacketSize;
  
  
  //Obtain filesystem reference to the file using the file path
  FSPathMakeRef ((const UInt8 *)fileName, fileRef, 0);
  //Open an AudioFile and obtain AudioFileID using the file system ref
  err = AudioFileOpen(fileRef, fsRdPerm,0,fileID);
  
  if(err){
	  PyErr_SetObject(PyExc_IOError,Py_BuildValue("(iss)",err,"Could not find File.",fileName));
	  return err;
  }
  
  size = sizeof(AudioStreamBasicDescription);
  memset(fileASBD, 0, size);
  
  //Fetch the AudioStreamBasicDescription of the audio file.   we can
  //skip calling AudioFileGetPropertyInfo because we already know the
  //size of a ASBD
  err = AudioFileGetProperty(*fileID,
                             kAudioFilePropertyDataFormat,
                             &size,
                             fileASBD);
  if(err){
	  PyErr_SetObject(PyExc_IOError,Py_BuildValue("(iss)",err,"Could not find format of File.",fileName));
	  return err;
  }
  
  //We need to get the total packet count, byte count, and max packet size
  //Theses values will be used later when grabbing data from the
  //audio file in the input callback procedure.
  size = sizeof(gTotalPacketCount); //type is UInt64
  err = AudioFileGetProperty(*fileID,
                             kAudioFilePropertyAudioDataPacketCount,
                             &size,
                             &gTotalPacketCount);
  if(err){
	  PyErr_SetObject(PyExc_IOError,Py_BuildValue("(iss)",err,"Could not find audio data packet count of File.",fileName));
	  return err;
  }
  
  size = sizeof(gFileByteCount); //type is UInt64
  err = AudioFileGetProperty(*fileID,
                             kAudioFilePropertyAudioDataByteCount,
                             &size,
                             &gFileByteCount);
  if(err){
	  PyErr_SetObject(PyExc_IOError,Py_BuildValue("(iss)",err,"Could not find audio data byte count of File.",fileName));
	  return err;
  }
  size = sizeof(gMaxPacketSize); //type is UInt32
  err = AudioFileGetProperty(*fileID,
                             kAudioFilePropertyMaximumPacketSize,
                             &size,
                             &gMaxPacketSize);
  


  if(err){
	  PyErr_SetObject(PyExc_IOError,Py_BuildValue("(iss)",err,"Could not find maximum packet size of File.",fileName));
	  return err;
  }
  /*
  size = sizeof(&self->channelLayout); //type is UInt32
  err = AudioFileGetProperty(*fileID,
                             kAudioFilePropertyChannelLayout,
                             &size,
                             &(&channelLayout));
  
  
  
  if(err)
	  return err;
  */
  self->maxPacketSize=gMaxPacketSize;
  self->totalPacketCount=gTotalPacketCount;
  self->fileByteCount=gFileByteCount;
  
  return err;
}

static OSStatus initAudioFile(AudioUnit *theOutputUnit){
  OSStatus err=noErr;
  return err;
}
///////////////////////
//
//end of C help functions AudioFile
//
///////////////////////

static PyObject * audio_AudioFileObject_init(audio_AudioFileObject *self, PyObject *args, PyObject *kwds){
  OSStatus err=noErr;
  
	if (!PyArg_ParseTuple(args, "s", &(self->fileName))){
		
		return NULL;
	}
  err=GetFileInfo(self);
  checkStatus(err);
  
  err=bufferFile(self);
  if(err){
	  PyErr_SetObject(PyExc_IOError,Py_BuildValue("(iss)",err,"buffer faild on File.",self->fileName));
  } checkStatus(err);
  
  return 0;
}

static void audio_AudioFileObject_dealloc(audio_AudioFileObject* self){
	free(self->fileBuffer);
    self->ob_type->tp_free((PyObject*)self);
}

void PrintStreamDesc (AudioStreamBasicDescription *inDesc)
{
	if (!inDesc) {
		printf ("Can't print a NULL desc!\n");
		return;
	}
	
	printf ("- - - - - - - - - - - - - - - - - - - -\n");
	printf ("  Sample Rate:%f\n", inDesc->mSampleRate);
	printf ("  Format ID:%s\n", (char*)&inDesc->mFormatID);
	printf ("  Format Flags:%lX\n", inDesc->mFormatFlags);
	printf ("  Bytes per Packet:%ld\n", inDesc->mBytesPerPacket);
	printf ("  Frames per Packet:%ld\n", inDesc->mFramesPerPacket);
	printf ("  Bytes per Frame:%ld\n", inDesc->mBytesPerFrame);
	printf ("  Channels per Frame:%ld\n", inDesc->mChannelsPerFrame);
	printf ("  Bits per Channel:%ld\n", inDesc->mBitsPerChannel);
	printf ("- - - - - - - - - - - - - - - - - - - -\n");
}
static int audio_AudioFileObject_setPos(audio_AudioFileObject *self, PyObject *value, void *closure)
{
  OSSpinLockLock((OSSpinLock *)&(self->tellLock));
  double pos=PyFloat_AsDouble(value);
  
  //PrintStreamDesc(&(self->fileASBD));
  if ((pos*(double)self->fileASBD.mSampleRate *(double)self->maxPacketSize/(double)self->fileASBD.mFramesPerPacket)>self->fileByteCount && pos<0) {
    OSSpinLockUnlock((OSSpinLock *)&(self->tellLock));
    PyErr_Format(PyExc_TypeError,"The value vas out of range should be >0 and < %f",self->fileByteCount/(self->fileASBD.mSampleRate *self->fileASBD.mBytesPerFrame));
  }
  self->packetTell=pos*(double)self->fileASBD.mSampleRate/(double)self->fileASBD.mFramesPerPacket;
  self->tell=self->packetTell*(double)self->maxPacketSize;
  OSSpinLockUnlock((OSSpinLock *)&(self->tellLock));
  return 0;
}
static PyObject * audio_AudioFileObject_getPos(audio_AudioFileObject *self, void *closure)
{
  OSSpinLockLock((OSSpinLock *)&(self->tellLock));
  //printf ("  nämnaren %d\n", (self->fileASBD.mSampleRate * self->fileASBD.mBytesPerFrame));
  double temp=self->tell;
  temp/=(double)self->maxPacketSize; //The number of bytes in a packet of data.
  temp*=(double)self->fileASBD.mFramesPerPacket;//The number of sample frames in each packet of data.
  temp/=(double)self->fileASBD.mSampleRate;//The number of sample frames per second of the data in the stream.

  PyObject * ans=Py_BuildValue("d",temp);
  OSSpinLockUnlock((OSSpinLock *)&(self->tellLock));
  return ans;
}
static PyObject * audio_AudioFileObject_getLength(audio_AudioFileObject *self, void *closure)
{
  double temp=self->fileByteCount;
  temp/=(double)self->maxPacketSize; //The number of bytes in a packet of data.
  temp*=(double)self->fileASBD.mFramesPerPacket;//The number of sample frames in each packet of data.
  temp/=(double)self->fileASBD.mSampleRate;//The number of sample frames per second of the data in the stream.
      
  PyObject * ans=Py_BuildValue("d",temp);
  return ans;
}
static audio_AudioFileObject_traverse(audio_OutputObject *self, visitproc visit, void *arg){
	return 0;
}
static PyGetSetDef audio_AudioFileObject_getseters[] = {
  {"pos", 
    (getter)audio_AudioFileObject_getPos, (setter)audio_AudioFileObject_setPos,
    "position of file in seconds",
    NULL},
  {"length",(getter)audio_AudioFileObject_getLength,NULL,"length of file in seconds",NULL},
  {NULL}  /// Sentinel 
};

static PyTypeObject audio_AudioFileType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "audio.AudioFile",             /*tp_name*/
  sizeof(audio_AudioFileObject), /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)audio_AudioFileObject_dealloc,                         /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,         /*tp_flags*/
  "AudioFile object for output of sound",           /* tp_doc */
  (traverseproc)audio_AudioFileObject_traverse,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  0,             /* tp_methods */
  0,             /* tp_members */
  audio_AudioFileObject_getseters,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc) audio_AudioFileObject_init,      /* tp_init */
  0,                         /* tp_alloc */
  0,                 /* tp_new */
};
///////////////////////
//
//The AudioConverter Object
//
///////////////////////

///////////////////////
//
//C help functions AudioConverter
//
///////////////////////
static OSStatus initConverter(audio_AudioConverterObject *self, AudioStreamBasicDescription *source_AudioStreamBasicDescription, AudioStreamBasicDescription *destination_AudioStreamBasicDescription){
  OSStatus err=noErr;
  err=AudioConverterNew(source_AudioStreamBasicDescription,destination_AudioStreamBasicDescription ,&(self->converter) );
  return err;
}
///////////////////////
//
//end of C help functions AudioConverter
//
///////////////////////

static PyObject * audio_AudioConverterObject_init(audio_AudioConverterObject *self, PyObject *args, PyObject *kwds){
  OSStatus err=noErr;
  PyObject *to=NULL;
  PyObject *from=NULL;
  if (!PyArg_ParseTuple(args,"O!O!:Converter",(PyObject *)&audio_AudioFileType,&from,(PyObject *)&audio_OutputType,&to)) {
    return NULL;
  }
    
  err=initConverter(self,&(((audio_AudioFileObject *)from)->fileASBD),&(((audio_OutputObject *)to)->theDesc));
  checkStatus(err);
   
  return 0;
}
static void audio_AudioConverterObject_dealloc(audio_OutputObject* self){
  self->ob_type->tp_free((PyObject*)self);
}
static audio_AudioConverterObject_traverse(audio_OutputObject *self, visitproc visit, void *arg){
	return 0;
}
static PyTypeObject audio_AudioConverterType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "audio.Converter",             /*tp_name*/
  sizeof(audio_AudioConverterObject), /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)audio_AudioConverterObject_dealloc,                         /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT| Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,        /*tp_flags*/
  "AudioConverter object for convertion betwen file and output.",           /* tp_doc */
  (traverseproc)audio_AudioConverterObject_traverse,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  0,             /* tp_methods */
  0,             /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc) audio_AudioConverterObject_init,      /* tp_init */
  0,                         /* tp_alloc */
  0,                 /* tp_new */
};
///////////////////////
//
//The Output Object
//
///////////////////////

//will not be used by uss att this point!
static PyObject * audio_OutputObject_new(PyTypeObject *type, PyObject *arg, PyObject *kwds){
  audio_OutputObject *self;  
  //allocates the object!
  self=(audio_OutputObject *)type->tp_alloc(type,0);
  if(self!=NULL){
    //setting defaoult values!
  }
  return (PyObject *)self;
}

// a C help function for initiating the Output object
OSStatus initOutput(AudioUnit *theOutputUnit){
  OSStatus err=noErr;
  
  //An AudioUnit is an OS component.
  //A component description must be setup, then used to
  //initialize an AudioUnit
  ComponentDescription desc;
  Component comp;
  
  //There are several Different types of AudioUnits.
  //Some audio units serve as Outputs, Mixers, or DSP
  //units. See AUComponent.h for listing
  
  desc.componentType = kAudioUnitType_Output;
  
  //Every Component has a subType, which will give a clearer picture
  //of what this components function will be.
  
  desc.componentSubType = kAudioUnitSubType_DefaultOutput;
  
  //All AudioUnits in AUComponent.h must use
  //"kAudioUnitManufacturer_Apple" as the Manufacturer
  
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;
  
  //Finds a component that meets the desc spec's
  comp = FindNextComponent(NULL, &desc);
  if (comp == NULL) exit (-1);
  
  //gains access to the services provided by the component
  err = OpenAComponent(comp, theOutputUnit);
  
  verify_noerr(AudioUnitInitialize(*theOutputUnit));
  
  return err;
}
OSStatus initDesc(AudioUnit *theUnit, AudioStreamBasicDescription *theDesc){
  //AudioUnit *theUnit - points to the current AudioUnit
  //AudioStreamBasicDescription *theDesc  - current ASBD for user output
  
  /***Getting the size of a Property***/
  UInt32 size = sizeof (AudioStreamBasicDescription);
  memset(theDesc, 0, size);
  
  Boolean outWritable;
  UInt32 theInputBus=0;
  
  
  
  //Gets the size of the Stream Format Property and if it is writable
  OSStatus result = AudioUnitGetPropertyInfo(*theUnit,
                                             kAudioUnitProperty_StreamFormat,
                                             kAudioUnitScope_Output,
                                             0,
                                             &size,
                                             &outWritable);
  
  //Get the current stream format of the output
  result = AudioUnitGetProperty (*theUnit,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Output,
                                 0,
                                 theDesc,
                                 &size);
  
  //Set the stream format of the output to match the input
  result = AudioUnitSetProperty (*theUnit,
                                 kAudioUnitProperty_StreamFormat,
                                 kAudioUnitScope_Input,
                                 theInputBus,
                                 theDesc,
                                 size);
  return result;
}


OSStatus MyACComplexInputProc (AudioConverterRef        inAudioConverter,
                               UInt32        *ioNumberDataPackets,
                               AudioBufferList             *ioData,
                               AudioStreamPacketDescription    **outDataPacketDescription,
                               void          *inUserData)
{
  //printf("enter: MyACComplexInputProc\n");
  OSStatus    err = noErr;
  UInt32  bytesCopied = 0;
  audio_AudioFileObject *self=(audio_AudioFileObject *)((audio_OutputObject *)inUserData)->currentFile;
  audio_OutputObject *out=((audio_OutputObject *)inUserData);
  // initialize in case of failure
  int i=0;
  //for (i = 0; i< ioData->mNumberBuffers; i++){
	  ioData->mBuffers[i].mData = NULL;
	  ioData->mBuffers[i].mDataByteSize = 0;
  //}
  //if there are not enough packets to satisfy request,
  //then read what's left
  if (self->packetTell + *ioNumberDataPackets > self->totalPacketCount){
    *ioNumberDataPackets = self->totalPacketCount - self->packetTell;
    printf("more packets then there are");
  }
  
  //the total amount of data requested by the AudioConverter
  if ((*ioNumberDataPackets * self->maxPacketSize +self->tell)>self->fileByteCount) {
      bytesCopied=self->fileByteCount-self->tell;
      //printf("endreached ioNumberDataPackets=%d, bytesCopied= %d/n",*ioNumberDataPackets,bytesCopied);
  }
  else{
      bytesCopied = *ioNumberDataPackets * self->maxPacketSize;
  }
  // do nothing if there are no packets available
  //printf("ioNumberDataPackets: MyACComplexInputProc =%d\n",*ioNumberDataPackets);
  if (*ioNumberDataPackets && bytesCopied && ((audio_OutputObject *)inUserData)->isPlaying)
  {
    //printf("buffer file: MyACComplexInputProc\n");
    if (out->CABuffer != NULL) {
      //printf("free buffer: MyACComplexInputProc\n");
      free(out->CABuffer);
      out->CABuffer = NULL;
    }
    


    //alloc a small buffer for the AudioConverter to use.
    //printf("calloc buffer: MyACComplexInputProc\n");
    out->CABuffer = (void *) calloc (1,bytesCopied);
    
    //copy the amount of data needed (bytesCopied)
    //from buffer of audio file
    //printf("coppy buffer: MyACComplexInputProc\n");
    memcpy(out->CABuffer, self->fileBuffer + self->tell,bytesCopied);
    
    // keep track of where we want to read from next time
    //printf("move tell: MyACComplexInputProc\n");
    //printf("shuld be= %d\n", *ioNumberDataPackets * (self->maxPacketSize) + self->tell);
    //printf("tell = %d * %d + %d =", *ioNumberDataPackets,(self->maxPacketSize),self->tell);
    self->tell +=bytesCopied;//*ioNumberDataPackets * (self->maxPacketSize);
    self->packetTell += bytesCopied/(self->maxPacketSize);//*ioNumberDataPackets;
    //printf(" %d\n",self->tell);

    //printf("point to buffer: MyACComplexInputProc\n");
    // tell the Audio Converter where it's source data is
	
	//for (i = 0; i< ioData->mNumberBuffers; i++){
		ioData->mBuffers[i].mData = out->CABuffer;
    // tell the Audio Converter how much data in each buffer
		ioData->mBuffers[i].mDataByteSize = bytesCopied;
		//tell the Audio Converter how manny Channels its should use
		//ioData->mBuffers[i].mNumberChannels=2;//(self->fileASBD).mChannelsPerFrame;
	//	}	
	}
  else
  {
    //printf("end of file: MyACComplexInputProc\n");
    // there aren't any more packets to read.
    // Set the amount of data read (mDataByteSize) to zero
    // and return noErr to signal the AudioConverter there are
    // no packets left.
      //for (i = 0; i< ioData->mNumberBuffers; i++){
		  ioData->mBuffers[i].mData = NULL;
		  ioData->mBuffers[i].mDataByteSize = 0;
	  //}
	((audio_OutputObject *)inUserData)->isPlaying=FALSE;
	Py_XDECREF(self);
    err = noErr;
  }

  //printf("exit: MyACComplexInputProc\n");
  return err;
  
}

OSStatus FileRenderProc(void     *inRefCon,
                          AudioUnitRenderActionFlags  *inActionFlags,
                          const AudioTimeStamp *inTimeStamp,
                          UInt32     inBusNumber,
                          UInt32    inNumFrames,
                          AudioBufferList     *ioData)

{
  //printf("enter: FileRenderProc\n");
  OSStatus err= noErr;
  PyObject * pyConverter=((audio_OutputObject *)inRefCon)->converter;
  audio_AudioFileObject *self=(audio_AudioFileObject *)((audio_OutputObject *)inRefCon)->currentFile;
  //To obtain a data buffer of converted data from a complex input
  //source(compressed files, etc.) use AudioConverterFillComplexBuffer.
  OSSpinLockLock((OSSpinLock *)&(self->tellLock));
  AudioConverterFillComplexBuffer(((audio_AudioConverterObject *)pyConverter)->converter,
                                  MyACComplexInputProc,
                                  inRefCon,
                                  &inNumFrames,
                                  ioData,
                                  0);
  OSSpinLockUnlock((OSSpinLock *)&(self->tellLock));

  //printf("exit: FileRenderProc\n");
  
  return err;

/*
 Parameters for AudioConverterFillComplexBuffer()
 
 converter - the converter being used
 
 MyACComplexInputProc() - input procedure to supply data to the Audio
 Converter.
 
 inNumFrames - The amount of requested data on input.  On output, this
 number is the amount actually received.
 
 ioData - Buffer of the converted data recieved on return
 */
}
OSStatus initRenderCallback(audio_OutputObject *self,AudioUnit *theOutputUnit, AURenderCallbackStruct *renderCallback){
  OSStatus err= noErr;
  memset(renderCallback, 0, sizeof(AURenderCallbackStruct));
  
  
  //inputProc takes a name of a method that will be used as the
  //input procedure when rendering data to the AudioUnit.
  //The input procedure will be called only when the Audio Converter needs
  //more data to process.
  
  
  //Set "fileRenderProc" as the name of the input proc
  renderCallback->inputProc = FileRenderProc;
  //Can pass ref Con with callback, but this isnt needed in out example
  renderCallback->inputProcRefCon =self;
  
  //Sets the callback for the AudioUnit to the renderCallback
  
  err = AudioUnitSetProperty (*theOutputUnit,
                              kAudioUnitProperty_SetRenderCallback,
                              kAudioUnitScope_Input,
                              0,
                              renderCallback,
                              sizeof(AURenderCallbackStruct));
  //Note: Some old V1 examples may use
  //"kAudioUnitProperty_SetInputCallback" which existed in
  //the old API, instead of "kAudioUnitProperty_SetRenderCallback".
  //"kAudioUnitProperty_SetRenderCallback" should
  //be used from now on.
  
  
  return err;
}

static PyObject * audio_OutputObject_play(audio_OutputObject* self,PyObject *args,PyObject *kwds){
  PyObject *file=NULL, *converter=NULL, *tmp;
  
  static char *kwlist[] = {"File", "Converter", NULL};
  
  if (! PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist,(PyObject *)&audio_AudioFileType,&file,(PyObject *)&audio_AudioConverterType ,&converter))
    return NULL;
  self->isPlaying=FALSE;
  if (file) {
    tmp=self->currentFile;
    Py_INCREF(file);
    self->currentFile=file;
    Py_XDECREF(tmp);
  }
  if (converter) {
    tmp=self->converter;
    Py_INCREF(converter);
    self->converter=converter;
    Py_XDECREF(tmp);
  }
  if (self->converter && self->currentFile) {
    self->isPlaying=true;
	Py_INCREF(self->currentFile);
    AudioOutputUnitStart(self->theOutputUnit);
  }
  
  return Py_BuildValue("");
}
static PyObject * audio_OutputObject_stop(audio_OutputObject* self,PyObject *args,PyObject *kwds){
  
  self->isPlaying=FALSE;
  return Py_BuildValue("");
}

static PyObject * audio_OutputObject_init(audio_OutputObject *self, PyObject *arg, PyObject *kwds){
  OSStatus err=noErr;
  
  err=initOutput(&(self->theOutputUnit));
  checkStatus(err)
    
  err=initDesc(&(self->theOutputUnit), &(self->theDesc));
  checkStatus(err)
  
  err=initRenderCallback(self,&(self->theOutputUnit),&(self->renderCallback));
  checkStatus(err)

  return 0;
}

static void audio_OutputObject_dealloc(audio_OutputObject* self){
  AudioOutputUnitStop(self->theOutputUnit);//you must stop the audio unit
    AudioUnitUninitialize(self->theOutputUnit);
    CloseComponent(self->theOutputUnit);
	free(self->CABuffer);
    
    PyObject *tmp;
    
    tmp = self->currentFile;
    self->currentFile = NULL;
    Py_XDECREF(tmp);
    
    tmp = self->converter;
    self->converter = NULL;
    Py_XDECREF(tmp);
    
    self->ob_type->tp_free((PyObject*)self);
}
static audio_OutputObject_traverse(audio_OutputObject *self, visitproc visit, void *arg){
  int vret;
  
  if (self->currentFile) {
    vret = visit(self->currentFile, arg);
    if (vret != 0)
      return vret;
  }
  if (self->converter) {
    vret = visit(self->converter, arg);
    if (vret != 0)
      return vret;
  }
  
  return 0;
}
static PyMemberDef audio_OutputObject_members[] = {
  {"CurrentFile", T_OBJECT_EX, offsetof(audio_OutputObject, currentFile), 0,
    "the file that shoul dbe playing."},
  {"CurrentConverter", T_OBJECT_EX, offsetof(audio_OutputObject, converter), 0,
    "the Converter that should be used for play back."},
  {"isPlaying", T_INT, offsetof(audio_OutputObject, isPlaying), 0,
    "if its playing"},
  {NULL}  /* Sentinel */
};
static PyMethodDef audio_OutputObject_methods[] = {
  {"play", (PyCFunction)audio_OutputObject_play, METH_KEYWORDS,
    "starts playing a file, if no file selected it dont start. play(file,converter)"
  },
  {"stop", (PyCFunction)audio_OutputObject_stop, METH_NOARGS,
    "stops playing current file"
  },
  {NULL}  /* Sentinel */
};
static PyTypeObject audio_OutputType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "audio.Output",             /*tp_name*/
  sizeof(audio_OutputObject), /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)audio_OutputObject_dealloc,                         /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT| Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC ,        /*tp_flags*/
  "AudioFile object for output of sound",           /* tp_doc */
  (traverseproc) audio_OutputObject_traverse,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  audio_OutputObject_methods,             /* tp_methods */
  audio_OutputObject_members,             /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc) audio_OutputObject_init,      /* tp_init */
  0,                         /* tp_alloc */
  0,                 /* tp_new */
};


///////////////////////
//
//The Audio Module
//
///////////////////////

PyMODINIT_FUNC initaudio(void){
  PyObject *m;
	
	audio_OutputType.tp_new=PyType_GenericNew;
	if (PyType_Ready(&audio_OutputType) < 0)
    return;
  audio_AudioFileType.tp_new=PyType_GenericNew;
	if (PyType_Ready(&audio_AudioFileType) < 0)
    return;
  audio_AudioConverterType.tp_new=PyType_GenericNew;
	if (PyType_Ready(&audio_AudioConverterType) < 0)
    return;	
  m=Py_InitModule3("daisyDelight.audio", AudioMethods,
                   "Module that should make it easyer to play sound on a mac with coreAudio."
                   );
	Py_INCREF(&audio_OutputType);
  Py_INCREF(&audio_AudioFileType);
  Py_INCREF(&audio_AudioConverterType);
	PyModule_AddObject(m, "Output", (PyObject *)&audio_OutputType);
	PyModule_AddObject(m, "AudioFile", (PyObject *)&audio_AudioFileType);
  PyModule_AddObject(m, "Converter", (PyObject *)&audio_AudioConverterType);

}

