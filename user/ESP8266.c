#include "stm32f10x.h"
#include "usart1.h"
#include <string.h>
#include <stdio.h>
#include "ESP8266.h"
#include "SysTick.h"
#include "dma.h"
#include "led.h"
#include "stm32f10x_it.h"


int to_index=0;  //ȫ�ֱ���������mqtt�ӿڵĽ������ݺ���transport_getdata()
void initCGQ(void) //�������ṹ��
{
	
	cgq_proper.Buzzer= FALSE;
	cgq_proper.LightSwitch= FALSE;  
	cgq_proper.shuibengzhuangtai= FALSE;
	cgq_proper.Env_lux=100;   //��Ҫ7λ
	cgq_proper.soilHumidity=60.0;   
	
	cgq_proper.CombustibleGasCheck=20.00; //��Ҫ 7λ
	cgq_proper.Temperature=29.2;
	cgq_proper.chuangliankaiguan= FALSE;
	
}

/*
 * ���ܣ�����1�жϷ�������
 * ������None
 * ����ֵ��None 
*/ 
void USART1_IRQHandler(void){
	  static u16  i=0;
  if(USART_GetITStatus(USART1, USART_IT_RXNE) ) 
  {
    RxBuffer1[i++] = USART_ReceiveData(USART1);  
    if(i==RxBuffer_size){
		   i = RxBuffer_size-1;
		}
	
	}
  if(USART_GetITStatus(USART1, USART_IT_IDLE))                   
  { 

    USART_ReceiveData(USART1); 
		i=0;
		
  }
} 



//�����ַ���
/*
 * ���ܣ������ַ����Ƿ����һ���ַ���
 * ����
 *       dest��������Ŀ���ַ���
 *       src��
 *       retry_cn ����ѯ��ʱʱ��
 * ����  
 *       1�ɹ� 0ʧ��
 * ˵�� 
 *       ������һ��ATָ�����Ҫһ��ʱ��ȴ�8266�ظ� 
 *       ���ʱ��ͨ���Ǽ���ms  ���� 
*/

static u8 findstr(char * dest,char * src,u16 retry_cn){
   u16 retry =retry_cn;
	 u8 result_flag=0;
	
	while(strstr(dest,src)==0 && --retry!=0){
   Delay_ms(10);
}
	if(retry==0){
	  return 0;
	}
  result_flag=1;
	
	if(result_flag){
	   return 1;
	}else{
	   return 0;
	}
		
}


u8 checkESP8266(void){
   memset(RxBuffer1,0,RxBuffer_size);
	 Usart_SendString(USART1,"AT\r\n");
	if(findstr(RxBuffer1,"OK",200)!=0){
		return 1;
	}else{
		return 0;
	}
	
}

//initESP8266
/*
 * 
*/
u8 initESP8266(void){
   Usart_SendString(USART1,"+++"); //�˳�͸��
	 Delay_ms(500);
	 Usart_SendString(USART1,"AT+RST\r\n");  //����
	 Delay_ms(1000);
	if(checkESP8266()==0){
		return 0; 
	}	
	memset(RxBuffer1,0,RxBuffer_size);
	Usart_SendString(USART1,"ATE0\r\n");
	if(findstr(RxBuffer1,"OK",200)==0){
	   memset(RxBuffer1,0,RxBuffer_size);
		 return 0;
	}
	return 1;
}



void restoreESP8266(void){
   Usart_SendString(USART1,"+++");
	 Delay_ms(500);
	 Usart_SendString(USART1,"AT+RESTORE\r\n");
	 NVIC_SystemReset();  //ͬʱ������Ƭ��;
}


/*
 * ����ֵ������ɹ�
*/
u8 connectAP(char* ssid, char* pwd){
   memset(RxBuffer1,0,RxBuffer_size);
	 Usart_SendString(USART1,"AT+CWMODE?\r\n");
	 if(findstr(RxBuffer1,"CWMODE:1",200)==0){
	    memset(RxBuffer1,0,RxBuffer_size);
		  Usart_SendString(USART1,"AT+CWMODE_CUR=1\r\n");
		  if(findstr(RxBuffer1,"OK",200)==0){
			    return 0;
			}
	 }
	 
	 memset(TxBuffer1,0,RxBuffer_size);
	 memset(RxBuffer1,0,RxBuffer_size);
	 sprintf(TxBuffer1,"AT+CWJAP_CUR=\"%s\",\"%s\"\r\n",ssid,pwd);
	 Usart_SendString(USART1,TxBuffer1);
	 if(findstr(RxBuffer1,"OK",800)!=0){
	    return 1;
	 }
	 return 0;
}

