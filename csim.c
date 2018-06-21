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

		if((check_line.valid == -1)) //if tag is valid and matching
		{
			return 0; //not full
		}
	}
	return 1; //full
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

	int l; //line index
	int cache_full = is_cache_full(cache_to_sim, set_index); //inplement this
	int old_hits = init_params.hits;

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

	if(cache_full == 1)
	{
		//evict and write

		int least_recent = check_lru(); // implement this

		check_set.lines[least_recent].tag = in_tag;
		check_set.lines[least_recent].hit_freq = 0;
		init_params.evictions = init_params.evictions + 1;
	}
	else
	{
		//we can write without evicting

		int empty_line = check_empty(); // implement this

		check_set.lines[empty_line].tag = in_tag;
		check_set.lines[empty_line].valid = 1;
		check_set.lines[empty_line].hit_freq = 0;
		
	}
	return init_params;
}