#pragma once
#include <stdbool.h>



typedef struct{
	char* chains_dir;
	char* dates_color;
	char* names_color;
	char* ghost_color;
	char* hard_colors[256];
	char* default_hard_color;
} configuration;

configuration new_configuration();
void free_configuration(configuration* config);
