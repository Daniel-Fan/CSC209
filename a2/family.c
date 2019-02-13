#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "family.h"

/* Number of word pointers allocated for a new family.
   This is also the number of word pointers added to a family
   using realloc, when the family is full.
*/
static int family_increment = 0;


/* Set family_increment to size, and initialize random number generator.
   The random number generator is used to select a random word from a family.
   This function should be called exactly once, on startup.
*/
void init_family(int size) {
    family_increment = size;
    srand(time(0));
}


/* Given a pointer to the head of a linked list of Family nodes,
   print each family's signature and words.

   Do not modify this function. It will be used for marking.
*/
void print_families(Family* fam_list) {
    int i;
    Family *fam = fam_list;
    
    while (fam) {
        printf("***Family signature: %s Num words: %d\n",
               fam->signature, fam->num_words);
        for(i = 0; i < fam->num_words; i++) {
            printf("     %s\n", fam->word_ptrs[i]);
        }
        printf("\n");
        fam = fam->next;
    }
}


/* Return a pointer to a new family whose signature is 
   a copy of str. Initialize word_ptrs to point to 
   family_increment+1 pointers, numwords to 0, 
   maxwords to family_increment, and next to NULL.
*/
Family *new_family(char *str) {
    Family *new_fam = malloc(sizeof(Family));
    new_fam->signature = malloc((strlen(str) + 1) * sizeof(char));
    strcpy(new_fam->signature, str);
    new_fam->word_ptrs = malloc((family_increment + 1) * sizeof(char *));
    new_fam->word_ptrs[family_increment] = NULL;
    new_fam->num_words = 0;
    new_fam->max_words = family_increment;
    new_fam->next = NULL;
    return new_fam;
}


/* Add word to the next free slot fam->word_ptrs.
   If fam->word_ptrs is full, first use realloc to allocate family_increment
   more pointers and then add the new pointer.
*/
void add_word_to_family(Family *fam, char *word) {
    if(fam->max_words == fam->num_words){
        fam->max_words += family_increment;
        fam->word_ptrs = realloc(fam->word_ptrs, (fam->max_words + 1) * sizeof(char *));
    }
    fam->word_ptrs[fam->num_words] = word;
    fam->word_ptrs[fam->num_words + 1] = NULL;
    fam->num_words++;
}


/* Return a pointer to the family whose signature is sig;
   if there is no such family, return NULL.
   fam_list is a pointer to the head of a list of Family nodes.
*/
Family *find_family(Family *fam_list, char *sig) {
    Family *found_fam = NULL;
    while(fam_list != NULL){
        if(strcmp(fam_list->signature, sig) == 0){
            found_fam = fam_list;
            break;
        }
        else{
            fam_list = fam_list->next;
        }
    }
    return found_fam;
}


/* Return a pointer to the family in the list with the most words;
   if the list is empty, return NULL. If multiple families have the most words,
   return a pointer to any of them.
   fam_list is a pointer to the head of a list of Family nodes.
*/
Family *find_biggest_family(Family *fam_list) {
    Family *biggest_fam = NULL;
    while(fam_list != NULL){
        if(biggest_fam == NULL){
            biggest_fam = fam_list;
        }
        else if(fam_list->num_words > biggest_fam->num_words){
            biggest_fam = fam_list;
        }
        fam_list = fam_list->next;
    }
    return biggest_fam;
}


/* Deallocate all memory rooted in the List pointed to by fam_list. */
void deallocate_families(Family *fam_list) {
    Family *next_fam;
    while(fam_list != NULL){
        next_fam = fam_list->next;
        free(fam_list->signature);
        free(fam_list->word_ptrs);
        free(fam_list);
        fam_list = next_fam;
    }
}


/* Generate and return a linked list of all families using words pointed to
   by word_list, using letter to partition the words.

   Implementation tips: To decide the family in which each word belongs, you
   will need to generate the signature of each word. Create only the families
   that have at least one word from the current word_list.
*/
Family *generate_families(char **word_list, char letter) {
    Family *fam_list = NULL;
    int j = 0;
    while(word_list[j] != NULL){
        int i = 0;
        int size = strlen(word_list[j]) + 1;
        char current_sig[size + 1];
        while(word_list[j][i] != '\0'){
            if(word_list[j][i] != letter){
                current_sig[i] = '-';
            }
            else{
                current_sig[i] = letter;
            }
            i++;
        }
        if(fam_list == NULL){
            fam_list = new_family(current_sig);
            add_word_to_family(fam_list, word_list[j]);
        }
        else if(find_family(fam_list, current_sig) == NULL){
            Family *head_fam = new_family(current_sig);
            add_word_to_family(head_fam, word_list[j]);
            head_fam->next = fam_list;
            fam_list = head_fam;
        }
        else{
            Family *current_fam = find_family(fam_list, current_sig);
            add_word_to_family(current_fam, word_list[j]);
        }
        j++;
    }
    return fam_list;
}


/* Return the signature of the family pointed to by fam. */
char *get_family_signature(Family *fam) {
    return fam->signature;
}


/* Return a pointer to word pointers, each of which
   points to a word in fam. These pointers should not be the same
   as those used by fam->word_ptrs (i.e. they should be independently malloc'd),
   because fam->word_ptrs can move during a realloc.
   As with fam->word_ptrs, the final pointer should be NULL.
*/
char **get_new_word_list(Family *fam) {
    int i = 0;
    char **words_list = NULL;
    words_list = malloc((fam->num_words + 1) * sizeof(char *));
    while(fam->word_ptrs[i] != NULL){
        words_list[i] = fam->word_ptrs[i];
        i++;
    }
    words_list[i] = NULL;
    return words_list;
}


/* Return a pointer to a random word from fam. 
   Use rand (man 3 rand) to generate random integers.
*/
char *get_random_word_from_family(Family *fam) {
    char *word = NULL;
    int position = rand() % fam->num_words;
    word = fam->word_ptrs[position];
    return word;
}
