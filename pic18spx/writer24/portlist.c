/* portlist.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "monit.h"
#include "portlist.h"
#include "util.h"

/*********************************************************************
 *	ポート名称	,  アドレス , 属性（バイト幅、よく参照されるかどうか）
 *********************************************************************
 */

#if	0
#include "portlist_2313.h"
#endif

#if	0
#include "portlist_mega88.h"
#endif

#if	1
#include "portlist_pic18.h"
#endif

#if	0
#include "portlist_14k50.h"
#endif


/*********************************************************************
 *	ポート名称からアドレスを求める.
 *********************************************************************
 */
int	portAddress(char *s)
{
	PortList *p = portList;
	while(p->name) {
		if(stricmp(s,p->name)==0) return p->adrs;
		p++;
	}
	return 0;
}

/*********************************************************************
 *
 *********************************************************************
 */
