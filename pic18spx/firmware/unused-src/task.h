/**********************************************************************
 *	Tiny Task Controler
 **********************************************************************
 *	考え方:
 *	・スケジュールはラウンドロビンのみ。
 *	・ノン・プリエンプティブ。
 *	・あらかじめtask_idと、その実行関数を定義（自分でmapping）しておくこと。
 *	・割り込みルーチンとは基本的に無関係.(フォアグラウンド動作)。
 *	・タスク毎のコンテキストは、関数ポインタだけである。
 *	  ・レジスタコンテキストなどは一切持たない。
 *	  ・関数ポインタを自他により切り替える行為はＯＫ。
 *	  ・スタックエリアは全タスクに対して１個であり共有。
 *	・タスクをまわすためには dispatch_task() の実装部を呼び出す。
 *	・スリープ以外に、たいしたサービス処理は無い。
 *	・タスクキューやリスト構造を持たないので、あまり多い数のタスクをこなせない。
 *	  想定数＝せいぜい１６個以内。
 */

#ifndef	_task_h_
#define	_task_h_


#ifndef __ASSEMBLER__
#ifndef uchar
#define uchar   unsigned char
#endif
#ifndef uint
#define uint   unsigned int
#endif
#ifndef schar
#define schar   signed char
#endif
#endif




#ifdef	__AVR_ATtiny2313__
#define	MAX_TASKS	2
typedef	uchar	UCNT;
#else
//	ATmega88
#define	MAX_TASKS	4
typedef	uint	UCNT;
#endif
/**********************************************************************
 *	TASK STATE
 **********************************************************************
 */
enum TSTAT_ENUM {
	TSTAT_DORMANT  = 0,	//休止.
	TSTAT_RUNNABLE = 1,	//実行可.
	TSTAT_WAIT     = 2,	//待機.
    TSTAT_SLEEP    = 4,	//休眠.
};	//今のところ0と1だけ有効.
/**********************************************************************
 *	TASK CONTROL BLOCK
 **********************************************************************
 */
typedef	struct {
	uchar stat;			//タスクステート.(enum)
	UCNT  sleep_cnt;	//休眠カウント 0=覚醒 !0=休眠中
	volatile void (*func)(void);	//関数エントリー.
} TCB;

extern	TCB   task_tcb[MAX_TASKS];	//
extern	uchar cur_tsk;				//現在実行中のid;

/**********************************************************************
 *	TASK CONTROL FUNCTION
 **********************************************************************
 */


#define	reg_tsk(id_,userfunc_) {	\
	 task_tcb[id_].func = userfunc_;		\
	 task_tcb[id_].sleep_cnt = 0;			\
	 task_tcb[id_].stat = TSTAT_RUNNABLE;	\
	}
//注:あとでRUNNABLEにする.


#define	kill_tsk(id_) {		\
	 task_tcb[id].stat = 0;				\
	 task_tcb[id].func = NULL;			\
	}
//注:先にstatをクリアする.


#define	sleep_id_tsk(id_,count_)	{	\
	 task_tcb[id_].sleep_cnt = count_;	\
	 task_tcb[id_].stat = TSTAT_SLEEP;	\
	}
//注:任意taskの次の起床時間を設定する.

#define	sleep_tsk(count_)	{	\
	sleep_id_tsk( cur_tsk , count_ );}
//注:自分の次の起床時間を設定する.

#define	get_tsk_id()	(cur_tsk)


//	TCBの初期化.
void init_tsk(void);
void dispatch_tsk(void);


#endif	//_task_h_
