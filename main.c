#include <STC8A.H>
#include <intrins.h>
#include <string.h>

sbit STAT = P2^7;        //UART ͨ�ŷ�ʽ:δ���ӵ͵�ƽ�����Ӻ�ߵ�ƽ
sbit PWRC = P2^6;        //������״̬����Ҫ�� AT ָ��ʱ���������ű��ֵ͵�ƽ
                         //��δ����״̬�´����Ų��ܸߵ͵�ƽ��Ϊ AT ָ��ģʽ
sbit RS485DIR = P3^5;    //����485��ʹ�ܽ�		

unsigned char Receive[11]={0}; 
unsigned char Receive4[9]={0};
unsigned char index = 0;
unsigned char index4 = 0;
	
/**************************************
	   ��ʱ��2��ʼ�����д��ڲ�����
**************************************/	
//frequency ��Ƭ��Ƶ�ʣ�baudrate ������
 void set_baudrate(float frequency,long int baudrate)
{	
	unsigned int itmp; 	
	unsigned char tlow,thigh;

	IRC24MCR = 0x80;//ʹ���ڲ�24M�߾���IRC����
	
	AUXR |= 0x01;	  //ѡ��ʱ��2Ϊ����1�����ʷ�����
	AUXR |= 0x04;		//��ʱ��2�ٶȿ���λ������Ƶ1T
	
	SCON = 0x50;    //�ɱ䲨����8λ���ݷ�ʽ
	PCON = 0x00;    //����1�ĸ���ģʽ�����ʶ����ӱ�,SMOD=0
	S2CON = 0x50;   //�����ڽ������ݣ�ѡ��ʱ��2Ϊ����2�����ʷ�����
	S3CON = 0x10;   //�����ڽ������ݣ�ѡ��ʱ��2Ϊ����3�����ʷ�����
	S4CON = 0x10;   //�����ڽ������ݣ�ѡ��ʱ��2Ϊ����4�����ʷ�����
	
	itmp = (int)(65536-(frequency*1000000)/(baudrate*4));
	thigh = itmp/256; 	
	tlow = itmp%256; 	

	TH2 = thigh; 	
	TL2 = tlow; 	

	AUXR |= 0x10;		//������ʱ��2
}

/**************************************
	           �����ʱ
**************************************/
void Delay100ms()		//@24.000MHz
{
	unsigned char i, j, k;

	_nop_();
	_nop_();
	i = 13;
	j = 45;
	k = 214;
	do
	{
		do
		{
			while (--k);
		} 
		while (--j);
	} 
	while (--i);
}

void Delay5ms()		  //@24.000MHz
{
	unsigned char i, j;

	_nop_();
	_nop_();
	i = 156;
	j = 213;
	do
	{
		while (--j);
	} while (--i);
}

/**************************************
	           ����ֵ�ȶ�
**************************************/
//ָ���ֽ����ڱȶԽ��һ�£�����0�����ȶԵ�ֵ��ͬ���ֽ�ʱ�����ص�ǰ�ֽڵĲ�ֵ
//*p1��*p2 �ַ����飻length �Ƚ��ֽ���
int string_cmp(const char *p1,const char *p2,unsigned int length)
{    
    const unsigned char *s1=(const unsigned char*)p1;    
    const unsigned char *s2=(const unsigned char*)p2;    
    unsigned char  c1,c2;
	  unsigned int   count = 0;
	
    do
    {
        c1=(unsigned char)*s1++;        
        c2=(unsigned char)*s2++;
        count += 1;	
			
        if(count == length)            
          return c1-c2;    
     }
     while(c1 == c2);    
        return c1-c2;        
}

/**************************************
	           ���ݷ���
**************************************/
/*----------------����1--------------*/

bit send_ok = 0;bit receive_ok = 0;
void UART_Send_Byte(char aChar)
{
	SBUF = aChar;
 	send_ok = 1;
	while(send_ok)         //�ȴ����ͻ���Ϊ��
	{
		if(TI)
		{
			TI=0;
			send_ok=0;
		}
	}
}

