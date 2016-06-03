#include "L1Cache.h"
#include "L2Cache.h"
#include "ComDef.h"
#include <iomanip>

template<unsigned int CPU_NUM=1>
class CacheSys
{
	public:

		CacheSys();

		void fetchProcess(unsigned int id,uint64_t t_addr);
		void loadProcess(unsigned int id,uint64_t t_addr);
		void storeProcess(unsigned int id,uint64_t t_addr);
		void end_of_simulation();

	public:

		L1Cache				*m_l1i[CPU_NUM];
		L1Cache				*m_l1d[CPU_NUM];

		L2Cache				*m_l2;

		CacheStatus			status;

		uint64_t			m_l1i_hit_cnt[CPU_NUM];
		uint64_t			m_l1d_hit_cnt[CPU_NUM];

		uint64_t			m_l1i_miss_cnt[CPU_NUM];
		uint64_t			m_l1d_miss_cnt[CPU_NUM];

		uint64_t			m_l2_hit_cnt;
		uint64_t			m_l2_miss_cnt;
};

template<unsigned int CPU_NUM>
CacheSys<CPU_NUM>::
CacheSys()
{
	m_l2 = new L2Cache(CPU_NUM*2);

	for(int i=0;i<CPU_NUM;i++)
	{
		m_l1i[i] = new L1Cache(i*2);
		m_l1d[i] = new L1Cache((i*2)+1);

		m_l1i[i]->setL2Ptr(m_l2);
		m_l1d[i]->setL2Ptr(m_l2);

		m_l2->setL1Ptr(i*2,m_l1i[i]);
		m_l2->setL1Ptr((i*2)+1,m_l1d[i]);

		m_l1i_hit_cnt[i] = 0;
		m_l1d_hit_cnt[i] = 0;

		m_l1i_miss_cnt[i] = 0;
		m_l1d_miss_cnt[i] = 0;

	}

	m_l2_hit_cnt = 0;
	m_l2_miss_cnt = 0;
}

template<unsigned int CPU_NUM>
void CacheSys<CPU_NUM>::
fetchProcess(unsigned int id,uint64_t t_addr)
{
	m_l1i[id]->load(t_addr,status);

	if(status.m_l1_hit)
	{	
		m_l1i_hit_cnt[id]++;
	}
	else if(status.m_l2_hit)
	{		
		m_l1i_miss_cnt[id]++;
		m_l2_hit_cnt++;
	}
	else
	{
		m_l1i_miss_cnt[id]++;
		m_l2_miss_cnt++;
	}
}

template<unsigned int CPU_NUM>
void CacheSys<CPU_NUM>::
loadProcess(unsigned int id,uint64_t t_addr)
{
	m_l1d[id]->load(t_addr,status);

	if(status.m_l1_hit)
	{	
		m_l1d_hit_cnt[id]++;
	}
	else if(status.m_l2_hit)
	{		
		m_l1d_miss_cnt[id]++;
		m_l2_hit_cnt++;
	}
	else
	{
		m_l1d_miss_cnt[id]++;
		m_l2_miss_cnt++;
	}
}

template<unsigned int CPU_NUM>
void CacheSys<CPU_NUM>::
storeProcess(unsigned int id,uint64_t t_addr)
{
	m_l1d[id]->store(t_addr,status);

	if(status.m_l1_hit)
	{	
		m_l1d_hit_cnt[id]++;
	}
	else if(status.m_l2_hit)
	{		
		m_l1d_miss_cnt[id]++;
		m_l2_hit_cnt++;
	}
	else
	{
		m_l1d_miss_cnt[id]++;
		m_l2_miss_cnt++;
	}
}

template<unsigned int CPU_NUM>
void CacheSys<CPU_NUM>::
end_of_simulation()
{
	uint64_t t_hit_cnt  = 0;
	uint64_t t_miss_cnt = 0;
	uint64_t t_total_cnt = 0;

	cout<<"+------------+------------+------------+------------+"<<endl;
	cout<<"|"<<std::setfill(' ')<<std::setw(12)<<"Cache"
		<<"|"<<std::setfill(' ')<<std::setw(12)<<"Cnt"
		<<"|"<<std::setfill(' ')<<std::setw(12)<<"Miss Cnt"
		<<"|"<<std::setfill(' ')<<std::setw(12)<<"Miss Rate"
		<<"|"<<endl;
	cout<<"+------------+------------+------------+------------+"<<endl;

	for(unsigned int j=0;j<CPU_NUM;j++)
	{
		t_hit_cnt = m_l1i_hit_cnt[j];
		t_miss_cnt = m_l1i_miss_cnt[j];
		t_total_cnt = t_hit_cnt + t_miss_cnt;

		cout<<"|"<<std::setfill(' ')<<std::setw(10)<<"L1I["<<j<<"]"
			<<"|"<<std::setfill(' ')<<std::setw(12)<<std::dec<<t_total_cnt
			<<"|"<<std::setfill(' ')<<std::setw(12)<<std::dec<<t_miss_cnt;
		if(t_total_cnt == 0)
			cout<<"|"<<std::setfill(' ')<<std::setw(12)<<"NA";
		else
			cout<<"|"<<std::setfill(' ')<<std::setw(10)
				<<(((uint64_t)(10000*t_miss_cnt/t_total_cnt))/100.0)
				<<" %";

		cout<<"|"<<endl;
				
		t_hit_cnt = m_l1d_hit_cnt[j];
		t_miss_cnt = m_l1d_miss_cnt[j];
		t_total_cnt = t_hit_cnt + t_miss_cnt;

		cout<<"|"<<std::setfill(' ')<<std::setw(10)<<"L1D["<<j<<"]"
			<<"|"<<std::setfill(' ')<<std::setw(12)<<std::dec<<t_total_cnt
			<<"|"<<std::setfill(' ')<<std::setw(12)<<std::dec<<t_miss_cnt;
		if(t_total_cnt == 0)
			cout<<"|"<<std::setfill(' ')<<std::setw(12)<<"NA";
		else
			cout<<"|"<<std::setfill(' ')<<std::setw(10)
				<<(((uint64_t)(10000*t_miss_cnt/t_total_cnt))/100.0)
				<<" %";
		
		cout<<"|"<<endl;
	}

	t_hit_cnt  = m_l2_hit_cnt;
	t_miss_cnt = m_l2_miss_cnt;
	t_total_cnt = t_hit_cnt + t_miss_cnt;

	cout<<"|"<<std::setfill(' ')<<std::setw(12)<<"L2"
		<<"|"<<std::setfill(' ')<<std::setw(12)<<std::dec<<t_total_cnt
		<<"|"<<std::setfill(' ')<<std::setw(12)<<std::dec<<t_miss_cnt;
	if(t_total_cnt == 0)
		cout<<"|"<<std::setfill(' ')<<std::setw(12)<<"NA";
	else
		cout<<"|"<<std::setfill(' ')<<std::setw(10)
			<<(((uint64_t)(10000*t_miss_cnt/t_total_cnt))/100.0)
			<<" %";

	cout<<"|"<<endl;
	cout<<"+------------+------------+------------+------------+"<<endl;

	cout<<" L2 Rd Cnt "<<dec<<m_l2->get_bus_rd_cnt()<<endl;
	cout<<" L2 Wr Cnt "<<dec<<m_l2->get_bus_wr_cnt()<<endl;
}


