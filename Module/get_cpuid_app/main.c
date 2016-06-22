#include <stdio.h>
#include <stdlib.h>

extern void test_asm(void* ptr,unsigned int eax,unsigned int ecx);

int main()
{
	long long		data[4];
	char			a[13];

	unsigned int	t_eax,t_ecx;

	t_eax = 0x0;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);

	a[0] = data[1]&0xff;
	a[1] = (data[1]>>8)&0xff;
	a[2] = (data[1]>>16)&0xff;
	a[3] = (data[1]>>24)&0xff;

	a[4] = data[3]&0xff;
	a[5] = (data[3]>>8)&0xff;
	a[6] = (data[3]>>16)&0xff;
	a[7] = (data[3]>>24)&0xff;

	a[8] = data[2]&0xff;
	a[9] = (data[2]>>8)&0xff;
	a[10] = (data[2]>>16)&0xff;
	a[11] = (data[2]>>24)&0xff;

	a[12] = '\0';

	printf( " app test: %s\n",a);

	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x1;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);


	//
	t_eax = 0x2;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x3;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x4;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x4;
	t_ecx = 0x1;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x4;
	t_ecx = 0x2;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x4;
	t_ecx = 0x3;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x5;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x6;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x7;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x8;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x9;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xa;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xb;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xb;
	t_ecx = 0x1;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xb;
	t_ecx = 0x2;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xc;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xd;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xd;
	t_ecx = 0x1;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xd;
	t_ecx = 0x2;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0xd;
	t_ecx = 0x3;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000000;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000001;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000002;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000003;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000004;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000005;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000006;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000007;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	//
	t_eax = 0x80000008;
	t_ecx = 0x0;
	test_asm(data,t_eax,t_ecx);
	printf("0x%08x%08x: [0x%08x, 0x%08x, 0x%08x, 0x%08x]\n",t_eax,t_ecx,(unsigned int)data[0],(unsigned int)data[1],(unsigned int)data[2],(unsigned int)data[3]);

	
	return 0;
}
