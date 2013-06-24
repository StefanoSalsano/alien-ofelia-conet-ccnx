#include "uthash/utstring.h"

#define BUFFERLEN 500

void fill_message(long tag, char *type, char* nid, unsigned long long csn, char *message);


/**
 * See the main() in c_json.c to see how to use it
 */
void json_messageOld(long content_name, char *type, char **message);
