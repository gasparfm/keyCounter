/**
*************************************************************
* @file keyCounter.c
*
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version 0.1
* @date 10 nov 2010
*
* Dependencies:
*   - libstst-dev
*   - x11proto-record-dev
*
* Compile:
*   - g++ -o keyCounter keyCounter.cpp cfileutils.cpp -lX11 -lXtst
*************************************************************/

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <ctime>
#include <unistd.h>
#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include "cfileutils.h"
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>

#define DEFAULT_MAX_IDLE_TIME 15
#define DEFAULT_MIN_STORE_TIME 120
#define DEFAULT_MAX_FILE_SIZE 100000
#define EXIT_ON_ESCAPE 0

using namespace std;

int Exit_signal = 0;

typedef struct
{
  int Status1, Status2, x, y, mmoved, doit;
  unsigned int QuitKey;
  Display *LocalDpy, *RecDpy;
  XRecordContext rc;
} Priv;

string itoa(int i)
{
  stringstream ss;
  ss<<i;
  
  return ss.str();
}

string strtime(time_t timestamp, string format)
{
  char ss[100];
  struct tm *tmp = localtime(&timestamp);
  if (tmp==NULL)
    return "";

  strftime(ss, 100, format.c_str(), tmp);

  return (string)ss;
}

const std::string whiteSpaces( " \f\n\r\t\v" );

void trimRight( std::string& str,
      const std::string& trimChars = whiteSpaces )
{
   std::string::size_type pos = str.find_last_not_of( trimChars );
   str.erase( pos + 1 );
}

void trimLeft( std::string& str,
      const std::string& trimChars = whiteSpaces )
{
   std::string::size_type pos = str.find_first_not_of( trimChars );
   str.erase( 0, pos );
}

string trim( std::string str, const std::string& trimChars = whiteSpaces )
{
   trimRight( str, trimChars );
   trimLeft( str, trimChars );
   return str;
}

void criticalError(string msg)
{
  cerr << "Error: "<< msg << endl;
  exit ( EXIT_FAILURE );
}

string getKeysymStr(Priv *p, unsigned int key)
{
    int kpkr;
    string res;

    KeySym *keysym = XGetKeyboardMapping(p->LocalDpy,
        key,
        1,
        &kpkr);

    res = XKeysymToString(keysym[0]);
    /* do something with keysym[0] */

    XFree(keysym);
    return res;
}

class KCAnalyzer
{
public:
  KCAnalyzer()
  {
  }
  ~KCAnalyzer()
  {
  }

  string keycount()
  {
    string s;
    this->getStats();

    for (map<string, unsigned>::iterator i=keyTimes.begin(); i!=keyTimes.end(); ++i)
      {
    	s+=(string)i->first+";"+(string)itoa(i->second)+"\n";
      }
    return s;
  }

  string burst()
  {
    string s;
    this->getStats();

    s=start_stop_history;
    cerr << "Max writing time: "<<max_writing<<"s since "<<strtime(max_writing_timestamp, "%d/%m/%Y %H:%M")<<endl;
    cerr << "Max idle time: "<<max_stopped<< "s since "<<strtime(max_stopped_timestamp, "%d/%m/%Y %H:%M")<<endl;
    cerr << "Min writing time: "<<min_writing<<"s since "<<strtime(min_writing_timestamp, "%d/%m/%Y %H:%M")<<endl;
    cerr << "Min idle time: "<<min_stopped<< "s since "<<strtime(min_stopped_timestamp, "%d/%m/%Y %H:%M")<<endl;

    return s;
  }

  string hourlyLog()
  {
    string s;
    this->getStats();

    for (map<time_t, unsigned>::iterator i=hourly.begin(); i!=hourly.end(); ++i)
      {
	cout << itoa(i->first)<<";"<<strtime(i->first, "%d/%m/%Y %H:%M")<<";"<<i->second<<endl;
      }

    return s;
  }

private:
  vector <string> fileList;
  map<string, unsigned> keyTimes;
  map<time_t, unsigned> hourly;
  int state;
  time_t last_started, last_stopped, last_saved;
  unsigned max_writing, max_stopped;
  unsigned min_writing, min_stopped;
  time_t max_writing_timestamp;
  time_t max_stopped_timestamp;
  time_t min_writing_timestamp;
  time_t min_stopped_timestamp;
  string start_stop_history;
  time_t current_time;