/*----------------����2---------------*/

bit send2_ok = 0;bit receive2_ok = 0;
void UART2_Send_Byte(char aChar)
{
	S2BUF = aChar;
 	send2_ok = 1;
	while(send2_ok)        //�ȴ����ͻ���Ϊ��
	{
		if(S2CON & 0x02)     //S3TI = 1
	  {
		  S2CON &= ~0x02;
			send2_ok=0;
		}
	}
}

/*----------------����3---------------*/

bit send3_ok = 0;
void UART3_Send_Byte(char aChar)
{
	S3BUF = aChar;
 	send3_ok = 1;	
	while(send3_ok)        //�ȴ����ͻ���Ϊ��
	{
		if(S3CON & 0x02)     //S3TI = 1
	  {
		  S3CON &= ~0x02;
			send3_ok=0;
		}
	}
}

/*-----------------����4--------------*/

bit send4_ok = 0;
void UART4_Send_Byte(char aChar)
{
	S4BUF = aChar;
 	send4_ok = 1;
	while(send4_ok)        //�ȴ����ͻ���Ϊ��
	{
		if(S4CON & 0x02)     //S4TI = 1
	  {
		  S4CON &= ~0x02;
			send4_ok=0;
		}
	}
}

/**************************************
	            LiDar
**************************************/	
//���Ϳ�ʼ����F4 05 00 00 00 00 CRC(У��) 4F
void UART_Send_Start()             
{
	unsigned char start[8];
	unsigned int i;
	start[0]=0xF4;start[1]=0x05;start[2]=0x00;start[3]=0x00;
  start[4]=0x00;start[5]=0x00;start[6]=0xCC;start[7]=0x4F;
	for ( i = 0; i< 8; i++ )
	{
		UART_Send_Byte( start[i] );
	}
}

//����ֹͣ����F4 06 00 00 00 00 88 4F
void UART_Send_Stop()              
{
	unsigned char stop[8];
	unsigned int i;
	stop[0]=0xF4;stop[1]=0x06;stop[2]=0x00;stop[3]=0x00;
  stop[4]=0x00;stop[5]=0x00;stop[6]=0x88;stop[7]=0x4F;
	for ( i = 0; i< 8; i++ )
	{
		UART_Send_Byte( stop[i] );
	}
}

/**************************************
	           �������
**************************************/	
//����Length-1������+1�������ۼӺ�
//void UART2_Send(unsigned char *Buffer, unsigned char Length)
//{
//	unsigned char i=0;
//	while(i<Length)
//	{
//		if(i<(Length-1))
//		{
//		 Buffer[Length-1]+=Buffer[i];
//		}
//		UART2_Send_Byte(Buffer[i]);
//		i+=1;
//	}
//}

//�����ȡ����������
//void read_code(unsigned char datas)
//{
//	unsigned char bytes[5];
//	memset(bytes,0,sizeof(bytes));
//	bytes[0]=0x3E;
//	bytes[1]=0x90;
//	bytes[2]=datas;     //���id
//	bytes[3]=0x00;
//	UART2_Send(bytes,5); //����֡ͷ�������ֽڡ�У���
//}

