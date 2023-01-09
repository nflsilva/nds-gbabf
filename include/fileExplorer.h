#pragma once

#include <fat.h>
#include <dirent.h>
#include <string.h>

#include "stack.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define WINDOW_SIZE 22
#define SHORT_NAME_LEN 32
#define MAX_DIRECTORY_CONTENTS 100
#define MAX_PATH_NAME_SIZE 1024

#define CLER  "\x1b[2J"	
#define KNRM  "\x1b[0m"
#define KRED  "\x1b[31m"
#define KGRN  "\x1b[32m"
#define KYEL  "\x1b[33m"
#define KBLU  "\x1b[34m"
#define KMAG  "\x1b[35m"
#define KCYN  "\x1b[36m"
#define KWHT  "\x1b[37m"

struct DirectoryItem {
	char name[256];
	char shortName[SHORT_NAME_LEN];
	bool isDirectory;
};
typedef struct DirectoryItem DirectoryItem_t;

struct FileExplorerContext {
	Stack_t* folderStack;
	DirectoryItem_t directoryContents[MAX_DIRECTORY_CONTENTS];
	int directorySize;
	int selectedItemIndex;
	char directoryPathBuffer[MAX_PATH_NAME_SIZE];
	char filePathBuffer[MAX_PATH_NAME_SIZE];
	int windowStartItemIndex;
};
typedef struct FileExplorerContext FileExplorerContext_t;
FileExplorerContext_t* context = NULL;

void fe_loadCurrentDirectoryContents() {
	if(!context) return;

	struct dirent* pent;
	strcpy(context->directoryPathBuffer, "/");
	int nPathParts = s_size(context->folderStack);
	for(int i = 0; i < nPathParts; i++) {
		strcat(context->directoryPathBuffer, s_peekAt(context->folderStack, i));
		strcat(context->directoryPathBuffer, "/");
	}
	DIR* pdir = opendir(context->directoryPathBuffer);

	if (pdir){

		context->directorySize = 0;

		while ((pent = readdir(pdir)) != NULL) {

			if(strcmp(".", pent->d_name) == 0) continue;

			DirectoryItem_t* item = &context->directoryContents[context->directorySize++];
			strcpy(item->name, pent->d_name);

			int nameLen = strlen(item->name);
			if(nameLen >= SHORT_NAME_LEN) {

				// find extention length
				int extentionLength = 1;
				for(int c = nameLen; c > 0; c--) {
					if(item->name[c] == '.')
						break;
					else
						extentionLength++;
				}

				// copy name until extension length
				for(int i = 0; i < SHORT_NAME_LEN - extentionLength; i++) {
					item->shortName[i] = item->name[i];
				}
					
				// copy extension
				for(int i = 0; i < extentionLength; i++) {
					item->shortName[SHORT_NAME_LEN - i - 2] = item->name[nameLen - i];
				}

				item->shortName[SHORT_NAME_LEN - extentionLength - 2] = '.';
				item->shortName[SHORT_NAME_LEN - 1] = '\0';
			}
			else {
				strcpy(item->shortName, item->name);
			}
			item->isDirectory = pent->d_type == DT_DIR;
		}
		closedir(pdir);
	} else {
		iprintf ("opendir() failure; terminating\n");
	}
}

void fe_drawCurrentDirectory() {
	if(!context) return;

	iprintf(CLER);
	iprintf("%sfat:%s%s\n", KGRN, context->directoryPathBuffer, KWHT);

	int maxIndex = MIN(context->windowStartItemIndex + WINDOW_SIZE, context->directorySize);

	for(int i = context->windowStartItemIndex; i < maxIndex; i++) {
		DirectoryItem_t item = context->directoryContents[i];

		if(i == context->selectedItemIndex)
			iprintf("%s ", KYEL);
		else
			iprintf("%s ", KWHT);

		iprintf("%s", item.shortName);

		if(item.isDirectory)
			iprintf("/");

		iprintf("\n");
			
	}
	iprintf(KNRM);
}

char* fe_selectItem() {
	if(!context) return NULL;

	DirectoryItem_t* selectedItem = &context->directoryContents[context->selectedItemIndex];
	if(!selectedItem->isDirectory) {
		strcpy(context->filePathBuffer, "fat:");
		strcat(context->filePathBuffer, context->directoryPathBuffer);
		strcat(context->filePathBuffer, selectedItem->name);
		return context->filePathBuffer;
	}

	if(strcmp(selectedItem->name, "..") == 0) {
		char* folderName = s_pop(context->folderStack);
		free(folderName);
	}
	else {
		char* folderName = (char*) malloc(sizeof(selectedItem->name));
		strcpy(folderName, selectedItem->name);
		s_push(context->folderStack, folderName);
	}

	context->selectedItemIndex = 0;
	context->windowStartItemIndex = 0;
	fe_loadCurrentDirectoryContents();
	fe_drawCurrentDirectory();
	return NULL;
}

void fe_browseOut() {
	if(!context || !context->folderStack || s_size(context->folderStack) == 0) return;
	context->selectedItemIndex = 0;
	fe_selectItem();
}

void fe_browseDown() {
	if(!context || context->selectedItemIndex == context->directorySize -1) return;
	context->selectedItemIndex++;

	int windowBottomBound = context->windowStartItemIndex + WINDOW_SIZE ;
	if(context->selectedItemIndex >= windowBottomBound)
		context->windowStartItemIndex++;

	fe_drawCurrentDirectory();
}

void fe_browseUp() {
	if(!context) return;
	if(context->selectedItemIndex == 0) return;
	context->selectedItemIndex--;

	if(context->selectedItemIndex < context->windowStartItemIndex)
		context->windowStartItemIndex--;

	fe_drawCurrentDirectory();
}

long fe_computeFileSize(char* filePath) {
	FILE* f = NULL;
	f = fopen(filePath, "r");
	if(f == NULL) return 0;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	fclose(f);

	return size;
}

void fe_open() {

	if (fatInitDefault()) {

		if(context == NULL)
			context = (FileExplorerContext_t*) malloc(sizeof(FileExplorerContext_t));

		context->folderStack = s_createStack();
		context->directorySize = 0;
		context->selectedItemIndex = 0;
		context->windowStartItemIndex = 0;

		fe_loadCurrentDirectoryContents();
		fe_drawCurrentDirectory();
		
	} else {
		iprintf("init failure: terminating\n");
	}
}

void fe_close() {
	if(!context) return;
	s_destroy(context->folderStack);
	free(context);
	iprintf(CLER);
}