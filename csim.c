#Siddhanth Lathar @ slathar

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <cachelab.h>

int main()
{
	return 0;
}

struct line //one line
{
	int valid;
	int offset;
	unsigned long long hit_freq;
	unsigned long long tag;
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
	int E;
	int S;
	int B;

	int s;
	int b;

	int hits;
	int misses;
	int evictions;
};

struct min_max_indices
{
	int min_freq;
	int max_freq;
	int min_index;
	int max_index;
};

struct cache make_cache(long long n_L, long long n_S, long long n_B)
{
	struct cache new_cache;
	struct set new_set;
	struct new_line;

	int l;
	int s;

	//alloc memory for cache sets
	new_cache.sets = (struct set*)malloc(sizeof(struct set)*n_S);

	for(s = 0; s < n_S; n_S++)
	{
		//alloc memory for cache lines
		new_set.lines = (struct line*)malloc(sizeof(struct line)*n_L);

		//set s-th set to have the above allocated lines
		new_cache.sets[s] = new_set;

		//initialize to defaults
		for(l = 0; l < n_L; l++)
		{
			new_line.valid = -1;
			new_line.tag = 0;
			new_line.offset = -1;
			new_set.lines[l] = new_line;
		}
	}
	return new_cache;
}

int is_cache_full(struct cache cache_to_sim, unsigned long long set_index)
{
	int l; //line index
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

struct min_max_indices get_lru(struct cache cache_to_sim, unsigned long long set_index, struct min_max_indices m_m_i)
{
	int l; //line index
	int min_freq;
	int max_freq = 0;
	int max_index = 0;
	int min_index = 0;
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




struct params sim_cache(struct cache cache_to_sim, struct params init_params, unsigned long long in_addr)
{
	int E = init_params.E;
	int S = init_params.S;
	int B = init_params.B;
	int s = init_params.s;
	int b = init_params.b;

	unsigned long long in_tag = in_addr >> (s + b);

	unsigned long long set_index = (in_addr << (64 - s - b)) >> (64 - s);

	struct set check_set = cache_to_sim.sets[set_index];
	struct line check_line;
	struct min_max_indices m_m_i;

	int l; //line index
	int cache_full;
	int old_hits = init_params.hits;
	int least_recent;
	int empty_line;


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

	cache_full = is_cache_full(cache_to_sim, set_index);

	if(cache_full == -1)
	{
		//evict and write
		m_m_i = get_lru(cache_to_sim, set_index, m_m_i);
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