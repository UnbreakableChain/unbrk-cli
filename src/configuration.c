#include <stdlib.h>
#include "configuration.h"



configuration new_configuration(){
	configuration config = {.chains_dir=NULL, .default_hard_color=NULL,
	                        .dates_color=NULL, .names_color=NULL,
	                        .ghost_color=NULL};

	for (int i=0; i < 256; i++)
		config.hard_colors[i] = NULL;

	return config;
}

void free_configuration(configuration* config){
	if (config->chains_dir != NULL) free(config->chains_dir);
	if (config->dates_color != NULL) free(config->dates_color);
	if (config->names_color != NULL) free(config->names_color);
	if (config->ghost_color != NULL) free(config->ghost_color);
	if (config->default_hard_color != NULL) free(config->default_hard_color);
	for (int i=0; i < 256; i++)
		if (config->hard_colors[i] != NULL) free(config->hard_colors[i]);
}
