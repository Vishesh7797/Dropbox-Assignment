//vxg5100   
//1001455100
//11/15/2019



// The MIT License (MIT)
//
// Copyright (c) 2016, 2017 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                               // so we need to define what delimits our tokens.
                               // In this case  white space
                               // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size
#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

#define NUM_BLOCKS 4226
#define BLOCK_SIZE 8192
#define NUM_FILES  128
#define MAX_FILE_SIZE 10240000

FILE * fd; 

uint8_t blocks[NUM_BLOCKS][BLOCK_SIZE];

struct Directory_Entry{
  uint8_t valid; 
  char filename[32];
  uint32_t inode;
};

struct Inode{
  uint8_t valid; 
  uint8_t attributes; 
  uint32_t size; 
  uint32_t blocks[1250];
};

struct Directory_Entry *dir; 
struct Inode *inodes;
uint8_t *freeBlockList; 
uint8_t *freeInodeList; 

void initializeInodes(){
  int i; 
  for (i = 0; i < NUM_FILES; i++){
    inodes[i].valid    = 0; 
    inodes[i].size     = 0; 
    inodes[i].attributes = 0;
    int j; 
    for ( j = 0; j < 1250; j++){
      inodes[i].blocks[j] = -1;
    }

  }
} 

void initializeBlockList(){
  int i; 
  for (i = 0; i < NUM_BLOCKS; i++){
    freeBlockList[i] = 1; 
  }
} 

void initializeInodeList(){
  int i; 
  for (i = 0; i <NUM_BLOCKS; i++){
    freeInodeList[i] = 1; 
  }
}

void initializeDirectory(){
  int i; 
  for (i = 0; i < NUM_FILES; i++){
    dir[i].valid = 0; 
    dir[i].inode = -1;

    memset( dir[i].filename, 0, 32);
  }
}

int df(){
  int i; 
  int free_space = 0;
  for (i = 0; i <NUM_BLOCKS; i++){
    if (freeBlockList[i])
      free_space+= BLOCK_SIZE;
  }
  return free_space;
}

int findDirectoryEntry(char *filename){
  int i; 
  int ret = -1; 
  for (i = 0; i <NUM_FILES; i++){
    if ( strcmp(filename, dir[i].filename) == 0)
      return i; 
  }

  for (i = 0; i < NUM_FILES; i++){
    if (dir[i].valid == 0){
      dir[i].valid = 1;
      return i; 
    }
  }
  return ret; 
}

int findFreeInode(){
  int z; 
  int ret = -1; 
  for (z = 0; z < NUM_FILES; z++){
    if (!inodes[z].valid){
      inodes[z].valid = 1;
      return z; 
    }
  }
  return ret; 

}

int findFreeBlock(){
  int i; 
  int ret = -1; 
  for (i = 10; i < NUM_BLOCKS; i++){
    if (freeBlockList[i]){
      freeBlockList[i] = 0; 
      return i; 
    }
  }
  return ret; 
}


int put( char * filename ){

  struct stat buf; 
  int ret; 
  ret = stat(filename, &buf);
  int size = buf.st_size; 
  if (ret == -1){
    printf("File does not exist\n");
    return -1; 
  }


  if ( size > MAX_FILE_SIZE ) 
  {
    printf("File size too big\n");
    return -1;
  }

  if ( size > df() ) 
  {
    printf("File exceeds remaining disk space\n");
    return -1;
  }
  int directoryIndex = findDirectoryEntry(filename);
  int inodeIndex = findFreeInode();
  int fblock = findFreeBlock();


  memcpy(dir[directoryIndex].filename, filename, strlen(filename));

  int offset = 0;

  fd = fopen(filename, "r");
  printf("Reading %d bytes from %s\n", size, filename );
  while (size > 0)
  {
    fseek(fd, offset,  SEEK_SET );

    int bytes = fread(blocks[fblock], BLOCK_SIZE, 1, fd);

    if ( bytes == 0 && !feof(fd) )
    {
      printf("\nAn error occured\n");
      return -1;
    }
    clearerr(fd); 
    size -= BLOCK_SIZE;  
    offset += BLOCK_SIZE; 
    fblock++ ;
  }

  fclose(fd);
  return 0;
}



