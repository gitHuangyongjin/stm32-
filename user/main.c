
/**************************************
 * �ļ���  ��main.c   
 * ʵ��ƽ̨��MINI STM32������ ����STM32F103C8T6
 * ��汾  ��ST3.0.0  																										  

*********************************************************/

#include "stm32f10x.h"
#include <string.h>
#include <stdio.h>
#include "MQTTPacket.h"
#include "usart1.h"
#include "ESP8266.h"
#include "SysTick.h"
#include "adc.h"
#include "dma.h"
#include "led.h"
#include "exti.h"


#define SSD "****"
#define PWD "****"

#ifndef  MQTT_MAX_BUF_SIZE
#define MQTT_MAX_BUF_SIZE 500
#endif

#define Near_IP1 "192.168.2.105"
#define Near_IP2 "192.168.2.114"
#define Near_port 8080

#ifndef MODE
#define MODE "TCP"
#define IP "a1xFa1xFHTXX9bA.iot-as-mqtt.cn-shanghai.aliyuncs.com"
#define PORT 1883


#endif

#define MQTT_SERVER_IP_AND_PORT "a1xFa1xFHTXX9bA.iot-as-mqtt.cn-shanghai.aliyuncs.com, 1883"



int main(void)
{  
	 //mqtt��ز���
	  u16  retry_count = 5;
    unsigned short submsgid;
    unsigned char buf[MQTT_MAX_BUF_SIZE];
    unsigned char sessionPresent, connack_rc;
    const char* payload = "mypayload";
    int payloadlen = strlen(payload);
    int ret = 0,len = 0,req_qos = 0,msgid = 1,granted_qos,subcount;

    MQTTString topicString = MQTTString_initializer;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;   
	//ϵͳ��ʼ��
  SystemInit();	
	USART1_Config(); 
	SysTick_Init();
	Adc_Init();
	DMA_Configuration();
	LED_GPIO_Config();
	EXTI_Configuration();
	initCGQ();
	
	
	if(initESP8266()==1)
			{
		    //��ʾOLED 
				if(connectAP(SSD,PWD)==1)
			{
		    
				//���ݰ�����ֵ ѭ������Զ������
		while(1){		
			    
				if(read_key_down()==0){  //��ʼ���İ���ֵΪ0
				  
					/////������������Զ������
					 disconnectServer();
					
           while(connectServer(MODE,IP,PORT)!=1){};
						 
					  data.clientID.cstring = "stm326666|securemode=3,signmethod=hmacsha1,timestamp=6666|";
            data.keepAliveInterval = 40;
            data.cleansession = 1;
            data.username.cstring = "stm32&a1xFHTXX9bA";
            data.password.cstring = "591AC4AFDE29C9127A479B1F6502FB37AE5FE1B3";
					  data.MQTTVersion = 4;
            
            len = MQTTSerialize_connect(buf, MQTT_MAX_BUF_SIZE, &data);
					  ret = transport_sendPacketBuffer(MQTT_SERVER_IP_AND_PORT, buf, len);
            if( ret != 1 ){
              continue;
            }
					  Delay_ms(1000);
						
					 /* wait for connack */
           if (MQTTPacket_read(buf, MQTT_MAX_BUF_SIZE, transport_getdata) != CONNACK)
           {
             continue;
           }
           if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, MQTT_MAX_BUF_SIZE) != 1 || connack_rc != 0)
           {
             continue;
           }
				 
				   memset(buf,0,MQTT_MAX_BUF_SIZE);
				 
          topicString.cstring = "/sys/a1xFHTXX9bA/stm32/thing/service/property/set";
          len = MQTTSerialize_subscribe(buf, MQTT_MAX_BUF_SIZE, 0, msgid, 1, &topicString, &req_qos);
           ret = transport_sendPacketBuffer(MQTT_SERVER_IP_AND_PORT, buf, len);
           if( ret != 1 ){
               continue;
            }
            Delay_ms(1000);
						
          if (MQTTPacket_read(buf, MQTT_MAX_BUF_SIZE, transport_getdata) != SUBACK)   /* wait for suback */
           {
             continue;
          }
		
          MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, MQTT_MAX_BUF_SIZE);
         if (granted_qos != 1)
         {
             continue;
          }
					
          memset(buf,0,MQTT_MAX_BUF_SIZE);

					
					/* loop getting msgs on subscribed topic  and Near_transmision*/
           topicString.cstring = "/sys/a1xFHTXX9bA/stm32/thing/event/property/post";
			      while( read_key_down()==0 ){   
              memset(buf,0,MQTT_MAX_BUF_SIZE);							
              
             /* transport_getdata() has a built-in 1 second timeout,
               your mileage will vary */
             if (MQTTPacket_read(buf, MQTT_MAX_BUF_SIZE, transport_getdata) == PUBLISH)
             {
                 int qos,payloadlen_in;
                 unsigned char dup,retained;
                 unsigned short msgid;
                  unsigned char* payload_in;
                 MQTTString receivedTopic;

                 MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                    &payload_in, &payloadlen_in, buf, MQTT_MAX_BUF_SIZE);
							 
							 //���յ��ƶ�����topic�����ô���ֵ����
							  processServer(payload_in);
							 
              }
						 							
						 //�ڷ�����ϢǰҲҪ��buf��Ϊ��
							memset(buf,0,MQTT_MAX_BUF_SIZE);		
							
							///����payload��payloadlen�Ϳ��Է�����
							payload=read_cgq();
							payloadlen=strlen(payload);
             len = MQTTSerialize_publish(buf, MQTT_MAX_BUF_SIZE, 0, 0, 0, 0, topicString, (unsigned char*)payload, payloadlen);

              //�ظ�����5�Σ������ɹ����˳�
							retry_count=2;
                while( retry_count-- > 0 ){
                    ret = transport_sendPacketBuffer(MQTT_SERVER_IP_AND_PORT, buf, len);
             
                    if( ret == 1 ){
                       break;
                     }
                    Delay_ms(100);
                }
                if( !retry_count && ret != 1 ){

                    continue;
                }
						   	// ÿ�η��ͣ��ȴ�3.5s
                Delay_ms(3500);	
								
            }
					
					}//Զ���˳�
				
          disconnectServer();
				
						
			 ///_______�������룺
						if(read_key_down()==1){
							unsigned char Near_buf[100];
							
								//����ѭ��	
              while(connectServer(MODE,Near_IP1,Near_port)==0&&
                    connectServer(MODE,Near_IP2,Near_port)==0){};
					    
							while(read_key_down()==1){
					        char string[30];
								  sprintf(string,"TEMP=28.2|RH=%g",Read_soil());
								  sendBuffertoServer(string);
								   Delay_ms(2000);
								   Near_transport_getdata(Near_buf);
								   Near_processServer(Near_buf);
							}
							disconnectServer();
				 }
						
				 
	 }/////����Զ��ѭ����
		
					
 }
}
	
	while(1){
	   //��֤������Ӳ���ж�
	}

}



