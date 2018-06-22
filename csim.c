#Siddhanth Lathar @ slathar

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <cachelab.h>

struct line //one line
{
	long valid;
	long offset;
	unsigned long hit_freq;
	unsigned long tag;
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
};

struct min_max_indices
{
	long min_freq;
	long max_freq;
	long min_index;
	long max_index;
};

struct cache make_cache(long n_E, long n_S)
{
	struct cache new_cache;
	struct set new_set;
	struct new_line;

	long l;
	long s;

	//alloc memory for cache sets
	new_cache.sets = (struct set*)malloc(sizeof(struct set)*n_S);

	for(s = 0; s < n_S; n_S++)
	{
		//alloc memory for cache lines
		new_set.lines = (struct line*)malloc(sizeof(struct line)*n_L);

		//set s-th set to have the above allocated lines
		new_cache.sets[s] = new_set;

		//initialize to defaults
		for(l = 0; l < n_E; l++)
		{
			new_line.valid = -1;
			new_line.tag = 0;
			new_line.offset = -1;
			new_set.lines[l] = new_line;
		}
	}
	return new_cache;
}

long is_cache_full(struct cache cache_to_sim, unsigned long set_index, long E)
{
	long l; //line index
	struct set check_set = cache_to_sim.sets[set_index];
	struct line check_line;

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
	long l; //line index
	long min_freq;
	long max_freq = 0;
	long max_index = 0;
	long min_index = 0;
	struct set check_set = cache_to_sim.sets[set_index];
	struct line check_line;

	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if(check_line.hit_freq >= max_freq)
		{
			max_freq = check_line.hit_freq;
			max_index = l;
		}
	}

	min_freq = max_freq;

	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if(check_line.hit_freq <= min_freq)
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




struct params sim_cache(struct cache cache_to_sim, struct params init_params, unsigned long in_addr)
{
	long E = init_params.E;
	long S = init_params.S;
	long B = init_params.B;
	long s = init_params.s;
	long b = init_params.b;

	unsigned long in_tag = in_addr >> (s + b);

	unsigned long set_index = (in_addr << (64 - s - b)) >> (64 - s);

	struct set check_set = cache_to_sim.sets[set_index];
	struct line check_line;
	struct min_max_indices m_m_i;

	long l; //line index
	long cache_full;
	long old_hits = init_params.hits;
	long least_recent;
	long empty_line;


	for(l = 0; l < E ; l++)
	{
		check_line = check_set.lines[l];

		if((check_line.valid != -1) && (check_line.tag == in_tag)) //if tag is valid and matching
		{
			//hit
			init_params.hits = init_params + 1;
			check_line.hit_freq = check_line.hit_freq + 1;
			return init_params;
		}
	}

	//no hit found!
	if(old_hits == init_params.hits)
	{
		init_params.misses = init_params.misses + 1;
	}

	cache_full = is_cache_full(cache_to_sim, set_index, E);

	if(cache_full == -1)
	{
		//evict and write
		m_m_i = get_lru(cache_to_sim, set_index, m_m_i, E);
		least_recent = m_m_i.min_index;

		check_set.lines[least_recent].tag = in_tag;
		check_set.lines[least_recent].hit_freq = m_m_i.max_freq + 1; //make it most recent
		init_params.evictions = init_params.evictions + 1;
	}
	else
	{
		//we can write without evicting

		empty_line = cache_full; // implement this

		check_set.lines[empty_line].tag = in_tag;
		check_set.lines[empty_line].valid = 1;
		check_set.lines[empty_line].hit_freq = m_m_i.max_freq + 1; //make it most recent
		
	}
	return init_params;
}


int main(int argc, char **argv)
{
	struct params init_params;
	struct cache cache_to_sim;
	char *trace_file_path;
	
	unsigned long in_addr;

	while(-1 != (opt = getopt(argc, argv, "s:E:b:t")))  //taken from rec slides s15
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
	init_params.S = pow(2.0, init_params.s);
	init_params.B = pow(2.0, init_params.b);

	cache_to_sim = make_cache(init_params.E, init_params.S);


	return 0;
}