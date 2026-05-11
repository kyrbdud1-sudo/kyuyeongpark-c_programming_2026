#ifndef DATA_LOADER_H
#define DATA_LOADER_H

#include <stddef.h>

typedef struct {
    char date[11];
    int year;
    int month;
    int day;
    double open;
    double high;
    double low;
    double close;
    unsigned long long volume;
} PriceData;

int parse_date(const char *date_text, int *year, int *month, int *day);
int load_csv(const char *filename, PriceData **out_data, size_t *out_count);
int validate_date_order(const PriceData *data, size_t count);
int compare_date_ymd(int y1, int m1, int d1, int y2, int m2, int d2);

#endif
