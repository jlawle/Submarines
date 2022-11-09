/*
John Lawler
ECE 2220
April 15th 2019

This is program 7. The purpose of this program is to use console window buffers to
view and track multiple o pe nconsole windows at one time. The console windows will act as
submarines and will be assigned values at random using fork(), getpid(), and wait() commands

signals from each submarine will be handled by functions signal(), alarm(), and kill()
*/

//------------------------------------------------------------------------------

typedef void(*handler);

#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

enum ExitCodes {EXIT_CRASHED, EXIT_SUCCEED, EXITS};

struct subGear {
  int fuel, distance, missles;
} subs[4];

int initialFuel[4];
FILE *termPtr[10];
int PID[10], childPID, curProcess, processes;


//------------------------------------------------------------------------------

void printendtime();
void parentWaits();
void watchSubs();
void fuel();
void missles();
void subAlarm();
void ChildProcess();
void forkstuff(int termCount);
void randomizeFuels();
int randomize(int lower, int upper);
int findBases(int *terminal, int termCount);

//------------------------------------------------------------------------------

int main(){
  time_t t;
  time(&t);
  int terminal[10] = { 0 };
  int terminals = 0;
  int *term = terminal;
  terminals = findBases(term, terminals);
  randomizeFuels();
  forkstuff(terminals);
  printendtime();
  return 0;
}

//------------------------------------------------------------------------------
 /* print the final time */
void printendtime(){
  time_t t;
  time(&t);
  printf("\nEnd of Mission! Time: %s\n", ctime(&t));
}

//------------------------------------------------------------------------------

/* this function is used to retrieve the process ID of the last child that exited,
    and then sets the bits 8 over to retrieve the exit byte in the ID.
    if the exit byte is 1, then mission success, otherwise failure */
void parentWaits(){
  char exit;
  int i, child_status[10], wait_ret[10];
  wait_ret[1] = wait(&child_status[1]);
  wait_ret[2] = wait(&child_status[2]);
  wait_ret[3] = wait(&child_status[3]);

  //loops through for each process
  for(i = 1; i < 4; i++){
    exit = child_status[i] >> 8;
    if(exit == 1){
      printf("Submarine %d mission is successful!\n", wait_ret[i]%10);
    } else {
      printf("Submarine %d mission is a failure.\n", wait_ret[i]%10);
    }
  }
}

//------------------------------------------------------------------------------

/* this function is used as a command input from the base terminal to communicate
    with the child subs. it refuels, launches their missles, and kills them */
void watchSubs(void){
  char userCMD[10];
  int processNum;
  processes = 3;
  //loop while there are children still active
  while(processes > 0){
    scanf("%s", userCMD);
    fflush(stdin);

    //if the user inputs a q, kill all child processes
    if(strlen(userCMD) == 1){
      if(userCMD[0] == 'q'){
        for (processNum = 1; processNum < 4; processNum++){
          kill(PID[processNum], 1);
          processes--;
        }
      }
    } else if(strlen(userCMD) == 2){
      processNum = atoi(&userCMD[1]);
      if((processNum <= 0) || (processNum >= 4)){
        printf("Not a valid submarine number. \n");
      } else {
        //switch statement to choose what to do with inputs of s, l, or r for
        //refueling, termination, and launching missles
        switch(userCMD[0]){
          case 's':
            //terminates the indicated sub
            kill(PID[processNum], 1);
            printf("Submarine %d has been terminated. \n", processNum);
            processes--;
            break;
          case 'r':
            //sends SIGUSR2 to the process indicated by user and refuels sub
            kill(PID[processNum], SIGUSR2);
            break;
          case 'l':
            //sends the signal SIGUSR1 and the function it calls (i.e. launch missiles)
            // when the user inputs a l# command
            kill(PID[processNum], SIGUSR1);
            break;
        }
      }
    }
  } // end of while (process > 0) loop
}

//------------------------------------------------------------------------------

/* this function refuels the submarines using the intial fuel set in randomizeFuels()
    */
void fuel(int signum){
  subs[curProcess].fuel = initialFuel[curProcess];
  fprintf(termPtr[curProcess], "Submarine %d to command: Thanks for Fuel!\n", curProcess);
}

//------------------------------------------------------------------------------

/* this function is called everytime the signal SIGUSR2 is called, and sends missles */
void missles(int signum){
  if (subs[curProcess].missles > 0){
    fprintf(termPtr[curProcess], "Submarine %d launching missile.\n",curProcess);
    if(--subs[curProcess].missles == 0){
      fprintf(termPtr[curProcess], "Out of missles - Submarine %d returning to base",
                curProcess);
    }
  } else {
    fprintf(termPtr[curProcess], "Submarine %d has no missles left.\n",curProcess);
  }

}

//------------------------------------------------------------------------------

/* the prupose or this function is to decrement fuel every second, report them
    time and info of each sub to the child process, and adjust the distance of each
    submarine from the base */
