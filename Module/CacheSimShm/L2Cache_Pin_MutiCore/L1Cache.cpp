#include "L1Cache.h"
#include "L2Cache.h"
#include <math.h>


L1Cache
::L1Cache(unsigned int id)
: m_cache_size(32*1024)
, m_way_num(8)
, m_line_size(64)
, m_id(id)
, m_l2_ptr(NULL)
{
	m_set_num = m_cache_size/(m_way_num*m_line_size);
	
	m_line_bits = (unsigned int)(log(m_line_size)/log(2));
	m_set_bits  = (unsigned int)(log(m_set_num)/log(2));
	
	m_set_lsb = m_line_bits;
	m_set_msb = m_set_lsb+m_set_bits-1;
	
	uint64_t t_mask = 0x1;	
	m_set_mask = ((t_mask<<m_set_bits)-1)<<m_set_lsb;
	
	
	cout<<" L1["<<id<<"]\n"
		<<"        cache size "<<dec<<m_cache_size<<" bytes\n"
		<<"           way num "<<dec<<m_way_num<<"\n"
		<<"           set num "<<dec<<m_set_num<<"\n"
		<<"           set bits - addr["<<dec<<m_set_msb<<":"<<m_set_lsb<<"]\n"
		<<"           set mask 0x"<<hex<<m_set_mask
		<<endl;

	m_tag_array = new L1Set* [m_set_num];
	for(unsigned int i=0;i<m_set_num;i++)
	{
		m_tag_array[i] = new L1Set (m_way_num);
	}
}

void L1Cache
::load(uint64_t addr,CacheStatus& status)
{
	unsigned int t_set_index = get_set(addr);
	L1Set* t_set;
	assert(t_set_index<m_set_num);

	t_set = m_tag_array[t_set_index];

	//check whether hit in L1
	for(unsigned int i=0;i<m_way_num;i++)
	{
		if(t_set->m_tag[i].m_mesi == I)
		{
			continue;
		}

		if(t_set->m_tag[i].m_tag == get_tag(addr))//hit
		{
			t_set->deleteLRUEntry(&t_set->m_tag[i]);
			t_set->addLRUEntry(&t_set->m_tag[i]);

			status.m_l1_hit						= true;
			status.m_l2_hit						= false;
			status.m_l1_victim					= false;
			status.m_l2_query_l1				= false;
			status.m_llc_victim					= false;

			return;
		}
	}

	//l1 miss
	//load from l2
	CacheMESI	t_l1_mesi;
	//find one way to fill this line
	if(!t_set->m_free_way_que.empty())//has free entry
	{
		unsigned int t_way_id = t_set->m_free_way_que.front();
	
		//assert
#if 0		
		if(t_set->m_tag[t_way_id].m_mesi != I)
		{
			cout<<" id = "<<m_id
				<<", set["<<dec<<t_set_index<<"]"
				<<", way["<<dec<<t_way_id<<"] mesi = "
				<<t_set->m_tag[t_way_id].m_mesi
				<<endl;

			dump_free_way_que(t_set_index);
		}
#endif
		assert(t_set->m_tag[t_way_id].m_mesi == I);

		t_set->m_free_way_que.pop_front();

		//load from l2
		status.m_l1_hit						= false;
		status.m_l1_victim					= false;
		
		m_l2_ptr->load(m_id,addr,t_l1_mesi,status);
		
		t_set->m_tag[t_way_id].m_tag = get_tag(addr);
		t_set->m_tag[t_way_id].m_mesi = t_l1_mesi;
		t_set->addLRUEntry(&t_set->m_tag[t_way_id]);
	}
	else//no free entry, need replace
	{
		//find the oldest way
		uint64_t	 t_replace_addr;
		unsigned int t_replace_way = 0;
		
		t_replace_way = t_set->getLRUFront();

		t_replace_addr = get_addr(t_set->m_tag[t_replace_way].m_tag,t_set_index);

		//evict this replaced way
		bool t_replace_dirty = (t_set->m_tag[t_replace_way].m_mesi == M)?true:false;

		status.m_l1_hit						= false;
		status.m_l1_victim					= true;
		status.m_l1_victim_clean			= !t_replace_dirty;

		m_l2_ptr->evict(m_id,t_replace_addr,t_replace_dirty);

		//load from l2
		m_l2_ptr->load(m_id,addr,t_l1_mesi,status);

		//update way
		t_set->m_tag[t_replace_way].m_tag = get_tag(addr);
		t_set->m_tag[t_replace_way].m_mesi = t_l1_mesi;
		t_set->deleteLRUEntry(&t_set->m_tag[t_replace_way]);
		t_set->addLRUEntry(&t_set->m_tag[t_replace_way]);
	}
	return;
}

