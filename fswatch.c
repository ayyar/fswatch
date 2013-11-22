#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <CoreServices/CoreServices.h> 

/* fswatch.c
 * 
 * usage: ./fswatch /some/directory[:/some/otherdirectory:...] "some command" 
 * "some command" is eval'd by bash when /some/directory generates any file events
 *
 * compile me with something like: gcc fswatch.c -framework CoreServices -o fswatch
 *
 * adapted from the FSEvents api PDF
*/

extern char **environ;
//the command to run
char *to_run;

int endsWith (char* base, char* str) {
  int blen = strlen(base);
  int slen = strlen(str);
  return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
}

//fork a process when there's any change in watch file
void callback( 
    ConstFSEventStreamRef streamRef, 
    void *clientCallBackInfo, 
    size_t numEvents, 
    void *eventPaths, 
    const FSEventStreamEventFlags eventFlags[], 
    const FSEventStreamEventId eventIds[]) 
{ 
  pid_t pid;
  int   status;

  int run_cmd = 0;
 
  /*printf("Callback called\n"); */
  for (int i=0; i<numEvents; ++i) {
    char *path = ((char **)eventPaths)[i];

    if (endsWith(path, ".py") ||
	endsWith(path, ".js") ||
	endsWith(path, ".gs") ||
	endsWith(path, ".html")) {
      run_cmd = 1;
    }
    printf("%x %s\n", eventFlags[i], path);
    fflush(stdout);
  }

  if (!run_cmd) {
    return;
  }

  if((pid = fork()) < 0) {
    fprintf(stderr, "error: couldn't fork \n");
    exit(1);
  } else if (pid == 0) {
    char *args[4] = {
      "/bin/bash",
      "-c",
      to_run,
      0
    };
    if(execve(args[0], args, environ) < 0) {
      fprintf(stderr, "error: error executing\n");
      exit(1);
    }
  } else {
    while(wait(&status) != pid)
      ;
  }
} 
 
//set up fsevents and callback
int main(int argc, char **argv) {

  if(argc != 3) {
    fprintf(stderr, "You must specify a directory to watch and a command to execute on change\n");
    exit(1);
  }

  to_run = argv[2];

  CFStringRef mypath = CFStringCreateWithCString(NULL, argv[1], kCFStringEncodingUTF8); 
  CFArrayRef pathsToWatch = CFStringCreateArrayBySeparatingStrings (NULL, mypath, CFSTR(":"));

  void *callbackInfo = NULL; 
  FSEventStreamRef stream; 
  CFAbsoluteTime latency = 1.0;

  stream = FSEventStreamCreate(NULL,
    &callback,
    callbackInfo,
    pathsToWatch,
    kFSEventStreamEventIdSinceNow,
    latency,
    //kFSEventStreamCreateFlagNone
    kFSEventStreamCreateFlagFileEvents
  ); 

  FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode); 
  FSEventStreamStart(stream);
  CFRunLoopRun();

}
