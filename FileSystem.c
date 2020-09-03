/* Author: Marshall Tanis
 * 
 * CSCI 460 Assignment 3
 * 
 * This program implements a FAT file system and maintains information needed to write and retrieve data using provided disk drivers.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Driver.h"
#include "FileSysAPI.h"
/* Fat entry and directory entry structures */
typedef struct fatEntry entry;
typedef struct dirEnt dir;
struct fatEntry{
    int nextFat;
};
struct dirEnt{
    char *FileName;
    int fatIx;
    int fileSize;
    dir *nextDir;
};


/* FAT Table, root directory and free list */
entry *fat[SECTORS] ={0};
dir *root[64]={NULL};
int Free[SECTORS] = {0};

/* prototypes */
int FileExists(char *FileName);
void insertIntoRoot(dir *entry);
int hash(char *FileName);
int howManyBlocks(int Size);
int findNextFat();
char **breakUpData(char *Data, int numBlocks);
/* Creates file system*/
int CSCI460_Format(){
    int works = DevFormat();
    if(works){
        fat[0] = (entry *)malloc(sizeof(entry));
        if(fat[0] < 0){
            perror("malloc");
            return 0;
        }
        fat[1] = (entry *)malloc(sizeof(entry));
        if(fat[1] < 0){
            perror("malloc");
            return 0;
        }
        return 1;
    }
    else{
        return 0;
    }
}
/* Writes Data to FileName with no more than Size characters. First checks if file is found, if so, deletes file and writes Data. Adds which blocks it uses
 * to FAT table and takes away from free list.
 */
