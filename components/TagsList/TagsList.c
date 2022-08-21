#include "TagsList.h"
#include <stdio.h>
#include <stdlib.h>

TagsList *CreateTagsList() {
  TagsList *newTagsList = (TagsList *)malloc(sizeof(TagsList));
  printf("Created tags List\n");
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
    } else if (tagsList->len > 0) {
      TagNode *tagNodeP = tagsList->firstNode;
      while (tagNodeP->nextNode) {
        tagNodeP = tagNodeP->nextNode;
      }
      tagNodeP->nextNode = newTagNode;
    }
    tagsList->len = tagsList->len + 1;
    return 1;
  } else {
    printf("MEMORY ERROR: Could not add new tag due to not enought memory\n");
    return 0;
  }
}

void freeTagsList(TagsList *tagsList) {
  if (tagsList->len == 1) {
    printf("\nliberando alloc da tag:\n");
    printTagNode(tagsList->firstNode);
    free(tagsList->firstNode);
  } else if (tagsList->len > 1) {
    TagNode *tagNodeP = tagsList->firstNode;
    TagNode *tmpTagNodeP = NULL;
    while (tagNodeP) {
      tmpTagNodeP = tagNodeP;
      tagNodeP = tagNodeP->nextNode;
      printf("\nliberando alloc da tag:\n");
      printTagNode(tmpTagNodeP);
      free(tmpTagNodeP);
    }
  }
  tagsList->firstNode = NULL;
  tagsList->len = 0;
}
