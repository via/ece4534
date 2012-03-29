typedef struct __timer0_thread_struct {
	// "persistent" data for this "lthread" would go here
	int	data;
} timer0_thread_struct;

int timer0_lthread(timer0_thread_struct *,int,int,unsigned char*);