int CSCI460_Write (	char *FileName, int Size, char *Data){
    if(fat[0] == NULL && fat[1] == NULL){
        printf("File system hasn't been created yet\n");
        return 0;
    }
    if(FileExists(FileName)){
        CSCI460_Delete(FileName);
    }
    char *newString = (char *) malloc(sizeof(char) * Size + 1);
    char *file = (char *) malloc(sizeof(char) * strlen(FileName) + 1);
    snprintf(file, strlen(FileName) + 1, "%s", FileName);
    snprintf(newString, Size + 1, "%s", Data);
    dir *current = (dir *) malloc (sizeof(dir));
    if(current < 0){
        perror("malloc");
        return 0;
    }
    int numBlocks = howManyBlocks(Size);
    if(numBlocks > 1){
        char **newData = breakUpData(Data, numBlocks);
        current->FileName = file;
        current->fileSize = Size;
        int newFat = findNextFat();
        if(newFat < 0){
            return 0;
        }
        current->fatIx = newFat;
        int ix = hash(current->FileName);
        insertIntoRoot(current);
        for(int i = 0; i < numBlocks; i ++){
            entry *currFat = (entry *)malloc(sizeof(entry));
            if(currFat < 0){
                perror("malloc");
                return 0;
            }
            int worked = DevWrite(newFat, newData[i]);
            if(!worked){
                return 0;
            }
            if(i == numBlocks - 1){
                currFat->nextFat = 0xFFFF;
                fat[newFat] = currFat;
                Free[newFat] = 1;
            }
            else{
                fat[newFat] = currFat;
                Free[newFat] = 1;
                fat[newFat]->nextFat = findNextFat();
            }
            newFat = fat[newFat]->nextFat;
        }
    }
    else{
        entry *first = (entry *) malloc(sizeof(entry));
        if(first < 0){
        perror("malloc");
        return 0;
        }
        int nextFat = findNextFat();
        if(nextFat < 0){
            return 0;
        }
        first->nextFat = 0xFFFF;
        current->fatIx = nextFat;
        fat[nextFat] = first;
        Free[nextFat] = 1;
        current->FileName = file;
        current->fileSize = Size;
        int ix = hash(current->FileName);
        insertIntoRoot(current);
        int worked = DevWrite(nextFat, newString);
        if(!worked){
            return 0;
        }
    }
    return 1;
}
/* Reads from file system at most MaxSize characters from FileName and stores it in Data */
int CSCI460_Read (	char *FileName, int MaxSize, char *Data){
    if(fat[0] == NULL && fat[1] == NULL){
        printf("File system hasn't been created yet\n");
        return 0;
    }
    int ix = hash(FileName);
    dir *current = root[ix];
    while(current != NULL && strcmp(current->FileName, FileName) != 0){
        current = current->nextDir;
    }
    if(current == NULL){
        printf("File not found\n");
        return 0;
    }
    int numBlocks = howManyBlocks(current->fileSize);
    if(numBlocks > 1){
        int remaining = current->fileSize;
        int howMuchRead = BYTES_PER_SECTOR;
        char *temp = (char *) malloc (sizeof(char) * (BYTES_PER_SECTOR + 1));
        int blockNum = current->fatIx;
        int ixTmp = 0;
        while(blockNum != 0xFFFF){
            int worked = DevRead(blockNum, temp);
            if(!worked){
                return 0;
            }
            blockNum = fat[blockNum]->nextFat;
            snprintf(&Data[ixTmp], howMuchRead, "%s", temp);
            ixTmp += howMuchRead - 1;
            remaining = remaining - howMuchRead + 2;
            if(ixTmp >= MaxSize){
                Data[MaxSize] = '\0';
                return MaxSize;
            }
            if(remaining - BYTES_PER_SECTOR < 0){
                howMuchRead = remaining;
            }
        }
        return ixTmp;
    }
    else{
        int worked = DevRead(current->fatIx, Data);
        if(worked){
            return current->fileSize;
        }
        else{
            return 0;
        }
    }
}
/* Deletes FileName from the Filesystem by removing the entries from the FAT and removing the directory entry reference to it */
int CSCI460_Delete(	char *Filename){
    if(fat[0] == NULL && fat[1] == NULL){
        printf("The file system hasn't been created\n");
        return 0;
    }
    int ix = hash(Filename);
    dir *current = root[ix];
    dir *parent = NULL;
    while(current != NULL && strcmp(current->FileName, Filename) != 0){
        parent = current;
        current = current->nextDir;
    }
    if(current == NULL){
        printf("File not found\n");
        return 0;
    }
    int next = current->fatIx;
    int tmp = 0;
    while(next != 0xFFFF){
        if(fat[next] != NULL){
            tmp = fat[next]->nextFat;
        }
        Free[next] = 0;
        fat[next] = NULL;
        next = tmp;
    }
    if(parent == NULL){
        root[ix] = NULL;
    }
    else if(current->nextDir != NULL){
        parent->nextDir = current->nextDir;
    }
    else{
        parent->nextDir = NULL;
    }
    free(current);
    return 1;
}
/* returns 0 if File doesn't exist, 1 if it does */
int FileExists(char *FileName){
    int ix = hash(FileName);
    dir *current = root[ix];
    while(current != NULL && strcmp(FileName,current->FileName) != 0){
        current = current->nextDir;
    }
    if(current == NULL){
        return 0;
    }
    else{
        return 1;
    }
}
/* finds the next available block to write Data to */
int findNextFat(){
    int next = 2;
    while(next < SECTORS && fat[next] != NULL){
        next ++;
    }
    if(next == SECTORS){
        printf("FAT is full\n");
        return -1;
    }
    return next;
}
/* Calculates how many blocks will be needed in order to fit Size of Data */
int howManyBlocks(int Size){
    int numBlocks;
    if(Size < BYTES_PER_SECTOR){
        numBlocks = 1;
    }
    else if(Size % BYTES_PER_SECTOR == 0){
        numBlocks = Size / BYTES_PER_SECTOR;
    }
    else{
        numBlocks = (Size / BYTES_PER_SECTOR) + 1;
    }
    return numBlocks;
}
/* If the Data will span multiple blocks, breaks up Data into an array of new blocks size of BYTES_PER_SECTOR */
char **breakUpData(char *Data, int numBlocks){
    char **newData = (char **) malloc (sizeof(char *) * numBlocks);
    if(newData < 0){
        perror("malloc");
        return NULL;
    }
    int DataStart = 0;
    for(int i = 0; i < numBlocks; i ++){
        newData[i] = (char *) malloc (sizeof(char) * (BYTES_PER_SECTOR + 1));
        if(newData[i] < 0){
            perror("malloc");
            return NULL;
        }
        int howMuch = snprintf(newData[i], BYTES_PER_SECTOR, "%s", &Data[DataStart]);
        if(howMuch < 0){
            perror("snprintf");
            return NULL;
        }
        DataStart += BYTES_PER_SECTOR - 1;
    }
    return newData;
}
/* Hashes FileName for efficient retrieval of Data and stores in root directory based on hash ix */
int hash(char *FileName){
    int howLong = strlen(FileName);
    int ix = (howLong * 5 + howLong) % 64;
    return ix;
}
/* Adds the directory entry into root */
void insertIntoRoot(dir *entry){
    int ix = hash(entry->FileName);
    dir *current = root[ix];
    if(root[ix] == NULL){
        root[ix] = entry;
    }
    else{
        while(current->nextDir != NULL){
            current = current->nextDir;
        }
        current->nextDir = entry;
    }
}
