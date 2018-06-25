//Siddhanth Lathar @ slathar

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "cachelab.h"

struct line /* one line */
{
	int valid;
	unsigned long hit_freq;
	unsigned long tag;
	int dirty_bit;
	char *block;
};

struct set /* lines make up a set */
{
	struct line *lines;
};

struct cache /* sets make up a cache */
{
	struct set *sets;
};

struct params /* struct to hold cache info */
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
	long dirty_bytes;
};

struct min_max_indices /* struct to hold lry info */
{
	long min_freq;
	long max_freq;
	long min_index;
	long max_index;
};


void free_mem(struct cache cache_to_sim, long E, long S)
{
	int s; /* set index */

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

	long l,s; /* lind and set indices */

	/* alloc memory for cache sets */
	new_cache.sets = (struct set*)malloc(sizeof(struct set)*n_S);

	for(s = 0; s < n_S; s++)
	{
		/* alloc memory for cache lines */
		new_set.lines = (struct line*)malloc(sizeof(struct line)*n_E);

		/* set s-th set to have the above allocated lines */
		new_cache.sets[s] = new_set;

		/* initialize to defaults */
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
	long l; /* line index */
	struct set check_set = cache_to_sim.sets[set_index]; /* set to check for capacity */
	struct line check_line; /* line to check if empty */

	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if((check_line.valid == -1)) /* not valid means empty */
		{
			return l; /* not full, return line index */
		}
	}
	return -1; /* full */
}

struct min_max_indices get_lru(struct cache cache_to_sim, unsigned long set_index, struct min_max_indices m_m_i, long E)
{
	long l = 0; /* line index */
	long min_freq = 0; 	/* min hit_freq of lines */
	long max_freq = 0; 	/* max hit_freq of lines */
	long max_index = 0; /* line index of max_freq */
	long min_index = 0; /* line index of min_freq */
	struct set check_set = cache_to_sim.sets[set_index]; /* set to check */
	struct line check_line; /* line to check */

	for(l = 0; l < E ; l++) /* get max index and freq */
	{
		check_line = check_set.lines[l];

		if(check_line.hit_freq > max_freq)
		{
			max_freq = check_line.hit_freq;
			max_index = l;
		}
	}

	min_freq = max_freq + 1; /* set it higher than max for fair comparision */

	for(l = 0; l < E ; l++) /* get min index and freq */
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

struct params sim_cache(struct cache cache_to_sim, struct params cache_params, unsigned long in_addr, int write)
{
	/* declare local variables */
	long E = cache_params.E;
	long s = cache_params.s;
	long b = cache_params.b;
	long l; /* line index */
	long cache_full;
	long old_hits = cache_params.hits;
	long least_recent;
	long empty_line;
	int was_miss = 0;
	struct min_max_indices m_m_i; /* struct containing max/min indices */

	/* get the tag and set index of set to look in */
	unsigned long in_tag = in_addr >> (s + b); 
	unsigned long set_index = (in_addr << (64 - s - b)) >> (64 - s);

	struct set check_set = cache_to_sim.sets[set_index]; /* set to look in */
	struct line check_line; 

	/* loop to iterate through check_set to look for a match */
	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if((check_line.valid != -1) && (check_line.tag == in_tag)) /* if tag is valid and matching its a hit */
		{
			cache_params.hits = cache_params.hits + 1;
			check_line.hit_freq = check_line.hit_freq + 1; /* maybe need to change this to max */
			if(write == 1) /* if its a write then we set the dirty bit because data is modified */
			{
				if(check_line.dirty_bit == 0)
				{
					check_line.dirty_bit = 1;
					cache_params.dirty_bytes += cache_params.B;
				}
			}
			check_set.lines[l] = check_line;
			break;
		}
	}

	/* no hit found, update miss count */
	if(old_hits == cache_params.hits)
	{
		cache_params.misses = cache_params.misses + 1;
		was_miss = 1;
	}

	/* get index to empty line in check_set or -1 if check_set full*/
	cache_full = is_cache_full(cache_to_sim, set_index, E);

	/* gets least recent used line index */
	m_m_i = get_lru(cache_to_sim, set_index, m_m_i, E);