  bool parseKeyPress(string line, int offset)
  {
    size_t pos, pos2;
    string keysym;
    int times;

    pos = line.find('(', offset);
    pos2 = line.find(')', pos);

    if ( (pos==string::npos) || (pos2==string::npos) )
      return false;
    keysym = line.substr(pos+1, pos2-pos-1);

    pos= line.find(':', pos2);
    if (pos==string::npos)
      return false;

    times = atoi(trim(line.substr(pos+1)).c_str());
    hourly[current_time]+=times;
    keyTimes[keysym]+=times;
    return true;
  }

  bool parseTime(string line, int offset, size_t &time)
  {
    size_t pos = line.find(':');
    if (pos==string::npos)
      return false;

    time = atoi(trim(line.substr(pos+1)).c_str());
    return true;
  }

  bool parseSaveState(string line, int offset)
  {
    size_t time;
    if (!parseTime(line, offset, time))
      return false;

    current_time = 3600* (time/3600);
    last_saved=time;
    return true;
  }

  bool parseStartTyping(string line, int offset)
  {
    size_t time;
    size_t diff;
    if (!parseTime(line, offset, time))
      return false;

    if (!(state&8))
      {
	if (state&4)		// It has been stopped at least once
	  {
	    diff = time-last_stopped;
	    start_stop_history+="Stop;"+itoa(last_stopped)+";"+itoa(diff)+"\n";
	    if (!(state&1))	// Don't have max && min time stopped
	      {
		max_stopped=diff;
		min_stopped=diff;
		max_stopped_timestamp=last_stopped;
		min_stopped_timestamp=last_stopped;
		state+=1;
	      }
	    else
	      {
		if (diff>max_stopped)
		  {
		    max_stopped=diff;
		    max_stopped_timestamp=last_stopped;
		  }
		if (diff<min_stopped)
		  {
		    min_stopped=diff;
		    min_stopped_timestamp=last_stopped;
		  }
	      }
	  }

	last_started=time;
	state+=8;
      }
    else
      {
	cerr << "Caution, we have just started typing... possible bug"<<endl;
      }
    return true;
  }

  bool parseStopTyping(string line, int offset)
  {
    size_t time;
    size_t diff;

    if (!parseTime(line, offset, time))
      return false;

    if (state&8)		// Typing started
      {
	if (!(state&4))		// First time stopped
	  state+=4;
	
	diff=time-last_started;
	start_stop_history+="Start;"+itoa(last_started)+";"+itoa(diff)+"\n";
	if (!(state&2))		// Dont have max && min time writing
	  {
	    max_writing=diff;
	    min_writing=diff;
	    max_writing_timestamp=last_started;
	    min_writing_timestamp=last_started;
	    state+=2;
	    
	  }
	else
	  {
	    if (diff>max_writing)
	      {
		max_writing=diff;
		max_writing_timestamp=last_started;
	      }
	    if (diff<min_writing)
	      {
		min_writing=diff;
		min_writing_timestamp=last_started;
	      }

	  }

	last_stopped=time;
	state-=8;
      }
    else
      {
	cerr << "Caution! Typing is already stopped... possible bug"<<endl;
      }
    return true;
  }

  bool parseStatLine(string line)
  {
    size_t pos;
    int command;

    pos = line.find(' ');
    if (pos==string::npos)
      return false;
    
    command = atoi(line.substr(0, pos).c_str());
    switch (command)
      {
      case 1:
	return parseKeyPress(line, pos);
      case 7:
	return parseStopTyping(line, pos);
      case 8:
	return parseStartTyping(line, pos);
      case 9:
	return parseSaveState(line, pos);
      default:
	return false;
      }
  }

  void getStats()
  {
    string line;

    this->state=0;
    generateFileList();
    for (unsigned i = 0; i<fileList.size(); ++i)
      {
	cout << "Reading "<<fileList[i]<<endl;
	ifstream ifs;
	ifs.open(fileList[i].c_str());
	if (!ifs.is_open())
	  {
	    cerr << "Skipping "+fileList[i]<<endl;
	    continue;
	  }
	cout << "read lines..."<<endl;
	while (!ifs.eof())
	  {
	    getline(ifs, line);
	    if ( (!this->parseStatLine(line)) && (!line.empty()))
	      cerr << "Wrong data line: \""+line+"\""<<endl;
	  }
      }
  }

