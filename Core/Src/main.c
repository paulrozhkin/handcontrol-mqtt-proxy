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
#include "timers.h"

#include "buffer.h"
#include "protocol_parser.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef struct {
	char *topicName;
	uint8_t *data;
	uint32_t dataLenght;
} MQTT_Protocol_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LOG_HEAP 0
#define LOG_ENABLE 0

#define sbiSTREAM_BUFFER_LENGTH_BYTES		( ( size_t ) 100 )
#define sbiSTREAM_BUFFER_TRIGGER_LEVEL_1	( ( BaseType_t ) 1 )

#define qpeekQUEUE_LENGTH (20)

// MQTT_ONLINE_PUBLISH_PERIOD для отправки ID клиента в топик MQTT_ONLINE_TOPIC
#define MQTT_ONLINE_PUBLISH_PERIOD_MS	60000
#define MQTT_CLIENT_ID 					"f2c8eb77-ea6e-4411-bf14-b8a564d17a97"

// Online/Offline
#define MQTT_ONLINE_TOPIC 				"controllers/online"
#define MQTT_OFFLINE_TOPIC				"controllers/offline"

// Telemetry
#define MQTT_TELEMETRY_TOPIC 			MQTT_CLIENT_ID "/data/telemetry"

// Settings
#define MQTT_GET_SETTINGS_TOPIC 		MQTT_CLIENT_ID "/data/settings"
#define MQTT_SET_SETTINGS_TOPIC 		MQTT_CLIENT_ID "/action/settings"

// Gestures
#define MQTT_GET_GESTURES_TOPIC 		MQTT_CLIENT_ID "/data/gestures"
#define MQTT_SAVE_GESTURES_TOPIC 		MQTT_CLIENT_ID "/action/gestures"
#define MQTT_DELETE_GESTURES_TOPIC 		MQTT_CLIENT_ID "/action/gestures/remove"

// Perform gesture
#define MQTT_PERFORM_GESTURE_ID_TOPIC 	MQTT_CLIENT_ID "/action/performGestureId"
#define MQTT_PERFORM_GESTURE_RAW_TOPIC 	MQTT_CLIENT_ID "/action/performGestureRaw"
#define MQTT_SET_POSITIONS_TOPIC 		MQTT_CLIENT_ID "/action/positions"
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
/* Stores the handle of the task that will be notified when UART receives data or receives a request to send data. */
static TaskHandle_t xTaskToNotifyUartSendAndReceive = NULL;
static TaskHandle_t xTaskToNotifyTelemetryReceive = NULL;
static ProtocolParserStruct protocolParser;

/* === MQTT PV === */
static QueueHandle_t xQueueMQTTSendMessage = NULL;
static QueueHandle_t xQueueMQTTReceiveMessage = NULL;
/* Stores the handle of the task that will be notified when MQTT receives data or receives a request to send data. */
static TaskHandle_t xTaskToNotifyMqttSendAndReceive = NULL;
static char *lastTopicMQTT;
static buffer_t *MQTTdataBuffer = NULL;
static SemaphoreHandle_t xSemaphoreMqttConnect;
static TimerHandle_t xOnlinePublishTimer;
static mqtt_client_t *client;

// TODO: global refactoring
static SemaphoreHandle_t xSemaphoreProsthesisConnect;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
void StartDefaultTask(void const *argument);
void StartUartTask(void const *argument);

