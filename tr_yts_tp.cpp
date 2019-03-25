#include "TR_YTS_TP.h"
#include<QString>
#include"QDateTime"
#include<QFile>
#include<QTextStream>
#include <QCoreApplication>
#include<QDir>
//获取业务号
int GetServiceTypeID()
{
	return 25;
}

//获取业务名称
QString GetServiceTypeName()
{
	QString name = QString::fromLocal8Bit("一体式墒情监测站");
	return name;
}

//获取版本号
QString GetVersionNo()
{
	QString Version = QString::fromLocal8Bit("1.0");
	return Version;
}

//获取端口号
int GetPort()
{
	return 27015;
}
//解析数据
LRESULT Char2Json(QString &buff, QJsonObject &json)
{
	//数据个数
	int Count = 0;
	//当前数据指
	int Current_P = buff.length();

	//遍历查找数据
	for (int i = 0; i < buff.length(); i++)
	{
		//帧头 aa
		if (buff[i] == 0xaa)
		{
			Current_P = i;//指针指向帧头
			for (int j = i; j < buff.length(); j++)
			{
				if (buff[j] == 0xdd)
				{
					Current_P = j + 1;//指针移动到帧尾下一个字符

					QString strBuff = buff.mid(i, j - i + 1);
					QJsonObject SubJson;
					SubJson.insert("ServiceTypeID", QString::number(TR_YTS_TP));

					Frame frame = { 0,NULL,NULL };
					//获取帧长度
					frame.len = ((strBuff[1]).unicode() & 0xFF) - 3;
					//判断帧长度
					if (strBuff.length() - frame.len != 8)
					{
						j++;//数据中含有DD
						continue;
					}
					Count += 1;//数据个数
							   //获取帧命令
					frame.Command = strBuff[2].unicode();
					//获取源地址
					frame.SrcAddr = strBuff[3].unicode();
					//获取目的地址
					frame.DesAddr = strBuff[4].unicode();
					if ((strBuff[4].unicode() == 0x01) || (strBuff[4].unicode() == 0x0A))
						frame.SrcAddr = strBuff[3].unicode();
					else
						frame.SrcAddr = 256 * strBuff[4].unicode() + strBuff[3].unicode();
					//区站号
					SubJson.insert("StationID", "NULL");
					//设备号
					SubJson.insert("DeviceID", QString::number(frame.SrcAddr));
					frame.data = strBuff.mid(5, frame.len);
					//判断接收命令类型
					switch (frame.Command)
					{
						//读取采集时钟发送命令 0x2
					case 2:
						Transform2Time(frame.data, SubJson);
						break;
						//通道号状态
					case 30:
						Transform2Channel(frame.data, SubJson);
						break;
						//读取IP和端口号设置 0x1f
					case 31:
						Transform2IPPort(frame.data, SubJson);
						break;
						//GPRS平均体积含水量和频率数据 0x72
					case 114:
						Transform2GPRSPerVolume(frame.data, SubJson);
						break;
						//服务器APN地址 0x20
					case 32:
						Transform2APNAddr(frame.data, SubJson);
						break;
						//心跳包数据 0x3A
					case 58:
						Transform2Heartbeat(frame.data, SubJson);
						break;
						//通讯结束
					case 15:
						Transform2CloeseConnection(frame.data, SubJson);
						break;
					default:
						SubJson.insert("Command", frame.Command);
						break;
					}
					//数据备份
					QDateTime current_date_time = QDateTime::currentDateTime();
					QString current_date = current_date_time.toString("yyyy.MM.dd hh:mm:ss");
					QString current_day = current_date_time.toString("yyyy-MM-dd");
					QString fileName = QCoreApplication::applicationDirPath() + "\\TR_YTS_TP\\" + QString::number(frame.SrcAddr) + "\\" + current_day;
					QDir dir(fileName);
					if (!dir.exists())
						dir.mkpath(fileName);//创建多级目录
					fileName += "\\data.txt";
					QFile file(fileName);

					if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
					{
					}
					QTextStream in(&file);

					QString ConvStr;
					for (int i = 0; i < strBuff.length(); i++)
					{
						ConvStr += (QString::number(strBuff[i].unicode(), 16)) + " ";
					}
					in << current_date << "\r\n" << ConvStr << "\r\n";
					file.close();
					json.insert(QString::number(Count), SubJson);
					i = j;//当前循环
					break;
				}
			}
		}
	}
	json.insert("DataLength", Count);//JSON数据个数
	if (Current_P >= buff.length())//判断当前指针位置
	{
		buff.clear();
	}//清除内存
	else
	{
		buff.remove(0, Current_P);
	}//将剩余字节存入缓存
	return 1;
}

