#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include <sstream>

#include "CacheSys.h"

using namespace std;

#define PIN_SC_SHARED_NUM 2
#define BUF_INST_NUM 10000
#define CORE_NUM 2

struct Inst
{
	uint64_t		m_fetch_addr;
	uint64_t		m_rd_addr;
	uint64_t		m_wr_addr;
	bool			m_rd_en;
	bool			m_wr_en;
};

struct Exchange
{
	bool			m_ready;
	bool			m_pin_complete;

	Inst			m_inst[BUF_INST_NUM];
	unsigned int	m_inst_cnt;
};


class Test
{
	public:
		Test(unsigned int id,int key,unsigned int pin_shm_num)
		: m_id(id)
		, m_key(key)
		, m_pin_shm_num(pin_shm_num)
		, m_shm_num(0)
		{
			m_shm   = new Exchange* [pin_shm_num];
			m_shmid = new int [pin_shm_num];
			 
			for(unsigned int i=0;i<pin_shm_num;i++)
			{
				void *t_shm = NULL;

				m_shmid[i] = shmget((key_t)(key+i),sizeof(Exchange),0666|IPC_CREAT);

				if(m_shmid[i] == -1)
				{
					cout<<" Pin create share memory failed"<<endl;
					exit(EXIT_FAILURE);
				}

				t_shm = shmat(m_shmid[i],(void*)0,0);
			
				if(t_shm == (void*)-1)
				{
					cout<<" Pin map share memory failed"<<endl;
					exit(EXIT_FAILURE);
				}
				m_shm[i] = (Exchange*)t_shm;
			}
		}

	public:
		Inst* get_inst_buf()
		{
			if(m_shm[m_shm_num]->m_ready == false)
			{
				return NULL;
			}
			else
			{
				return m_shm[m_shm_num]->m_inst;
			}
		}

		bool get_pin_complete()
		{
			return m_shm[m_shm_num]->m_pin_complete;
		}

		unsigned int get_inst_cnt()
		{
			return m_shm[m_shm_num]->m_inst_cnt;
		}

		void retire_shm()
		{
			m_shm[m_shm_num]->m_inst_cnt = 0;
			m_shm[m_shm_num]->m_ready = false;

			m_shm_num++;
			m_shm_num %= m_pin_shm_num;
		}

		void release_shm()
		{
			for(unsigned int i=0;i<m_pin_shm_num;i++)
			{
				if(shmdt(m_shm[i]) == -1)
				{
					cout<<" Error, shmdt fail"<<endl;
					exit(EXIT_FAILURE);
				}

				if(shmctl(m_shmid[i],IPC_RMID,0) == -1)
				{
					cout<<" Error, shmctl fail"<<endl;
					exit(EXIT_FAILURE);
				}
			}
		}

	private:
		unsigned int	m_id;
		unsigned int	m_key;
		unsigned int	m_pin_shm_num;

		Exchange**		m_shm;
		int*			m_shmid;

		unsigned int	m_shm_num;
};