void L1Cache
::store(uint64_t addr, CacheStatus& status)
{
	unsigned int t_set_index = get_set(addr);
	L1Set* t_set;
	assert(t_set_index<m_set_num);

	t_set = m_tag_array[t_set_index];

	//check whether hit in L1
	for(unsigned int i=0;i<m_way_num;i++)
	{
		if(t_set->m_tag[i].m_mesi == I)
		{
			continue;
		}

		if(t_set->m_tag[i].m_tag == get_tag(addr))//hit
		{
			if((t_set->m_tag[i].m_mesi == E)
			|| (t_set->m_tag[i].m_mesi == M))
			{
				t_set->m_tag[i].m_mesi = M;
				t_set->deleteLRUEntry(&t_set->m_tag[i]);
				t_set->addLRUEntry(&t_set->m_tag[i]);

				status.m_l1_hit						= true;
				status.m_l2_hit						= false;
				status.m_l1_victim					= false;
				status.m_l2_query_l1				= false;
				status.m_llc_victim					= false;
			}
			else if(t_set->m_tag[i].m_mesi == S)
			{
				status.m_l1_hit						= true;
				status.m_l1_victim					= false;

				m_l2_ptr->loadInv(0,m_id,addr,status);//invalid other copies
				t_set->m_tag[i].m_mesi = M;
				t_set->deleteLRUEntry(&t_set->m_tag[i]);
				t_set->addLRUEntry(&t_set->m_tag[i]);

				assert(status.m_l2_hit == true);
				//assert(status.m_l2_query_l1 == true);
				//assert(status.m_l2_query_l1_clean == true);
				assert(status.m_llc_victim == false);
			}
			else assert(false);

			return;
		}
	}

	//l1 miss
	//load from l2
//	CacheMESI	t_l1_mesi;
	//find one way to fill this line
	if(!t_set->m_free_way_que.empty())//has free entry
	{
		unsigned int t_way_id = t_set->m_free_way_que.front();
		t_set->m_free_way_que.pop_front();

		assert(t_set->m_tag[t_way_id].m_mesi == I);
		//load from l2
		m_l2_ptr->loadInv(1,m_id,addr,status);

		status.m_l1_hit						= false;
		status.m_l1_victim					= false;
		
		t_set->m_tag[t_way_id].m_tag = get_tag(addr);
		t_set->m_tag[t_way_id].m_mesi = M;
		t_set->addLRUEntry(&t_set->m_tag[t_way_id]);
	}
	else//no free entry, need replace
	{
		//find the oldest way
		uint64_t	t_replace_addr;
		unsigned int t_replace_way = 0;

		t_replace_way = t_set->getLRUFront();

		t_replace_addr = get_addr(t_set->m_tag[t_replace_way].m_tag,t_set_index);

		//evict this replaced way
		bool t_replace_dirty = (t_set->m_tag[t_replace_way].m_mesi == M)?true:false;

		status.m_l1_hit						= false;
		status.m_l1_victim					= true;
		status.m_l1_victim_clean			= !t_replace_dirty;

		m_l2_ptr->evict(m_id,t_replace_addr,t_replace_dirty);

		//load from l2
		m_l2_ptr->loadInv(2,m_id,addr,status);

		//update way
		t_set->m_tag[t_replace_way].m_tag = get_tag(addr);
		t_set->m_tag[t_replace_way].m_mesi = M;
		t_set->deleteLRUEntry(&t_set->m_tag[t_replace_way]);
		t_set->addLRUEntry(&t_set->m_tag[t_replace_way]);
	}
	return;

}