//λ�ñջ�����1(Ŀ��λ�ã���Ȧ�Ƕ��ۼ�ֵ��)
//send ���id;*Data ����λ��(8���ֽ�)
void locate_1(unsigned char send,unsigned char *Data)
{
	unsigned char TX_DATA2[14],k=0,j=0,l=5,m=0;
	memset(TX_DATA2,0,sizeof(TX_DATA2));//��շ�������
	TX_DATA2[0]=0X3E;                   //֡ͷ
	TX_DATA2[1]=0XA3;                   //֡ͷ
	TX_DATA2[2]=send;                   //���id
	TX_DATA2[3]=0x08;                   //���ݳ���
	
	while(j<4)                          //1~4λУ���
	{
		TX_DATA2[4]+=TX_DATA2[j];
		j+=1;
	}
	
	for(k=0;k<8;k++)                    //�������ݵ�����TX_DATA����
	{
		TX_DATA2[k+5]=Data[k];
	}
	
	while(l<13)                         //6~13λУ���
	{
		TX_DATA2[13]+=TX_DATA2[l];
		l+=1;
	}
	
	while(m<14)
	{
		UART2_Send_Byte(TX_DATA2[m++]);   //��֡����
	}
}

//λ�ñջ�����4(ת������Ŀ��λ�ã���Ȧ�Ƕ�ֵ���͵��ת��������ٶ�)
//id_code ���id;direction ת������*angle ����λ��(8���ֽ�)��*speed ת���ٶ�
void locate_4(unsigned char id_code,unsigned char direction,unsigned char *angle,unsigned char *speed)
{
	unsigned char TX_DATA2[14],k=0,j=0,i=0,l=5,m=0;
	memset(TX_DATA2,0,sizeof(TX_DATA2));//��շ�������
	TX_DATA2[0]=0X3E;                   //֡ͷ
	TX_DATA2[1]=0XA6;                   //֡ͷ
	TX_DATA2[2]=id_code;                //���id
	TX_DATA2[3]=0x08;                   //���ݳ���
	
	while(j<4)                          //1~4λУ���
	{
		TX_DATA2[4]+=TX_DATA2[j];
		j+=1;
	}
	
	TX_DATA2[5]=direction;              //���뷽�����ݵ�����TX_DATA����
	
	for(k=0;k<3;k++)                    //����Ƕ����ݵ�����TX_DATA����
	{
		TX_DATA2[k+6]=angle[k];
	}
	
	for(i=0;i<4;i++)                    //�����ٶ����ݵ�����TX_DATA����
	{
		TX_DATA2[i+9]=speed[i];
	}
	
	while(l<13)                         //6~13λУ���
	{
		TX_DATA2[13]+=TX_DATA2[l];
		l+=1;
	}
	
	while(m<14)
	{
		UART2_Send_Byte(TX_DATA2[m++]);
	}
}

//ʮ������ת�������ֽ���ǰ��
unsigned char intTohex[3],intTohex4[4],intTohex8[8];
unsigned char change3(long number)     //3���ֽ�
{
	intTohex[0]=(char)number;
	intTohex[1]=(number>>8)&0x00ff;
	intTohex[2]=(number>>16)&0x00ff;
	return intTohex;
}
unsigned char change4(long number)     //4���ֽ�
{
	intTohex4[0]=(char)number;
	intTohex4[1]=(number>>8)&0x00ff;
	intTohex4[2]=(number>>16)&0x00ff;
	intTohex4[3]=(number>>24)&0x00ff;
	return intTohex4;
}
unsigned char change8(long number)     //8���ֽ�
{
	intTohex8[0]=(char)number;
	intTohex8[1]=(number>>8)&0x00ff;
	intTohex8[2]=(number>>16)&0x00ff;
	intTohex8[3]=(number>>24)&0x00ff;
	intTohex8[4]=(number>>32)&0x00ff;
	intTohex8[5]=(number>>40)&0x00ff;
	intTohex8[6]=(number>>48)&0x00ff;
	intTohex8[7]=(number>>56)&0x00ff;
	return intTohex8;
}

/**************************************
	          ttl����ͷ
**************************************/	
void UART4_Send_String(char *aString, unsigned int StringLength)
{
	unsigned int i;
	for ( i = 0; i< StringLength; i++ )
	{
		UART4_Send_Byte( aString[i] );
	}
}

