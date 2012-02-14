typedef struct __timer1_thread_struct {
	// "persistent" data for this "lthread" would go here
	unsigned int	msgcount;
} timer1_thread_struct;

void init_timer1_lthread(timer1_thread_struct *);
int timer1_lthread(timer1_thread_struct *,int,int,unsigned char*);
