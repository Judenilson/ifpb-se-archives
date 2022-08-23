#define TAG_ID_LEN 16
#define TAG_NAME_LEN 50

typedef struct TagNode TagNode;
struct TagNode {
  char id[TAG_ID_LEN];
  char name[TAG_NAME_LEN];
  TagNode *nextNode;
};

typedef struct TagsList TagsList;
struct TagsList {
  int len;
  TagNode *firstNode;
};

TagsList* CreateTagsList();

void printTagNode(TagNode *tagNode);

void printTagsList(TagsList *tagsList);

int tagsListAppend(TagsList *tagsList, const char id[], const char name[]);

void freeTagsList(TagsList *tagsList);

int idExits(TagsList *tagsList, const char id[]);

int getNameById(TagsList *tagsList, const char id[], char name[]);

void getNamesHtml(TagsList *tagsList, char htmlList[]);

void getTagsHtml(TagsList *tagsList, char htmlList[]);

void getString(TagsList *tagsList, char string[]);

TagsList* CreateTagsListFromString(const char string[]);

void tagsListDelete(TagsList *tagsList, const char id[]);
