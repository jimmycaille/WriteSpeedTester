/******************************************************
*
* Tool to test filesystem writing speeds
*
* Author  : Caille Jimmy
*
* Compile : gcc -Wall -Wextra -o write_files write_files.c
* Run     : ./write_files -h
*
*      [param] [remark]            [default value]
* Usage: -h    help, this message  -none-
*        -f    folder              ./
*        -s    small files size    1024
*        -S    small files amount  1000
*        -b    big files size      1024000
*        -B    big files amount    1
*        -m    run multiple times  1
*        -p    cpu/task priority   0
*        -d    shows deleted files disabled
*
* Version : v0.3 - possibility to run multiple times (-m)
*                  with min/max/avg calcuation in sec/MBps
*           v0.2 - deletes generated files and subfolder
*           v0.1 - create subfolder for the test files
*
******************************************************/

#define _GNU_SOURCE
#define STR_MAX_LEN 258
#define SUB_FOLDER "TEST_FOLDER"
#define BPS_UNIT "MBps"
#define BPS_RATIO 1000000

#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>   //opendir()
#include <sys/stat.h> //mkdir()                  ### TODO ADAPT FOR WINDOWS ###
#include <stdbool.h>  //boolean
#include <float.h>    //FLT_MIN and FLT_MAX

#include "write_files.h"

bool show_deletion=false;

int main (int argc, char* argv[]){
  int       ret;
  cpu_set_t set;
  struct sched_param sp = {.sched_priority = 50, };
  char   subf[STR_MAX_LEN];

  //default values
  char folder[STR_MAX_LEN]="./";  //path to write files to
  int cpu     = 0;       //cpu and task priority
  int sfile_s = 1024;    //small file size
  int sfile_a = 1000;    //small files amount
  int bfile_s = 1024000; //big file size
  int bfile_a = 1;       //big file amount
  int multiple= 1;       //number of tests       ### TODO IMPLEMENT MULTIPLE TESTS AND AVERAGES ###
  //options
  int opt;
  
  //parsing arguments
  //info: start with ':' to distinguish between '?' and ':' 
  while((opt = getopt(argc, argv, ":hdf:s:S:b:B:m:p:")) != -1){  
    switch(opt){
      case 'f':
        strncpy(folder,optarg,STR_MAX_LEN);
      break;
      case 's':
        sfile_s = atoi(optarg);
      break;
      case 'S':
        sfile_a = atoi(optarg);
      break;
      case 'b':
        bfile_s = atoi(optarg);
      break;
      case 'B':
        bfile_a = atoi(optarg);
      break;
      case 'm':
        multiple = atoi(optarg);
      break;
      case 'p':
        cpu = atoi(optarg);
      break;
      case 'd':
        show_deletion = true;
      break;
      case ':':  
        printf("option needs a value\n");  
      break;
      case '?':
        printf("unknown option: %c\n", optopt);
      case 'h':
        printf("      [param] [remark]            [default value] \n");
        printf("Usage:  -h    help, this message  -none-\n");
        printf("        -f    folder              %s\n",folder);
        printf("        -s    small files size    %d\n",sfile_s);
        printf("        -S    small files amount  %d\n",sfile_a);
        printf("        -b    big files size      %d\n",bfile_s);
        printf("        -B    big files amount    %d\n",bfile_a);
        printf("        -m    run multiple times  %d\n",multiple);
        printf("        -p    cpu/task priority   %d\n",cpu);
        printf("        -d    shows deleted files disabled\n");
        exit(EXIT_SUCCESS);
      break;
    }  
  }
  //add final '/' if not present
  if(folder[strlen(folder)-1]!='/'){
    strcat(folder,"/");
  }
  snprintf(subf,STR_MAX_LEN,"%s%s",folder,SUB_FOLDER); //create subfolder path
  
  printf("Path                    : %s\n",folder);
  printf("Small files size (Bytes): %d\n",sfile_s);
  printf("Small files amount      : %d\n",sfile_a);
  printf("Big files size (Bytes)  : %d\n",bfile_s);
  printf("Big files amount        : %d\n",bfile_a);
  printf("Number of runs          : %d\n",multiple);
  printf("CPU/Task priority       : %d\n",bfile_a);
  printf("Show deleted files      : %s\n",show_deletion ? "enabled" : "disabled");
  
  CPU_ZERO (&set);
  CPU_SET (cpu, &set);
  ret=sched_setaffinity (0, sizeof(set), &set);
  printf("Set affinity            : %d\n", ret);

  ret=sched_setscheduler (0, SCHED_FIFO, &sp);
  printf("Set scheduler           : %d\n", ret);
  
  printf("\nCreating subfolder...");
  createFolder(folder, SUB_FOLDER);

  printf("\nWriting small files...\n");
  writeFiles(sfile_a,sfile_s,subf,"small_",multiple);
  
  printf("\nWriting big   files...\n");
  writeFiles(bfile_a,bfile_s,subf,"big_",multiple);

  cleanFolder(subf);
  
  return (0);
}
//test if folder exists and create a subfolder to write files
static int createFolder(char* root, char* sub){
  char subf[STR_MAX_LEN];
  DIR* dir = opendir(root);
  
  //test if root folder exists
  if(!dir){
    printf("Error when opening folder %s (Does it exists ?)\nExiting...\n",root);
    exit(EXIT_FAILURE);
  }else{
    closedir(dir);
  }
  //create subfolder
  snprintf(subf,STR_MAX_LEN,"%s%s",root,sub);
  dir = opendir(subf);
  if(dir){
    printf(" %s already existing !\nExiting...\n",subf);
    closedir(dir);
    exit(EXIT_FAILURE);
  }else{
    if(mkdir(subf,0700)){
      printf(" error creating folder %s\nExiting...\n",subf);
      exit(EXIT_FAILURE);
    }else{
      printf("success !\n");
    }
  }
  return 0;
}
//remove all files from a folder before deleting it
static int cleanFolder(char* folder){
  DIR* dir;
  struct dirent *d;
  char filename[STR_MAX_LEN];
  char input;
  bool all_f_removed;
  
  printf("\nDo you want to delete the folder %s (and its files) ? [y/n] :",folder);
  scanf(" %c",&input);
  if(input == 'y' || input == 'Y'){
    dir = opendir(folder);
    if(dir){
      if(show_deletion) printf("Files are :\n");
      d = readdir(dir);//skips .
      d = readdir(dir);//skips ..
      all_f_removed=true;
      while ((d = readdir(dir)) != NULL){
        snprintf(filename,STR_MAX_LEN,"%s/%s",folder,d->d_name);
        if(show_deletion) printf("%s... ", filename);
        if(remove(filename)){
           if(show_deletion) printf("failed...\n");
          all_f_removed=false;
        }else{
           if(show_deletion) printf("removed !\n");
        }
      }
      if(all_f_removed){
        printf("%s... ",folder);
        if(!rmdir(folder)){
          printf("removed !\n");
        }else{
          printf("failed...\n");
        }
      }else{
        printf("\nError while deleting one or more file, delete folder manually...");
      }
    }else{
      printf("Folder not found...\n");
    }
  }else{
    printf("Ignoring...\n");
  }
  return 0;
}

