#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "unbrk.h"
#include "inih/ini.h"



configuration config;

int main(int argc, char** argv){
	config = new_configuration();
	if (ini_parse(UBCLI_INI_FILE, inih_handler, &config) < 0){
		fprintf(stderr, "Can't load " UBCLI_INI_FILE "\n");
		return 1;
	}
	complete_config(&config);

	yuck_t argp[1];
	yuck_parse(argp, argc, argv);
	switch (argp->cmd){
		case UNBRK_CMD_CHECK: return check((struct yuck_cmd_check_s*)argp);
		case UNBRK_CMD_SEE: return see((struct yuck_cmd_see_s*)argp);
		case UNBRK_CMD_STATS: return stats((struct yuck_cmd_stats_s*)argp);
		case UNBRK_CMD_MERGE: return merge((struct yuck_cmd_merge_s*)argp);
		case UNBRK_CMD_NEW: return new((struct yuck_cmd_new_s*)argp);
	}

	yuck_free(argp);
	free_configuration(&config);
	return 0;
}


int inih_handler(void* user, const char* section,
		 const char* name, const char* value){
	configuration* config = (configuration*)user;

	#define SECTION(s) strcmp(section, s) == 0
	#define MATCH(s, n)  SECTION(s) && strcmp(name, n) == 0
	if(MATCH("chainfiles", "chains_dir")){
		config->chains_dir = strdup(value);
	} else if (MATCH("color", "dates")){
		config->dates_color = strdup(value);
	} else if(MATCH("color", "names")){
		config->names_color = strdup(value);	
	} else if (MATCH("color", "default")){
		config->default_hard_color = strdup(value);
	} else if (MATCH("color", "ghost")){
		config->ghost_color = strdup(value);
	} else if (SECTION("color")){
		int hardness = atoi(name);
		if (hardness <= 0 || hardness >= 256) return 0;
		config->hard_colors[hardness] = strdup(value);
	}

	return 1;
}


void complete_config(configuration* config){
	// Default directory to look for the chains
	if (config->chains_dir == NULL){
		#ifndef UBCLI_DEFAULT_CHAINS_DIR
		#define UBCLI_DEFAULT_CHAINS_DIR "/etc/unbrk/chains/"
		#endif
		config->chains_dir = strdup(UBCLI_DEFAULT_CHAINS_DIR);
	}

	// Adds a / at the end of chains_dir
	int dir_len = strlen(config->chains_dir);
	if (config->chains_dir[dir_len-1] != '/'){
		char* dir = config->chains_dir;
		config->chains_dir = malloc(sizeof(char) * (dir_len+2));
		sprintf(config->chains_dir, "%s/", dir);
		free(dir);
	}

	if (config->dates_color == NULL)
		config->dates_color = strdup("");

	if (config->names_color == NULL)
		config->names_color = strdup("");

	if (config->ghost_color == NULL)
		config->ghost_color = strdup("");

	if (config->default_hard_color == NULL)
		config->default_hard_color = strdup("");

	for (int i=0; i < 256; i++)
		if (config->hard_colors[i] == NULL)
			config->hard_colors[i] = strdup(config->default_hard_color);
}


int check(struct yuck_cmd_check_s* argp){
	uint8_t hardness;
	if (argp->hardness_arg == NULL){
		hardness = 1;
	}else if (isnumber(argp->hardness_arg)){
		int h = atoi(argp->hardness_arg);
		if (h >= 0 && h <= 255){
			hardness = (uint8_t)h;
		} else {
			fprintf(stderr, "not 0-255\n");
		}
	} else {
		fprintf(stderr, "h no number\n");
		return -1;
	}

	UbDate date;
	if (argp->date_arg == NULL){
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		date = tm_to_UbDate(&tm);
	} else if (isdate(argp->date_arg)){
		int year, mon, day;
		sscanf(argp->date_arg, "%d-%d-%d", &year, &mon, &day);
		date = (UbDate){.year=year, .mon=mon, .day=day};
	} else {
		fprintf(stderr, "d no date\n");
		return -1;
	}

	uint8_t flags = 0;
	if (argp->ghost_flag){
		flags = UB_GHOST_LINK;
	}

	if (argp->nargs == 0){
		fprintf(stderr, "no chains\n");
		return -1;
	}

	FILE* chain_files[argp->nargs];
	if (open_chain_files(chain_files, argp->args, argp->nargs) != 0) return -1;
	UbChain* chains[argp->nargs];
	if (read_chains(chains, chain_files, argp->args, argp->nargs) != 0){
		close_files(chain_files, argp->nargs);
		return -1;
	}

	UbLink link = {.hardness=hardness, .flags=flags};
	for (size_t i=0; i < argp->nargs; i++){
		add_UbLink(chains[i], link, date);
		fseek(chain_files[i], 0, SEEK_SET);
		enum UbIOErr err = fwrite_UbChain(chains[i], chain_files[i]);
		switch (err){
			case UB_IO_WERR: fprintf(stderr, "err writing %s file\n",
						 argp->args[i]);
					 break;
		}
	}

	close_files(chain_files, argp->nargs);
	for (size_t i=0; i < argp->nargs; i++) free_UbChain(chains[i]);
	return 0;
}