//��������a\b\d
void take_photo(unsigned char a,unsigned char b)
{
	unsigned char TX_DATA[5];
	memset(TX_DATA,0,sizeof(TX_DATA));//��շ�������
	TX_DATA[0]=0x56;                  //֡ͷ
	TX_DATA[1]=0x00;                  //���к�
	TX_DATA[2]=a;                     //������
	TX_DATA[3]=0x01;                  //���ݳ���
	TX_DATA[4]=b;                     
	
	UART4_Send_String(TX_DATA,5);
}

//��������c  56 00 32 0C 00 0C 00 00 00 00 xx xx xx xx 00 FF
void take_data(unsigned char ah,unsigned char bh,unsigned char ch,unsigned char dh)
{
	unsigned char TX_DATA[16];
	memset(TX_DATA,0,sizeof(TX_DATA));//��շ�������
	TX_DATA[0]=0x56;                  //֡ͷ
	TX_DATA[1]=0x00;                  //���к�
	TX_DATA[2]=0x32;                  
	TX_DATA[3]=0x0C;                  
	TX_DATA[4]=0x00;                  
	TX_DATA[5]=0x0C;                  //������ʽ
	TX_DATA[6]=0x00;                  //4bytes��ʼ��ַ
	TX_DATA[7]=0x00;
	TX_DATA[8]=0x00;
	TX_DATA[9]=0x00;
	TX_DATA[10]=ah;                   //4bytesͼ�񳤶�
  TX_DATA[11]=bh;
	TX_DATA[12]=ch;
	TX_DATA[13]=dh;
	TX_DATA[14]=0x00;                 //2byte��ʱʱ��
	TX_DATA[15]=0xFF;                 //��ʱʱ��ָ���񷵻����������ݼ��ʱ��������λΪ0.01mS
	
	UART4_Send_String(TX_DATA,16);
}

//����ͷ��������
//��������������쳣������������������ִ��
void UART4_work()
{
	long     int  last,num=0;          //������
	unsigned char getd;                //������ת
	unsigned char ah,bh,ch,dh;
	unsigned char res1[5],res2[5],res3[5];
	
	//ָ��ȶ�
	res1[0]=0x76;res1[1]=0x00;res1[2]=0x36;res1[3]=0x00;res1[4]=0x00;
	res2[0]=0x76;res2[1]=0x00;res2[2]=0x34;res2[3]=0x00;res2[4]=0x04;
	res3[0]=0x76;res3[1]=0x00;res3[2]=0x26;res3[3]=0x00;res3[4]=0x00;
		 
	memset(Receive4,0,9);	
	index4 = 0;
	IE2 = 0x00;	
	take_photo(0x36,0x00);             //ֹͣ��ǰ֡ˢ��	  
	
	IE2 |= ES4;		
	while(index4!=5);
  if(string_cmp(Receive4,res1,5)!= 0)return;
			
	
	IE2 = 0x00;	    
	memset(Receive4,0,9);
	index4 = 0;
	take_photo(0x34,0x00);             //��ȡͼƬ����
			
	IE2 |= ES4;
	while(index4!=9);
	if(string_cmp(Receive4,res2,5)!= 0)return;

	IE2 = 0x00;
	
	ah = Receive4[5];                  //ȡ��ͼ�񳤶���Ϣ
	bh = Receive4[6]; 
	ch = Receive4[7];
	dh = Receive4[8];  
	last = (ah << 24)|(bh << 16)|(ch << 8)|dh;
	last += 10;
	
	take_data(ah,bh,ch,dh);            //��ȡͼƬ
	while(num != last)
	{
		if(S4CON & 0x01) 
	  {
			S4CON &= ~0x01; 
			getd = S4BUF;
			UART3_Send_Byte(getd);
			num += 1;
		}		 
	} 
			
	memset(Receive4,0,9);
	index4 = 0;
	take_photo(0x36,0x02);             //�ָ�֡ˢ��
				
	IE2 |= ES4;
	while(index4 != 5);
  if(string_cmp(Receive4,res1,5)!= 0)return;
			
  if(P_SW2 == 0x00)                  //����ͷ�����л�
	{
		P_SW2 = 0x04;
	}
  else P_SW2 = 0x00;			
}