/* USER CODE BEGIN PFP */
void StartProtocolParserTask(void *argument);
void vOnlinePublishCallback(TimerHandle_t xTimer);
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
int main(void) {
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
	xSemaphoreMqttConnect = xSemaphoreCreateBinary();
	xSemaphoreGive(xSemaphoreMqttConnect);

	xSemaphoreProsthesisConnect = xSemaphoreCreateBinary();
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	xOnlinePublishTimer = xTimerCreate("PublishTimer",
			pdMS_TO_TICKS(MQTT_ONLINE_PUBLISH_PERIOD_MS),
			pdTRUE, (void*) 0, vOnlinePublishCallback);
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
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };

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
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Activate the Over-Drive mode
	 */
	if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART2;
	PeriphClkInitStruct.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

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
	if (HAL_UART_Init(&huart2) != HAL_OK) {
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
static void MX_GPIO_Init(void) {

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */
static char *input_topic;

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

#if( LOG_ENABLE == 1 )
	printf("mqtt_connect: start try to connect to 92.53.124.175:1883\n");
#endif
	/* Setup an empty client info structure */
	memset(&ci, 0, sizeof(ci));

	/* Minimal amount of information required is client identifier, so set it here */
	ci.client_id = MQTT_CLIENT_ID;
	ci.will_topic = MQTT_OFFLINE_TOPIC;
	ci.will_retain = 0;
	ci.will_qos = 2;
	ci.will_msg = MQTT_CLIENT_ID;

	ip_addr_t mqttServerIP;
	IP4_ADDR(&mqttServerIP, 92, 53, 124, 175);
	u16_t MQTT_PORT = 1883;

	/* Initiate client and connect to server, if this fails immediately an error code is returned
	 otherwise mqtt_connection_cb will be called with connection result after attempting
	 to establish a connection with the server.
	 For now MQTT version 3.1.1 is always used */

	err = mqtt_client_connect(client, &mqttServerIP, MQTT_PORT,
			mqtt_connection_cb, 0, &ci);

	/* For now just print the result code if something goes wrong */
	if (err != ERR_OK) {
#if( LOG_ENABLE == 1 )
		printf("mqtt_connect return %d\n", err);
#endif /* LOG */
	}
}

/**
 * Подключение прошло успешно.
 */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg,
		mqtt_connection_status_t status) {
	err_t err;
	if (status == MQTT_CONNECT_ACCEPTED) {
#if( LOG_ENABLE == 1 )
		printf("mqtt_connection_cb: Successfully connected\n");
#endif /* LOG */

		/* Setup callback for incoming publish requests */
		mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb,
				mqtt_incoming_data_cb, arg);

		// Подписываемся на все топики, которые нужны
		/* Subscribe to a topic named "serverData" with QoS level 1, call mqtt_sub_request_cb with result */
		err = mqtt_subscribe(client, MQTT_SET_SETTINGS_TOPIC, 1,
				mqtt_sub_request_cb, arg);

		if (err != ERR_OK) {
#if( LOG_ENABLE == 1 )
			printf(
					"MQTT_SET_SETTINGS_TOPIC mqtt_connection_cb: mqtt_subscribe return: %d\n",
					err);
#endif /* LOG */

			Error_Handler();
		}

		err = mqtt_subscribe(client, MQTT_SAVE_GESTURES_TOPIC, 1,
				mqtt_sub_request_cb, arg);

		if (err != ERR_OK) {
#if( LOG_ENABLE == 1 )
			printf(
					"MQTT_SAVE_GESTURES_TOPIC mqtt_connection_cb: mqtt_subscribe return: %d\n",
					err);
#endif /* LOG */
			Error_Handler();
		}

		err = mqtt_subscribe(client, MQTT_DELETE_GESTURES_TOPIC, 1,
				mqtt_sub_request_cb, arg);

		if (err != ERR_OK) {
#if( LOG_ENABLE == 1 )
			printf(
					"MQTT_DELETE_GESTURES_TOPIC mqtt_connection_cb: mqtt_subscribe return: %d\n",
					err);
#endif /* LOG */
			Error_Handler();
		}

		err = mqtt_subscribe(client, MQTT_PERFORM_GESTURE_ID_TOPIC, 1,
				mqtt_sub_request_cb, arg);

		if (err != ERR_OK) {
#if( LOG_ENABLE == 1 )
			printf(
					"MQTT_PERFORM_GESTURE_ID_TOPIC mqtt_connection_cb: mqtt_subscribe return: %d\n",
					err);
#endif /* LOG */
			Error_Handler();
		}

		err = mqtt_subscribe(client, MQTT_PERFORM_GESTURE_RAW_TOPIC, 1,
				mqtt_sub_request_cb, arg);

		if (err != ERR_OK) {
#if( LOG_ENABLE == 1 )
			printf(
					"MQTT_PERFORM_GESTURE_RAW_TOPIC mqtt_connection_cb: mqtt_subscribe return: %d\n",
					err);
#endif /* LOG */
			Error_Handler();
		}

		err = mqtt_subscribe(client, MQTT_SET_POSITIONS_TOPIC, 1,
				mqtt_sub_request_cb, arg);

		if (err != ERR_OK) {
#if( LOG_ENABLE == 1 )
			printf(
					"MQTT_SET_POSITIONS_TOPIC mqtt_connection_cb: mqtt_subscribe return: %d\n",
					err);
#endif /* LOG */
			Error_Handler();
		}

		if (xTimerStart( xOnlinePublishTimer, 500) == pdTRUE) {
#if( LOG_ENABLE == 1 )
			printf("mqtt_connection_cb: Start online publisher\n");
#endif /* LOG */
		}

		// Публикуем в брокер, что мы онлайн в первый раз
		online_publish(client, arg);

		// Оповещеняем основную таску, что клиент подключен
		xSemaphoreGive(xSemaphoreMqttConnect);
	} else {
#if( LOG_ENABLE == 1 )
		printf("mqtt_connection_cb: Disconnected, reason: %d\n", status);
#endif /* LOG */

		xSemaphoreTake(xSemaphoreMqttConnect, portMAX_DELAY);
		/* Its more nice to be connected, so try to reconnect */
		xTimerStop(xOnlinePublishTimer, portMAX_DELAY);
		mqtt_connect(client);
	}
}

