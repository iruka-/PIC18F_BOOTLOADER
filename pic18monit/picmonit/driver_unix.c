//----------------------------------------------------------------------------
//	
//----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
//--------------------------------------
#include <usb.h>

/*********************************************************************
 *
 *********************************************************************
 */

//----------------------------------------------------------------------------
void Sleep(int n)
{
	usleep(n * 1000);
//	printf("Sleep(%d)\n",n);
}
//----------------------------------------------------------------------------
int gr_box(void)
{
	return 0;
}
int gr_boxfill(void)
{
	return 0;
}
int gr_break(void)
{
	return 0;
}
int gr_circle_arc(void)
{
	return 0;
}
int gr_close(void)
{
	return 0;
}
int gr_hline(void)
{
	return 0;
}
int gr_init(void)
{
	return 0;
}
int gr_line(void)
{
	return 0;
}
int gr_pset(void)
{
	return 0;
}
int gr_puts(void)
{
	return 0;
}
int gr_vline(void)
{
	return 0;
}
//----------------------------------------------------------------------------
int kbhit(void)
{
	return 0;
}
int getch(void)
{
	return 0;
}
//----------------------------------------------------------------------------
int	stricmp(char *s1,char *s2)
{
	return strcasecmp(s1,s2);
}
//----------------------------------------------------------------------------
void strupr(char *s)
{
	while(*s) {
		*s = toupper(*s);
		s++;
	}
}