int get (char *filename, char *newfilename)
{
 struct stat buf;
 int status;
 status = stat(filename, &buf ); 
 int size = buf.st_size;          

 int directoryIndex = findDirectoryEntry(newfilename);
 int inodeIndex = findFreeInode();
 int fblock = findFreeBlock();
 int block_index = 0;
 if (status == -1) {   
  printf("File %s does not exist\n", filename);
  return -1;
 }

 if (fd == NULL){
   printf("Could not open file: %s\n", newfilename);
   return -1;
 }

 int i;
 for (i = 0; i < NUM_FILES; i++){
   if ( strcmp(filename, dir[i].filename) == 0){
     int offset = 0;

     fd = fopen(newfilename, "w");
     printf("Writing %d bytes from %s to %s\n", size, filename, newfilename);
     while ( size > 0){
       int num_bytes;
       if (size < BLOCK_SIZE){
         num_bytes = size;
       }
       else{
         num_bytes = BLOCK_SIZE;
       }
       fwrite(blocks[block_index], num_bytes, 1, fd);

       size -= BLOCK_SIZE;
       offset += BLOCK_SIZE;
       block_index++; 

       fseek(fd, offset, SEEK_SET);
     }

     fclose(fd);

     return 0;
   }
 }

 printf("error: File not found\n");
 return -1;
}

void createfs(char *filename){
  fd = fopen(filename, "w");
  memset( &blocks[0], 0, NUM_BLOCKS *BLOCK_SIZE);
  initializeDirectory(); 
  initializeBlockList();
  initializeInodeList();
  initializeInodes();
  fwrite( &blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);
  fclose(fd);
}

int attrib(char *attrib, char *filename){
  struct stat buf;
  int status;
  status = stat(filename, &buf ); 

  if (status == -1){
    printf("File not found\n");
    return -1;
  }

  if (!strcmp(attrib, "+h")){
    inodes[status].attributes += 10;
    return 1; 
  }

  if (!strcmp(attrib, "-h")){
    if (inodes[status].attributes == 10){
      inodes[status].attributes -= 10;  
    }
    return 1;
  }

  if (!strcmp(attrib, "+r")){
    inodes[status].attributes += 20;
    return 1;
  }

  if (!strcmp(attrib, "-r")){
    if (inodes[status].attributes == 20){
    inodes[status].attributes -= 20; 
    } 
    return 1;
  }
  return 1; 
}

int open(char * filename)
{
  fd = fopen(filename, "r");
  if (fd == NULL){
    perror("File not found ");
    return -1;
  }
  else{
    fread(&blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);
  }
  return 0;
}



int close(char * filename){
  if(filename == NULL){
    return -1;
  }

  fd = fopen(filename, "w");
  if (fd == NULL){
    perror("Error opening file ");
    return -1;
  }
  else{
    fwrite(&blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);
    fclose(fd);
    fd = NULL;
    return 0;
  }

}


int list()
{
  
 
  if (dir->valid == 0) 
  {
    printf("No files found\n");
    return -1;
  }
  int i; 
  for ( i = 0; i < NUM_FILES; i++)
  {
    if (dir[i].valid == 1)
    {
      struct stat buf; 
      int ret; 
      ret = stat(dir[i].filename, &buf);
      int size = buf.st_size; 
      time_t val = buf.st_mtime;
      int inodeIndex = dir[i].inode;
      if (size != 0 && (inodes[ret].attributes == 0 || inodes[ret].attributes == 20)) {
        printf(" %s %d %s \n", dir[i].filename, size, asctime(localtime(&buf.st_mtime)));
      }

      if (size == 0){
        printf("%s has been deleted \n", dir[i].filename);
      }
  }
  }
  return 0;
}