  void generateFileList()
  {
    if (fileList.size()!=0)
      return;

    string origin = getHomeDir();
    origin+="/.keyCounter";

    if (directory_exists(origin.c_str())<1)
      criticalError("No data to analyze");

    DIR *dir;
    struct dirent *ent;

    dir = opendir (origin.c_str());

    if (dir == NULL) 
      criticalError("Can't open log directory");
  
    while ((ent = readdir (dir)) != NULL) 
      {
	/* Nos devolverá el directorio actual (.) y el anterior (..), como hace ls */
	if ( (strcmp(ent->d_name, ".")!=0) && (strcmp(ent->d_name, "..")!=0) )
	  {
	    fileList.push_back(origin+(string)"/"+ent->d_name);
	  }
      }
    closedir (dir);
  }
};

class GEventRecorder
{
public:
  static GEventRecorder* getInstance()
  {
    if (instance==NULL)
      instance=new GEventRecorder();

    return instance;
  }

  void monitorKey(int action, string keysymStr)
  {
    time_t tstamp = time(NULL);
    if (lastTimestamp+this->maxIdleTime<tstamp)
      {
    	if (lastTimestamp!=0)
    	  intervalLog+="7 Stop typing: "+itoa(lastTimestamp)+"\n";
    	intervalLog+="8 Start typing: "+itoa(tstamp)+"\n";
      }
    keyTimes[keysymStr]++;
    lastTimestamp=tstamp;
    storeData();
  }

private:
  static GEventRecorder *instance;
  time_t lastTimestamp;
  time_t lastStore;
  string logPath;
  map<string, unsigned> keyTimes;
  string intervalLog;
  string currentFile;

  unsigned maxIdleTime;
  unsigned minStoreTime;
  unsigned maxFileSize;

  GEventRecorder()
  {
    short result;

    // Configuration... already manual
    maxIdleTime = DEFAULT_MAX_IDLE_TIME;
    minStoreTime = DEFAULT_MIN_STORE_TIME;
    maxFileSize = DEFAULT_MAX_FILE_SIZE;

    cerr << "Searching path..." << endl;
    logPath = getHomeDir();
    logPath+="/.keyCounter";
    result = directory_exists(logPath.c_str());
    if (result<0)
      criticalError("Error getting log directory");
    else if(result==0)
      {
	if (createDir (logPath.c_str(), 0744)<0)
	  criticalError("Error creating log directory");
      }

    createNewFile();
    lastTimestamp = 0;
    lastStore = time(NULL);
  }

  ~GEventRecorder()
  {
  }

  string keyDebug()
  {
    stringstream ss;
    for (map<string,unsigned>::iterator i=keyTimes.begin(); i!=keyTimes.end(); ++i)
      ss << "1 Press ("<<i->first<<") : "<<i->second<<endl;

    return ss.str();
  }

  void storeData()
  {
    time_t current = time(NULL);
    ofstream of;

    if (lastStore+minStoreTime>current)
      return;

    if (file_size((char*)currentFile.c_str())>maxFileSize)
      createNewFile();

    of.open(currentFile.c_str(), ios::app);
    if (!of.is_open())
      criticalError("Can't open log file");
    of << "9 Save: "<<current<<endl;
    of << intervalLog;
    of << keyDebug();
    of.close();
    lastStore=current;
    intervalLog="";
    keyTimes.clear();
  }

  void createNewFile()
  {
    stringstream ss;
    ofstream of;

    ss<<time(NULL)<<".log";
    currentFile = logPath+"/"+ss.str();

    of.open(currentFile.c_str(), ios::trunc);
    if (!of.is_open())
      criticalError("Failed to create file "+currentFile);

    of.close();
  }

};

GEventRecorder* GEventRecorder::instance=NULL;

void eventCallback(XPointer priv, XRecordInterceptData *d)
{
  Priv *p=(Priv *) priv;
  unsigned int type, detail;
  unsigned char *ud1, type1, detail1;
  GEventRecorder *er = GEventRecorder::getInstance();

  if (d->category!=XRecordFromServer || p->doit==0)
    {
      cerr << "Skipping..." << endl;
    }
  else
    {
      ud1=(unsigned char *)d->data;

      type1=ud1[0]&0x7F; type=type1;
      detail1=ud1[1]; detail=detail1;

      switch (type) 
	{
	case KeyPress:
	  cout << "Press "<<detail<<" ("<<getKeysymStr(p, detail)<<")"<<endl;
	  er->monitorKey(0, getKeysymStr(p, detail));
	  if ( (EXIT_ON_ESCAPE) && (getKeysymStr(p, detail)=="Escape") )
	    p->doit=false;
	  break;
      
	case KeyRelease:
	  // cout << "KeyRelease " << getKeysymStr(p,detail) << endl;
	  break;
	default: 
	  cout <<"Nothing here"<<endl; // Press event sometimes
	}
    }
  XRecordFreeData(d);
}

