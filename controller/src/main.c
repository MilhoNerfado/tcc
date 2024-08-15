
#include <stdio.h>
#include <string.h>
#include <app/drivers/lora_tcp.h>

#include "app/drivers/lora_tcp.h"

int main(void)
{
	char *data = "Testing data";

	lora_tcp_init(-1);
	lora_tcp_send(-2, (uint8_t *)data, strlen(data) + 1);

	return 0;
}