void subAlarm(int signum){
  time_t t;
  time(&t);
  static int info, distance, fuel;

  subs[curProcess].fuel -= randomize(100, 200);
  //set fuel to zero if it goes out of bounds
  if(subs[curProcess].fuel < 0 ){ subs[curProcess].fuel = 0; }
  //wait for the subalarm to be called 3 times (3 seconds) to display info to child
  if(++info == 3){
    info = 0;
    fprintf(termPtr[curProcess], "\nTime: %s", ctime(&t));
    fprintf(termPtr[curProcess], "Sub %d to base: %d gallons left, %d missles left, and %d miles from base.\n",
    curProcess, subs[curProcess].fuel, subs[curProcess].missles, subs[curProcess].distance);
  }
  //decrement fuel by random within a range of 100 to 200 gallons
  if(subs[curProcess].fuel == 0){
    fprintf(termPtr[curProcess],"Submarine %d dead in the water.\n", curProcess);
    printf("Rescue is on the way, sub %d.\n",curProcess);
    exit(EXIT_CRASHED);
  } else if(subs[curProcess].fuel < 500){
      fprintf(termPtr[curProcess],"Sub %d is running out of fuel!\n", curProcess);
  }

  //change the distance of the sub, increment if going to target, decrement if
  //going back to base
  if(++distance == 2){
    distance = 0;
      //if sub is headed towards target (has bombs) increment distance
      if(subs[curProcess].missles != 0){
        subs[curProcess].distance += randomize(5, 10);
      } else {
        //if sub is out of bombs decrement the distance from the target
        subs[curProcess].distance -= randomize(3, 8);
      }
    }
  //set sub distance to 0 if goes out of bounds
  if(subs[curProcess].distance < 0){ subs[curProcess].distance = 0; }

  //checks to see if mission success: i.e. when each subs has 0 missles and is at base
  if(subs[curProcess].distance == 0 && subs[curProcess].missles == 0){
    fprintf(termPtr[curProcess], "Sub %d Mission Success.\n", curProcess);
    exit(EXIT_SUCCEED);
  }

  //set the alarm to be called every second
  alarm(1);
}

//------------------------------------------------------------------------------

/* this function intializes the signals to the functions they will be using when
    the signal is called either by the user input or by the child processes */
void ChildProcess(){
  childPID = getpid();
  signal(SIGALRM, (handler)subAlarm);
  signal(SIGUSR1, (handler)missles);
  signal(SIGUSR2, (handler)fuel);
  //initially calls alarm
  alarm(1);
  while(1){}
}

//------------------------------------------------------------------------------
/* this function is used to create the child processes in the terminals and
    keep track of their processID in the PID array */
void forkstuff(int termCount){
  int i, process, parent = 1;
  curProcess = 0;
  PID[curProcess] = getpid();

  //intialize the 3 child processes and fork them
  for(process = 1; process <= 3; process++){
    if(parent == 1){
      PID[curProcess = process] = fork();
      if(PID[process] == 0){
        parent = 0;
        printf("Child %d is online. \n", process);
        ChildProcess();
      }
    }
  }

  //go to parent and use to take input from base terminal
  if(parent == 1){
    PID[4] = fork();
    if(PID[4] == 0){
      //call a function to observe the parent input for talking to submarines
      watchSubs();
    } else {
      //calls the function end the program and let all child processes to sync
      parentWaits();
      kill(PID[4], 1);
      //close the terminals after finished using them
      for(i = 0; i < termCount; i++){
        fclose(termPtr[i]);
      }
    }
  }
}

//------------------------------------------------------------------------------

/* this function sets the randomized fuel and missles for the submarines */
void randomizeFuels(){
  int i;
  for(i = 1; i <= 3; i++){
    subs[i].fuel = randomize(1000, 5000);
    subs[i].missles = randomize(6, 10);
    subs[i].distance = 0;
    initialFuel[i] = subs[i].fuel; //store the intial values
    }
}
//------------------------------------------------------------------------------

//function to randomly set random numbers for fuel and etc
int randomize(int lower, int upper){
  srand(time(NULL));
  int num;
  num = (rand() % (upper - lower + 1)) + lower;
  return num;
}
//------------------------------------------------------------------------------

/* this function is used to first find the terminals and open them in a file pointer
  array to be used elsewhere in the program */
int findBases(int *terminal, int termCount){
  int i;
  char buffer[100];

  //loop through 100 or so terminal numbers to check every possible terminal
  for(i = 0; i < 100; i++){
    //save the directory name to access the terminals into a char buffer
    sprintf(buffer, "/dev/pts/%d", i);
    if ((termPtr[termCount] = fopen(buffer, "w")) != NULL){
      terminal[termCount] = i;
      termCount++;
    } else {
      printf("Unable to open %s\n", buffer);
    }
  }
  for(i = 0; i < termCount; i++){
    printf("Terminal[%d] = %d\n", i, terminal[i]);
  }

  return termCount;
}

//------------------------------------------------------------------------------
