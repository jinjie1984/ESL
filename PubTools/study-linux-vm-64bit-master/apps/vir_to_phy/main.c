#define _FILE_OFFSET_BITS 64
#define _LARGEFILE64_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

static uint64_t get_cr3_reg_info()
{
	char*		buf;
    int			rd_size;
	uint64_t	cr3_value;

    int fd = open("/proc/sys_reg", O_RDONLY);
    if(fd < 0){
        printf("open failed\n");
        return 0;
    }

    buf = (char*)malloc(100);
    if(buf == NULL){
        printf("malloc failed\n");
        return 0;
    }

    rd_size = read(fd, buf, 100 - 1);
    if(rd_size < 0){
        printf("read failed\n");
        return 0;
    }

    buf[rd_size] = 0;

    //printf("%s", buf);

	sscanf(buf,"%lx",&cr3_value);

	free(buf);

    close(fd);

	return cr3_value;
}


static uint64_t read_mem_dword(int fd, uint64_t addr)//8 bytes
{
	addr = addr & 0xFFFFFFFFFFFFFFF8L;

	__off64_t off = lseek64(fd, (__off64_t)addr, SEEK_SET);
	if(off < 0)
	{
        printf("lseek fail\n");
        return 0;
    }

	uint64_t data;
	int read_size = read(fd, (char*)&data, 8);
	if(read_size < 0)
	{
		printf("read failed\n");
    	return 0;
    }

	return data;
}

static uint64_t vir_to_phy(int fd, uint64_t cr3, uint64_t vir_addr)
{
	uint64_t map_addr,page_addr,phy_addr;
	//1 level
	map_addr = (vir_addr>>39)&0x1f;
	page_addr = cr3 + map_addr*8;
	//printf(" 1.1 addr = 0x%lx\n",page_addr);

	page_addr = read_mem_dword(fd,page_addr);
	//printf(" 1.2 addr = 0x%lx\n",page_addr);
	
	page_addr &= 0xFFFFFFFFFFFFFF00;
	//printf(" 1.3 addr = 0x%lx\n",page_addr);

	//2 level
	map_addr = (vir_addr>>30)&0x1f;
	page_addr = page_addr + map_addr*8;
	//printf(" 2.1 addr = 0x%lx\n",page_addr);

	page_addr = read_mem_dword(fd,page_addr);
	//printf(" 2.2 addr = 0x%lx\n",page_addr);
	
	page_addr &= 0xFFFFFFFFFFFFFF00;
	//printf(" 2.3 addr = 0x%lx\n",page_addr);

	//3 level
	map_addr = (vir_addr>>21)&0x1f;
	page_addr = page_addr + map_addr*8;
	//printf(" 3.1 addr = 0x%lx\n",page_addr);

	page_addr = read_mem_dword(fd,page_addr);
	//printf(" 3.2 addr = 0x%lx\n",page_addr);
	
	page_addr &= 0xFFFFFFFFFFFFFF00;
	//printf(" 3.3 addr = 0x%lx\n",page_addr);

	//4 level
	map_addr = (vir_addr>>12)&0x1f;
	page_addr = page_addr + map_addr*8;
	//printf(" 4.1 addr = 0x%lx\n",page_addr);

	page_addr = read_mem_dword(fd,page_addr);
	//printf(" 4.2 addr = 0x%lx\n",page_addr);
	
	page_addr &= 0x7FFFFFFFFFFFF000;
	//printf(" 4.3 addr = 0x%lx\n",page_addr);

	//phy addr
	phy_addr = page_addr + (vir_addr&0xFFF);

	//printf(" Physical addr = 0x%lx\n",phy_addr);

	//test
	//uint64_t data = read_mem_dword(fd,phy_addr);
	//printf(" Get data = 0x%lx\n",data);

	return phy_addr;
}

unsigned long a;


int main()
{
	a = 0xA5A5AA550000FFFF;

    printf("a = %lX, addr: %p \n", a, &a);
	printf("------------------------\n");

	uint64_t cr3 = get_cr3_reg_info();

	//printf(" CR3 = %lX\n",cr3);

	int fd = open("/dev/phy_mem", O_RDONLY);
    if(fd < 0)
	{
        printf("phy_mem open fail\n");
        return 0;
    }


	printf("------------------------\n");
	printf("\nPlease Enter any key for continue...");
	
	char str[10];
	scanf("%s",str);

	close(fd);
	
    return 0;
}

