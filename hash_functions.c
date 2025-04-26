#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//..............SETTINGS.........................
//..............................................
#define HASH_SIZE 100 //even num better
#define HASH_COALESCENT_SIZE (HASH_SIZE)*(0.70) //70% main room other for collision_resolution 

typedef enum{
  TYPE_OPADDRESS_DOUBLE,
  TYPE_OPADDRESS_LINEAR,
  TYPE_OPADDRESS_QUADRATIC,
  TYPE_DIRECT_CHAIN,
  TYPE_DIRECT_COAL
}COLLISION_METHODS;
COLLISION_METHODS COLLISION_METHOD = TYPE_OPADDRESS_DOUBLE; // default

typedef enum {
  TYPE_MULTIPLICATION,
  TYPE_DIVISION,
  TYPE_MIDDLE,
  TYPE_FOLD,
  TYPE_COMPRESSION,
}HASH_METHOD;
HASH_METHOD METHOD = TYPE_MULTIPLICATION; // default:
HASH_METHOD PROBING_METHOD = TYPE_FOLD; // default 

//..................DATA..........................
typedef struct hash_item{
  char *key;
  struct hash_item *next;
  int offset; // openaddressing offset for search function not to break
  int data;
  
}hash_item;

typedef struct hash_table{
  int  used;
  hash_item *items[HASH_SIZE];
}hash_table;


hash_table *hash_table_init(){
  hash_table *new_table = (hash_table*)malloc(sizeof(hash_table));
  new_table->used = 0;
  
  for(int i = 0; i < HASH_SIZE; i++){
    new_table->items[i] = NULL;
  }

  return new_table;
}

int long multiplication_hash(const char *str){
  double hash = (1 + sqrt(5)) / 2; // 1.6.....golden ratio irrational
  unsigned long temp = 0;

  for(; *str; str++){
    temp = (temp * 31) + *str;
  }
  //isolation the fraction;
  hash = hash * temp;
  temp = hash;
  hash = hash - temp;
  temp = hash * HASH_SIZE;
  return temp;
}

unsigned long mid_hash(const char *str){
  unsigned long hash = 0;

  //hashing the string
  for(; *str; str++){
    hash = (hash * 31) + *str;
    }
  hash = pow(2 , hash);
  
  int len = log10(hash) + 1; //digits 

  if(len <= 4) return hash;
  else{
    hash = hash / 100; // remove first 2 digits
    hash = hash - ((hash / 100) * 100); // 1234 / 100 -> 12  -> 1200 -> 1234 - 1200 = 34 done
    printf("The hash mid is : %i\n", (int)hash);
  }
  

  return hash;
}


unsigned long folding_hash(const char *str){
  unsigned long hash = 0;
  int len = strlen(str);
  unsigned int chunk_size = 3; //bytes 
  
  for(int i = 0; i < len; i += chunk_size){
    unsigned long chunk = 0; // 8 bytes all zeros 000 000 000 .....

    for(int j = 0; j < chunk_size && (j + i) < len; j++){
      chunk = (chunk << 8) | str[i + j];
      
    }
    hash ^= chunk; // combining them by XOR can be just add +
  }
  return hash;
}

unsigned long compression_hash(const char *str){
  unsigned long hash = 5381; // prime
  unsigned int c = 0;
  while((c = *str++)){
    hash = ((hash << 5) + hash) + c; // old way of hash * 33 where << is 32 and + hash to be 33;
  }
  return hash;
}

unsigned long division_hash(const char *str){
  unsigned long hash = 0;
  for(; *str; str++){
        hash = (hash * 31) + *str;
        }
  return hash;

}

unsigned long strhash_code(const char *str, HASH_METHOD type){
  unsigned long hash = 0;
  switch (type) {
    case TYPE_DIVISION: 
      hash= division_hash(str);
      return hash;
  
    case TYPE_FOLD:
      hash = folding_hash(str);
      return hash;
    
    case TYPE_MIDDLE:
      hash = mid_hash(str);
      return hash;
    
    case TYPE_COMPRESSION:
      hash = compression_hash(str);
      return hash;
    
    case TYPE_MULTIPLICATION:
      hash = multiplication_hash(str);
      return hash;
  }

}


