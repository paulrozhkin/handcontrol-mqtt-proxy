/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <mqtt.h>
#include <stdio.h>
#include <string.h>
#include "stream_buffer.h"
#include "queue.h"
#include "semphr.h"
#include "list.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
enum ProtocolCommand { Empty = 0, Telemetry = 1};

typedef struct {
	char* topicName;
	uint8_t* data;
	uint32_t dataLenght;
} MQTT_Protocol_t;

typedef struct {
	enum ProtocolCommand protocolCommand;
	uint8_t* data;
	uint32_t dataLenght;
} Prothesis_Protocol_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define sbiSTREAM_BUFFER_LENGTH_BYTES		( ( size_t ) 100 )
#define sbiSTREAM_BUFFER_TRIGGER_LEVEL_1	( ( BaseType_t ) 1 )

#define qpeekQUEUE_LENGTH (20)

#define MQTT_CLIENT_ID 					"f2c8eb77-ea6e-4411-bf14-b8a564d17a97"
#define MQTT_ONLINE_TOPIC 				"controllers/online"
#define MQTT_OFFLINE_TOPIC				"controllers/offline"
#define MQTT_TELEMETRY_TOPIC 			MQTT_CLIENT_ID "/telemetry"
#define MQTT_SETTINGS_TOPIC 			MQTT_CLIENT_ID "/settings"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

UART_HandleTypeDef huart2;

osThreadId defaultTaskHandle;
osThreadId uartTaskHandle;
/* USER CODE BEGIN PV */

/* === UART PV === */
/* Receive data */
static uint8_t uartRxData;
/* The stream buffer that is used to send received data from an interrupt to the task. */
static StreamBufferHandle_t xStreamReceiveBuffer = NULL;
static QueueHandle_t xQueueUARTSendMessage = NULL;
static QueueHandle_t xQueueUARTReceiveMessage = NULL;

/* === MQTT PV === */
static QueueHandle_t xQueueMQTTSendMessage = NULL;
static QueueHandle_t xQueueMQTTReceiveMessage = NULL;
/* Stores the handle of the task that will be notified when connect to MQTT is complete. */
static TaskHandle_t xTaskToNotifyMqttConnect = NULL;
static list MQTTdataBuffer;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void const * argument);
void StartUartTask(void const * argument);

/* USER CODE BEGIN PFP */
void StartProtocolParserTask(void * argument);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len) {
	/* Implement your write code here, this is used by puts and printf for example */
	int i = 0;
	for (i = 0; i < len; i++)
		ITM_SendChar((*ptr++));
	return len;
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of uartTask */
  osThreadDef(uartTask, StartUartTask, osPriorityNormal, 0, 256);
  uartTaskHandle = osThreadCreate(osThread(uartTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInitStruct.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */
static int inpub_id;

static void mqtt_connection_cb(mqtt_client_t *client, void *arg,
		mqtt_connection_status_t status);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void mqtt_incoming_publish_cb(void *arg, const char *topic,
		u32_t tot_len);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len,
		u8_t flags);
static void mqtt_pub_request_cb(void *arg, err_t result);
void online_publish(mqtt_client_t *client, void *arg);

void mqtt_connect(mqtt_client_t *client) {
	struct mqtt_connect_client_info_t ci;
	err_t err;

	/* Setup an empty client info structure */
	memset(&ci, 0, sizeof(ci));

	/* Minimal amount of information required is client identifier, so set it here */
	ci.client_id = MQTT_CLIENT_ID;
	ci.will_topic = MQTT_OFFLINE_TOPIC;
	ci.will_retain = 0;
	ci.will_qos = 2;
	ci.will_msg = MQTT_CLIENT_ID;

	ip_addr_t mqttServerIP;
	IP4_ADDR(&mqttServerIP, 192, 168, 0, 3);
	u16_t MQTT_PORT = 1883;

	/* Initiate client and connect to server, if this fails immediately an error code is returned
	 otherwise mqtt_connection_cb will be called with connection result after attempting
	 to establish a connection with the server.
	 For now MQTT version 3.1.1 is always used */

	err = mqtt_client_connect(client, &mqttServerIP, MQTT_PORT,
			mqtt_connection_cb, 0, &ci);

	/* For now just print the result code if something goes wrong */
	if (err != ERR_OK) {
		printf("mqtt_connect return %d\n", err);
	}
}

/**
 * Подключение прошло успешно.
 */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg,
		mqtt_connection_status_t status) {
	err_t err;
	if (status == MQTT_CONNECT_ACCEPTED) {
		printf("mqtt_connection_cb: Successfully connected\n");

		/* Setup callback for incoming publish requests */
		mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb,
				mqtt_incoming_data_cb, arg);

		xTaskNotifyGive(xTaskToNotifyMqttConnect);

		online_publish(client, arg);

		/* Subscribe to a topic named "serverData" with QoS level 1, call mqtt_sub_request_cb with result */
		err = mqtt_subscribe(client, MQTT_SETTINGS_TOPIC, 1, mqtt_sub_request_cb, arg);

		//test_publish(client, 0);

		if (err != ERR_OK) {
			printf("mqtt_subscribe return: %d\n", err);
		}
	} else {
		printf("mqtt_connection_cb: Disconnected, reason: %d\n", status);

		xTaskNotifyStateClear(xTaskToNotifyMqttConnect);
		/* Its more nice to be connected, so try to reconnect */
		mqtt_connect(client);
	}
}

