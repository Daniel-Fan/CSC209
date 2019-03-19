#ifndef _HELPER_H
#define _HELPER_H

#define SIZE 44

struct rec {
    int freq;
    char word[SIZE];
};

int get_file_size(char *filename);
int compare_freq(const void *rec1, const void *rec2);
void merge(struct rec *smallest_rec, struct rec *source, int size, char *outfile, int *index);

#endif /* _HELPER_H */