int main(int argc, char *argv[])
{
  dir = (struct Directory_Entry *) &blocks[0];
  inodes = (struct Inode *)&blocks[3];
  freeInodeList = (uint8_t *)&blocks[1];
  freeBlockList = (uint8_t *)&blocks[2];

  char *cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
  int h = 0;
  int r = 0;
  char hidden[NUM_FILES+1][MAX_COMMAND_SIZE];
  char read_only[NUM_FILES+1][MAX_COMMAND_SIZE];

  initializeDirectory();
  initializeBlockList();
  initializeInodeList();
  initializeInodes();


 while( 1 )
 {
   // Print out the msh prompt
   printf ("msh> ");

   // Read the command from the commandline.  The
   // maximum command that will be read is MAX_COMMAND_SIZE
   // This while command will wait here until the user
   // inputs something since fgets returns NULL when there
   // is no input
   while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

   /* Parse input */
   char *token[MAX_NUM_ARGUMENTS];

   int   token_count = 0;

   // Pointer to point to the token
   // parsed by strsep
   char *arg_ptr;

   // save a copy of the command line since strsep will end up
   // moving the pointer head
   char *working_str  = strdup( cmd_str );

   // we are going to move the working_str pointer so
   // keep track of its original value so we can deallocate
   // the correct amount at the end
   char *working_root = working_str;

   // Tokenize the input stringswith whitespace used as the delimiter
   while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
             (token_count<MAX_NUM_ARGUMENTS))
   {
     token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
     if( strlen( token[token_count] ) == 0 )
     {
       token[token_count] = NULL;
     }
       token_count++;
   }
     // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    // int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // printf("token[%d] = %s\n", token_index, token[token_index] ); 

   if (token[0] != NULL)
   {
     if( !strcmp(token[0], "quit") || !strcmp(token[0], "Quit") || !strcmp(token[0], "Exit") || !strcmp(token[0], "exit") || !strcmp(token[0], "Q") || !strcmp(token[0], "q")){
      return 0;
    }
     else if( !strcmp(token[0], "put") || !strcmp(token[0], "Put")){
      put(token[1]);
    }
  
     else if( !strcmp(token[0], "get") || !strcmp(token[0], "Get")){
      if (token[2] == NULL){
        get(token[1], token[1]);
      }
      else{
        get(token[1],token[2]);
      }
    }
     else if (!strcmp (token[0], "del")){
      struct stat xyz; 
      int retxyz; 
      retxyz = stat(token[1], &xyz);
      if (inodes[retxyz].attributes == 20 || inodes[retxyz].attributes == 30){
        printf("File is read-only, can not be deleted \n");
      }
      else{

        int try; 
        try = remove(token[1]);
           if(try == 0) {
              printf("File deleted successfully \n");
              struct stat del; 
              int deleted; 
              deleted = stat(token[1], &del);
              int size_del = del.st_size; 
              size_del -= BLOCK_SIZE ; 
            }
            else{
              printf("Error: unable to delete the file \n");
            }
     }
   }
     else if (!strcmp (token[0], "list")) {
      list();
      //del(token[1]);
    }
     else if (!strcmp (token[0], "df") || !strcmp(token[0], "DF")) {
      printf("Disk Space remaining %d bytes\n", df());
    }
     else if (!strcmp (token[0], "open")){
        fd = fopen(token[1], "r");
        if (fd == NULL){
            perror("File not found ");
          }
        else{
            fread(&blocks[0], NUM_BLOCKS, BLOCK_SIZE, fd);
          }
     }
     else if (!(strcmp(token[0], "close"))){
       if(token[1] == NULL){
         printf("close: File not found. \n");
       }
       close(token[1]);
     }
     else if (!(strcmp("attrib", token[0]))){
       attrib(token[1], token[2]);
     }
     else if (!(strcmp("createfs", token[0]))){
       if (token[1] == 0){
         printf("createfs: File not found. \n");
       }
       else{
         createfs(token[1]);
         printf("Created new disk image called %s\n", token[1]);
       }
     }
     else{
       printf("Command not recognized. Try again. \n");
     }

   }
   free( working_root );
 }
 return 0; 
}
