/*
 *	Source file Driver.c
 *
 *	Device driver for CSCI 460 Assignment3
 *
 *	David Bover, WWU, August 2010
 */

#include <stdio.h>
#include <string.h>

#include "Driver.h"


int established = 0;

FILE *partition = NULL;


int DevFormat( )	// Formats the device
{

	// Create the partition's file
	char *name = "CSCI460_data";
	partition = fopen(name, "w+b");
	if (partition == NULL) {
		printf("DevFormat error: cannot create data file\n");
		return 0;
	}

	// Write zeros to the entire extent of the file
	char data[BYTES_PER_SECTOR];
	for (int i = 0; i < BYTES_PER_SECTOR; i++) data[i] = '\0';
	for (int i = 0; i < SECTORS; i++)
		if (fwrite(data, 1, BYTES_PER_SECTOR, partition)  == 0) {
			printf("DevFormat error: cannot initialize the file\n");
			return 0;
		}

	established = 1;
	return 1;
}


int DevWrite ( int BlockNumber,		// Logical block number
			   char *Data			// Data to be written
			 ) {

	// Check for an existing file system
	if (!established) {
		printf("DevWrite error: no file system exists in the partition\n");
		return 0;
	}

	// Check for invalid block number
	if (BlockNumber < 0 || BlockNumber > SECTORS-1) {
		printf("DevWrite error: invalid block number %d \n", BlockNumber);
		return 0;
	}

	// Calculate the position for the write
	int pos = BlockNumber * BYTES_PER_SECTOR;

    // Seek to that position
    if (fseek(partition, pos, SEEK_SET)) {
	   printf("DevWrite error: seek error at position %ld\n", pos);
	   return 0;
   }

   // Write the data
   if (fwrite(Data, 1, BYTES_PER_SECTOR, partition) == 0) {
	   printf("DevWrite error: write error at position %ld\n", pos);
	   return 0;
   }
   else
	   return 1;
   }

int DevRead ( int BlockNumber,		// Logical block number 
			  char *Data			// Data received
			  ) {

	// Check for an existing file system
	if (!established) {
		printf("DevRead error: no file system exists in the partition\n");
		return 0;
	}

	// Check for invalid block number
	if (BlockNumber < 0 || BlockNumber > SECTORS-1) {
		printf("DevRead error: invalid block number %d \n", BlockNumber);
		return 0;
	}
	
	// Calculate the position for the read
    int pos = BlockNumber * BYTES_PER_SECTOR;

   // Seek to that position
   if (fseek(partition, pos, SEEK_SET)) {
	   printf("DevRead error: seek error at position %ld\n", pos);
	   return 0;
   }

   // Read the data
   if (fread(Data, 1, BYTES_PER_SECTOR, partition) == 0) {
	   printf("DevRead error: read error at position %ld\n", pos);
	   return 0;
   }
   else
	   return 1;
}