unsigned int hash_index(const char *str, HASH_METHOD type){
  unsigned long hash = strhash_code(str, type);
  if(type == TYPE_MULTIPLICATION) return hash;// gives index already
  else if (COLLISION_METHOD == TYPE_DIRECT_COAL) return hash % HASH_COALESCENT_SIZE; // keeps index in range 
  else return hash % HASH_SIZE;
}

hash_item *hash_item_alloc(const char *string, int data){
  hash_item *new_item = (hash_item*)calloc(1, sizeof(hash_item));
  new_item->key = strdup(string);
  new_item->data = data;
  new_item->offset = 0;
  new_item->next = NULL;

  return new_item;
}
//....................................................................
//............COLLISION SECTION.......................................
//...................................................................
void write_report(hash_table *table){
  //report file
    FILE *fptr = fopen("collision_report.txt", "a");
    if(fptr == NULL){
      perror("error");
      exit(1);
      
    }
    
    if(COLLISION_METHOD == TYPE_DIRECT_COAL) fprintf(fptr, "DIRECT COALESCENT METHOD \n______THE MAIN ROOM_______\n");
    for(int count = 0; count < HASH_SIZE; count++){
      if(table->items[count] != NULL){ 
        if(COLLISION_METHOD == TYPE_DIRECT_COAL && count == HASH_COALESCENT_SIZE) fprintf(fptr, "______THE COLLISION ROOM_______\n");
        fprintf(fptr, "AT SLOT (%i) --> %s", count, table->items[count]->key);
        //print chain if found
          if(table->items[count]->next != NULL){
            hash_item *current = table->items[count]->next;
              while(current != NULL){ 
                fprintf(fptr, "-----> %s", current->key);
                current = current->next;
              }}
        fprintf(fptr, "\n");
           
      }else{
        fprintf(fptr, "AT SLOT (%i)--> empty \n", count);
      }
    }
     fclose(fptr);
}

short collision_checker(hash_table *table, unsigned int index){
  if(table->items[index] != NULL){ // collision slot not empty
    return 1; //collision
  }else{ 
  return 0; // no collision
  }
}
// COLLISION TYPES
void opaddress_doublehash(hash_table *table, hash_item *item, int index, HASH_METHOD type){
  int probing_count = 1;
  unsigned int new_index = (index + probing_count) % HASH_SIZE;
  
  while(probing_count < HASH_SIZE){
    if(collision_checker(table, new_index) == 1) {
      probing_count++;
      new_index = (index + (probing_count * probing_count) * hash_index(item->key, type)) % HASH_SIZE;
      continue;
    }else{
      table->items[new_index] = item;
      table->items[new_index]->offset = probing_count * hash_index(item->key, type);
      break;
    }
  }
}

void opaddress_quadratic(hash_table *table,hash_item *item, int index){
  int probing_count = 1;
  unsigned int new_index = (index + probing_count) % HASH_SIZE;
  
  while(probing_count < HASH_SIZE){
    if(collision_checker(table, new_index) == 1) {
      probing_count++;
      new_index = (index + (probing_count * probing_count)) % HASH_SIZE;
    }else{
      table->items[new_index] = item;
      table->items[new_index]->offset = pow(2, probing_count);
      break;
    }
    
  }
}

void opaddress_linear(hash_table *table, hash_item *item, int index){
  int probing_count = 1;
  unsigned int new_index = (index + probing_count) % HASH_SIZE;
  
  while(probing_count < HASH_SIZE){
    if(collision_checker(table, new_index) == 1) {
    probing_count++;
    new_index = (index + probing_count) % HASH_SIZE;
    }else{
      table->items[new_index] = item;
      table->items[new_index]->offset = probing_count;
      break;
    }
    
  }
}