/*
 * ���ܣ� ʹ��ָ��Э�����ӵ�������
 * ������
 *        mode:Э������"TCP"/"UDP"
 *        
 * ����ֵ��
           1�ɹ���0ʧ��
   ˵����  
           ʧ��ԭ�������¼��֣�USART��ESP8266����������£�
           1. ip��port����
           2. δ����AP
           3. ��������ֹ���
*/
u8 connectServer(char* mode,char*  ip,u16 port){
   memset(RxBuffer1,0,RxBuffer_size);
	 memset(TxBuffer1,0,RxBuffer_size);
	 
	 Usart_SendString(USART1,"+++");          ///���˳�͸��;
	 Delay_ms(500);
	/*��ʽ��������AT����*/
	 sprintf(TxBuffer1,"AT+CIPSTART=\"%s\",\"%s\",%d\r\n",mode,ip,port);
	Usart_SendString(USART1,TxBuffer1);
	if(findstr(RxBuffer1,"CONNECT",800)!=0){
	    memset(RxBuffer1,0,RxBuffer_size);
		  Usart_SendString(USART1,"AT+CIPMODE=1\r\n");  //͸��ģʽ;
		  if(findstr(RxBuffer1,"OK",200)!=0){
			   memset(RxBuffer1,0,RxBuffer_size);
				 Usart_SendString(USART1,"AT+CIPSEND\r\n");  //����
				 if(findstr(RxBuffer1,">",200)!=0){
				     return 1;
				 }else{
				     return 0;
				 }
				 
			}else{
			    return 0;
				
			}
	}else{
	     return 0;
	}
}

/*
 * ���ܣ������ͷ������Ͽ�����
   ������None
   ����ֵ��
         ����Ͽ��ɹ���0�Ͽ�ʧ��
*/
u8 disconnectServer(void){
    Usart_SendString(USART1,"+++");   //�˳�͸��
	  Delay_ms(300);
	  memset(RxBuffer1,0,RxBuffer_size);
	  Usart_SendString(USART1,"AT+CIPCLOSE\r\n"); 
	  
	  if(findstr(RxBuffer1,"CLOSED",200)!=0) {
		   return 1;
		}else return 0;
}

/*
  ���ܣ� ͸��ģʽ�µ����ݷ��ͺ���
  ������ 
   
   ���أ� None
*/
void sendBuffertoServer(char* buffer){
    //memset(RxBuffer1,0,RxBuffer_size);
    Usart_SendString(USART1,buffer);	
}
/*
  ���ܣ���ֲmqttʵ�ַ������ݽӿ�
  ������ 
   
   ���أ� int
*/
int transport_sendPacketBuffer(const char*server_ip_and_port,unsigned char* buf, int buflen){  
   
	 memset(RxBuffer1,0,RxBuffer_size);
	 Usart_SendString(USART1,"+++");          ///���˳�͸��;
	 Delay_ms(500);
   Usart_SendString(USART1,"AT+CIPSTATUS\r\n");//��esp8266��ѯ����״̬���Ƿ������˷�����
	 
	 if(findstr(RxBuffer1,"STATUS:3",200)!= NULL){  //�Ѿ�����tcp
		//����͸��
		memset(RxBuffer1,0,RxBuffer_size);
		Usart_SendString(USART1,"AT+CIPMODE=1\r\n");  //͸��ģʽ;
		 
		if(findstr(RxBuffer1,"OK",200)!= NULL){
		     memset(RxBuffer1,0,RxBuffer_size);
				 Usart_SendString(USART1,"AT+CIPSEND\r\n");  //����
			   
			   if(findstr(RxBuffer1,">",200)!=NULL){
					 //���Է���������
					 memset(RxBuffer1,0,RxBuffer_size);
					 Usart1_SendU8Array(buf,buflen);  
					 memset(buf,0,MQTT_MAX_BUF_SIZE);
					 //�������±���Ϊ0
					 to_index=0;
					 //Delay_ms(1000);  ///���Ҫ����
				   return 1;
	    	}
				 //����͸��ʧ��
				 else return 0;
		
	  }
		//����͸��ʧ��
	  else return 0;
	 
 }
		//���û�н�������,�ͽ���������tcp
	  if(connectServer(MODE,IP,PORT)!=0){
			  memset(RxBuffer1,0,RxBuffer_size);
		    Usart1_SendU8Array(buf,buflen);
			 memset(buf,0,MQTT_MAX_BUF_SIZE);
			  to_index=0;
		    return 1;
		}
		//���ܽ���tcp;
		else{ return 0;}
		
}  
/*
  ���ܣ���ֲmqttʵ�ֽ������ݽӿ�
  ������ 
   
   ���أ� int
*/

int transport_getdata(unsigned char*buf, int count){
    
	  
	  //char*Reb =&RxBuffer1[0];
	  //int count2=strlen(Reb);
	 ///����Ӱ��̫���ˣ�
	  memcpy(buf,&RxBuffer1[to_index],count);
	  to_index+=count;
	 
	 //���ճɹ�����˼
	 return count;
    //count+=count2;
	  
}