int see(struct yuck_cmd_see_s* argp){
	struct tm since;
	if (argp->since_arg == NULL){
		time_t t = time(NULL);
		since = *localtime(&t);
		add_days(&since, -6);
	} else if (isdate(argp->since_arg)){
		int year, mon, day;
		sscanf(argp->since_arg, "%d-%d-%d", &year, &mon, &day);
		fill_tm(&since, year, mon, day);
	} else {
		fprintf(stderr, "since no date\n");
		return -1;
	}

	struct tm to;
	if (argp->to_arg == NULL){
		time_t t = time(NULL);
		to = *localtime(&t);
	} else if (isdate(argp->to_arg)){
		int year, mon, day;
		sscanf(argp->to_arg, "%d-%d-%d", &year, &mon, &day);
		fill_tm(&to, year, mon, day);
		if (cmp_tm(since, to) > 0){
			fprintf(stderr, "since must be lower than to\n");
			return -1;
		}
	} else {
		fprintf(stderr, "to no date\n");
		return -1;
	}


	FILE* chain_files[argp->nargs];
	if (open_chain_files(chain_files, argp->args, argp->nargs) != 0) return -1;

	int chains_num = argp->nargs;
	if (argp->input_flag) chains_num++;
	UbChain* chains[chains_num];
	if (argp->input_flag){
		enum UbIOErr err = fread_UbChain(&(chains[chains_num-1]), stdin);
		if (err != UB_IO_OK){
			fprintf(stderr, "Error reading the standard input chain\n");
			return -1;
		}
	}
	if (read_chains(chains, chain_files, argp->args, argp->nargs) != 0){
		close_files(chain_files, argp->nargs);
		return -1;
	}

	for (size_t i=0; i < chains_num; i++){
		printf("%s%s\x1b[0m\n\n", config.names_color, chains[i]->name);
		struct tm tm = since;
		while (cmp_tm(tm, to) <= 0){
			int j;
			printf("%s", config.dates_color);
			for (j=0; j < 7 && cmp_tm(tm, to) <= 0; j++){
				printf(" %c", week_day_letter(tm.tm_wday));
				if (tm.tm_mday < 10) printf("0");
				printf("%d", tm.tm_mday);
				add_days(&tm, 1);
			}	
			printf("\x1b[0m\n");
			add_days(&tm, -j);

			for (int k=0; k < j; k++){
				UbLink l = get_UbLink(chains[i], tm_to_UbDate(&tm));
				printf("-");
				if ((l.flags & UB_UNINIT_LINK) == 0 &&
			    	    (l.hardness > 0 || (l.flags & UB_GHOST_LINK) != 0)){
					char* color;
					link_color(&color, l);
					printf("%s(%d)\x1b[0m", color, l.hardness);
					free(color);
				} else {
					printf("   ");
				}
				add_days(&tm, 1);
			}
			printf("-\n");
		}
		printf("\n\n");
	}

	close_files(chain_files, argp->nargs);
	for (size_t i=0; i < chains_num; i++) free_UbChain(chains[i]);
	return 0;
}


void fill_tm(struct tm* tm, int year, int mon, int day){
	*tm = (struct tm){
		.tm_sec=0,
		.tm_min=0,
		.tm_hour=0,
		.tm_mday=day,
		.tm_mon=mon-1,
		.tm_year=year-1900
	};

	// fills tm_wday with the correct value
	time_t time = mktime(tm);
	*tm = *localtime(&time);
}


int cmp_tm(struct tm tm1, struct tm tm2){
	time_t time1 = mktime(&tm1);
	time_t time2 = mktime(&tm2);

	return (time1 > time2) - (time1 < time2);
}


void add_days(struct tm* tm, int days){
	time_t time = mktime(tm) + (days * 24 * 60 * 60);
	*tm = *localtime(&time);
}


char week_day_letter(int wday){
	switch(wday){
		case 0: return 'S';
		case 1: return 'M';
		case 2: return 'T';
		case 3: return 'W';
		case 4: return 'T';
		case 5: return 'F';
		case 6: return 'S';
		default: return '?';
	} 
}


void link_color(char** color, UbLink link){
	if (link.flags & UB_GHOST_LINK != 0){
		*color = strdup(config.ghost_color);
	} else {
		*color = strdup(config.hard_colors[link.hardness]);
	}
}