static void mqtt_sub_request_cb(void *arg, err_t result) {
	/* Just print the result code here for simplicity,
	 normal behaviour would be to take some action if subscribe fails like
	 notifying user, retry subscribe or disconnect from server */
	printf("Subscribe result: %d\n", result);
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic,
		u32_t tot_len) {
	printf("Incoming publish at topic %s with total length %u\n", topic,
			(unsigned int) tot_len);

	/* Decode topic string into a user defined reference */
	if (strcmp(topic, MQTT_SETTINGS_TOPIC) == 0) {
		inpub_id = 0;
	} else if (strcmp(topic, "another_topic") == 0) {
		inpub_id = 1;
	} else {
		/* For all other topics */
		inpub_id = 2;
	}
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len,
		u8_t flags) {
	printf("Incoming publish payload with length %d, flags %u\n", len,
			(unsigned int) flags);

	for (int i = 0; i < len; i++)
	{
		list_append(&MQTTdataBuffer, &(data[i]));
	}

	if (flags & MQTT_DATA_FLAG_LAST) {
		/* Last fragment of payload received (or whole part if payload fits receive buffer
		 See MQTT_VAR_HEADER_BUFFER_LEN)  */

		/* Call function or do action depending on reference, in this case inpub_id */
		if (inpub_id == 0) {
			/* Don't trust the publisher, check zero termination */
			int size = list_size(&MQTTdataBuffer);
			uint8_t* payload = (uint8_t*)malloc(size);
			for (int i = 0; i < size; i++)
			{
				payload[i] = *((uint8_t*)(list_get(&MQTTdataBuffer, i)->data));
			}

			MQTT_Protocol_t sendData;
			sendData.topicName = "subtopic";
			sendData.data = payload;
			sendData.dataLenght = size;
			xQueueSend(xQueueMQTTReceiveMessage, &sendData, 0);

			list_destroy(&MQTTdataBuffer);
			list_new(&MQTTdataBuffer, sizeof(uint8_t), NULL);
		} else {
			printf("mqtt_incoming_data_cb: Ignoring payload...\n");
		}
	} else {
		/* Handle fragmented payload, store in buffer, write to file or whatever */
	}
}

void online_publish(mqtt_client_t *client, void *arg) {
	err_t err;
	u8_t qos = 2;
	u8_t retain = 0;
	err = mqtt_publish(client, MQTT_ONLINE_TOPIC, MQTT_CLIENT_ID, strlen(MQTT_CLIENT_ID),
			qos, retain, mqtt_pub_request_cb, arg);
	if (err != ERR_OK) {
		printf("Publish online err: %d\n", err);
	}
}

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result) {
	if (result != ERR_OK) {
		printf("Publish result: %d\n", result);
	}
}

