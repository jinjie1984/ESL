#!/usr/bin/perl

open(input,"<$ARGV[0]") or die ("Can't open yaml file\n");
open(output,">output.cc") or die ("Can't open output file\n");

printf output "switch(gax->dword[0])\n";
printf output "{\n";

$lines=<input>;
$lines =~ /0x([0-9a-f]{8})([0-9a-f]{8}):\s\[0x([0-9a-f]{8}),\s0x([0-9a-f]{8}),\s0x([0-9a-f]{8}),\s0x([0-9a-f]{8})\]/;

$gax_0 = $1;
$gcx_0 = $2;

$eax_0 = $3;
$ebx_0 = $4;
$ecx_0 = $5;
$edx_0 = $6;

$level = 1;


while($lines=<input>)
{
	$lines =~ /0x([0-9a-f]{8})([0-9a-f]{8}):\s\[0x([0-9a-f]{8}),\s0x([0-9a-f]{8}),\s0x([0-9a-f]{8}),\s0x([0-9a-f]{8})\]/;

	$gax_1 = $1;
	$gcx_1 = $2;
	
	$eax_1 = $3;
	$ebx_1 = $4;
	$ecx_1 = $5;
	$edx_1 = $6;

	if($gax_0 eq $gax_1)
	{
		if($level == 1)
		{
			printf output "    case 0x$gax_0:\n";
			printf output "        switch(gcx->dword[0])\n";
			printf output "        {\n";
		
			$level = 2;
		}

		printf output "            case 0x$gcx_0:\n";
		printf output "                res.eax = 0x$eax_0;res.ebx = 0x$ebx_0;res.ecx = 0x$ecx_0;res.edx = 0x$edx_0;break;\n";
	}
	else
	{
		if($level == 2)
		{
			printf output "            case 0x$gcx_0:\n";
			printf output "                res.eax = 0x$eax_0;res.ebx = 0x$ebx_0;res.ecx = 0x$ecx_0;res.edx = 0x$edx_0;break;\n";
			printf output "            default:\n";
			printf output "                AsmCpuid(gax->dword[0],gcx->dword[0],res);\n";
			printf output "        }\n";
			printf output "        break;\n";

			$level = 1;
		}
		else
		{
			printf output "    case 0x$gax_0:\n";
			printf output "        res.eax = 0x$eax_0;res.ebx = 0x$ebx_0;res.ecx = 0x$ecx_0;res.edx = 0x$edx_0;break;\n";
		}
	}

	$gax_0 = $gax_1;
	$gcx_0 = $gcx_1;
	              
	$eax_0 = $eax_1;
	$ebx_0 = $ebx_1;
	$ecx_0 = $ecx_1;
	$edx_0 = $edx_1;
}

if($level == 2)
{
	printf output "            case 0x$gcx_0:\n";
	printf output "                res.eax = 0x$eax_0;res.ebx = 0x$ebx_0;res.ecx = 0x$ecx_0;res.edx = 0x$edx_0;break;\n";
	printf output "            default:\n";
	printf output "                AsmCpuid(gax->dword[0],gcx->dword[0],res);\n";
	printf output "        }\n";
	printf output "        break;\n";
}
else
{
	printf output "    case 0x$gax_0:\n";
	printf output "        res.eax = 0x$eax_0;res.ebx = 0x$ebx_0;res.ecx = 0x$ecx_0;res.edx = 0x$edx_0;break;\n";
}

printf output "    default:\n";
printf output "        AsmCpuid(gax->dword[0],gcx->dword[0],res);\n";
printf output "}\n";

close(input);
close(output);
