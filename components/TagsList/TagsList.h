#define TAG_ID_LEN 20
#define TAG_NAME_LEN 20

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

TagsList *CreateTagsList();

void printTagNode(TagNode *tagNode);

void printTagsList(TagsList *tagsList);

int tagsListAppend(TagsList *tagsList, const char id[], const char name[]);

void freeTagsList(TagsList *tagsList);

int idExits(TagsList *tagsList, const char id[]);

int getNameById(TagsList *tagsList, const char id[], char name[]);