QString Convert2Time(QString strTime)
{
	QString tmp;
	tmp = strTime.mid(0, 4) + "-" + strTime.mid(4, 2) + "-" + strTime.mid(6, 2) + " " + strTime.mid(8, 2) + ":" + strTime.mid(10, 2) + ":" + strTime.mid(12, 2);
	return tmp;
}

//通道号
void Transform2Channel(QString data, QJsonObject &json)
{
	json.insert("DataType", 2);//数据类型 观测数据
	json.insert("ValueCount", 2);//返回值个数

								 //获取通道
	int channel = (int)data[0].unicode();
	//状态
	int status = (int)data[1].unicode();
	json.insert("RecvValue1", QString::number(channel));
	json.insert("RecvValue2", QString::number(status));
}

//采集时钟发送命令
void Transform2Time(QString data, QJsonObject &json)
{
	json.insert("DataType", 2);//数据类型 观测数据
	json.insert("ValueCount", 1);//返回值个数
								 //时间
	QString year = QString("20%1").arg((int)data[0].unicode(), 2, 10, QChar('0'));
	QString month = QString("%1").arg((int)data[1].unicode(), 2, 10, QChar('0'));
	QString day = QString("%1").arg((int)data[2].unicode(), 2, 10, QChar('0'));
	QString hour = QString("%1").arg((int)data[3].unicode(), 2, 10, QChar('0'));
	QString min = QString("%1").arg((int)data[4].unicode(), 2, 10, QChar('0'));
	QString time = Convert2Time(year + month + day + hour + min + "00");
	json.insert("RecvValue1", time);
}

//IP地址和端口号
void Transform2IPPort(QString data, QJsonObject &json)
{
	json.insert("DataType", 2);//数据类型 观测数据
	json.insert("ValueCount", 2);//返回值个数

								 //获取IP
	QString IP;
	IP.sprintf("%d.%d.%d.%d", (int)data[1].unicode(), (int)data[2].unicode(), (int)data[3].unicode(), (int)data[4].unicode());
	json.insert("RecvValue1", IP);

	//获取端口
	int Port = data[5].unicode();
	Port |= (data[6].unicode() << 8);
	json.insert("RecvValue2", QString::number(Port));
}

//服务器APN地址
void Transform2APNAddr(QString data, QJsonObject &json)
{
	json.insert("DataType", 2);//数据类型 观测数据
	json.insert("ValueCount", 1);//返回值个数
	json.insert("RecvValue2", data);	//获取APN
}

