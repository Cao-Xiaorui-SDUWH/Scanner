#include <STC8A.H>
#include <intrins.h>
#include <string.h>

sbit STAT = P2^7;        //UART 通信方式:未连接低电平，连接后高电平
sbit PWRC = P2^6;        //在连接状态下需要发 AT 指令时，将此引脚保持低电平
                         //在未连接状态下此引脚不管高低电平均为 AT 指令模式
sbit RS485DIR = P3^5;    //定义485的使能脚		

unsigned char Receive[11]={0}; 
unsigned char Receive4[9]={0};
unsigned char index = 0;
unsigned char index4 = 0;
	
/**************************************
	   定时器2初始化所有串口波特率
**************************************/	
//frequency 单片机频率；baudrate 波特率
 void set_baudrate(float frequency,long int baudrate)
{	
	unsigned int itmp; 	
	unsigned char tlow,thigh;

	IRC24MCR = 0x80;//使能内部24M高精度IRC振荡器
	
	AUXR |= 0x01;	  //选择定时器2为串口1波特率发生器
	AUXR |= 0x04;		//定时器2速度控制位，不分频1T
	
	SCON = 0x50;    //可变波特率8位数据方式
	PCON = 0x00;    //串口1的各个模式波特率都不加倍,SMOD=0
	S2CON = 0x50;   //允许串口接收数据，选择定时器2为串口2波特率发生器
	S3CON = 0x10;   //允许串口接收数据，选择定时器2为串口3波特率发生器
	S4CON = 0x10;   //允许串口接收数据，选择定时器2为串口4波特率发生器
	
	itmp = (int)(65536-(frequency*1000000)/(baudrate*4));
	thigh = itmp/256; 	
	tlow = itmp%256; 	

	TH2 = thigh; 	
	TL2 = tlow; 	

	AUXR |= 0x10;		//启动定时器2
}

/**************************************
	           软件延时
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
	           返回值比对
**************************************/
//指定字节数内比对结果一致，返回0；当比对到值不同的字节时，返回当前字节的差值
//*p1、*p2 字符数组；length 比较字节数
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
	           数据发送
**************************************/
/*----------------串口1--------------*/

bit send_ok = 0;bit receive_ok = 0;
void UART_Send_Byte(char aChar)
{
	SBUF = aChar;
 	send_ok = 1;
	while(send_ok)         //等待发送缓存为空
	{
		if(TI)
		{
			TI=0;
			send_ok=0;
		}
	}
}

/*----------------串口2---------------*/

bit send2_ok = 0;bit receive2_ok = 0;
void UART2_Send_Byte(char aChar)
{
	S2BUF = aChar;
 	send2_ok = 1;
	while(send2_ok)        //等待发送缓存为空
	{
		if(S2CON & 0x02)     //S3TI = 1
	  {
		  S2CON &= ~0x02;
			send2_ok=0;
		}
	}
}

/*----------------串口3---------------*/

bit send3_ok = 0;
void UART3_Send_Byte(char aChar)
{
	S3BUF = aChar;
 	send3_ok = 1;	
	while(send3_ok)        //等待发送缓存为空
	{
		if(S3CON & 0x02)     //S3TI = 1
	  {
		  S3CON &= ~0x02;
			send3_ok=0;
		}
	}
}

/*-----------------串口4--------------*/