void main()
{
	unsigned long i,j,r,sum;
	unsigned char count;
  unsigned char real[4],angle[3];
	
	for(r = 0;r<30;r++)Delay100ms();   //�ϵ���ʱ
	
	real[0]=0xF4;real[1]=0x07;real[2]=0x00;real[3]=0x00;

	set_baudrate(24, 115200); 
			
	EA = 1;
	IE2 = 0x00;
	
	RS485DIR=1;                        //��485ģ�鷢��ʹ�ܣ��رս��չ���
	
	change8(0);
	locate_1(0x01,intTohex8);          //ʹ��λ�ñջ�����1������ٶȵ���Ĭ�ϳ�ʼλ��
	Delay100ms();

	locate_1(0x04,intTohex8);
	Delay100ms();

	locate_1(0x03,intTohex8);
	Delay100ms();
	
	for(r=0;r<20;r++)Delay100ms();
	
	while (STAT == 0);                 //�������Ӽ��
	
	P_SW1 = 0x80;
			
//	r=0;
//	do
//	{
//    ES = 0;
//		UART_Send_Byte(0xA5);//���Ͷ���λ��ָ��
//	  UART_Send_Byte(0x45);
//	  UART_Send_Byte(0xEA);
//		
//		index = 0;SBUF = 0x00;RI = 0;
//		ES = 1;
//			
//		while(index!=11);
//		
////		if(receive_ok) //���ڽ������
////		{
//			ES = 0;

//			UART_Send_Byte(0xA5);//����ֹͣ����λ��ָ��
//		  UART_Send_Byte(0x75);
//		  UART_Send_Byte(0x1A);
//			
//			for(sum=0,i=0;i<(Receive[3]+4);i++)//��λ��angle_data[3]=2
//			{
//				sum+=Receive[i];
//			}
//			
//			if(sum==Receive[10])                //У����ж�
//			{
//				angle[0]=(Receive[4]<<8)|Receive[5];
//				angle[1]=(Receive[6]<<8)|Receive[7];
//				angle[2]=(Receive[8]<<8)|Receive[9];//ʵ��ֵ��100��
//				
//			  UART3_Send_Byte(Receive[0]);
//				UART3_Send_Byte(Receive[1]);
//				UART3_Send_Byte(Receive[2]);
//				UART3_Send_Byte(Receive[3]);
//	
//			  UART3_Send_Byte(Receive[4]);
//				UART3_Send_Byte(Receive[5]);
//				UART3_Send_Byte(Receive[6]);
//				UART3_Send_Byte(Receive[7]);
//				UART3_Send_Byte(Receive[8]);
//				UART3_Send_Byte(Receive[9]);
//				r+=1;
//	}			
//			receive_ok=0;                               //����������ϱ�־			
////		}
//	}while(r<3);



//do
//			{
		index = 0;  
		UART_Send_Byte(0xA5);//���Ͷ���λ��ָ��
	  UART_Send_Byte(0x45);
	  UART_Send_Byte(0xEA);
			
		SBUF = 0x00;RI = 0;
		ES = 1;
	  Delay5ms();
			
		while(index!=11);
    ES = 0;
			    
		UART_Send_Byte(0xA5);//����ֹͣ����λ��ָ��
		UART_Send_Byte(0x75);
	  UART_Send_Byte(0x1A);
		Delay5ms();
//			    if(string_cmp(Receive,real,4)==0)
//			    {
		UART3_Send_Byte(Receive[4]);
		UART3_Send_Byte(Receive[5]);
		UART3_Send_Byte(Receive[6]);
	  UART3_Send_Byte(Receive[7]);
		UART3_Send_Byte(Receive[8]);
		UART3_Send_Byte(Receive[9]);
//					 count += 1;
//			    }	
//			    index = 0;

//		  }while(count == 0);
    


	P_SW1 = 0x40;
	
	change4(18000);
	for (i=0;i<=18;i++)
	{      		
		change3(i*1000); 
		
		locate_4(0x01, 0x00, intTohex, intTohex4);
		
		Delay100ms();
		
    for (j=0;j<=27;j++)
		{ 
			count = 0;
			change3(j*1000);
      RS485DIR = 1;	
      IE2 = 0x00; 
      locate_4(0x03, 0x00, intTohex, intTohex4);
			Delay100ms();
			
		 do
		 {
			index = 0;  
			UART_Send_Start();
			
			SBUF = 0x00;RI = 0;
			ES = 1;
		  Delay5ms();
			
			while(index!=8);
      ES = 0;
			    
			UART_Send_Stop();
			
			if(string_cmp(Receive,real,4)==0)
			{
				UART3_Send_Byte(Receive[0]);
				UART3_Send_Byte(Receive[1]);
				UART3_Send_Byte(Receive[2]);
				UART3_Send_Byte(Receive[3]);
			  UART3_Send_Byte(Receive[4]);
				UART3_Send_Byte(Receive[5]);
			  UART3_Send_Byte(Receive[6]);
				UART3_Send_Byte(Receive[7]);
				count += 1;
			}	
			index = 0;

		 }while(count == 0);
    }
		
		change3(0);

    locate_4(0x03, 0x01, intTohex, intTohex4);
		
		for(r=0;r<=15;r++)Delay100ms();
  }	
  
	change3(0);
	locate_4(0x01, 0x01, intTohex, intTohex4);
	for (i=0;i<=20;i++)Delay100ms();
	
	change4(36000);
	for (i=0;i<=5;i++)
	{      		
    change3(i*6000); 
		
		locate_4(0x01, 0x00, intTohex, intTohex4);
		
		for(r=0;r<=5;r++)Delay100ms();
		
    for (j=0;j<=5;j++)
		{ 
			change3(j*6000);
      RS485DIR = 1;
      IE2 = 0x00; 
      locate_4(0x03, 0x00, intTohex, intTohex4);
			for(r=0;r<=5;r++)Delay100ms();
			
			UART4_work();Delay100ms();
			UART4_work();
			
    }
		
		change3(0);

    locate_4(0x03, 0x01, intTohex, intTohex4);
		
		for(r=0;r<=15;r++)Delay100ms();
  }	
  while(1);	
}

