#include "boost.h"

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim1;
extern ADC_HandleTypeDef hadc1;

/* Ĭ�ϣ����� ˫���� ��ѹģʽ */
uint8_t arming_sta = 0;							//����״̬ 1������ 0������
uint8_t zone_mode = DOUBLE_ZONE;
uint8_t protection_level = HIGH_VOLTAGE_MODE;
uint8_t touch_net_mode = 0;  				//0:����ģʽ(����ⴥ��)   1:����ģʽ

uint8_t boost_delay_cnt = 0;
uint8_t release_pulse_time_cnt = 0;
uint16_t zone1_alarm_delay_cnt = 0;
uint16_t zone2_alarm_delay_cnt = 0;
//uint8_t broken_short_detect_time_cnt = 0;

uint8_t boost_finish_flag = 0;
uint8_t boost_delay_finish_mask = 0;
uint8_t release_pulse_finish_mask = 0;
uint8_t zone1_alarm_delay_finish_mask = 0;
uint8_t zone2_alarm_delay_finish_mask = 0;

uint16_t broken_short_a1_line_cnt = 0;
uint16_t broken_short_a2_line_cnt = 0;
uint16_t broken_short_a3_line_cnt = 0;
uint16_t broken_short_a4_line_cnt = 0;
uint16_t broken_short_b1_line_cnt = 0;
uint16_t broken_short_b2_line_cnt = 0;
uint16_t broken_short_b3_line_cnt = 0;
uint16_t broken_short_b4_line_cnt = 0;

uint16_t zone1_alarm_delay_time = 90;		// ���������ȼ���ó���ʱ����ֵ  * 10ms  Ĭ�ϣ�������Ϊ3  3:900ms   2:2.5s 1:5s
uint16_t zone2_alarm_delay_time = 90;		

alarm_sta_typedef zone1_alarm_sta = NORMAL_STA;
alarm_sta_typedef zone2_alarm_sta = NORMAL_STA;

alarm_sta_typedef pre_zone1_alarm_sta = NORMAL_STA;
alarm_sta_typedef pre_zone2_alarm_sta = NORMAL_STA;

uint8_t zone1_short_mask = 0;	//��·���α�־ �����������߱����Ͷ�·���������������������⵽��ѹ ˵��û�ж�·�� 0�������� 1������
uint8_t zone2_short_mask = 0;

/*��ʼǰ����ѹ*/
void start_primary_boost(void)
{
	HAL_GPIO_WritePin(PRIMARY_BOOST_ENABLE_PIN_GPIO_Port, PRIMARY_BOOST_ENABLE_PIN_Pin, GPIO_PIN_SET);	/*ʹ����ѹ���ƽ�*/
	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);	/*������ѹPWM*/
}

/*ֹͣǰ����ѹ*/
void stop_primary_boost(void)
{
	HAL_GPIO_WritePin(PRIMARY_BOOST_ENABLE_PIN_GPIO_Port, PRIMARY_BOOST_ENABLE_PIN_Pin, GPIO_PIN_RESET); /*ʧ����ѹ���ƽ�*/
	HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);	/*ֹͣ��ѹPWM*/
}

/*�����趨��ѹ ��ֹͣ*/
void primary_boost(uint8_t level)
{
	start_primary_boost();	/*��ʼǰ����ѹ */
	
	/*������ѹ�ȼ� �ȴ������趨�ȼ���Ӧ��ѹ*/
	if(level == LOW_VOLTAGE_MODE)
	{
		while(!HAL_GPIO_ReadPin(DETECT_LEVEL_LOW_GPIO_Port, DETECT_LEVEL_LOW_Pin));	
	}
	else if(level == HIGH_VOLTAGE_MODE)
	{
		while(!HAL_GPIO_ReadPin(DETECT_LEVEL_HIGH_GPIO_Port, DETECT_LEVEL_HIGH_Pin));	
	}
	//Ӧ�ӳ�ʱ�ж�  ��ֹ�ջ���·
	
	stop_primary_boost(); /*ֹͣǰ����ѹ*/
	boost_finish_flag = 1; /*��ѹ��ɱ�־��1*/
}