//GPRS发送小时平均体积含水量和频率数据
void Transform2GPRSPerVolume(QString data, QJsonObject &json)
{

	//获取层数
	int count = data.count();
	//判断数据完整性
	if (((count - 11) % 8) != 0)
		return;
	//数据类型 观测数据
	json.insert("DataType", 1);
	//bytes转float
	FLOAT_UNION  f;
	//时间
	QString year = QString("20%1").arg((int)data[0].unicode(), 2, 10, QChar('0'));
	QString month = QString("%1").arg((int)data[1].unicode(), 2, 10, QChar('0'));
	QString day = QString("%1").arg((int)data[2].unicode(), 2, 10, QChar('0'));
	QString hour = QString("%1").arg((int)data[3].unicode(), 2, 10, QChar('0'));
	QString min = QString("%1").arg((int)data[4].unicode(), 2, 10, QChar('0'));
	QString time = Convert2Time(year + month + day + hour + min + "00");
	json.insert("ObserveTime", time);

	//获取电压值浮点型
	f.float_byte.low_byte = data[count - 6].unicode();
	f.float_byte.mlow_byte = data[count - 5].unicode();
	f.float_byte.mhigh_byte = data[count - 4].unicode();
	f.float_byte.high_byte = data[count - 3].unicode();
	json.insert("MainClctrVltgVal", QJsonValue(f.value));

	//获取GPRS信号强度
	int PowerOfGPRS = data[count - 2].unicode();
	json.insert("SignalStrength", PowerOfGPRS);

	//获取GPRS误码率
	int GPRSBER = data[count - 1].unicode();
	json.insert("ErrorRate", GPRSBER);

	QString SoilVolume, SoilFrequency;

	int LayerCount = (count - 11) / 8;
	for (int i = 0; i < LayerCount; i++)
	{
		if (i == 0)
		{
			//体积含水量
			f.float_byte.low_byte = data[i * 4 + 5].unicode();//第i层乘以4加上前时间5个偏移量
			f.float_byte.mlow_byte = data[i * 4 + 1 + 5].unicode();
			f.float_byte.mhigh_byte = data[i * 4 + 2 + 5].unicode();
			f.float_byte.high_byte = data[i * 4 + 3 + 5].unicode();
			SoilVolume += QString::number(f.value);

			//频率
			f.float_byte.low_byte = data[i * 4 + 5 + LayerCount * 4].unicode();
			f.float_byte.mlow_byte = data[i * 4 + 1 + 5 + LayerCount * 4].unicode();
			f.float_byte.mhigh_byte = data[i * 4 + 2 + 5 + LayerCount * 4].unicode();
			f.float_byte.high_byte = data[i * 4 + 3 + 5 + LayerCount * 4].unicode();
			SoilFrequency += QString::number(f.value);
		}
		else
		{
			//体积含水量
			f.float_byte.low_byte = data[i * 4 + 5].unicode();
			f.float_byte.mlow_byte = data[i * 4 + 1 + 5].unicode();
			f.float_byte.mhigh_byte = data[i * 4 + 2 + 5].unicode();
			f.float_byte.high_byte = data[i * 4 + 3 + 5].unicode();
			SoilVolume += "," + QString::number(f.value);

			//频率
			f.float_byte.low_byte = data[i * 4 + 5 + LayerCount * 4].unicode();
			f.float_byte.mlow_byte = data[i * 4 + 1 + 5 + LayerCount * 4].unicode();
			f.float_byte.mhigh_byte = data[i * 4 + 2 + 5 + LayerCount * 4].unicode();
			f.float_byte.high_byte = data[i * 4 + 3 + 5 + LayerCount * 4].unicode();
			SoilFrequency += "," + QString::number(f.value);
		}
	}
	json.insert("SoilVolume", SoilVolume);
	json.insert("SoliFrequency", SoilFrequency);
}

//接收到心跳包
void Transform2Heartbeat(QString data, QJsonObject &json)
{
	json.insert("DataType", 3);//数据类型 3心跳数据

}

//通信结束
void Transform2CloeseConnection(QString data, QJsonObject &json)
{
	json.insert("DataType", 2);//数据类型 观测数据
	json.insert("ValueCount", 1);
	json.insert("RecvValue1", QString::fromLocal8Bit("通信结束"));
}

//返回值反馈
void SetValueToControlWidget(QStringList list)
{
}