CacheMESI L1Cache
::query(uint64_t addr)
{
	unsigned int t_set_index = get_set(addr);
	L1Set* t_set;
	CacheMESI t_mesi;

	assert(t_set_index<m_set_num);
	
	t_set = m_tag_array[t_set_index];

	for(unsigned int i=0;i<m_way_num;i++)
	{
		if(t_set->m_tag[i].m_mesi == I)
		{
			continue;
		}
		
		if(t_set->m_tag[i].m_tag == get_tag(addr))//hit
		{
			t_mesi = t_set->m_tag[i].m_mesi;

			t_set->m_tag[i].m_mesi = S;

			return t_mesi;
		}
	}
	assert(false);
}

CacheMESI L1Cache
::queryInv(uint64_t addr)
{
	unsigned int t_set_index = get_set(addr);
	L1Set* t_set;
	CacheMESI t_mesi;

	assert(t_set_index<m_set_num);
	
	t_set = m_tag_array[t_set_index];

	for(unsigned int i=0;i<m_way_num;i++)
	{
		if(t_set->m_tag[i].m_mesi == I)
		{
			continue;
		}
		
		if(t_set->m_tag[i].m_tag == get_tag(addr))//hit
		{
			t_mesi = t_set->m_tag[i].m_mesi;

			t_set->m_tag[i].m_mesi = I;

			t_set->deleteLRUEntry(&t_set->m_tag[i]);

			t_set->m_free_way_que.push_back(i);
			return t_mesi;
		}
	}
	assert(false);
}
void L1Cache::
dump(unsigned int set_index)
{
	L1Set* t_set = m_tag_array[set_index];
	
	cout<<"---- set["<<dec<<set_index<<"]: ";
	for(unsigned int i=0;i<m_way_num;i++)
	{
		cout<<"0x"<<hex<<t_set->m_tag[i].m_tag<<"|"<<((t_set->m_tag[i].m_mesi==I)?"0":"1")
			<<((i == m_way_num-1)?" ":", ");
	}
	cout<<endl;
}

void L1Cache::
dump_addr(unsigned int addr)
{
	unsigned int set_index = get_set(addr);
	L1Set* t_set = m_tag_array[set_index];
	
	cout<<"---- "<<" set["<<dec<<set_index<<"]: ";
	for(unsigned int i=0;i<m_way_num;i++)
	{
		cout<<"0x"<<hex<<t_set->m_tag[i].m_tag<<"|"<<((t_set->m_tag[i].m_mesi==I)?"0":"1")
			<<((i == m_way_num-1)?" ":", ");
	}
	cout<<endl;
}

void L1Cache::
dump_free_way_que(unsigned int set_index)
{
	L1Set* t_set = m_tag_array[set_index];
	
	cout<<"---- set["<<dec<<set_index<<"] free que: ";
	for(unsigned int i=0;i<t_set->m_free_way_que.size();i++)
	{
		cout<<dec<<t_set->m_free_way_que[i]<<" "
			<<"[mesi="<<t_set->m_tag[t_set->m_free_way_que[i]].m_mesi<<"]  ";
	}
	cout<<endl;

}

unsigned int L1Cache::
get_set(uint64_t addr)
{
	return (unsigned int)((addr&m_set_mask)>>m_set_lsb);
}

uint64_t L1Cache::
get_tag(uint64_t addr)
{
	return (addr>>(m_line_bits+m_set_bits)<<(m_line_bits+m_set_bits));
}

uint64_t L1Cache::
get_addr(uint64_t tag, unsigned int set)
{
	uint64_t addr = 0x0;
	
	addr = tag | (set<<m_set_lsb);
	return addr;
}

