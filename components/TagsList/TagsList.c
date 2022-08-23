#include "TagsList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TAGS 5
#define TAG_ID_LEN 16
#define TAG_NAME_LEN 50

const char delimit_id_name[] = ":";
const char delimit_node[] = ";";

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
        strcpy(name, tagNodeP->name);
        return 1;
      }
      tagNodeP = tagNodeP->nextNode;
    }
  }
  return 0;
}

void getNamesHtml(TagsList *tagsList, char htmlList[]) {
  if (tagsList->len > 0) {
    TagNode *tagNodeP = tagsList->firstNode;
    // htmlList[9 + ((9 + TAG_NAME_LEN) * 5) + 1] = "";
    strcat(htmlList, "<ul>");
    while (tagNodeP) {
      strcat(htmlList, "<li>");
      strcat(htmlList, tagNodeP->name);
      strcat(htmlList, "</li>");
      tagNodeP = tagNodeP->nextNode;
    }
    strcat(htmlList, "</ul>");
  }
  else strcat(htmlList, "<ul></ul>");
}

void getTagsHtml(TagsList *tagsList, char htmlList[]) {
  if (tagsList->len > 0) {
    TagNode *tagNodeP = tagsList->firstNode;
    // htmlList[9 + ((18 + TAG_NAME_LEN + TAG_ID_LEN) * 5) + 1] = "";
    strcat(htmlList, "<ul>");
    while (tagNodeP) {
      strcat(htmlList, "<li>");
      strcat(htmlList, tagNodeP->id);
      strcat(htmlList, "</li>");
      strcat(htmlList, "<li>");
      strcat(htmlList, tagNodeP->name);
      strcat(htmlList, "</li>");
      tagNodeP = tagNodeP->nextNode;
    }
    strcat(htmlList, "</ul>");
  }
  else strcat(htmlList, "<ul></ul>");
}

void getString(TagsList *tagsList, char string[]) {
  char tmpString[((TAG_ID_LEN + TAG_NAME_LEN + 1 + 1) * MAX_TAGS) + 1] = "";

  if (tagsList->len > 0) {
    TagNode *tagNodeP = tagsList->firstNode;
    while (tagNodeP) {
      strcat(tmpString, tagNodeP->id);
      strcat(tmpString, delimit_id_name);
      strcat(tmpString, tagNodeP->name);
      strcat(tmpString, delimit_node);
      tagNodeP = tagNodeP->nextNode;
    }
  }

  strcpy(string, tmpString);
}

TagsList* CreateTagsListFromString(const char string[]) {
  TagsList* newTagsList = CreateTagsList();
  char id[TAG_ID_LEN] = "";
  char name[TAG_NAME_LEN] = "";
  int idxId = 0;
  int idxName = 0;
  int reading_id = 1;
  int counter = 0;
  for (int i = 0; i < strlen(string); i++) {
    if (string[i] == ':') {
      reading_id = 0;
    } else if (string[i] == ';') {
      id[idxId] = '\0';
      name[idxName] = '\0';
      tagsListAppend(newTagsList, id, name);
      strcpy(id, "");
      strcpy(name, "");
      idxId = 0;
      idxName = 0;
      reading_id = 1;

      counter = counter + 1;
      if (counter >= MAX_TAGS) {
        break;
      }
    } else {
      if (reading_id == 1) {
        id[idxId] = string[i]; 
        idxId++;
      }
      else {
        name[idxName] = string[i];
        idxName++;
      }
    }
  }

  return newTagsList;
}

void tagsListDelete(TagsList *tagsList, const char id[]) {
  if (tagsList->len > 0) {
    TagNode *tagNodeP = tagsList->firstNode;
    TagNode *preTagNodeP = NULL;
    while (tagNodeP) {
      if (strcmp(tagNodeP->id, id) == 0) {
        if (preTagNodeP) { // passou do primeiro node
          preTagNodeP->nextNode = tagNodeP->nextNode;
        } else { // ta no primeiro node
          tagsList->firstNode = tagNodeP->nextNode;
        }
        tagsList->len = tagsList->len - 1;
        free(tagNodeP);
        break;
      }
      preTagNodeP = tagNodeP;
      tagNodeP = tagNodeP->nextNode;
    }
  }
}
