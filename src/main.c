/**
  ******************************************************************************
  * @file    Template/main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    20-September-2013
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "stm32f4xx.h"
#include "stm32_f4.h"
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <string.h>

/* Filesystem includes */
#include "filesystem.h"
#include "fio.h"
#include "romfs.h"

#include "clib.h"
#include "shell.h"
#include "host.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* _sromfs symbol can be found in main.ld linker script
 * it contains file system structure of test_romfs directory
 */
extern const unsigned char _sromfs;

//static void setup_hardware();

volatile xSemaphoreHandle serial_tx_wait_sem = NULL;
/* Add for serial input */
volatile xQueueHandle serial_rx_queue = NULL;

/* IRQ handler to handle USART1 interruptss (both transmit and receive
 * interrupts). */
void USART1_IRQHandler()
{
	static signed portBASE_TYPE xHigherPriorityTaskWoken;

	/* If this interrupt is for a transmit... */
	if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET) {
		/* "give" the serial_tx_wait_sem semaphore to notfiy processes
		 * that the buffer has a spot free for the next byte.
		 */
		xSemaphoreGiveFromISR(serial_tx_wait_sem, &xHigherPriorityTaskWoken);

		/* Diables the transmit interrupt. */
		USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		/* If this interrupt is for a receive... */
	}else if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
		char msg = USART_ReceiveData(USART1);

		/* If there is an error when queueing the received byte, freeze! */
		if(!xQueueSendToBackFromISR(serial_rx_queue, &msg, &xHigherPriorityTaskWoken))
			while(1);
	}
	else {
		/* Only transmit and receive interrupts should be enabled.
		 * If this is another type of interrupt, freeze.
		 */
		while(1);
	}

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}
}

void send_byte(char ch)
{
	/* Wait until the RS232 port can receive another byte (this semaphore
	 * is "given" by the RS232 port interrupt when the buffer has room for
	 * another byte.
	 */
	while (!xSemaphoreTake(serial_tx_wait_sem, portMAX_DELAY));

	/* Send the byte and enable the transmit interrupt (it is disabled by
	 * the interrupt).
	 */
	USART_SendData(USART1, ch);
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}

char recv_byte()
{
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	char msg;
	while(!xQueueReceive(serial_rx_queue, &msg, portMAX_DELAY));
	return msg;
}

void command_prompt(void *pvParameters)
{
	char buf[128];
	char *argv[20];
    char hint[] = USER_NAME "@" USER_NAME "-STM32:~$ ";

	fio_printf(1, "\rWelcome to FreeRTOS Shell\r\n");
	while(1){
        fio_printf(1, "%s", hint);
		fio_read(0, buf, 127);

		int n=parse_command(buf, argv);

		/* will return pointer to the command function */
		cmdfunc *fptr=do_command(argv[0]);
		if(fptr!=NULL)
			fptr(n, argv);
		else
			fio_printf(2, "\r\n\"%s\" command not found.\r\n", argv[0]);
	}

}

int main(void)
{
	init_rs232();
	enable_rs232_interrupts();
	enable_rs232();

	fs_init();
	fio_init();
	
//	register_romfs("romfs", &_sromfs);
	
	/* Create the queue used by the serial task.  Messages for write to
	 * the RS232. */
	vSemaphoreCreateBinary(serial_tx_wait_sem);
	/* Add for serial input 
	 * Reference: www.freertos.org/a00116.html */
	serial_rx_queue = xQueueCreate(1, sizeof(char));

	/* Create a task to output text read from romfs. */
	xTaskCreate(command_prompt,
	             "CLI",
	            512 /* stack size */, NULL, tskIDLE_PRIORITY + 2, NULL);

	/* Start running the tasks. */
	vTaskStartScheduler();

	return 0;
}

