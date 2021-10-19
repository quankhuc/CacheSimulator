#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

/*
 *each cache has 2^s sets which are linked like linked list.
 *each set has E lines which are linked like linked list.
 *each line has a valid bit and a tag and an address
 *a line in cache is the fundamental part of the cache
 *the address has tag, set index (don't need block offset)
*/

/*
 *define global variables to calculate or return in the later functions
 */
int hit = 0;
int miss = 0;
int evict = 0;
int dirty_bytes_evicted = 0;
int dirty_bytes_active = 0;
int double_refs = 0;
int s,E,b,t;
const int memory_address = sizeof(long)*8;
int h_flag = 0;
int v_flag = 0;
char L,S,M;


/*
 *define the data structure of a line in cache as linked list
*/

struct Line
{
    int valid;
    unsigned tag;
    int dirty_bit;
    struct Line *next; //address of the next node
};

/*
 *define the data structure of a set in cache as linked list
 */

struct Set
{
    int E;
    struct Line *head_line; //address of the head node of the line
    struct Set *next; //address of the next node   
};

/*
 * Implement the cache as a linked list that the matched node is the MRU node and the last node is LRU node
 * Therefore, if I want to implement LRU policy, I just need to remove the last node
 */

struct Cache
{
    int S;
    struct Set *head_set; //address of the head node of the set
};


//initialize a set with the capacity of E number of lines
void initialize_set(struct Set *set, int E)
{
    set->E = E;
    set->head_line = NULL;
    set->next = NULL;
}

//initialize cache with 2^s empty sets

void initialize_cache(struct Cache* cache, int s, int E)
    {
        cache -> S = (1 << s);
        struct Set* current_set = malloc(sizeof(struct Set));
        initialize_set(current_set,E); //need to initialize the head_set before going through the for-loop
        cache -> head_set = current_set;
        for(int i=0; i < cache -> S - 1; ++i){
            struct Set *next_set = malloc(sizeof(struct Set));
            initialize_set(next_set, E);
            current_set -> next = next_set;
            current_set = next_set;
        }
    }

//calculate number of valid lines in a set. Will use it to determine when the set is full

int set_size(struct Set *set)
{
    struct Line *current_line = set -> head_line; //starting from the head node of the line
    int size = 0;
    while (current_line != NULL)
    {
        ++size;
        current_line = current_line -> next;
    }
    return size;
}

//create a function to evict the last line. Need to have access to 3 lines which are the null node, MRU node and LRU node
void evict_last_line(struct Set* set){
    struct Line *current_line = set -> head_line;
    struct Line *previous_line = NULL;
    struct Line *previous_line_1 = NULL;
    while(current_line != NULL){
        previous_line_1 = previous_line;
        previous_line = current_line;
        current_line = current_line -> next;
    }
    if(previous_line_1 != NULL){
        previous_line_1-> next = NULL;} //it is the last line so need to assign the next address to point to a NULL
    else{
        set -> head_line = NULL;}
    if(previous_line -> dirty_bit)
        dirty_bytes_evicted++;
    if(previous_line != NULL)
        free(previous_line);
}


//add a valid line to the head of a set (only if the set is still available), evict the line when the set is over its capacity
void add_new_line_to_head(struct Line* line, struct Set* set){
    //checking if the set is full, if the set is full then evict the current line of the set and increase the eviction count
    if(set_size(set) == set -> E){
            evict_last_line(set);
            evict++;
    }
    line -> next = set -> head_line; //address of the next line points to the head_line
    set -> head_line = line; //update the line to be the head_line of the set
}

//a helper function to move the matched line to head (MRU node)
void move_match_line_to_head(struct Set* set, struct Line* current_line, struct Line* previous_line){
    if(previous_line != NULL){
        previous_line -> next = current_line -> next;
        current_line -> next = set-> head_line;
        set-> head_line = current_line;
    }
} 