Display * localDisplay () {

  // open the display
  Display * D = XOpenDisplay ( 0 );

  if ( ! D ) {
    criticalError((string)": could not open display \""+(string)XDisplayName ( 0 )+"\", aborting.");
  }

  // return the display
  return D;
}

void eventLoop (Display * LocalDpy, int LocalScreen,
                                Display * RecDpy) {

  Window       Root, rRoot, rChild;
  XRecordContext rc;
  XRecordRange *rr;
  XRecordClientSpec rcs;
  Priv         priv;
  int rootx, rooty, winx, winy;
  unsigned int mmask;
  Bool ret;
  Status sret;

  // get the root window and set default target
  Root = RootWindow ( LocalDpy, LocalScreen );

  ret=XQueryPointer(LocalDpy, Root, &rRoot, &rChild, &rootx, &rooty, &winx, &winy, &mmask);
  cerr << "XQueryPointer returned: " << ret << endl;
  rr=XRecordAllocRange();
  if (!rr)
  {
        cerr << "Could not alloc record range, aborting." << endl;
        exit(EXIT_FAILURE);
  }
  rr->device_events.first=KeyPress;
  rr->device_events.last=KeyRelease;
  rcs=XRecordAllClients;
  rc=XRecordCreateContext(RecDpy, 0, &rcs, 1, &rr, 1);
  if (!rc)
  {
        cerr << "Could not create a record context, aborting." << endl;
        exit(EXIT_FAILURE);
  }
  priv.x=rootx;
  priv.y=rooty;
  priv.mmoved=1;
  priv.Status2=0;
  priv.Status1=2;
  priv.doit=1;
  priv.LocalDpy=LocalDpy;
  priv.RecDpy=RecDpy;
  priv.rc=rc;

  if (!XRecordEnableContextAsync(RecDpy, rc, eventCallback, (XPointer) &priv))
  {
        cerr << "Could not enable the record context, aborting." << endl;
        exit(EXIT_FAILURE);
  }

  while ((priv.doit) && (!Exit_signal) ) 
    {
      XRecordProcessReplies(RecDpy);
      usleep(1000);
    }

  sret=XRecordDisableContext(LocalDpy, rc);
  if (!sret) cerr << "XRecordDisableContext failed!" << endl;
  sret=XRecordFreeContext(LocalDpy, rc);
  if (!sret) cerr << "XRecordFreeContext failed!" << endl;
  XFree(rr);
}


void do_exit(int s)
{
  Exit_signal = 1;
}

void captureKeys()
{
  int Major, Minor;

  signal (SIGINT, do_exit);

  // open the local display twice
  Display * LocalDpy = localDisplay ();
  Display * RecDpy = localDisplay ();

  // get the screens too
  int LocalScreen  = DefaultScreen ( LocalDpy );

  if ( ! XRecordQueryVersion (RecDpy, &Major, &Minor ) ) 
    {
      // nope, extension not supported
      XCloseDisplay ( RecDpy );
      criticalError((string)"XRecord extension not supported on server \"" + (string) DisplayString(RecDpy) + (string)"\"");
    }

  // print some information
  cerr << "XRecord for server \"" << DisplayString(RecDpy) << "\" is version "
           << Major << "." << Minor << "." << endl << endl;;

  eventLoop ( LocalDpy, LocalScreen, RecDpy);

  cerr << "Exiting... " << endl;
  // Mirar si tengo que grabar algo mas
  XCloseDisplay ( LocalDpy );
}

void analyzeData(int argc, char *argv[])
{
  KCAnalyzer analyzer;

  if (argc>2)
    {
      if ( (string)argv[2]=="keycount")
	cout << analyzer.keycount() << endl;
      else if ( (string)argv[2]=="burst")
	cout << analyzer.burst() << endl;
      else if ( (string)argv[2]=="hourly")
	cout << analyzer.hourlyLog() << endl;
    }
  else
    {
      cerr << "Plase try to analyze with these options: "<<endl;
      cerr << "   "<<argv[0]<<" analyze keycount - To check wich are the most used keys"<<endl;
      cerr << "   "<<argv[0]<<" analyze burst - To check typing pauses"<<endl;
      cerr << "   "<<argv[0]<<" analyze hourly - To check hourly stats"<<endl;
    }
}

int main(int argc, char *argv[])
{
  if (argc>1)
    {
      if ( (string)argv[1]=="analyze" )
	analyzeData(argc, argv);
      else
	criticalError("Unrecognised command, try 'analyze' or no command");
    }
  else
    captureKeys();

  return EXIT_SUCCESS;
}

