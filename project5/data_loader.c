#include "data_loader.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_date(const char *date_text, int *year, int *month, int *day)
{
    char extra;

    if (date_text == NULL || year == NULL || month == NULL || day == NULL) {
        return 0;
    }

    if (sscanf(date_text, "%4d-%2d-%2d%c", year, month, day, &extra) != 3) {
        return 0;
    }

    if (*year < 1900 || *month < 1 || *month > 12 || *day < 1 || *day > 31) {
        return 0;
    }

    return 1;
}

static int parse_double_field(const char *text, double *out)
{
    char *end = NULL;

    if (text == NULL || *text == '\0') {
        return 0;
    }

    errno = 0;
    *out = strtod(text, &end);
    return errno == 0 && end != text && *end == '\0';
}

static int parse_ull_field(const char *text, unsigned long long *out)
{
    char *end = NULL;

    if (text == NULL || *text == '\0') {
        return 0;
    }

    errno = 0;
    *out = strtoull(text, &end, 10);
    return errno == 0 && end != text && *end == '\0';
}

static void trim_newline(char *text)
{
    size_t len;

    if (text == NULL) {
        return;
    }

    len = strlen(text);
    while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
        text[len - 1] = '\0';
        len--;
    }
}

int load_csv(const char *filename, PriceData **out_data, size_t *out_count)
{
    enum { LINE_SIZE = 1024 };
    FILE *fp;
    char line[LINE_SIZE];
    size_t capacity = 20000;
    size_t count = 0;
    size_t line_number = 0;
    PriceData *data;

    if (filename == NULL || out_data == NULL || out_count == NULL) {
        return 0;
    }

    *out_data = NULL;
    *out_count = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open CSV file: %s\n", filename);
        return 0;
    }

    /* The initial capacity already covers the current files and still grows safely if needed. */
    data = (PriceData *)malloc(capacity * sizeof(PriceData));
    if (data == NULL) {
        fclose(fp);
        fprintf(stderr, "Failed to allocate memory for CSV data.\n");
        return 0;
    }

    if (fgets(line, sizeof(line), fp) == NULL) {
        free(data);
        fclose(fp);
        fprintf(stderr, "CSV file is empty: %s\n", filename);
        return 0;
    }
    line_number++;
    trim_newline(line);
    if (strcmp(line, "Date,Open,High,Low,Close,Volume") != 0) {
        fprintf(stderr, "Unexpected CSV header in %s: %s\n", filename, line);
        free(data);
        fclose(fp);
        return 0;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *fields[6];
        char *token;
        int field_count = 0;
        PriceData row;

        line_number++;
        trim_newline(line);
        if (line[0] == '\0') {
            continue;
        }

        /* The input CSV has no quoted fields, so simple comma tokenizing is sufficient here. */
        token = strtok(line, ",");
        while (token != NULL && field_count < 6) {
            fields[field_count++] = token;
            token = strtok(NULL, ",");
        }

        if (field_count != 6 || token != NULL) {
            fprintf(stderr, "Skipping malformed CSV row %zu in %s.\n", line_number, filename);
            continue;
        }

        memset(&row, 0, sizeof(row));
        if (!parse_date(fields[0], &row.year, &row.month, &row.day) ||
            !parse_double_field(fields[1], &row.open) ||
            !parse_double_field(fields[2], &row.high) ||
            !parse_double_field(fields[3], &row.low) ||
            !parse_double_field(fields[4], &row.close) ||
            !parse_ull_field(fields[5], &row.volume)) {
            fprintf(stderr, "Skipping row %zu with invalid field in %s.\n", line_number, filename);
            continue;
        }

        strncpy(row.date, fields[0], sizeof(row.date) - 1);

        if (count == capacity) {
            PriceData *grown;
            capacity *= 2;
            grown = (PriceData *)realloc(data, capacity * sizeof(PriceData));
            if (grown == NULL) {
                free(data);
                fclose(fp);
                fprintf(stderr, "Failed to grow memory for CSV data.\n");
                return 0;
            }
            data = grown;
        }

        data[count++] = row;
    }

    fclose(fp);

    if (count == 0) {
        free(data);
        fprintf(stderr, "No valid data rows found in %s.\n", filename);
        return 0;
    }

    *out_data = data;
    *out_count = count;
    return 1;
}

int validate_date_order(const PriceData *data, size_t count)
{
    size_t i;

    if (data == NULL) {
        return 0;
    }

    for (i = 1; i < count; i++) {
        if (compare_date_ymd(data[i - 1].year, data[i - 1].month, data[i - 1].day,
                             data[i].year, data[i].month, data[i].day) >= 0) {
            fprintf(stderr, "Date order error near %s and %s.\n", data[i - 1].date, data[i].date);
            return 0;
        }
    }

    return 1;
}

int compare_date_ymd(int y1, int m1, int d1, int y2, int m2, int d2)
{
    if (y1 != y2) return (y1 < y2) ? -1 : 1;
    if (m1 != m2) return (m1 < m2) ? -1 : 1;
    if (d1 != d2) return (d1 < d2) ? -1 : 1;
    return 0;
}
