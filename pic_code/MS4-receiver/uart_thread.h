typedef struct __uart_thread_struct {
	// "persistent" data for this "lthread" would go here
	int	data;
} uart_thread_struct;

int uart_lthread(uart_thread_struct *,int,int,unsigned char*);