int merge(struct yuck_cmd_merge_s* argp){
	int chains_num = (argp->nargs)-1;
	FILE* chain_files[chains_num];
	if (open_chain_files(chain_files, argp->args+1, chains_num) != 0) return -1;

	UbChain* chains[chains_num+1];
	chains[chains_num] = NULL;
	if (read_chains(chains, chain_files, argp->args+1, chains_num) != 0){
		close_files(chain_files, chains_num);
		return -1;
	}

	UbChain* merged = merge_UbChain_v(chains, argp->args[0]);
	fwrite_UbChain(merged, stdout);		

	free_UbChain(merged);
	for (size_t i=0; i < chains_num; i++) free_UbChain(chains[i]);

	return 0;
}

int stats(struct yuck_cmd_stats_s* argp){
	FILE* chain_files[argp->nargs];
	if (open_chain_files(chain_files, argp->args, argp->nargs) != 0) return -1;

	UbChain* chains[argp->nargs];
	if (read_chains(chains, chain_files, argp->args, argp->nargs) != 0){
		close_files(chain_files, argp->nargs);
		return -1;
	}

	for (size_t i=0; i < argp->nargs; i++){
		printf("%s\n\n", chains[i]->name);
		printf("number of links: %d\n", ub_links_number(chains[i]));
		printf("number of fragments: %d\n", ub_fragments_number(chains[i]));
		printf("current fragment size: %d\n", ub_current_fragment_size(chains[i]));
		printf("largest framgent size: %d\n", ub_largest_fragment_size(chains[i]));	
		printf("frament size mean: %.2f\n", ub_fragment_size_mean(chains[i]));
		printf("hardest link: %d\n", ub_hardest_link(chains[i]));
		printf("hardness mean: %.2f\n", ub_hardness_mean(chains[i]));
		printf("ghost links ratio: %.2f%\n", ub_ghost_links_ratio(chains[i])*100);
		printf("\n\n");
	}

	close_files(chain_files, argp->nargs);
	for (size_t i=0; i < argp->nargs; i++) free_UbChain(chains[i]);
	return 0;
}


int new(struct yuck_cmd_new_s* argp){
	for (size_t i=0; i < argp->nargs; i++){
		UbChain* chain = new_UbChain(argp->args[i]);
		char* file_name = malloc(sizeof(char) *
				         (strlen(config.chains_dir) +
					  strlen(argp->args[i]) + 1));
		strcpy(file_name, config.chains_dir);
		strcat(file_name, argp->args[i]);
		FILE* f = fopen(file_name, "w");
		free(file_name);
		fwrite_UbChain(chain, f);
		fclose(f);
		free_UbChain(chain);
	}
	if (argp->input_flag){
		UbChain* chain;
		enum UbIOErr err = fread_UbChain(&chain, stdin);
		if (err != UB_IO_OK){
			fprintf(stderr, "Error reading the standard input chain\n");
			return -1;
		}
		char* file_name = malloc(sizeof(char) *
				         (strlen(config.chains_dir) +
					  strlen(chain->name) + 1));
		strcpy(file_name, config.chains_dir);
		strcat(file_name, chain->name);
		FILE* f = fopen(file_name, "w");
		free(file_name);
		fwrite_UbChain(chain, f);
		fclose(f);
		free_UbChain(chain);
	}


	return 0;
}


bool isnumber(char* str){
	bool is_number = true;

	for (int i=0; is_number && str[i] != '\0'; i++){
		is_number = is_number && isdigit(str[i]);
	}

	return is_number;
}


bool isdate(char* str){
	int year, mon, day;
	bool is_date = sscanf(str, "%d-%d-%d", &year, &mon, &day) == 3;
	return is_date;
}


int open_chain_files(FILE** chain_files, char** names, size_t n){
	for (size_t i=0; i < n; i++){
		char* name = malloc(sizeof(char) * (strlen(config.chains_dir)+strlen(names[i])));
		strcpy(name, config.chains_dir);
		chain_files[i] = fopen(strcat(name, names[i]), "r+");
		if (chain_files[i] == NULL){
			fprintf(stderr, "can't open %s file\n", names[i]);
			close_files(chain_files, i-1);
			return -1;
		}
		free(name);
	}

	return 0;
}


void close_files(FILE** files, size_t n){
	for (size_t i=0; i < n; i++){
		fclose(files[i]);
	}
}


int read_chains(UbChain** chains, FILE** chain_files, char** names, size_t n){
	for (size_t i=0; i < n; i++){
		enum UbIOErr err = fread_UbChain(&(chains[i]), chain_files[i]);
		switch (err){
			case UB_IO_RERR: fprintf(stderr, "err reading %s file\n",
						 names[i]);
					 return -1;
			case UB_IO_PERR: fprintf(stderr, "err parsing %s file\n",
						 names[i]);
					 return -1;
		}
	}

	return 0;
}