//cache operation helper function
void access_cache(struct Cache* cache, char operation, unsigned long address)
{
    int tag_bits = address >> (s + b);
    int set_index = (address << t) >> (t + b);
    //check if it is in the "L" operation and "S" operation 
    //how to get the correct set
    struct Set* current_set = cache -> head_set;
    for (int i = 0; i < set_index; ++i)
    {
        current_set = current_set -> next; //access each set
    }
    //need to check for the line for a hit
    struct Line* current_line = current_set -> head_line; // access the first line of the set
    struct Line* previous_line = NULL;
    while (current_line != NULL){
        if((current_line -> tag == tag_bits) && (current_line -> valid)){
            hit++;
           // move_match_line_to_head(current_set, current_line, previous_line);
            if(operation == 'S')
                current_line -> dirty_bit = 1; // it is a store operation so still need to change the dirty bit to 1
            if(current_line -> tag == current_set -> head_line -> tag) //if the current line -> dirty bit just recently changed to 1 then the if statement could be wrong
                double_refs++; 
            move_match_line_to_head(current_set, current_line, previous_line);
            return; //get out of the loop and increment hit
        }
        previous_line = current_line;
        current_line = current_line -> next;
    }
    miss++;
    struct Line* new_line = malloc(sizeof(struct Line)); //create a new line
    new_line -> valid = 1;
    new_line -> tag = tag_bits;
    if(operation == 'S'){
        new_line -> dirty_bit = 1;}
    else{
        new_line -> dirty_bit = 0;
    }
    //a function to write a line to the head_line which is the MRU node
    add_new_line_to_head(new_line,current_set);    
}

//a helper function to count how many dirty bytes active at the end of the simulation
void count_dirty_bytes_active(struct Cache* cache){
    struct Set* current_set = cache -> head_set;
    while(current_set != NULL){
        struct Line* current_line = current_set -> head_line;// start from the head line
        //current_set = current_set -> next; //moving to the next set until reaching null
        while(current_line != NULL){
            if(current_line -> dirty_bit)
                ++dirty_bytes_active;
            current_line = current_line -> next; //moving to the next line until reaching null
        
        } 
        current_set = current_set -> next; //moving to the next set until reaching null
    }
}

//free cache, free every set, free every line
void free_cache(struct Cache* cache){
    //start with clearing the head set in cache
    struct Set* free_this_set = cache -> head_set;
    //clear all set except the head set then clear the head set
    while(!free_this_set){
        struct Line* free_this_line = free_this_set-> head_line;
        //clear all line except the head line then clear the head line
        while(!free_this_line){
            struct Line* free_other_lines = free_this_line -> next;
            free(free_other_lines);
            free_this_line = free_other_lines; //traverse through the linked list
        }
        //using the same logic to clear all lines to clear all sets
        struct Set* free_other_sets = free_this_set -> next;
        free(free_other_sets);
        free_this_set = free_other_sets;
    }
}


int main(int argc, char*argv[])
{
    int opt;
    FILE* tracefile;
    char* trace;

    /* parse flag commands by using getopt() */
    while((opt = getopt(argc,argv, "hvs:E:b:t:")) != -1) //":" specify an argument expected after the flag
        switch (opt)
        {
            case 'h':
                h_flag = 1;
                break;
            case 'v':
                v_flag = 1;
                break;   
            case 's':
                sscanf(optarg, "%d", &s);//optarg is a char* pointing to the value of the option argument, can use sscanf 
                //printf("this is the value of s: %d",s);
                break;
            case 'E':
                sscanf(optarg, "%d", &E);
                //printf("this is the value of E: %d", E);
                break;
            case 'b':
                sscanf(optarg, "%d", &b);
                //printf("This is the value of b: %d", b);
                break;
            case 't':
                trace = optarg;
                tracefile = fopen(trace, "r");
                //printf("It is reading the tracefile");
                if(!tracefile)
                {
                    printf("Error: Can't open the file or the file does not exist");
                    exit(-1);
                }
                break;
            default:
                break;
        }
    struct Cache *my_cache = malloc(sizeof(struct Cache));
    initialize_cache(my_cache,s,E);

//reading through each line by fscanf. A line is composed of an operation, operation address, size
//convert the operation address into set index and tag
    t = memory_address  - s - b;
    char operation;
    unsigned long operation_address;
    int size;
    while(fscanf(tracefile,"%c" "%lx" "%d", &operation, &operation_address, &size) > 0){
            switch(operation){
                case 'I':
                    break;
                case 'L':
                    //need a function to read and access something from "my cache" and "operation_address"
                    access_cache(my_cache, 'L', operation_address);
                    break;
                case 'S':
                    access_cache(my_cache, 'S', operation_address);
                    break;
                case 'M':
                    access_cache(my_cache, 'L', operation_address);
                    access_cache(my_cache, 'S', operation_address); //modify contains both load and store so repeat the process to access cache twice
                    break;
                default:
                    break;
            } 
            if(h_flag){
                printf("there is no usage info file.\n");
            }
            if(v_flag){
                printf("%c" "%lx" "%d", operation, operation_address, size);
            }

    }
    count_dirty_bytes_active(my_cache);
    fclose(tracefile);
    free_cache(my_cache);
    printSummary(hit, miss, evict,(1 << b)*dirty_bytes_evicted,(1 << b)*dirty_bytes_active,double_refs);
    return 0;
}