static void mqtt_sub_request_cb(void *arg, err_t result) {
	/* Just print the result code here for simplicity,
	 normal behaviour would be to take some action if subscribe fails like
	 notifying user, retry subscribe or disconnect from server */
#if( LOG_ENABLE == 1 )
	printf("mqtt_sub_request_cb: subscribe result is %d\n", result);
#endif /* LOG */
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic,
		u32_t tot_len) {
#if( LOG_ENABLE == 1 )
	printf("Incoming publish at topic %s with total length %u\n", topic,
			(unsigned int) tot_len);
#endif /* LOG */

	// Очищаем предыдущий буфер, если он остался (такого быть по идее не должно, только если пропало соединение)
	if (MQTTdataBuffer != NULL) {
#if( LOG_ENABLE == 1 )
		printf(
				"Receive buffer is set for topic with id %s. Destroy receive buffer with length %d bytes\n",
				lastTopicMQTT, buffer_lenght(MQTTdataBuffer));
#endif /* LOG */
		buffer_destroy(MQTTdataBuffer);
		vPortFree(MQTTdataBuffer);
		MQTTdataBuffer = NULL;
	}

	lastTopicMQTT = input_topic;

	// Определяем id топика для получения данных в mqtt_incoming_data_cb
	/* Decode topic string into a user defined reference */
	if (strcmp(topic, MQTT_SET_SETTINGS_TOPIC) == 0) {
		input_topic = MQTT_SET_SETTINGS_TOPIC;
	} else if (strcmp(topic, MQTT_SAVE_GESTURES_TOPIC) == 0) {
		input_topic = MQTT_SAVE_GESTURES_TOPIC;
	} else if (strcmp(topic, MQTT_DELETE_GESTURES_TOPIC) == 0) {
		input_topic = MQTT_DELETE_GESTURES_TOPIC;
	} else if (strcmp(topic, MQTT_PERFORM_GESTURE_ID_TOPIC) == 0) {
		input_topic = MQTT_PERFORM_GESTURE_ID_TOPIC;
	} else if (strcmp(topic, MQTT_PERFORM_GESTURE_RAW_TOPIC) == 0) {
		input_topic = MQTT_PERFORM_GESTURE_RAW_TOPIC;
	} else if (strcmp(topic, MQTT_SET_POSITIONS_TOPIC) == 0) {
		input_topic = MQTT_SET_POSITIONS_TOPIC;
	} else {
		/* For all other topics */
		input_topic = NULL;
	}

	// Создаем буффер для приема данных.
#if( LOG_ENABLE == 1 )
	printf("Create new receive buffer for topic %s\n", input_topic);
#endif /* LOG */

	MQTTdataBuffer = pvPortMalloc(sizeof(buffer_t));
	buffer_init(MQTTdataBuffer, 0);
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len,
		u8_t flags) {

	configASSERT(MQTTdataBuffer != NULL);

	buffer_array_add(MQTTdataBuffer, (uint8_t*) data, len);

#if( LOG_HEAP == 1 )
	// Log свободного места в куче после приема данных
	printf("[MQTT Heap] Receive bytes %d\n", len);
	printf("[MQTT Heap] FreeHeap %d\n", xPortGetFreeHeapSize());
	printf("[MQTT Heap] MinimumEver %d\n", xPortGetMinimumEverFreeHeapSize());
#endif /* LOG_HEAP */

	if (flags & MQTT_DATA_FLAG_LAST) {
		/* Last fragment of payload received (or whole part if payload fits receive buffer
		 See MQTT_VAR_HEADER_BUFFER_LEN)  */

#if( LOG_ENABLE == 1 )
		printf("Incoming publish payload with length %d, flags %u\n", len,
				(unsigned int) flags);
#endif /* LOG */

		/* Call function or do action depending on reference, in this case inpub_id */
		if (input_topic != NULL) {
			/* Don't trust the publisher, check zero termination */
			int size = buffer_lenght(MQTTdataBuffer);
			uint8_t *payload = MQTTdataBuffer->data;
			MQTTdataBuffer->data = NULL;
			vPortFree(MQTTdataBuffer);
			MQTTdataBuffer = NULL;

			MQTT_Protocol_t sendData;
			sendData.topicName = input_topic;
			sendData.data = payload;
			sendData.dataLenght = size;
			xQueueSend(xQueueMQTTReceiveMessage, &sendData, 0);

			// Оповещаем MQTT таску, что пришли новые данные.
			xTaskNotifyGive(xTaskToNotifyMqttSendAndReceive);
		} else {
#if( LOG_ENABLE == 1 )
			printf("mqtt_incoming_data_cb: Ignoring payload...\n");
#endif /* LOG */
		}
	} else {
		/* Handle fragmented payload, store in buffer, write to file or whatever */
	}
}

