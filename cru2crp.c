#include <string.h>
#include <stdio.h>

main()
{

	int c;

	while((c=getchar())!=EOF)
	{
		if(c==0x0A) putchar(0x0D);
		putchar(c);
	}
}