bit send4_ok = 0;
void UART4_Send_Byte(char aChar)
{
	S4BUF = aChar;
 	send4_ok = 1;
	while(send4_ok)        //等待发送缓存为空
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
//发送开始接收F4 05 00 00 00 00 CRC(校验) 4F
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

//发送停止接收F4 06 00 00 00 00 88 4F
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
	           电机控制
**************************************/	
//发送Length-1个数据+1个数据累加和
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

//电机读取编码器命令
//void read_code(unsigned char datas)
//{
//	unsigned char bytes[5];
//	memset(bytes,0,sizeof(bytes));
//	bytes[0]=0x3E;
//	bytes[1]=0x90;
//	bytes[2]=datas;     //电机id
//	bytes[3]=0x00;
//	UART2_Send(bytes,5); //发送帧头、功能字节、校验和
//}

//位置闭环控制1(目标位置（多圈角度累计值）)
//send 电机id;*Data 到达位置(8个字节)
void locate_1(unsigned char send,unsigned char *Data)
{
	unsigned char TX_DATA2[14],k=0,j=0,l=5,m=0;
	memset(TX_DATA2,0,sizeof(TX_DATA2));//清空发送数据
	TX_DATA2[0]=0X3E;                   //帧头
	TX_DATA2[1]=0XA3;                   //帧头
	TX_DATA2[2]=send;                   //电机id
	TX_DATA2[3]=0x08;                   //数据长度
	
	while(j<4)                          //1~4位校验和
	{
		TX_DATA2[4]+=TX_DATA2[j];
		j+=1;
	}
	
	for(k=0;k<8;k++)                    //存入数据到缓存TX_DATA数组
	{
		TX_DATA2[k+5]=Data[k];
	}
	
	while(l<13)                         //6~13位校验和
	{
		TX_DATA2[13]+=TX_DATA2[l];
		l+=1;
	}
	
	while(m<14)
	{
		UART2_Send_Byte(TX_DATA2[m++]);   //整帧发送
	}
}

//位置闭环控制4(转动方向、目标位置（单圈角度值）和电机转动的最大速度)
//id_code 电机id;direction 转动方向；*angle 到达位置(8个字节)；*speed 转动速度
void locate_4(unsigned char id_code,unsigned char direction,unsigned char *angle,unsigned char *speed)
{
	unsigned char TX_DATA2[14],k=0,j=0,i=0,l=5,m=0;
	memset(TX_DATA2,0,sizeof(TX_DATA2));//清空发送数据
	TX_DATA2[0]=0X3E;                   //帧头
	TX_DATA2[1]=0XA6;                   //帧头
	TX_DATA2[2]=id_code;                //电机id
	TX_DATA2[3]=0x08;                   //数据长度
	
	while(j<4)                          //1~4位校验和
	{
		TX_DATA2[4]+=TX_DATA2[j];
		j+=1;
	}
	
	TX_DATA2[5]=direction;              //存入方向数据到缓存TX_DATA数组
	
	for(k=0;k<3;k++)                    //存入角度数据到缓存TX_DATA数组
	{
		TX_DATA2[k+6]=angle[k];
	}
	
	for(i=0;i<4;i++)                    //存入速度数据到缓存TX_DATA数组
	{
		TX_DATA2[i+9]=speed[i];
	}
	
	while(l<13)                         //6~13位校验和
	{
		TX_DATA2[13]+=TX_DATA2[l];
		l+=1;
	}
	
	while(m<14)
	{
		UART2_Send_Byte(TX_DATA2[m++]);
	}
}

//十六进制转换（低字节在前）
unsigned char intTohex[3],intTohex4[4],intTohex8[8];
unsigned char change3(long number)     //3个字节
{
	intTohex[0]=(char)number;
	intTohex[1]=(number>>8)&0x00ff;
	intTohex[2]=(number>>16)&0x00ff;
	return intTohex;
}
unsigned char change4(long number)     //4个字节
{
	intTohex4[0]=(char)number;
	intTohex4[1]=(number>>8)&0x00ff;
	intTohex4[2]=(number>>16)&0x00ff;
	intTohex4[3]=(number>>24)&0x00ff;
	return intTohex4;
}
unsigned char change8(long number)     //8个字节
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
	          ttl摄像头
**************************************/	
void UART4_Send_String(char *aString, unsigned int StringLength)
{
	unsigned int i;
	for ( i = 0; i< StringLength; i++ )
	{
		UART4_Send_Byte( aString[i] );
	}
}

//拍照流程a\b\d
void take_photo(unsigned char a,unsigned char b)
{
	unsigned char TX_DATA[5];
	memset(TX_DATA,0,sizeof(TX_DATA));//清空发送数据
	TX_DATA[0]=0x56;                  //帧头
	TX_DATA[1]=0x00;                  //序列号
	TX_DATA[2]=a;                     //命令字
	TX_DATA[3]=0x01;                  //数据长度
	TX_DATA[4]=b;                     
	
	UART4_Send_String(TX_DATA,5);
}

//拍照流程c  56 00 32 0C 00 0C 00 00 00 00 xx xx xx xx 00 FF
void take_data(unsigned char ah,unsigned char bh,unsigned char ch,unsigned char dh)
{
	unsigned char TX_DATA[16];
	memset(TX_DATA,0,sizeof(TX_DATA));//清空发送数据
	TX_DATA[0]=0x56;                  //帧头
	TX_DATA[1]=0x00;                  //序列号
	TX_DATA[2]=0x32;                  
	TX_DATA[3]=0x0C;                  
	TX_DATA[4]=0x00;                  
	TX_DATA[5]=0x0C;                  //操作方式
	TX_DATA[6]=0x00;                  //4bytes起始地址
	TX_DATA[7]=0x00;
	TX_DATA[8]=0x00;
	TX_DATA[9]=0x00;
	TX_DATA[10]=ah;                   //4bytes图像长度
        TX_DATA[11]=bh;
	TX_DATA[12]=ch;
	TX_DATA[13]=dh;
	TX_DATA[14]=0x00;                 //2byte延时时间
	TX_DATA[15]=0xFF;                 //延时时间指摄像返回命令与数据间的时间间隔，单位为0.01mS
	
	UART4_Send_String(TX_DATA,16);
}

//摄像头工作函数
//过程中如果出现异常，将继续返回主函数执行
void UART4_work()
{
	long     int  last,num=0;          //计数器
	unsigned char getd;                //数据中转
	unsigned char ah,bh,ch,dh;
	unsigned char res1[5],res2[5],res3[5];
	
	//指令比对
	res1[0]=0x76;res1[1]=0x00;res1[2]=0x36;res1[3]=0x00;res1[4]=0x00;
	res2[0]=0x76;res2[1]=0x00;res2[2]=0x34;res2[3]=0x00;res2[4]=0x04;
	res3[0]=0x76;res3[1]=0x00;res3[2]=0x26;res3[3]=0x00;res3[4]=0x00;
		 
	memset(Receive4,0,9);	
	index4 = 0;
	IE2 = 0x00;	
	take_photo(0x36,0x00);             //停止当前帧刷新	  
	
	IE2 |= ES4;		
	while(index4!=5);
        if(string_cmp(Receive4,res1,5)!= 0)return;
			
	
	IE2 = 0x00;	    
	memset(Receive4,0,9);
	index4 = 0;
	take_photo(0x34,0x00);             //获取图片长度
			
	IE2 |= ES4;
	while(index4!=9);
	if(string_cmp(Receive4,res2,5)!= 0)return;

	IE2 = 0x00;
	
	ah = Receive4[5];                  //取出图像长度信息
	bh = Receive4[6]; 
	ch = Receive4[7];
	dh = Receive4[8];  
	last = (ah << 24)|(bh << 16)|(ch << 8)|dh;
	last += 10;
	
	take_data(ah,bh,ch,dh);            //获取图片
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
	take_photo(0x36,0x02);             //恢复帧刷新
				
	IE2 |= ES4;
	while(index4 != 5);
        if(string_cmp(Receive4,res1,5)!= 0)return;
			
        if(P_SW2 == 0x00)                  //摄像头串口切换
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
	
	for(r = 0;r<30;r++)Delay100ms();   //上电延时
	
	real[0]=0xF4;real[1]=0x07;real[2]=0x00;real[3]=0x00;

	set_baudrate(24, 115200); 
			
	EA = 1;
	IE2 = 0x00;
	
	RS485DIR=1;                        //打开485模块发送使能，关闭接收功能
	
	change8(0);
	locate_1(0x01,intTohex8);          //使用位置闭环控制1以最快速度到达默认初始位置
	Delay100ms();

	locate_1(0x04,intTohex8);
	Delay100ms();

	locate_1(0x03,intTohex8);
	Delay100ms();
	
	for(r=0;r<20;r++)Delay100ms();
	
	while (STAT == 0);                 //蓝牙连接检测
	
	P_SW1 = 0x80;
			
//	r=0;
//	do
//	{
//        ES = 0;
//	  UART_Send_Byte(0xA5);//发送读方位角指令
//	  UART_Send_Byte(0x45);
//	  UART_Send_Byte(0xEA);
//		
//		index = 0;SBUF = 0x00;RI = 0;
//		ES = 1;
//			
//		while(index!=11);
//		
////		if(receive_ok) //串口接收完毕
////		{
//			ES = 0;

//		  UART_Send_Byte(0xA5);//发送停止读方位角指令
//		  UART_Send_Byte(0x75);
//		  UART_Send_Byte(0x1A);
//			
//			for(sum=0,i=0;i<(Receive[3]+4);i++)//方位角angle_data[3]=2
//			{
//				sum+=Receive[i];
//			}
//			
//			if(sum==Receive[10])                //校验和判断
//			{
//				angle[0]=(Receive[4]<<8)|Receive[5];
//				angle[1]=(Receive[6]<<8)|Receive[7];
//				angle[2]=(Receive[8]<<8)|Receive[9];//实际值的100倍
//				
//			        UART3_Send_Byte(Receive[0]);
//				UART3_Send_Byte(Receive[1]);
//				UART3_Send_Byte(Receive[2]);
//				UART3_Send_Byte(Receive[3]);
//	
//			        UART3_Send_Byte(Receive[4]);
//				UART3_Send_Byte(Receive[5]);
//				UART3_Send_Byte(Receive[6]);
//				UART3_Send_Byte(Receive[7]);
//				UART3_Send_Byte(Receive[8]);
//				UART3_Send_Byte(Receive[9]);
//				r+=1;
//	}			
//			receive_ok=0;                               //处理数据完毕标志			
////		}
//	}while(r<3);



//do
//			{
		index = 0;  
		UART_Send_Byte(0xA5);//发送读方位角指令
	        UART_Send_Byte(0x45);
	        UART_Send_Byte(0xEA);
			
		SBUF = 0x00;RI = 0;
		ES = 1;
	        Delay5ms();
			
		while(index!=11);
                ES = 0;
			    
		UART_Send_Byte(0xA5);//发送停止读方位角指令
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
	          串口中断函数
**************************************/	
void Usart() interrupt 4
{

    if (TI)
    {
			TI = 0;                 //清除TI位
			send_ok=0;
    }
		
    if (RI)
    {      
			Receive[index]=SBUF;
			index += 1;
			RI = 0;                 //清除RI位
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
