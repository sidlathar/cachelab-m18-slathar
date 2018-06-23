//Siddhanth Lathar @ slathar

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "cachelab.h"

struct line //one line
{
	int valid;
	unsigned long hit_freq;
	unsigned long tag;
	int dirty_bit;
	char *block;
};

struct set //lines make up a set
{
	struct line *lines;
};

struct cache //sets make up a cache
{
	struct set *sets;
};

struct params
{
	long E;
	long S;
	long B;

	long s;
	long b;

	long hits;
	long misses;
	long evictions;

	long bytes_evicted;
};

struct min_max_indices
{
	long min_freq;
	long max_freq;
	long min_index;
	long max_index;
};


void free_mem(struct cache cache_to_sim, long E, long S)
{
	int s;

	for(s = 0; s < S; s++)
	{
		struct set to_free = cache_to_sim.sets[s];
		if(to_free.lines != NULL)
		{
			free(to_free.lines);
		}
	}

	if(cache_to_sim.sets != NULL)
	{
		free(cache_to_sim.sets);
	}

	return;
}

struct cache make_cache(long n_E, long n_S)
{
	struct cache new_cache;
	struct set new_set;
	struct line new_line;

	long l,s;

	//alloc memory for cache sets
	new_cache.sets = (struct set*)malloc(sizeof(struct set)*n_S);

	for(s = 0; s < n_S; s++)
	{
		//alloc memory for cache lines
		new_set.lines = (struct line*)malloc(sizeof(struct line)*n_E);

		//set s-th set to have the above allocated lines
		new_cache.sets[s] = new_set;

		//initialize to defaults
		for(l = 0; l < n_E; l++)
		{
			new_line.valid = -1;
			new_line.tag = 0;
			new_line.dirty_bit = 0;
			new_line.hit_freq = 0;
			new_set.lines[l] = new_line;
		}
	}
	return new_cache;
}

long is_cache_full(struct cache cache_to_sim, unsigned long set_index, long E)
{
	long l; //line index
	struct set check_set = cache_to_sim.sets[set_index]; //set to check for capacity
	struct line check_line; //line to check if empty

	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if((check_line.valid == -1)) //not valid means empty
		{
			return l; //not full, return index
		}
	}
	return -1; //full
}

struct min_max_indices get_lru(struct cache cache_to_sim, unsigned long set_index, struct min_max_indices m_m_i, long E)
{
	long l = 0; //line index
	long min_freq = 0; 	//min hit_freq of lines
	long max_freq = 0; 	//max hit_freq of lines
	long max_index = 0; //line index of max_freq
	long min_index = 0; //line index of min_freq
	struct set check_set = cache_to_sim.sets[set_index]; //set to check
	struct line check_line; //line to check

	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if(check_line.hit_freq > max_freq)
		{
			max_freq = check_line.hit_freq;
			max_index = l;
		}
	}

	min_freq = max_freq + 1; //set it higher than max for fair comparision

	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if(check_line.hit_freq < min_freq)
		{
			min_freq = check_line.hit_freq;
			min_index = l;
		}
	}

	m_m_i.max_index = max_index;
	m_m_i.min_index = min_index;
	m_m_i.max_freq = max_freq;
	m_m_i.min_freq = min_freq;

	return m_m_i;
}

struct params sim_cache(struct cache cache_to_sim, struct params init_params, unsigned long in_addr, int write, int size)
{
	long E = init_params.E;
	long s = init_params.s;
	long b = init_params.b;

	unsigned long in_tag = in_addr >> (s + b); //tag to read/write at

	unsigned long set_index = (in_addr << (64 - s - b)) >> (64 - s); //set index to read/write at

	struct set check_set = cache_to_sim.sets[set_index]; //set to wrte in
	struct line check_line; 
	struct min_max_indices m_m_i; //struct containing max/min indices

