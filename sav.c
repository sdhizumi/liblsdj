#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sav.h"

const unsigned int HEADER_START = 0x8000;
const unsigned int BLOCKS_TABLE_SIZE = 191;

typedef struct
{
	char project_names[32 * 8];
	unsigned char versions[32 * 1];
	char empty[30];
	char init[2];
	int active_project;
    char blocks_table[BLOCKS_TABLE_SIZE];
} header_t;

lsdj_sav_t* lsdj_open(const char* path, lsdj_error_t** error)
{
    // Try to open the sav file at the given path
	FILE* file = fopen(path, "r");
	if (!file)
	{
		lsdj_create_error(error, "could not open file");
		return NULL;
	}

    // Skip memory representing the currently open song
	fseek(file, HEADER_START, SEEK_SET);

	// Read the header block, before we start processing each song
	header_t header;
	fread(&header, sizeof(header), 1, file);

	// Check the initialization characters. If they're not 'jk', we're
	// probably not dealing with an actual LSDJ sav format file.
	if (header.init[0] != 'j' || header.init[1] != 'k')
	{
		lsdj_create_error(error, "SRAM initialization check wasn't 'jk'");
		fclose(file);
		return NULL;
	}

	// Create the sav file structure to return from this function
	lsdj_sav_t* sav = malloc(sizeof(lsdj_sav_t));

	// Find out how many projects there are
	sav->project_count = 0;
	char* ptr = header.project_names;
	while (1)
	{
		size_t length = strlen(ptr);
        if (length > 8)
            length = 8;
        
		if (length == 0)
			break;

		sav->project_count += 1;
        ptr += length;
        if (*ptr == '\0')
            ptr += 1;
	}
    
    // Allocate data for all the projects and store their names
    sav->projects = malloc(sizeof(lsdj_project_t) * sav->project_count);
    ptr = header.project_names;
    for (int i = 0; i < sav->project_count; ++i)
    {
        // Store the project name
    	strncpy(sav->projects[i].name, ptr, 8);
        size_t length = strlen(ptr);
        ptr += (length < 8 ? length + 1 : 8);
        
        // Store the project version
        sav->projects[i].version = header.versions[i];
    }

	// Store the active project index
	sav->active_project = header.active_project == 0xFF ? -1 : header.active_project;
    
    // Gather the size of each project in blocks
    for (int i = 0; i < BLOCKS_TABLE_SIZE; ++i)
    {
        int project = header.blocks_table[i];
        sav->projects[project].compressed_data.block_count++;
    }

    // Clean-up and close the file
	fclose(file);

	return sav;
}

void lsdj_close(lsdj_sav_t* sav)
{
    free(sav->projects);
	free(sav);
}