//write nb files of size
static int writeFiles(int nb, int size, char* folder, char* filePrefix, int multiple){
  struct timespec tStart, tEnd;
  FILE * fp;
  char filename[STR_MAX_LEN];
  float sec;
  float mbps;
  float s_min=FLT_MAX,s_max=FLT_MIN,s_tot=0;
  
  char* buffer = (char*)calloc(size,sizeof(char)); //buffer for one file
  if(buffer == NULL){
    printf("Error while allocating writing buffer\nExiting...\n");
    exit(EXIT_FAILURE);
  }
  
  for(int i=0;i<multiple;i++){
    printf("Round %d...",i);

    //create each file
    clock_gettime(CLOCK_REALTIME, &tStart);	
    for(int i=0;i<nb;i++){
      snprintf(filename,STR_MAX_LEN,"%s/%s%d",folder,filePrefix,i);
      fp = fopen(filename,"wb");
      if(fp != NULL){
        fwrite(buffer,size,sizeof(char),fp);
        fclose (fp);
      }else{
        printf("Error when opening file %s (Do you have write rights ?)\nExiting...\n",filename);
        exit(EXIT_FAILURE);
      }
    }
    clock_gettime(CLOCK_REALTIME, &tEnd);
    
    sec = diff(tStart, tEnd);
    mbps = size*nb/sec/BPS_RATIO;
    printf("DONE in %f second(s), Estimated speed is %.2f %s\n",sec,mbps,BPS_UNIT);
    s_tot += sec;
    if(sec>s_max) s_max=sec;
    if(sec<s_min) s_min=sec;
  }
    
  printf("MIN/MAX/AVG : %.2f/%.2f/%.2f [s] %.2f/%.2f/%.2f [%s]\n",s_min,
                                                                    s_max,
                                                                    s_tot/multiple,
                                                                    size*nb/s_max/BPS_RATIO,
                                                                    size*nb/s_min/BPS_RATIO,
                                                                    size*nb/(s_tot/multiple)/BPS_RATIO,
                                                                    BPS_UNIT);
  
  return 0;
}

//return the number of seconds between two timestamps
static double diff(struct timespec start, struct timespec end){
  double t1, t2;

  t1 = (double)start.tv_sec;
  t1 = t1 + ((double)start.tv_nsec)/1000000000.0;
  t2 = (double)end.tv_sec;
  t2 = t2 + ((double)end.tv_nsec)/1000000000.0;

  return (t2-t1);
}