	long l; //line index
	long cache_full;
	long old_hits = init_params.hits;
	long least_recent;
	long empty_line;


	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if((check_line.valid != -1) && (check_line.tag == in_tag)) //if tag is valid and matching its a hit
		{
			// if(check_line.hit_freq >= 1 && (write == 1)) //set dirty bit
			// {
			// 	check_line.dirty_bit = 1;
			// }
			init_params.hits = init_params.hits + 1;
			check_line.hit_freq = check_line.hit_freq + 1;
			check_set.lines[l] = check_line;
			return init_params;
		}
	}

	//no hit found, update miss count
	if(old_hits == init_params.hits)
	{
		init_params.misses = init_params.misses + 1;
	}

	cache_full = is_cache_full(cache_to_sim, set_index, E); //holds empty line index
	m_m_i = get_lru(cache_to_sim, set_index, m_m_i, E); //gets least recent used line index

	if(cache_full == -1)
	{
		//evict
		least_recent = m_m_i.min_index;
		if(write == 1)
		{
			check_set.lines[least_recent].dirty_bit = 1;
			init_params.bytes_evicted = init_params.bytes_evicted + init_params.B;	
		}
		//init_params.bytes_evicted = init_params.bytes_evicted + init_params.B;
		check_set.lines[least_recent].tag = in_tag;
		check_set.lines[least_recent].hit_freq = m_m_i.max_freq + 1; //make it most recent
		init_params.evictions = init_params.evictions + 1;
	}
	else
	{
		//cache is empty
		empty_line = cache_full;
		if(write == 1)
		{
			check_set.lines[empty_line].dirty_bit = 1;
		}
		check_set.lines[empty_line].tag = in_tag;
		check_set.lines[empty_line].valid = 1;
		check_set.lines[empty_line].hit_freq = m_m_i.max_freq + 1; //make it most recent
	}
	return init_params;
}


long count_dirty_bits(struct cache cache_to_sim, long E, long S)
{
	long l, s;
	int dirty_bit_count = 0;

	struct set check_set;

	for(s = 0; s < S; s++)
	{
		check_set = cache_to_sim.sets[s];
		for(l = 0; l < E; l++)
		{
			if(check_set.lines[l].dirty_bit == 1)
			{
				dirty_bit_count = dirty_bit_count + 1;
			}
		}
	}
	return dirty_bit_count;
}


int main(int argc, char **argv)
{
	struct params init_params;
	struct cache cache_to_sim;
	char *trace_file_path;
	int write;
	int dirty_bit_count = 0;

	int opt;
	//int report;
	
	unsigned long in_addr;

	FILE *trace;

	while(-1 != (opt = getopt(argc, argv, "s:E:b:t:")))  //taken from rec slides s15
	{
		switch(opt)
		{
			case 's':
				init_params.s = atoi(optarg);
				break;
			case 'E':
				init_params.E = atoi(optarg);
				break;
			case 'b':
				init_params.b = atoi(optarg);
				break;
			case 't':
				trace_file_path = optarg;
				break;
			default:
				printf("wrong args\n");
				break;
		}
	}

	init_params.hits = 0;
	init_params.misses = 0;
	init_params.evictions = 0;
	init_params.bytes_evicted = 0;
	init_params.S = pow(2.0, init_params.s);
	init_params.B = pow(2.0, init_params.b);

	cache_to_sim = make_cache(init_params.E, init_params.S);

	char type;
	int size;

	trace = fopen(trace_file_path, "r");
	
	if(trace != NULL)
	{
		while(fscanf(trace, " %c %lx,%d", &type, &in_addr, &size) == 3)
		{
			switch(type)
			{
				case 'L':
					write = 0;
					init_params = sim_cache(cache_to_sim, init_params, in_addr, write, size);
					break;
				case 'S':
					write = 1;
					init_params = sim_cache(cache_to_sim, init_params, in_addr, write, size);
					break;
				default:
					break;
			}
		}
	}

	dirty_bit_count = count_dirty_bits(cache_to_sim, init_params.E, init_params.S);

	printSummary(init_params.hits, init_params.misses, init_params.evictions, init_params.B*dirty_bit_count, init_params.bytes_evicted);

	free_mem(cache_to_sim, init_params.E, init_params.S);
	fclose(trace);


	return 0;
}