/*�ͷ�����*/
void ralease_pulse(uint8_t zone_num)
{
	if(zone_num == 1)
	{
		HAL_GPIO_WritePin(ZONE1_PULSE_RELEASE_PIN_GPIO_Port, ZONE1_PULSE_RELEASE_PIN_Pin, GPIO_PIN_RESET);
	}
	else if(zone_num == 2)
	{
		HAL_GPIO_WritePin(ZONE2_PULSE_RELEASE_PIN_GPIO_Port, ZONE2_PULSE_RELEASE_PIN_Pin, GPIO_PIN_RESET);	
	}
	
	HAL_Delay(1);
	touch_net_dectec(zone_num);
	
	while(!release_pulse_finish_mask);
	
	release_pulse_finish_mask = 0;
	boost_finish_flag = 0;
	
	if(zone_num == 1)
	{
		HAL_GPIO_WritePin(ZONE1_PULSE_RELEASE_PIN_GPIO_Port, ZONE1_PULSE_RELEASE_PIN_Pin, GPIO_PIN_SET);
	}
	else if(zone_num == 2)
	{
		HAL_GPIO_WritePin(ZONE2_PULSE_RELEASE_PIN_GPIO_Port, ZONE2_PULSE_RELEASE_PIN_Pin, GPIO_PIN_SET);	
	}
	
}

/*��ѹ�����ͷ�����*/
void boost_release(void)
{
	if(boost_delay_finish_mask)
	{
		boost_delay_finish_mask = 0;
		primary_boost(protection_level);
		ralease_pulse(ZONE1);
		broken_detect(ZONE1);
		if(zone_mode == DOUBLE_ZONE)
		{
			primary_boost(protection_level);
			ralease_pulse(ZONE2);
			broken_detect(ZONE2);
		}
	}
}

/*����������*/
void arming_disarm(uint8_t sta)
{
	arming_sta = sta;
	
	/*������� �������ֵ �ͱ�־λ*/
	if(!arming_sta)
	{
		boost_delay_cnt = 0;
		release_pulse_time_cnt = 0;
		zone1_alarm_delay_cnt = 0;
		zone2_alarm_delay_cnt = 0;
//		broken_short_detect_time_cnt = 0;

		boost_finish_flag = 0;
		boost_delay_finish_mask = 0;
		release_pulse_finish_mask = 0;
		zone1_alarm_delay_finish_mask = 0;
		zone2_alarm_delay_finish_mask = 0;
		
		broken_short_a1_line_cnt = 0;
		broken_short_a2_line_cnt = 0;
		broken_short_a3_line_cnt = 0;
		broken_short_a4_line_cnt = 0;
		broken_short_b1_line_cnt = 0;
		broken_short_b2_line_cnt = 0;
		broken_short_b3_line_cnt = 0;
		broken_short_b4_line_cnt = 0;
		
		zone1_alarm_sta = NORMAL_STA;
		pre_zone1_alarm_sta = NORMAL_STA;
		
		zone2_alarm_sta = NORMAL_STA;
		pre_zone2_alarm_sta = NORMAL_STA;
	}
}

/*����������*/
void set_sensitivity(uint8_t zone_num, uint8_t level)
{
	if(zone_num == ZONE1)
	{
		switch(level)
		{
			case 1: zone1_alarm_delay_time = 500;	break;
			case 2: zone1_alarm_delay_time = 250; break;
			case 3: zone1_alarm_delay_time = 90; break;
			default: break;
		}
	}
	
	if(zone_num == ZONE2)
	{
		switch(level)
		{
			case 1: zone2_alarm_delay_time = 500; 	break;
			case 2: zone2_alarm_delay_time = 250; break;
			case 3: zone2_alarm_delay_time = 90; break;
			default: break;
		}		
	}
}

/*���ߡ���·���*/
void broken_detect(uint8_t zone_num)
{
	if(zone_num == ZONE1)
	{
			/*�����һ���߻����û�м�⵽ �����ֵ�������� ������˵���0  ����*/
			if(!(broken_short_a1_line_cnt * broken_short_a2_line_cnt * broken_short_a3_line_cnt * broken_short_a4_line_cnt))
			{
				/*��������߾�û�з������� �������߼�����Ϊ0 �����Ϊ0 ���Ϊ0 ��·���� */
				if(!(broken_short_a1_line_cnt + broken_short_a2_line_cnt + broken_short_a3_line_cnt + broken_short_a4_line_cnt))
				{
					if(zone1_short_mask)
					{
						zone1_alarm_sta = BROKEN_STA;
					}
					else
					{
						zone1_alarm_sta = SHORT_STA;
					}
				}
				else
				{
					zone1_alarm_sta = BROKEN_STA;
				}
			}
			else if(zone1_alarm_sta != TOUCH_NET_STA)
			{
				zone1_alarm_sta = NORMAL_STA;
			}
			broken_short_a1_line_cnt = 0;
			broken_short_a2_line_cnt = 0;
			broken_short_a3_line_cnt = 0;
			broken_short_a4_line_cnt = 0;
	}
	else if(zone_num == ZONE2)
	{
			if(!(broken_short_b1_line_cnt * broken_short_b2_line_cnt * broken_short_b3_line_cnt * broken_short_b4_line_cnt))
			{
				if(!(broken_short_b1_line_cnt + broken_short_b2_line_cnt + broken_short_b3_line_cnt + broken_short_b4_line_cnt))
				{
					if(zone2_short_mask)
					{
						zone2_alarm_sta = BROKEN_STA;
					}
					else
					{
						zone2_alarm_sta = SHORT_STA;
					}
				}
				else
				{
					zone2_alarm_sta = BROKEN_STA;
				}
			}
			else if(zone2_alarm_sta != TOUCH_NET_STA)
			{
				zone2_alarm_sta = NORMAL_STA;
			}
			
			broken_short_b1_line_cnt = 0;
			broken_short_b2_line_cnt = 0;
			broken_short_b3_line_cnt = 0;
			broken_short_b4_line_cnt = 0;
	}
}