//终端命令
void SetCommand(uint Socket, int CommandType, QString Params1, QString Params2, QString StationID) //<>
{
	//设备终端命令
	switch (CommandType)
	{
	case 1001://设置时钟
	{
		//获取当前时钟
		QDateTime nowtime = QDateTime::currentDateTime();
		QString datetime = nowtime.toString("yyyy.MM.dd hh:mm:ss");
		QString year, month, day, hour, min, sec;
		year = nowtime.toString("yy");
		month = nowtime.toString("MM");
		day = nowtime.toString("dd");
		hour = nowtime.toString("hh");
		min = nowtime.toString("mm");
		sec = nowtime.toString("ss");

		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x0a;//֡
		bytes[2] = 0x81;//֡
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;//
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = 0x14;//20
		chk += bytes[5];
		bytes[6] = year.toInt();
		chk += bytes[6];
		bytes[7] = month.toInt();
		chk += bytes[7];
		bytes[8] = day.toInt();
		chk += bytes[8];
		bytes[9] = hour.toInt();
		chk += bytes[9];
		bytes[10] = min.toInt();
		chk += bytes[10];
		bytes[11] = sec.toInt();
		chk += bytes[11];
		bytes[12] = chk & 0xff;//У��λ �Ͱ�λ
		bytes[13] = (chk >> 8) & 0xff;//�߰�λ
		bytes[14] = 0xdd;
		::send(Socket, (char *)bytes, 15, 0);
	}
	break;
	case 1002://读取时钟
	{
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x03;//帧命令
		bytes[2] = 0x82;//֡����
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;//Դ��ַ
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = chk & 0xff;//У��λ �Ͱ�λ
		bytes[6] = (chk >> 8) & 0xff;//�߰�λ
		bytes[7] = 0xdd;
		::send(Socket, (char *)bytes, 8, 0);
	}
	break;
	case 1003://补抄命令
	{
		QDateTime Time_B, Time_E;
		Time_B = QDateTime::fromString(Params1, "yyyy-MM-dd hh:mm:ss");
		Time_E = QDateTime::fromString(Params2, "yyyy-MM-dd hh:mm:ss");

		QString yearB, monthB, dayB, hourB, minB, yearE, monthE, dayE, hourE, minE;
		yearB = Time_B.toString("yy");;
		monthB = Time_B.toString("MM");
		dayB = Time_B.toString("dd");
		hourB = Time_B.toString("hh");
		minB = Time_B.toString("mm");

		yearE = Time_E.toString("yy");//结束时间
		monthE = Time_E.toString("MM");
		dayE = Time_E.toString("dd");
		hourE = Time_E.toString("hh");
		minE = Time_E.toString("mm");
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x0d;//֡
		bytes[2] = 0x94;//֡
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = yearB.toInt() - 2000;
		chk += bytes[5];
		bytes[6] = monthB.toInt();
		chk += bytes[6];
		bytes[7] = dayB.toInt();
		chk += bytes[7];
		bytes[8] = hourB.toInt();
		chk += bytes[8];
		bytes[9] = minB.toInt();
		chk += bytes[9];
		bytes[10] = yearE.toInt() - 2000;
		chk += bytes[10];
		bytes[11] = monthE.toInt();
		chk += bytes[11];
		bytes[12] = dayE.toInt();
		chk += bytes[12];
		bytes[13] = hourE.toInt();
		chk += bytes[13];
		bytes[14] = minE.toInt();
		chk += bytes[14];
		bytes[15] = chk & 0xff;
		bytes[16] = (chk >> 8) & 0xff;
		bytes[17] = 0xdd;
		::send(Socket, (char *)bytes, 18, 0);
	}
	break;
	case 1004://通道状态
	{
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x04;//֡帧长度
		bytes[2] = 0x9d;//֡帧命令
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;//源地址
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = Params1.toInt();
		chk += bytes[5];
		bytes[6] = chk & 0xff;//校验和 低八位
		bytes[7] = (chk >> 8) & 0xff;//高八位
		bytes[8] = 0xdd;
		::send(Socket, (char *)bytes, 9, 0);
	}
	break;
	case 1005://打开通道
	{
		//获取通道号
		int channel = Params1.toInt();
		//打开通道
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x05;
		bytes[2] = 0x9c;
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = channel;
		chk += bytes[5];
		bytes[6] = 0x1;
		chk += bytes[6];
		bytes[7] = chk & 0xff;
		bytes[8] = (chk >> 8) & 0xff;
		bytes[9] = 0xdd;
		::send(Socket, (char *)bytes, 10, 0);
	}
	break;
	case 1006://关闭通道
	{
		//获取通道号
		int channel = Params1.toInt();
		//关闭通道
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x05;
		bytes[2] = 0x9c;
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = channel;
		chk += bytes[5];
		bytes[6] = 0x0;
		chk += bytes[6];
		bytes[7] = chk & 0xff;
		bytes[8] = (chk >> 8) & 0xff;
		bytes[9] = 0xdd;
		::send(Socket, (char *)bytes, 10, 0);
	}
	break;
	case 1007://写入IP
	{
		//获取通道号
		int channel = Params1.toInt();
		//写入IP
		int ip1 = 0, ip2 = 0, ip3 = 0, ip4 = 0, port = 0;
		QList<QString> iplist = Params2.split('.');
		ip1 = iplist[0].toInt();
		ip2 = iplist[1].toInt();
		ip3 = iplist[2].toInt();
		ip4 = iplist[3].toInt();
		port = iplist[4].toInt();
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x10;//帧长度
		bytes[2] = 0x8e;//帧命令
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;//源地址
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = channel;//通道号
		chk += bytes[5];
		bytes[6] = ip1;//IP
		chk += bytes[6];
		bytes[7] = ip2;//IP
		chk += bytes[7];
		bytes[8] = ip3;//IP
		chk += bytes[8];
		bytes[9] = ip4;//IP
		chk += bytes[9];
		bytes[10] = port & 0xff;//port
		chk += bytes[10];
		bytes[11] = (port >> 8) & 0xff;//port
		chk += bytes[11];
		bytes[12] = chk & 0xff;//校验和 低八位
		bytes[13] = (chk >> 8) & 0xff;//高八位
		bytes[14] = 0xdd;
		::send(Socket, (char *)bytes, 15, 0);
	}
	break;
	case 1008://读取IP
	{
		//获取通道号
		int channel = Params1.toInt();
		//读取IP
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x04;
		bytes[2] = 0x8f;
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = channel;
		chk += bytes[5];
		bytes[6] = chk & 0xff;
		bytes[7] = (chk >> 8) & 0xff;
		bytes[8] = 0xdd;
		::send(Socket, (char *)bytes, 9, 0);
	}
	break;
	case 1009://读取APN
	{
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 0x03;
		bytes[2] = 0x96;
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];
		bytes[5] = chk & 0xff;//
		bytes[6] = (chk >> 8) & 0xff;//
		bytes[7] = 0xdd;
		::send(Socket, (char *)bytes, 8, 0);
	}
	break;
	case 1010://设置APN
	{
		int chk = 0;
		int SrcAdrr = StationID.toInt();
		QString strAPN = Params1;
		QByteArray data = strAPN.toLatin1();
		int nCount = data.count();
		BYTE bytes[1024] = { 0 };
		bytes[0] = 0xaa;
		bytes[1] = 3 + nCount;
		bytes[2] = 0x95;
		chk += bytes[2];
		bytes[3] = SrcAdrr & 0xff;//低八位
		chk += bytes[3];
		bytes[4] = (SrcAdrr >> 8) & 0xff;
		chk += bytes[4];

		for (int i = 0; i < nCount; i++)
		{
			bytes[5 + i] = data.at(i);
			chk += bytes[5 + i];
		}
		bytes[5 + nCount] = chk & 0xff;
		bytes[6 + nCount] = (chk >> 8) & 0xff;
		bytes[7 + nCount] = 0xdd;
		::send(Socket, (char *)bytes, 8 + nCount, 0);
	}
	break;

	}
}

