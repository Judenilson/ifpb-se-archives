#include "TagsList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TagsList *CreateTagsList() {
  TagsList *newTagsList = (TagsList *)malloc(sizeof(TagsList));
  newTagsList->len = 0;
  newTagsList->firstNode = NULL;
  return newTagsList;
}

void printTagNode(TagNode *tagNode) {
  printf("name: %s\n", tagNode->name);
  printf("id: %s\n", tagNode->id);
  printf("nextNode: %p\n", tagNode->nextNode);
}

void printTagsList(TagsList *tagsList) {
  printf("\nTagsList: \n");
  printf("len: %d\n", tagsList->len);
  if (tagsList->len > 0) {
    TagNode *tagNodeP = tagsList->firstNode;
    while (tagNodeP->nextNode) {
      printTagNode(tagNodeP);
      tagNodeP = tagNodeP->nextNode;
    }
    printTagNode(tagNodeP);
  }
}

int tagsListAppend(TagsList *tagsList, const char id[], const char name[]) {
  TagNode *newTagNode = (TagNode *)malloc(sizeof(TagNode));
  if (newTagNode) {
    strcpy(newTagNode->id, id);
    strcpy(newTagNode->name, name);
    newTagNode->nextNode = NULL;
    if (tagsList->len == 0) {
      tagsList->firstNode = newTagNode;
      tagsList->len = tagsList->len + 1;
      return 1;
    } else if (idExits(tagsList, id) == 1) {
      printf("DUPLICITY ERROR: Could not add new tag cause it already was registred\n");
      return 0;
    } else {
      TagNode *tagNodeP = tagsList->firstNode;
      while (tagNodeP->nextNode) {
        tagNodeP = tagNodeP->nextNode;
      }
      tagNodeP->nextNode = newTagNode;
      tagsList->len = tagsList->len + 1;
      return 1;
    }
  } else {
    printf("MEMORY ERROR: Could not add new tag due to not enought memory\n");
    return 0;
  }
}

void freeTagsList(TagsList *tagsList) {
  if (tagsList->len == 1) {
    free(tagsList->firstNode);
  } else if (tagsList->len > 1) {
    TagNode *tagNodeP = tagsList->firstNode;
    TagNode *tmpTagNodeP = NULL;
    while (tagNodeP) {
      tmpTagNodeP = tagNodeP;
      tagNodeP = tagNodeP->nextNode;
      free(tmpTagNodeP);
    }
  }
  tagsList->firstNode = NULL;
  tagsList->len = 0;
}

int idExits(TagsList *tagsList, const char id[]) {
  if (tagsList->len > 0) {
    TagNode *tagNodeP = tagsList->firstNode;
    while (tagNodeP) {
      if (strcmp(tagNodeP->id, id) == 0) return 1;
      tagNodeP = tagNodeP->nextNode;
    }
  }
  return 0;
}

int getNameById(TagsList *tagsList, const char id[], char name[]) {
  if (tagsList->len > 0) {
    TagNode *tagNodeP = tagsList->firstNode;
    while (tagNodeP) {
      if (strcmp(tagNodeP->id, id) == 0) {
        strcmp(tagNodeP->name, name);
        return 1;
      }
      tagNodeP = tagNodeP->nextNode;
    }
  }
  return 0;
}