void direct_seprate_chain(hash_table *table, hash_item *item, int index){
  hash_item *current = table->items[index];
  while(current->next != NULL){
    current = current->next;
  } 
  current->next = item;

}

void direct_coalescent(hash_table *table, hash_item *item, int index){
  unsigned int range = HASH_SIZE - HASH_COALESCENT_SIZE; // 30% remain room
  for(int i = HASH_SIZE - range; i < HASH_SIZE; i++){
    if(table->items[i] == NULL){
      table->items[i] = item;
      table->items[index]->next = item; // main slot pointes to item
      break;
    }
  }
}

void collision_resolution(hash_table *table, hash_item *item, int index){
  COLLISION_METHODS type = COLLISION_METHOD;
  switch(type){
    case TYPE_OPADDRESS_DOUBLE:
      opaddress_doublehash(table , item, index, PROBING_METHOD);
      break;
    case TYPE_OPADDRESS_QUADRATIC:
      opaddress_quadratic(table, item, index);
      break;
    case TYPE_OPADDRESS_LINEAR:
      opaddress_linear(table, item, index);
      break;
    case TYPE_DIRECT_CHAIN:
      direct_seprate_chain(table, item, index);
      break;
    case TYPE_DIRECT_COAL:
      direct_coalescent(table, item, index);
    break;
   }
}

void incert(hash_table *table, const char *string, int data){
  hash_item *new_item = hash_item_alloc(string, data); //with input
  unsigned int index = hash_index(string, METHOD);
  // collision fixing
  short trigger = collision_checker(table, index);
  if(trigger == 1) collision_resolution(table, new_item, index); // fix collision
  else table->items[index] = new_item;

  
  table->used++; // data count
  printf("incert: %s \n", new_item->key);
}
 
hash_item *search(hash_table *table, const char *str){
  unsigned int index = hash_index(str, METHOD);
  if(table->items[index] == NULL){
    printf("Empty Item Slot!\n");
    return NULL;
  }else if(strcmp(table->items[index]->key, str) == 0){
    printf("Item found! \n");
    return table->items[index];
  }
  else{
    printf("item not found ! \n");
    return NULL;
  }
}

void hash_delete(hash_table *table, const char *str){
  hash_item *item = search(table, str);
  if(item == NULL){
    printf("Nothing to free!\n");
    return;
  }
  free(item->key);
  free(item);
  table->used--;
  printf("item has been freed");
}

void file_test(hash_table *table, FILE *fptr){
  char word[20];
  while (fscanf(fptr, "%19s", word) == 1) {
    incert(table, word, 1);
  }
  write_report(table);// collision_report file
}

// TO DO LIST:
// search funtion works only on first index but not collision_resolution shifting  
// delete depens on search so the same problem
// one more method which is by BUCKETS 

int main(){
  /*-----------OPTIONS-----------: 
  TYPE_MULTIPLICATION,
  TYPE_DIVISION,
  TYPE_MIDDLE,
  TYPE_FOLD,
  TYPE_COMPRESSION,*/
  METHOD = TYPE_DIVISION;// HASHING METHOD
  PROBING_METHOD = TYPE_FOLD; // double hashing setting 
  //...................................
  /*-----------------COLLISION_METHODS:
  TYPE_OPADDRESS_DOUBLE,
  TYPE_OPADDRESS_LINEAR,
  TYPE_OPADDRESS_QUADRATIC,
  TYPE_DIRECT_CHAIN,
  TYPE_DIRECT_COAL
   */
  COLLISION_METHOD = TYPE_DIRECT_COAL; // collision_resolution settings 


  hash_table *table = hash_table_init();
  FILE *test_file = fopen("test_input.txt", "r"); // input file put words in it 
  FILE *report = fopen("collision_report.txt", "a"); // output report
  if(test_file == NULL){
    perror("test file error");
    exit(1);
  }
  fprintf(report, "\n\nTest on Method Number: %i\n - - - - - - - - - - - - - -\n", METHOD);
  fclose(report);
  //input file reading func
  file_test(table, test_file);
  fclose(test_file);
 
  
return 0;
}
