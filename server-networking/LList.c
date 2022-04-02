#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
typedef struct LList{
  char* key;
  int length;
  char* value;
  struct LList* next;
}LList;

void destroyLList(LList* l){
  if(l==NULL){
    return;
  }
  if(l !=NULL) destroyLList(l->next);
    free(l->value);
    free(l->key);
  free(l);
}
void init_LL(LList* l){
  // assume l is already malloc'd
  l->key = calloc(2, sizeof(char));
  l->key[0] = '\0';
  l->value = calloc(2, sizeof(char));
  l->value[0] = '\0';
  l->length = 0;
 l->next = NULL;
}
void printLL(LList* l){
  printf("-------printLL---------\n");
  if(l==NULL) printf("empty list!\n");
  LList* temp = l;
  while(temp != NULL){

    printf("this is key: %s\nthis is value: %s\nthis is length: %d\n", temp->key, temp->value, temp->length);
    temp = temp->next;
  }
  printf("---------endLL---------\n");
}
// assume key and value is malloc'd
// assume it works might not work
LList* insertLL(LList* l, char* key, char* value, int length){
  if(l == NULL){
    l = malloc(sizeof(LList));
    l->key = key;
    l->value = value;
    l->next = NULL;
    l->length = length;
    return l;
  }
  LList* temp = l;
  // case: first node in LL
  if(temp->key[0] == '\0')
  {
    free(temp->key);
    free(temp->value);
    temp->key = key;
    temp->value = value;
    temp->length = length;
    // new key is added with its value
    return l;
  }
  //if Return value < 0 then it indicates str1 is less than str2.
//if Return value > 0 then it indicates str2 is less than str1.
  LList* prev = NULL;
  // iterate through the LL to check if the key exists
  while(temp !=NULL && strcmp(temp->key, key) < 0)
  {
    prev = temp;
    temp = temp->next;
  }
  // key doesn't exist
  if(temp == NULL) {
    // create a new node with the key and value
    temp = malloc(sizeof(LList));
    temp->key = key;
    temp->value = value;
    temp->next = NULL;
    temp->length = length;
    // add the new node to the end of LL
    LList* last = l;
    while(last->next!=NULL) last=last->next;
    last->next = temp;
    return l;
  }
  if(strcmp(temp->key, key)== 0){
    //the key already exists
    //TODO: this after we get answer on piazza
    free(temp->value);
    free(key);
    temp->value = value;
    return l;
  }
  // add it to the beginning
  if(prev == NULL){
    prev = malloc(sizeof(LList));
    prev->key = key;
    prev->value = value;
    prev->next = l;
    prev->length = length;
    return prev;
  }
  free(key);
  free(value);
  // there is already a key in the LL wit that value
  return l;
}

bool inLLKey(LList* l, char* key){
  LList* temp = l;
  while(temp != NULL && strcmp(temp->key, key) != 0){
    temp = temp->next;
  }
  if(temp == NULL|| temp->value[0]=='\0'){
    // did not find the key user inputted
    return false;
  }
  // there is a key in the LL that corresponds with the user input
  return true;
}


void getLLKey(LList* l, char* key, int fd){
  LList* temp = l;
  while(temp != NULL && strcmp(temp->key, key) != 0){
    temp = temp->next;
  }
  //printf("found in LL\n");
  char* retValue = temp->value;
  int retLength = temp->length;
  char retLengthstr[100000];
  //itoa(retLength, retLengthstr, 10);
  sprintf(retLengthstr, "%d", retLength);
    write(fd, "OKG\n", 4);
    write(fd, retLengthstr, strlen(retLengthstr));
    write(fd, "\n", 1);
    write(fd, retValue, strlen(retValue));
    write(fd, "\n", 1);
  //printf("key: %s, value: %s, length: %d\n", temp->key, retValue, temp->length);

}


LList* deleteLLKey(LList* l, char* key, int fd){
  LList* temp = l;
  LList* prev = NULL;
  while(temp != NULL && strcmp(temp->key, key) != 0){
    prev = temp;
    temp = temp->next;
  }
  if(temp == NULL){
    // did not find the key user inputted
    return l;
  }
  else if(prev!=NULL && prev->key[0] =='\0'){
    //list is empty
  //  printf("after\n");
    return l;
  }

  // it's smewhere in the LL
  else{

  //  printf("deleting: key: %s, value: %s, length: %d\n", temp->key, temp->value, temp->length);
    char* retValue = temp->value;
    int retLength = temp->length;
    char retLengthstr[100000];
    //itoa(retLength, retLengthstr, 10);
    sprintf(retLengthstr, "%d", retLength);
    write(fd, "OKD\n", 4);
    write(fd, retLengthstr, strlen(retLengthstr));
    write(fd, "\n", 1);
    write(fd, retValue, strlen(retValue));
    write(fd, "\n", 1);
    free(temp->value);
    temp->value = malloc(sizeof(char));
    temp->value[0] = '\0';
    return l;
  }
  // there is a key in the LL that corresponds with the user input
  //return true;
}
/*
List* deleteLLKey(LList* l, char* key, int fd){
  LList* temp = l;
  LList* prev = NULL;
  while(temp != NULL && strcmp(temp->key, key) != 0){
    prev = temp;
    temp = temp->next;
  }
  if(temp == NULL){
    // did not find the key user inputted
    return l;
  }
  // if its the first node in the LL
  else if(prev == NULL){
    prev = temp;
    temp = temp->next;
  //  printf("deleting: key: %s, value: %s, length: %d\n", prev->key, prev->value, prev->length);
    char* retValue = prev->value;
    int retLength = prev->length;
    char retLengthstr[100000];
    //itoa(retLength, retLengthstr, 10);
    sprintf(retLengthstr, "%d", retLength);
      write(fd, "OKD\n", 4);
      write(fd, retLengthstr, strlen(retLengthstr));
      write(fd, "\n", 1);
      write(fd, retValue, strlen(retValue));
      write(fd, "\n", 1);

    free(prev->key);
    free(prev->value);
    free(prev);
    return(temp);
  }
  else if(prev->key[0] =='\0'){
    //list is empty
    return l;
  }
  // it's smewhere in the LL
  else{
    prev->next = temp->next;
  //  printf("deleting: key: %s, value: %s, length: %d\n", temp->key, temp->value, temp->length);
    char* retValue = temp->value;
    int retLength = temp->length;
    char retLengthstr[100000];
    //itoa(retLength, retLengthstr, 10);
    sprintf(retLengthstr, "%d", retLength);
    write(fd, "OKD\n", 4);
    write(fd, retLengthstr, strlen(retLengthstr));
    write(fd, "\n", 1);
    write(fd, retValue, strlen(retValue));
    write(fd, "\n", 1);
    free(temp->key);
    free(temp->value);
    free(temp);
    return l;
  }
  // there is a key in the LL that corresponds with the user input
  //return true;
}
}
*/
