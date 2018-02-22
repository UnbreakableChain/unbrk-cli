#pragma once
#include <stdbool.h>
#include "ub.h"
#include "unbrk.yucc"
#include "configuration.h"

#define UBCLI_INI_FILE "./tests/test_files/unbrk.ini"



int inih_handler(void* user, const char* section, const char* name,
	         const char* value);
void complete_config(configuration* config);
int check(struct yuck_cmd_check_s* argp);
int see(struct yuck_cmd_see_s* argp);
void fill_tm(struct tm* tm, int year, int mon, int day);
int cmp_tm(struct tm tm1, struct tm tm2);
void add_days(struct tm* tm, int days);
char week_day_letter(int wday);
void link_color(char** color, UbLink link);
int merge(struct yuck_cmd_merge_s* argp);
int stats(struct yuck_cmd_stats_s* argp);
bool isnumber(char* str);
bool isdate(char* str);
int open_chain_files(FILE** chain_files, char** names, size_t n);
void close_files(FILE** files, size_t n);
int read_chains(UbChain** chains, FILE** chain_files, char** names, size_t n);
