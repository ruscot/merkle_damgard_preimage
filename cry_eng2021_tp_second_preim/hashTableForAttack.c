#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZEATTACK 9000000

struct DataItem {
   __uint32_t message[4];   
   __uint64_t hashKey;
   struct DataItem *next;
};

struct DataItem* hashArrayAttack[SIZEATTACK]; 

int hashCode(__uint64_t key) {
   return key % SIZEATTACK;
}

void freeHashTableAttack() {
    int i;
    struct DataItem* currentItem;
    struct DataItem* saveCurrentItem;
    for(i = 0; i < SIZEATTACK; i++){
        currentItem = hashArrayAttack[i];
        while(currentItem != NULL){
            saveCurrentItem = currentItem->next;
            free(currentItem);
            currentItem = saveCurrentItem;
        }
    }
}

int searchHashTableAttack(__uint64_t key, __uint32_t message[4]) {
   //get the hash 
   int hashIndex = hashCode(key);  
	
   //move in array until an empty 
   struct DataItem* currentItem = hashArrayAttack[hashIndex];
   while(currentItem != NULL) {
	
      if(currentItem->hashKey == key) {
        message[0] = currentItem->message[0];
        message[1] = currentItem->message[1];
        message[2] = currentItem->message[2];
        message[3] = currentItem->message[3];
        return 1;
      } 
			
      currentItem = currentItem->next;
   }
	
   return 0;        
}

void insertHassTableAttack(__uint64_t key, __uint32_t message[4]) {

   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   item->message[0] = message[0];
   item->message[1] = message[1];
   item->message[2] = message[2];
   item->message[3] = message[3];  
   item->hashKey = key;

   //get the hash 
   int hashIndex = hashCode(key);
   item->next = hashArrayAttack[hashIndex];
   hashArrayAttack[hashIndex] = item;
}