/**************************************
	          �����жϺ���
**************************************/	
void Usart() interrupt 4
{

	  if (TI)
    {
			TI = 0;                 //���TIλ
			send_ok=0;
    }
		
    if (RI)
		{      
			Receive[index]=SBUF;
			index += 1;
			RI = 0;                 //���RIλ
//			if (index==11)
//      {
//				index = 0;
//				receive_ok = 1;
//      }				
		}
		
}

//void UART2_Isr() interrupt 8
//{
//	
//	if(S2CON & 0x02)          //S2TI = 1
//	{
//		S2CON &= ~0x02;
//		send2_ok = 0;
//	}
//		
//	if(S2CON & 0x01)          //S2RI = 1
//	{
//		Receive2[index2]=S2BUF;
//		index2 += 1;
//		S2CON &= ~0x01;         
//		if (index2==8)
//    {
//			index2 = 0;
//			receive2_ok = 1;
//    }		
//	}
//	
//}

void UART4_Isr() interrupt 18
{
	
	if(S4CON & 0x02)          //S4TI = 1
	{
		S4CON &= ~0x02;
		send4_ok = 0;
	}
		
	if(S4CON & 0x01)          //S4RI = 1
	{
		S4CON &= ~0x01;
		Receive4[index4]=S4BUF;
	  index4 +=1;
	}
	
}