/* This callback is called by the HAL_UART_IRQHandler when the given number of bytes are received */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		/* Send the next four bytes to the stream buffer. */
		xStreamBufferSendFromISR(xStreamReceiveBuffer, (void*) &uartRxData, 1,
		NULL);

		/* Receive one byte in interrupt mode */
		HAL_UART_Receive_IT(&huart2, &uartRxData, 1);
	}
}

// Таска для парсинга протокола протеза.
void StartProtocolParserTask(void *pvParameters) {
	configASSERT(xStreamReceiveBuffer != NULL);

	for (;;) {
		uint8_t receivedData;
		xStreamBufferReceive(xStreamReceiveBuffer, (void*) &receivedData, 1,
		portMAX_DELAY);

		uint8_t* data = malloc(sizeof(uint8_t));
		data[0] = receivedData;
		Prothesis_Protocol_t prothesisSendData;
		prothesisSendData.protocolCommand = Telemetry;
		prothesisSendData.data = data;
		prothesisSendData.dataLenght = 1;
		xQueueSend(xQueueUARTReceiveMessage, &prothesisSendData, portMAX_DELAY);
	}

	/* Tasks must not attempt to return from their implementing
	 function or otherwise exit.  In newer FreeRTOS port
	 attempting to do so will result in an configASSERT() being
	 called if it is defined.  If it is necessary for a task to
	 exit then have the task call vTaskDelete( NULL ) to ensure
	 its exit is clean. */
	vTaskDelete( NULL);
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
	MX_LWIP_Init();
	/* USER CODE BEGIN 5 */

	// Initialize MQTT queue
	xQueueMQTTSendMessage = xQueueCreate(qpeekQUEUE_LENGTH,
			sizeof(MQTT_Protocol_t));
	xQueueMQTTReceiveMessage = xQueueCreate(qpeekQUEUE_LENGTH,
			sizeof(MQTT_Protocol_t));

	printf("[MQTT Task] Start MQTT\n");
	list_new(&MQTTdataBuffer, sizeof(uint8_t), NULL);
	xTaskToNotifyMqttConnect = xTaskGetCurrentTaskHandle();
	mqtt_client_t *client = mqtt_client_new();
	if (client != NULL) {
		mqtt_connect(client);
	}

	// Ожидаем пока MQTT клиент будет запущен
	printf("[MQTT Task] Wait MQTT client start\n");
	ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
	printf("[MQTT Task] Start receive MQTT data\n");

	/* Infinite loop */
	for (;;) {
		// TODO: Добавить семафор на отправку, т.к. впустую трачу процессорное время на прокрутку в цикле, другие потоки не будут работать в это время
		// Смотрим очередь отправки на сервер
		if (uxQueueMessagesWaiting(xQueueMQTTSendMessage)) {
			MQTT_Protocol_t mqttSendData;
			if (xQueueReceive(xQueueMQTTSendMessage, &mqttSendData,
					(TickType_t) 0) == pdPASS) {
				char buffer[100];
				snprintf(buffer,
						sizeof(buffer),
						"[MQTT Task] Required send new MQTT data (%u bytes) on topic `%s`\n",
						(unsigned int)mqttSendData.dataLenght,
						mqttSendData.topicName);
				printf(buffer);
				err_t err = mqtt_publish(client, mqttSendData.topicName, mqttSendData.data, mqttSendData.dataLenght,
							2, 0, mqtt_pub_request_cb, 0);
				if (err == ERR_OK) {
					printf("[MQTT Task] Publish success\n");
				} else {
					printf("[MQTT Task] Publish err: %d\n", err);
				}

				free(mqttSendData.data);
			}
		}

		// Смотрим очередь приема с сервера
		if (uxQueueMessagesWaiting(xQueueMQTTReceiveMessage)) {
			MQTT_Protocol_t mqttReceiveData;
			if (xQueueReceive(xQueueMQTTReceiveMessage, &mqttReceiveData,
					(TickType_t) 0) == pdPASS) {

				printf("[MQTT Task] Receive new data from topic %s with length: %u", mqttReceiveData.topicName, (unsigned int)mqttReceiveData.dataLenght);

				Prothesis_Protocol_t prothesisSendData;

				if (strcmp(mqttReceiveData.topicName, "subtopic") == 0)
				{
					prothesisSendData.protocolCommand = Telemetry;
				}

				configASSERT(prothesisSendData.protocolCommand != Empty);

				prothesisSendData.data = mqttReceiveData.data;
				prothesisSendData.dataLenght = mqttReceiveData.dataLenght;
				xQueueSend(xQueueUARTSendMessage, &prothesisSendData, portMAX_DELAY);
			}
		}
		osDelay(50);
	}
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartUartTask */
/**
 * @brief Function implementing the uartTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartUartTask */
void StartUartTask(void const *argument) {
	/* USER CODE BEGIN StartUartTask */
	/* Infinite loop */
	// Создаем стрим для UART
	xStreamReceiveBuffer = xStreamBufferCreate(sbiSTREAM_BUFFER_LENGTH_BYTES,
			sbiSTREAM_BUFFER_TRIGGER_LEVEL_1);

	// Создаем очереди приема/передачи
	xQueueUARTReceiveMessage = xQueueCreate(qpeekQUEUE_LENGTH,
				sizeof(Prothesis_Protocol_t));

	xQueueUARTSendMessage = xQueueCreate(qpeekQUEUE_LENGTH,
					sizeof(Prothesis_Protocol_t));


	// Создаем таску для парсинга протокола
	BaseType_t xReturned;
	TaskHandle_t xHandle = NULL;
	xReturned = xTaskCreate(StartProtocolParserTask, "protocolPareserTask", 256, NULL, osPriorityNormal, &xHandle);

	if( xReturned == pdPASS )
	{
		/* The task was created. */
		printf("[UART Task] the protocol parser task was create\n");
	}

	configASSERT(xReturned == pdPASS);

	// Активируем прерывание UART.
	HAL_UART_Receive_IT(&huart2, &uartRxData, 1);
	printf("[UART Task] start UART interrupt and receive data\n");

	for (;;) {
		// TODO: Добавить семафор на отправку, т.к. впустую трачу процессорное время на прокрутку в цикле, другие потоки не будут работать в это время
		// Смотрим очередь отправки на протез
		if (uxQueueMessagesWaiting(xQueueUARTSendMessage)) {
			Prothesis_Protocol_t sendData;
			if (xQueueReceive(xQueueUARTSendMessage, &sendData,
					(TickType_t) 0) == pdPASS) {

				printf("[UART Task] Required send new UART data (%u bytes), command %02X\n",
						(unsigned int)sendData.dataLenght, sendData.protocolCommand);

				HAL_UART_Transmit(&huart2, sendData.data, sendData.dataLenght, 1000);
				free(sendData.data);
			}
		}

		// Смотрим очередь приема с сервера
		if (uxQueueMessagesWaiting(xQueueUARTReceiveMessage)) {
			Prothesis_Protocol_t sendData;
			if (xQueueReceive(xQueueUARTReceiveMessage, &sendData,
					(TickType_t) 0) == pdPASS) {

				printf("[UART Task] Receive new data with length: %u bytes and command %02X\n",
						(unsigned int)sendData.dataLenght, sendData.protocolCommand);


				MQTT_Protocol_t mqttSendData;

				switch (sendData.protocolCommand) {
				case Telemetry:
					mqttSendData.topicName = MQTT_TELEMETRY_TOPIC;
					break;
				default:
					mqttSendData.topicName = NULL;
					break;
				}
				configASSERT(mqttSendData.topicName);

				mqttSendData.data = sendData.data;
				mqttSendData.dataLenght = sendData.dataLenght;
				xQueueSend(xQueueMQTTSendMessage, &mqttSendData, portMAX_DELAY);
			}
		}

		osDelay(100);
	}
	/* USER CODE END StartUartTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
