#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

//#define SIZE 9000000

struct DataItem {
   __uint32_t message[4];   
   __uint64_t hashKey;
   struct DataItem *next;
};

//struct DataItem* hashArray[SIZE]; 

int hashCode(__uint64_t key, int SIZE) {
   return key % SIZE;
}

void freeHashTable(struct DataItem** hashArray, int SIZE) {
    int i;
    struct DataItem* currentItem;
    struct DataItem* saveCurrentItem;
    for(i = 0; i < SIZE; i++){
        currentItem = hashArray[i];
        while(currentItem != NULL){
            saveCurrentItem = currentItem->next;
            free(currentItem);
            currentItem = saveCurrentItem;
        }
    }
}

int searchHashTable(__uint64_t key, __uint32_t message[4], struct DataItem** hashArray, int SIZE) {
   //get the hash 
   int hashIndex = hashCode(key, SIZE);  
	
   //move in array until an empty 
   struct DataItem* currentItem = hashArray[hashIndex];
   while(currentItem != NULL) {
	
      if(currentItem->hashKey == key) {
        message[0] = currentItem->message[0];
        message[1] = currentItem->message[1];
        message[2] = currentItem->message[2];
        message[3] = currentItem->message[3];
        //printf("\nKey found \n%lx\n", currentItem->hashKey);
        return 1;
      } 
			
      currentItem = currentItem->next;
   }
	
   return 0;        
}

void insertHashTable(__uint64_t key, __uint32_t message[4], struct DataItem** hashArray, int SIZE) {

   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   item->message[0] = message[0];
   item->message[1] = message[1];
   item->message[2] = message[2];
   item->message[3] = message[3];  
   item->hashKey = key;

   //get the hash 
   int hashIndex = hashCode(key, SIZE);
   item->next = hashArray[hashIndex];
   hashArray[hashIndex] = item;
}


/*
void countValue() {
   int i = 0;
	int counter = 0;
   for(i = 0; i<SIZE; i++) {
	
      if(hashArrayAttack[i] != NULL)
         counter++;
   }
   printf("\nCounter %d\n", counter);
	
   printf("\n");
}
*/
/*struct DataItem* delete(struct DataItem* item) {
   int key = item->hashKey;

   //get the hash 
   int hashIndex = hashCode(key);

   //move in array until an empty
   while(hashArray[hashIndex] != NULL) {
	
      if(hashArray[hashIndex]->hashKey == key) {
         struct DataItem* temp = hashArray[hashIndex]; 
			
         //assign a dummy item at deleted position
         hashArray[hashIndex] = dummyItem; 
         return temp;
      }
		
      //go to next cell
      ++hashIndex;
		
      //wrap around the table
      hashIndex %= SIZE;
   }      
	
   return NULL;        
}

void display() {
   int i = 0;
	
   for(i = 0; i<SIZE; i++) {
	
      if(hashArray[i] != NULL)
         printf(" (%d,%d)",hashArray[i]->key,hashArray[i]->data);
      else
         printf(" ~~ ");
   }
	
   printf("\n");
}

int testHashTable() {
   dummyItem = (struct DataItem*) malloc(sizeof(struct DataItem));
   dummyItem->data = -1;  
   dummyItem->key = -1; 

   insert(1, 20);
   insert(2, 70);
   insert(42, 80);
   insert(4, 25);
   insert(12, 44);
   insert(14, 32);
   insert(17, 11);
   insert(13, 78);
   insert(37, 97);

   display();
   item = search(37);

   if(item != NULL) {
      printf("Element found: %d\n", item->data);
   } else {
      printf("Element not found\n");
   }

   delete(item);
   item = search(37);

   if(item != NULL) {
      printf("Element found: %d\n", item->data);
   } else {
      printf("Element not found\n");
   }
}*/