int main(int argc, char **argv)
{
	time_t			timep;

	int				t_key_base = 0;

	Test*			t_test[CORE_NUM];
	bool			t_complete[CORE_NUM];
	int				t_uncomplete_num = CORE_NUM;

	uint64_t		t_inst_cnt[CORE_NUM];
	uint64_t		t_addr_offset[CORE_NUM];/* Inst addr needn't offset, but data addr need offset */
	unsigned int	t_inst_slice_cnt = 0;
	uint64_t		t_last_bus_rd_cnt = 0;
	uint64_t		t_last_bus_wr_cnt = 0;

	if(argc != (CORE_NUM+1))
	{
		cout<<" Error, simulation argc must be "<<dec<<(CORE_NUM+1)
			<<", current is "<<argc<<endl;
		exit(-1);
	}


	//--------------------
	for(int i=0;i<CORE_NUM;i++)
	{
		std::stringstream	t_ss;
		t_ss<<argv[i+1];t_ss>>t_key_base;t_ss.flush();

		cout<<" key_base["<<i<<"] "<<dec<<t_key_base<<endl;

		t_test[i] = new Test(i,t_key_base,PIN_SC_SHARED_NUM);
		t_complete[i] = false;

		t_inst_cnt[i] = 0;
		t_addr_offset[i] = ((uint64_t)i)<<62;

		cout<<" Core "<<dec<<i<<" addr offset 0x"<<hex<<t_addr_offset[i]<<endl;
	}

	//-------------------
	bool  t_get_inst = false;
	Inst* t_inst_buf = NULL;

	//Cache
	CacheSys<CORE_NUM>	m_cache;

	time(&timep);
	cout<<"Start time: "<<asctime(gmtime(&timep))<<endl;

	while(1)
	{
		t_get_inst = false;

		for(int i=0;i<CORE_NUM;i++)
		{
			if(t_complete[i])continue;

			t_inst_buf = t_test[i]->get_inst_buf();
			if(t_inst_buf != NULL)
			{
				t_inst_slice_cnt = t_test[i]->get_inst_cnt();

				cout.flush();
				//call internal function
				//To do...
				for(unsigned int j=0;j<t_inst_slice_cnt;j++)
				{
					uint64_t fetch_addr = 0;
					uint64_t read_addr	= 0;
					uint64_t write_addr = 0;

					t_inst_cnt[i]++;

					if((i == 0) && (t_inst_cnt[i]%100000000 == 0))//100,000,000
					{
						cout<<" Core "<<dec<<i<<" insts "<<dec<<t_inst_cnt[i]
						    <<", bus rd "<<dec<<(m_cache.m_l2->get_bus_rd_cnt()-t_last_bus_rd_cnt)
						    <<", bus wr "<<dec<<(m_cache.m_l2->get_bus_wr_cnt()-t_last_bus_wr_cnt)
						    <<endl;
						t_last_bus_rd_cnt = m_cache.m_l2->get_bus_rd_cnt();
						t_last_bus_wr_cnt = m_cache.m_l2->get_bus_wr_cnt();
					}

					if((t_inst_buf[j].m_rd_en == false)&&(t_inst_buf[j].m_wr_en == false))	//fetch only
					{
						fetch_addr = t_inst_buf[j].m_fetch_addr;
						m_cache.fetchProcess(i,fetch_addr);
					}
					else if((t_inst_buf[j].m_rd_en == true)&&(t_inst_buf[j].m_wr_en == false))	//fetch and read
					{
						fetch_addr = t_inst_buf[j].m_fetch_addr;
						read_addr = t_inst_buf[j].m_rd_addr;
						m_cache.fetchProcess(i,fetch_addr);
						m_cache.loadProcess(i,read_addr+t_addr_offset[i]);
					}
					else if((t_inst_buf[j].m_rd_en == false)&&(t_inst_buf[j].m_wr_en == true))	//fetch and write
					{
						fetch_addr = t_inst_buf[j].m_fetch_addr;
						write_addr = t_inst_buf[j].m_wr_addr;
						m_cache.fetchProcess(i,fetch_addr);
						m_cache.storeProcess(i,write_addr+t_addr_offset[i]);
					}
					else	//fetch and read and write
					{
						fetch_addr = t_inst_buf[j].m_fetch_addr;
						read_addr = t_inst_buf[j].m_rd_addr;
						write_addr = t_inst_buf[j].m_wr_addr;
						m_cache.fetchProcess(i,fetch_addr);
						m_cache.loadProcess(i,read_addr+t_addr_offset[i]);
						m_cache.storeProcess(i,write_addr+t_addr_offset[i]);
					}
				}

				if(t_test[i]->get_pin_complete())
				{
					cout<<" Core "<<i<<" complete, inst number "<<dec<<t_inst_cnt[i]<<endl;
					t_complete[i] = true;
					t_uncomplete_num--;
				}

				t_test[i]->retire_shm();

				t_get_inst = true;
			}
		}
		

		if(t_uncomplete_num == 0)
		{
			m_cache.end_of_simulation();
			time(&timep);
			cout<<"Complete time: "<<asctime(gmtime(&timep))<<endl;
			break;
		}

		if(!t_get_inst)
		{
			usleep(1);
		}
	}

	usleep(1000);
	for(int i=0;i<CORE_NUM;i++)
	{
		t_test[i]->release_shm();
	}

    return 0;
}