void vOnlinePublishCallback(TimerHandle_t xTimer) {
	if (xSemaphoreTake(xSemaphoreMqttConnect, 1000) == pdTRUE) {
		online_publish(client, (void*) 0);
		xSemaphoreGive(xSemaphoreMqttConnect);
	}
}

void online_publish(mqtt_client_t *client, void *arg) {
	err_t err;
	u8_t qos = 2;
	u8_t retain = 0;
	err = mqtt_publish(client, MQTT_ONLINE_TOPIC, MQTT_CLIENT_ID,
			strlen(MQTT_CLIENT_ID), qos, retain, mqtt_pub_request_cb, arg);

#if( LOG_ENABLE == 1 )
	printf("online_publish: Send online status\n");
#endif /* LOG */

	if (err != ERR_OK) {
#if( LOG_ENABLE == 1 )
		printf("Publish online err: %d\n", err);
#endif /* LOG */
	}
}

/* Called when publish is complete either with sucess or failure */
static void mqtt_pub_request_cb(void *arg, err_t result) {
	if (result != ERR_OK) {
#if( LOG_ENABLE == 1 )
		printf("Publish result: %d\n", result);
#endif /* LOG */
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

	ProtocolParser_Init(&protocolParser);
	bool isFirstTelemetry = true;

	for (;;) {
		uint8_t receivedData;
		xStreamBufferReceive(xStreamReceiveBuffer, (void*) &receivedData, 1,
		portMAX_DELAY);

		ProtocolParser_Update(&protocolParser, receivedData, HAL_GetTick());

		if (protocolParser.state == PROTOCOL_PARSER_RECEIVED) {
			ProtocolPackageStruct package;
			ProtocolParser_PopPackage(&protocolParser, &package);

			if (package.type != PROTOCOL_COMMAND_TELEMETRY)
			{
#if( LOG_ENABLE == 1 )
				printf("[Parser] New protocol command received: %d (%u bytes)\n",
						package.type, (unsigned int) package.size);
#endif /* LOG */
			}

			// Телеметрию шлем напрямую
			if (package.type == PROTOCOL_COMMAND_TELEMETRY) {
				if (xQueueMQTTSendMessage != NULL) {
					MQTT_Protocol_t mqttSendData;

					mqttSendData.topicName = MQTT_TELEMETRY_TOPIC;
					mqttSendData.data = package.payload;
					mqttSendData.dataLenght = package.size;

					xQueueSend(xQueueMQTTSendMessage, &mqttSendData,
							portMAX_DELAY);
				}
				// Оповещаем MQTT таску, что данные готовы.

				if (isFirstTelemetry == true) {
#if( LOG_ENABLE == 1 )
					printf("[Parser] First telemetry received\n");
#endif /* LOG */

					xSemaphoreGive(xSemaphoreProsthesisConnect);
					isFirstTelemetry = false;
				}

				xTaskNotifyGive(xTaskToNotifyMqttSendAndReceive);
			} else {
				xQueueSend(xQueueUARTReceiveMessage, &package, portMAX_DELAY);
				// Оповещаем UART таску, что приняты данные с протеза.
				xTaskNotifyGive(xTaskToNotifyUartSendAndReceive);
			}
		}
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
void StartDefaultTask(void const *argument) {
	/* init code for LWIP */
	MX_LWIP_Init();
	/* USER CODE BEGIN 5 */

	// Initialize MQTT queue
	xQueueMQTTReceiveMessage = xQueueCreate(qpeekQUEUE_LENGTH,
			sizeof(MQTT_Protocol_t));

#if( LOG_ENABLE == 1 )
	printf("[MQTT Task] Start MQTT\n");
#endif /* LOG */

	xTaskToNotifyMqttSendAndReceive = xTaskGetCurrentTaskHandle();
	client = mqtt_client_new();

	// Ожидаем подключения протеза
#if( LOG_ENABLE == 1 )
	printf("[MQTT Task] Wait Prosthesis connect\n");
#endif /* LOG */

	xSemaphoreTake(xSemaphoreProsthesisConnect, portMAX_DELAY);
	xSemaphoreGive(xSemaphoreProsthesisConnect);
	if (client != NULL) {
		mqtt_connect(client);
	} else {
#if( LOG_ENABLE == 1 )
		printf("Error creating MQTT client\n");
#endif /* LOG */
	}

	configASSERT(client != NULL);

	// Ожидаем пока MQTT клиент будет запущен
#if( LOG_ENABLE == 1 )
	printf("[MQTT Task] Wait MQTT client start\n");
#endif /* LOG */

	xSemaphoreTake(xSemaphoreMqttConnect, portMAX_DELAY);
	xSemaphoreGive(xSemaphoreMqttConnect);

	xQueueMQTTSendMessage = xQueueCreate(qpeekQUEUE_LENGTH,
			sizeof(MQTT_Protocol_t));

#if( LOG_ENABLE == 1 )
	printf("[MQTT Task] Start receive MQTT data\n");
#endif /* LOG */

	/* Infinite loop */
	for (;;) {

		ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
		//printf("[MQTT Task] start handle notification\n");

		// Смотрим очередь отправки на сервер
		if (uxQueueMessagesWaiting(xQueueMQTTSendMessage)) {
			MQTT_Protocol_t mqttSendData;
			if (xQueueReceive(xQueueMQTTSendMessage, &mqttSendData,
					(TickType_t) 0) == pdPASS) {

				if (strcmp(mqttSendData.topicName, MQTT_TELEMETRY_TOPIC) != 0)
				{
#if( LOG_ENABLE == 1 )
					printf(
							"[MQTT Task] Required send new MQTT data (%u bytes) on topic `%s`\n",
							(unsigned int) mqttSendData.dataLenght,
							mqttSendData.topicName);
#endif /* LOG */
				}

				err_t err = mqtt_publish(client, mqttSendData.topicName,
						mqttSendData.data, mqttSendData.dataLenght, 2, 0,
						mqtt_pub_request_cb, 0);
				if (err == ERR_OK) {
					//printf("[MQTT Task] Publish success\n");
				} else {
#if( LOG_ENABLE == 1 )
					printf("[MQTT Task] Publish err: %d\n", err);
#endif /* LOG */
				}

				vPortFree(mqttSendData.data);
			}
		}

		// Смотрим очередь приема с сервера
		if (uxQueueMessagesWaiting(xQueueMQTTReceiveMessage)) {
			MQTT_Protocol_t mqttReceiveData;
			if (xQueueReceive(xQueueMQTTReceiveMessage, &mqttReceiveData,
					(TickType_t) 0) == pdPASS) {

#if( LOG_ENABLE == 1 )
				printf(
						"[MQTT Task] Receive new data from topic %s with length: %u\n",
						mqttReceiveData.topicName,
						(unsigned int) mqttReceiveData.dataLenght);
#endif /* LOG */

				// Формируем пакет данных для отправки по UART
				enum ProtocolCommandType type = PROTOCOL_COMMAND_EMPTY;

				char *topic = mqttReceiveData.topicName;
				if (strcmp(topic, MQTT_SET_SETTINGS_TOPIC) == 0) {
					type = PROTOCOL_COMMAND_SET_SETTINGS;
				} else if (strcmp(topic, MQTT_SAVE_GESTURES_TOPIC) == 0) {
					type = PROTOCOL_COMMAND_SAVE_GESTURES;
				} else if (strcmp(topic, MQTT_DELETE_GESTURES_TOPIC) == 0) {
					type = PROTOCOL_COMMAND_DELETE_GESTURES;
				} else if (strcmp(topic, MQTT_PERFORM_GESTURE_ID_TOPIC) == 0) {
					type = PROTOCOL_COMMAND_PERFORM_GESTURE_ID;
				} else if (strcmp(topic, MQTT_PERFORM_GESTURE_RAW_TOPIC) == 0) {
					type = PROTOCOL_COMMAND_PERFORM_GESTURE_RAW;
				} else if (strcmp(topic, MQTT_SET_POSITIONS_TOPIC) == 0) {
					type = PROTOCOL_COMMAND_SET_POSITIONS;
				}

				configASSERT(type != PROTOCOL_COMMAND_EMPTY);

				ProtocolPackageStruct package = ProtocolParser_CreatePackage(
						type, mqttReceiveData.data, mqttReceiveData.dataLenght);

				xQueueSend(xQueueUARTSendMessage, &package, portMAX_DELAY);

				// Оповещаем UART таску, что данные готовы.
				xTaskNotifyGive(xTaskToNotifyUartSendAndReceive);
			}
		}
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
			sizeof(ProtocolPackageStruct));

	xQueueUARTSendMessage = xQueueCreate(qpeekQUEUE_LENGTH,
			sizeof(ProtocolPackageStruct));

	xTaskToNotifyUartSendAndReceive = xTaskGetCurrentTaskHandle();
	xTaskToNotifyTelemetryReceive = xTaskGetCurrentTaskHandle();

	// Создаем таску для парсинга протокола
	BaseType_t xReturned;
	TaskHandle_t xHandle = NULL;
	xReturned = xTaskCreate(StartProtocolParserTask, "protocolPareserTask", 256,
	NULL, osPriorityNormal, &xHandle);

	if (xReturned == pdPASS) {
		/* The task was created. */
#if( LOG_ENABLE == 1 )
		printf("[UART Task] the protocol parser task was create\n");
#endif /* LOG */
	}

	configASSERT(xReturned == pdPASS);

	// Активируем прерывание UART.
	HAL_UART_Receive_IT(&huart2, &uartRxData, 1);
#if( LOG_ENABLE == 1 )
	printf("[UART Task] start UART interrupt and receive data\n");
#endif /* LOG */

	// Ожидаем подключеия протеза.
#if( LOG_ENABLE == 1 )
	printf("[UART Task] Wait Prosthesis connect\n");
#endif /* LOG */

	xSemaphoreTake(xSemaphoreProsthesisConnect, portMAX_DELAY);
	xSemaphoreGive(xSemaphoreProsthesisConnect);

	// Ожидаем пока MQTT клиент будет запущен
#if( LOG_ENABLE == 1 )
	printf("[UART Task] Wait MQTT client start\n");
#endif /* LOG */

	xSemaphoreTake(xSemaphoreMqttConnect, portMAX_DELAY);
	xSemaphoreGive(xSemaphoreMqttConnect);

	// Запросы на GetSettings и GetGestures
	ProtocolPackageStruct gesturesPackage = ProtocolParser_CreatePackage(PROTOCOL_COMMAND_GET_GESTURES, NULL, 0);
	ProtocolPackageStruct settingsPackage = ProtocolParser_CreatePackage(PROTOCOL_COMMAND_GET_SETTINGS, NULL, 0);
	xQueueSend(xQueueUARTSendMessage, &gesturesPackage, portMAX_DELAY);
	xQueueSend(xQueueUARTSendMessage, &settingsPackage, portMAX_DELAY);

#if( LOG_ENABLE == 1 )
	printf("[UART Task] start receive data\n");
#endif /* LOG */

	for (;;) {
		// Если очереди пусты, то ждем
		if (!(uxQueueMessagesWaiting(xQueueUARTSendMessage) || uxQueueMessagesWaiting(xQueueUARTReceiveMessage)))
		{
			ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
		}

#if( LOG_ENABLE == 1 )
		printf("[UART Task] start handle notification\n");
#endif /* LOG */

		// Смотрим очередь отправки на протез
		if (uxQueueMessagesWaiting(xQueueUARTSendMessage)) {
			ProtocolPackageStruct sendData;
			if (xQueueReceive(xQueueUARTSendMessage, &sendData,
					(TickType_t) 0) == pdPASS) {

#if( LOG_ENABLE == 1 )
				printf(
						"[UART Task] Required send new UART data (%u bytes), command %d\n",
						(unsigned int) sendData.size, sendData.type);
#endif /* LOG */

				buffer_t package;
				buffer_init(&package, ProtocolParser_GetCommonSize(&sendData));
				ProtocolParser_AddPackageToBuffer(&sendData, &package);

				HAL_UART_Transmit(&huart2, package.data, package.length, 5000);

				buffer_destroy(&package);

				ProtocolPackageStruct response;
				if (xQueueReceive(xQueueUARTReceiveMessage, &response,
						pdMS_TO_TICKS(5000)) != pdPASS) {
#if( LOG_ENABLE == 1 )
					printf("[UART Task] response timeout\n");
#endif /* LOG */
					continue;
				}

				if (response.type == PROTOCOL_COMMAND_ERR
						|| response.type == PROTOCOL_COMMAND_EMPTY) {
#if( LOG_ENABLE == 1 )
					printf("[UART Task] response error\n");
#endif /* LOG */
					continue;
				}

				if (response.type != PROTOCOL_COMMAND_ERR) {
#if( LOG_ENABLE == 1 )
					printf("[UART Task] Prosthesis send response\n");
#endif /* LOG */

					if (sendData.type == PROTOCOL_COMMAND_SAVE_GESTURES || sendData.type == PROTOCOL_COMMAND_DELETE_GESTURES)
					{
						xQueueSend(xQueueUARTSendMessage, &gesturesPackage, portMAX_DELAY);
					}

					if (sendData.type == PROTOCOL_COMMAND_SET_SETTINGS)
					{
						xQueueSend(xQueueUARTSendMessage, &settingsPackage, portMAX_DELAY);
					}

					if (response.type != PROTOCOL_COMMAND_ACK)
					{
#if( LOG_ENABLE == 1 )
						printf("[UART Task] Response %d sending to uart receive queue\n", response.type);
#endif /* LOG */

						xQueueSend(xQueueUARTReceiveMessage, &response,
								portMAX_DELAY);
						// Оповещаем UART таску, что данные готовы.
						xTaskNotifyGive(xTaskToNotifyUartSendAndReceive);
					}
				} else
				{
#if( LOG_ENABLE == 1 )
					printf("[UART Task] Error response\n");
#endif /* LOG */
				}

				vPortFree(sendData.payload);
			}
		}

		// Смотрим очередь приема с протеза
		if (uxQueueMessagesWaiting(xQueueUARTReceiveMessage)) {
			ProtocolPackageStruct receiveData;
			if (xQueueReceive(xQueueUARTReceiveMessage, &receiveData,
					(TickType_t) 0) == pdPASS) {

#if( LOG_ENABLE == 1 )
				printf(
						"[UART Task] Receive new data with length: %u bytes and command %d\n",
						(unsigned int) receiveData.size, receiveData.type);
#endif /* LOG */

				MQTT_Protocol_t mqttSendData;

				switch (receiveData.type) {
				case PROTOCOL_COMMAND_GET_GESTURES:
					mqttSendData.topicName = MQTT_GET_GESTURES_TOPIC;
					break;
				case PROTOCOL_COMMAND_GET_SETTINGS:
					mqttSendData.topicName = MQTT_GET_SETTINGS_TOPIC;
					break;
				default:
					mqttSendData.topicName = NULL;
					break;
				}

				configASSERT(mqttSendData.topicName);

				mqttSendData.data = receiveData.payload;
				mqttSendData.dataLenght = receiveData.size;

				if (xQueueMQTTSendMessage != NULL)
				{
					xQueueSend(xQueueMQTTSendMessage, &mqttSendData, portMAX_DELAY);
					// Оповещаем MQTT таску, что данные готовы.
					xTaskNotifyGive(xTaskToNotifyMqttSendAndReceive);
				}


			}
		}
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
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
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
void Error_Handler(void) {
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