	/* case when its a LOAD(L) and MISS */
	if(write  == 0 && was_miss == 1)
	{
		if(cache_full == -1) /* set full, evict lru and set new tag; update lru info */
		{
			least_recent = m_m_i.min_index; /* least recent used line index in check_set */
			if(check_set.lines[least_recent].dirty_bit == 1)
			{
				cache_params.bytes_evicted += cache_params.B; /* updates bytes evicted count */
				cache_params.dirty_bytes -= cache_params.B;
			}
			check_set.lines[least_recent].tag = in_tag;
			check_set.lines[least_recent].valid = 1;
			check_set.lines[least_recent].dirty_bit = 0;
			check_set.lines[least_recent].hit_freq = m_m_i.max_freq + 1; /* make it most recent */
			cache_params.evictions = cache_params.evictions + 1; /* update evict count */
		}
		else /* set has a empty line, update tag and lru info */
		{
			empty_line = cache_full; /* gets empty line index in set check_set*/

			check_set.lines[empty_line].tag = in_tag;
			check_set.lines[empty_line].valid = 1;
			check_set.lines[empty_line].dirty_bit = 0;
			check_set.lines[empty_line].hit_freq = m_m_i.max_freq + 1; /* make it most recent */
		}
	}
	/* case when STORE(S) and MISS */
	else if(write == 1 && was_miss == 1)
	{
		if(cache_full == -1) /* set full, evict lru and set new tag; update lru info */
		{
			least_recent = m_m_i.min_index; /* least recent used line index in check_set */
			if(check_set.lines[least_recent].dirty_bit == 1)
			{
				cache_params.bytes_evicted += cache_params.B; /* updates bytes evicted count */
				cache_params.dirty_bytes -= cache_params.B;
			}
			check_set.lines[least_recent].tag = in_tag;
			check_set.lines[least_recent].valid = 1;
			check_set.lines[least_recent].dirty_bit = 1;
			cache_params.dirty_bytes += cache_params.B;
			check_set.lines[least_recent].hit_freq = m_m_i.max_freq + 1; /* make it most recent */
			cache_params.evictions = cache_params.evictions + 1; /* update evict count */
		}
		else /* set has a empty line, update tag and lru info */
		{
			empty_line = cache_full; /* gets empty line index in set check_set*/

			check_set.lines[empty_line].tag = in_tag;
			check_set.lines[empty_line].valid = 1;
			check_set.lines[empty_line].dirty_bit = 1;
			cache_params.dirty_bytes += cache_params.B;
			check_set.lines[empty_line].hit_freq = m_m_i.max_freq + 1; /* make it most recent */
		}
	}
	return cache_params;
}

int main(int argc, char **argv)
{
	/* declare arguments to pass to sim function */
	struct params cache_params;
	struct cache cache_to_sim;
	char *trace_file_path;
	int write;

	/*return value of getopt */
	int opt;

	while(-1 != (opt = getopt(argc, argv, "s:E:b:t:")))  /* taken from rec slides s15 */
	{
		switch(opt)
		{
			case 's':
				cache_params.s = atoi(optarg);
				break;
			case 'E':
				cache_params.E = atoi(optarg);
				break;
			case 'b':
				cache_params.b = atoi(optarg);
				break;
			case 't':
				trace_file_path = optarg;
				break;
			default:
				printf("wrong args\n");
				break;
		}
	}

	/* initialize parameters that hold cache info */
	cache_params.hits = 0;
	cache_params.misses = 0;
	cache_params.evictions = 0;
	cache_params.bytes_evicted = 0;
	cache_params.dirty_bytes = 0;
	cache_params.S = pow(2.0, cache_params.s);
	cache_params.B = pow(2.0, cache_params.b);

	/* our cache model */
	cache_to_sim = make_cache(cache_params.E, cache_params.S);

	/* trace variables */
	char type;
	int size;
	unsigned long in_addr;
	FILE *trace;

	/* start reading trace */
	trace = fopen(trace_file_path, "r");
	
	if(trace != NULL)
	{
		while(fscanf(trace, " %c %lx,%d", &type, &in_addr, &size) == 3)
		{
			switch(type)
			{
				case 'L':
					write = 0;
					cache_params = sim_cache(cache_to_sim, cache_params, in_addr, write);
					break;
				case 'S':
					write = 1;
					cache_params = sim_cache(cache_to_sim, cache_params, in_addr, write);
					break;
				default:
					break;
			}
		}
	}

	fclose(trace);
	/* finish reading trace */

	/* send info to printSummary */
	printSummary(cache_params.hits, cache_params.misses, cache_params.evictions, cache_params.dirty_bytes, cache_params.bytes_evicted);

	/* free memory */
	free_mem(cache_to_sim, cache_params.E, cache_params.S);

	return 0;
}