//矫正时钟
void SetTime(QString StationID, uint Socket)
{
	//获取当前时钟
	QDateTime nowtime = QDateTime::currentDateTime();
	QString datetime = nowtime.toString("yyyy.MM.dd hh:mm:ss");
	QString year, month, day, hour, min, sec;
	year = nowtime.toString("yy");
	month = nowtime.toString("MM");
	day = nowtime.toString("dd");
	hour = nowtime.toString("hh");
	min = nowtime.toString("mm");
	sec = nowtime.toString("ss");
	//����ʱ��
	int chk = 0;
	int SrcAdrr = StationID.toInt();
	BYTE bytes[1024] = { 0 };
	bytes[0] = 0xaa;
	bytes[1] = 0x0a;//֡����
	bytes[2] = 0x81;//֡����
	chk += bytes[2];
	bytes[3] = SrcAdrr & 0xff;//Դ��ַ
	chk += bytes[3];
	bytes[4] = (SrcAdrr >> 8) & 0xff;
	chk += bytes[4];
	bytes[5] = 0x14;//20
	chk += bytes[5];
	bytes[6] = year.toInt();
	chk += bytes[6];
	bytes[7] = month.toInt();
	chk += bytes[7];
	bytes[8] = day.toInt();
	chk += bytes[8];
	bytes[9] = hour.toInt();
	chk += bytes[9];
	bytes[10] = min.toInt();
	chk += bytes[10];
	bytes[11] = sec.toInt();
	chk += bytes[11];
	bytes[12] = chk & 0xff;//У��λ �Ͱ�λ
	bytes[13] = (chk >> 8) & 0xff;//�߰�λ
	bytes[14] = 0xdd;
	::send(Socket, (char *)bytes, 15, 0);
}

//设置窗体
void  GetControlWidget(QString StationID, uint Socket, QWidget* parent)
{

}

//关闭窗体
void CloseControlWindow()
{

}

//获取设备信息
void GetFacilityInfo(uint Socket)
{

	int chk = 0;
	int SrcAdrr = 0;
	BYTE bytes[1024] = { 0 };
	bytes[0] = 0xaa;
	bytes[1] = 0x03;
	bytes[2] = 0x82;
	chk += bytes[2];
	bytes[3] = SrcAdrr & 0xff;
	chk += bytes[3];
	bytes[4] = (SrcAdrr >> 8) & 0xff;
	chk += bytes[4];
	bytes[5] = chk & 0xff;
	bytes[6] = (chk >> 8) & 0xff;
	bytes[7] = 0xdd;
	::send(Socket, (char *)bytes, 8, 0);
}
