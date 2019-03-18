#ifndef _HELPER_H
#define _HELPER_H

#define SIZE 44

struct rec {
    int freq;
    char word[SIZE];
};

int get_file_size(char *filename);
int compare_freq(const void *rec1, const void *rec2);
int *divide_records(int num_chunks, int num_records);
int merge(struct rec *source, int size, char *outfile, int num_times);

#endif /* _HELPER_H */