/*����״̬��ѯ */
void alarm_inquire(void)
{
	if(zone1_alarm_delay_finish_mask)
	{
		zone1_alarm_delay_finish_mask = 0;
		
		if(zone1_alarm_sta != pre_zone1_alarm_sta)
		{
			HAL_Delay(5);
			pre_zone1_alarm_sta = zone1_alarm_sta;
			send_sta_msg(ZONE1_STA, (uint8_t)zone1_alarm_sta);
		}
	}
	
	if(zone2_alarm_delay_finish_mask)
	{
		zone2_alarm_delay_finish_mask = 0;
		
		if(zone2_alarm_sta != pre_zone2_alarm_sta)
		{
			HAL_Delay(5);
			pre_zone2_alarm_sta = zone2_alarm_sta;
			send_sta_msg(ZONE2_STA, (uint8_t)zone2_alarm_sta);
		}
	}
}

float get_return_voltage(uint8_t zone_num)
{
	ADC_ChannelConfTypeDef sConfig;
	float reference_voltage = 1.24f;		//�ο���ѹ
	uint16_t vref_value;
	uint16_t get_value;
	float get_voltage;
	
	sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
	
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 10);
	vref_value = (uint16_t)HAL_ADC_GetValue(&hadc1);
	
	if(zone_num == ZONE1)
	{
		sConfig.Channel = ADC_CHANNEL_5;
	}
	else
	{
		sConfig.Channel = ADC_CHANNEL_6;
	}
	
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, 10);
	get_value = (uint16_t)HAL_ADC_GetValue(&hadc1);
	
	get_voltage = (float)(reference_voltage*get_value/vref_value);
	
	return get_voltage;
}

void touch_net_dectec(uint8_t zone_num)
{
	float voltage = 0.0f;
	float pre_voltage = 0.0f;
	float normal_voltage;
	float touch_net_voltage;
	
	if(protection_level == HIGH_VOLTAGE_MODE)
	{
		normal_voltage = 0.3f;
		touch_net_voltage = 1.3f;
	}
	else if(protection_level == LOW_VOLTAGE_MODE)
	{
		normal_voltage = 0.04f;
		touch_net_voltage = 0.2f;		
	}
	
	
//	
	while(voltage >= pre_voltage)
	{
		pre_voltage = voltage;
		voltage = get_return_voltage(zone_num);
		
		if(voltage == 0.0f)
		{
			break;
		}
	}
	voltage = pre_voltage;
//	voltage = get_return_voltage(zone_num);
	
	if((voltage >= touch_net_voltage) && touch_net_mode)
	{
		if(zone_num == ZONE1)           
		{
			zone1_alarm_sta = TOUCH_NET_STA;
		}
		else if(zone_num == ZONE2)
		{
			zone2_alarm_sta = TOUCH_NET_STA;
		}
	}
	else if(voltage >= normal_voltage)
	{
		if(zone_num == ZONE1)
		{
			zone1_short_mask = 1;
			zone1_alarm_sta = NORMAL_STA;
		}
		else if(zone_num == ZONE2)
		{
			zone2_short_mask = 1;
			zone2_alarm_sta = NORMAL_STA;
		}
	}
	else
	{
		if(zone_num == ZONE1)
		{
			zone1_short_mask = 0;
			zone1_alarm_sta = NORMAL_STA;
		}
 		else if(zone_num == ZONE2)
		{
			zone2_short_mask = 0;
			zone2_alarm_sta = NORMAL_STA;
		}
	}
}