/*����ͨ�Ż�ȡ�������ݣ��ݶ�100���ֽڵĽ���*/
void Near_transport_getdata(unsigned char*buf){
    
   memcpy(buf,&RxBuffer1,100);
	 memset(RxBuffer1,0,RxBuffer_size);
}


u8 led=0,shuibeng=0,fengming=0;

/*����ͨ�Ŵ�����յ�����*/
void Near_processServer(unsigned char*buf){
  
  if(strstr((const char*)buf,"ACL")!=NULL){ //LED
	    if(led%2==0){
	       LED(ON);
				 cgq_proper.LightSwitch=TRUE;
		  }
			else if(led%2==1){
			   LED(OFF);
				 cgq_proper.LightSwitch=FALSE;
				
			}
			++led;
	}

	if(strstr((const char*)buf,"ACF")!=NULL){	//ˮ��
	    if(shuibeng%2==0){
	       Shuibeng(ON);
				 cgq_proper.shuibengzhuangtai=TRUE;
		  }
			else if(shuibeng%2==1){
			   Shuibeng(OFF);
				 cgq_proper.shuibengzhuangtai=FALSE;
				
			}
			++shuibeng;
	}
	if(strstr((const char*)buf,"ACB")!=NULL){ //������
	    if(fengming%2==0){
	       Fengming(ON);
				 cgq_proper.Buzzer=TRUE;
		  }
			else if(fengming%2==1){
			   Fengming(OFF);
				 cgq_proper.Buzzer=FALSE;
				
			}
			++fengming;
	}
	
	
	
}


/*
 * ���ܣ���ȡ������������ֵ������ֵ��ȡ��CGQ_Buffer�У�
 * ������void
 * ����ֵ��char����
*/
char*  read_cgq(void){
  
	cgq_proper.Env_lux=(double)Read_lux();
	cgq_proper.soilHumidity=(double)Read_soil();
	
	sprintf(TxBuffer1,"{\"id\":\"123\",\"iotId\":\"JjvYMZ7j2ZGenDj3sZWU000100\", \
	\"method\":\"thing.event.property.post\",\"params\":{\"Buzzer\":%d,\"chuangliankaiguan\":%d,\
	\"CombustibleGasCheck\":%g,\"Env_lux\":%g,\"LightSwitch\":%d,\
	\"shuibengzhuangtai\":%d,\"soilHumidity\":%g,\"Temperature\":%g},\
	\"topic\":\"/sys/a1xFHTXX9bA/stm32/thing/event/property/post\",\"uniMsgId\":\"5081150873750638592\",\
	\"version\":\"1.0\" }",cgq_proper.Buzzer,cgq_proper.chuangliankaiguan,cgq_proper.CombustibleGasCheck,\
	cgq_proper.Env_lux,cgq_proper.LightSwitch,cgq_proper.shuibengzhuangtai,\
	cgq_proper.soilHumidity,cgq_proper.Temperature);
	
	return TxBuffer1;

}
/*������ƽ̨payload��������ִ����*/
void processServer(unsigned char*payload_IN){

	
	if(strstr((const char*)payload_IN,"\"shuibengzhuangtai\":1")!=NULL ||
		strstr((const char*)payload_IN,"\"sb_switch\":1")!=NULL){
	   Shuibeng(ON);
		 cgq_proper.shuibengzhuangtai=TRUE;
	}
	else if(strstr((const char*)payload_IN,"\"shuibengzhuangtai\":0")!=NULL || 
		  strstr((const char*)payload_IN,"\"sb_switch\":0")!=NULL){
	   Shuibeng(OFF);
		 cgq_proper.shuibengzhuangtai=FALSE;
	}
	
	if(strstr((const char*)payload_IN,"\"Buzzer\":1")!=NULL || 
		strstr((const char*)payload_IN,"\"buzzer_switch\":1")!=NULL){
	   Fengming(ON);
		 cgq_proper.Buzzer=TRUE;
	}
	else if(strstr((const char*)payload_IN,"\"Buzzer\":0")!=NULL || 
		strstr((const char*)payload_IN,"\"buzzer_switch\":0")!=NULL){
	   Fengming(OFF);
		 cgq_proper.Buzzer=FALSE;
	}
	
	if(strstr((const char*)payload_IN,"\"LightSwitch\":1")!=NULL || 
		strstr((const char*)payload_IN,"\"LED_switch\":1")!=NULL){
	   LED(ON);
		 cgq_proper.LightSwitch=TRUE;
	}
	else if(strstr((const char*)payload_IN,"\"LightSwitch\":0")!=NULL|| 
		strstr((const char*)payload_IN,"\"LED_switch\":0")!=NULL){
	   LED(OFF);
		 cgq_proper.LightSwitch=FALSE;
	}
	
}


