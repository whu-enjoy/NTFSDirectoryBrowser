/*******************************************************************************
程序员       : enjoy5512
最后修改时间 : 2016年4月14日 15:15:36
程序说明     : 本程序,用于像目录浏览器一样根据用户输入显示相应文件信息
测试环境     : Windows XP SP2 + VC6.0
待解决问题   : 磁盘第二个扇区的信息获取不到,SetFilePointer()第二个参数0-1023的
               结果是一样的
*******************************************************************************/

#include <windows.h>  
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*******************************************************************************
                                 结构体定义
*******************************************************************************/
/*
	数据节点结构体
  */
typedef struct FileData
{
	int clusterSize;            //簇大小
	int baseSec;                //文件所在分区首扇区号
	int firstSec;               //这个数据段首扇区号
	int size;                   //这个数据段占用的扇区数
	struct FileData *nextFData; //下个数据节点
}FData;

/*
	文件节点结构体
  */
typedef struct FileNode
{
	int clusterSize;                   //簇大小
	int baseSec;                       //文件所在分区首扇区号
	int fatherSec;                     //父目录的首扇区号
	int firstSec;                      //本文件首扇区号
	int size;                          //本文件大小
	int attribute;                     //本文件属性
	int mftSec;                        //本文件所在分区MFT记录首扇区号
	char name[30];                     //本文件名
	struct FileData *fData;            //本文件的数据段指针
	struct FileNode *nextBrotherFNode; //同级目录下下个文件指针
	struct CatalogNode *fatherCNode;   //父目录指针
}FNode;

/*
	目录节点结构体
  */
typedef struct CatalogNode
{
	int mftSec;                           //本目录所在分区MFT记录首扇区号
	int clusterSize;                      //簇大小
	int baseSec;                          //文件所在分区首扇区号
	int fatherSec;                        //父目录首扇区号
	int firstSec;                         //本目录文件记录首扇区号
	int attribute;                        //本目录属性
	char name[30];                        //本目录名
	struct FileNode *firstChildFNode;     //子目录第一个文件指针
	struct CatalogNode *firstChildCNode;  //子目录第一个目录指针
	struct CatalogNode *nextBrotherCNode; //同级目录下一个目录指针
	struct CatalogNode *fatherCNode;      //父目录指针
}CNode;

/*
	分区节点结构体
  */
typedef struct PartitionNode
{
	int patitionNum;                   //分区编号
	int bootFlag;                      //引导标志
	int patitionType;                  //分区类型
	int firstSec;                      //分区首地址
	float patitionSize;                //分区大小
	int ePatitionFlag;                 //扩展分区标志
	struct CatalogNode *rootCNode;     //根目录指针
	struct PartitionNode *nextPNode;   //下一分区指针
}PNode;

/*
	硬盘节点结构体
  */
typedef struct DiskNode
{
	struct PartitionNode *firstPNode;   //本硬盘第一个分区指针
}DNode;

/*******************************************************************************
                             结构体初始化函数定义
*******************************************************************************/
int InitFData(FData **fData);
int InitFNode(FNode **fNode);
int InitCNode(CNode **cNode);
int InitPNode(PNode **pNode);
int InitDNode(DNode **dNode);
/*******************************************************************************
                              结构体释放函数定义
*******************************************************************************/
int DestroyFData(FData *fData);
int DestroyFNode(FNode *fNode);
int DestroyCNode(CNode *cNode);
int DestroyPNode(PNode *pNode);
int DestroyDNode(DNode *dNode);
/*******************************************************************************
                                  其它函数定义
*******************************************************************************/
int ShowHelp(void);
int GetHandle(HANDLE *hDisk, int nDisk);
int GetBuff(HANDLE hDisk,unsigned char buff[] , int sector,DWORD* dCount,DWORD size);
int ShowBuff(char buff[],int addrH, int addrL);
int GetPatitionList(HANDLE hDisk,DNode **dNode);
int ShowPatition(PNode *pNode);
int GetRootNode(HANDLE hDisk,PNode *pNode,CNode **returnCNode, int patitionNum);
int GetFData(HANDLE hDisk, FNode **fNode);
int GetAIndex(HANDLE hDisk, unsigned char buff[], CNode *fatherCNode, CNode **lastChildCNode, FNode **lastChildFNode, int i);
int GetIndexList(HANDLE hDisk,CNode *cNode,int addr,int size);
int GetDir(HANDLE hDisk,CNode *cNode);
int ShowDir(CNode *cNode);
int GetCatalogNode(HANDLE hDisk, CNode **cNode, char parameter[]);
int ShowPatitionInfo(PNode *pNode, int patitionNum);
int ShowCatalogInfo(CNode *cNode);
int ShowFileInfo(FNode *fNode);

int main()
{
	unsigned char buff[513 ] = {0}; //保存512字节信息,因为字符串数组后面会加\0,所以申请513字节
	HANDLE hDisk;                   //磁盘句柄
	DWORD dCount=0;                 //返回读取的字节数
	int sector = 0;                 //扇区号
	int nDisk = 0;                  //磁盘号
	char command[5] = {0};          //六个命令,ls,cd,exit,sec,disk,help
	char parameter[30] = {0};       //cd命令参数

	int dirFlag = 0;                //判断当前位置是否是目录
	int catalogFlag = 0;            //目录查找结果,找到为0,否则为-1
	int patitionNum = 0;            //分区序列号

	DNode *dNode = NULL;            //硬盘指针
	CNode *currentDir = NULL;       //当前目录指针
	CNode *searchCNode = NULL;      //用作目录搜索的指针
	FNode *searchFNode = NULL;      //用作文件搜索的指针

	InitDNode(&dNode);              //初始化硬盘指针              
	GetHandle(&hDisk,0);            //获取磁盘句柄
	GetPatitionList(hDisk,&dNode);  //得到分区列表
	InitCNode(&currentDir);         //初始化当前目录指针
	currentDir = dNode->firstPNode->rootCNode; //将当前目录设为第一个分区根目录

	ShowHelp();

	for(;;)
	{
		scanf("%s",command);
		if(0 == strcmp(command,"help"))
		{
			ShowHelp();
		}
		else  if(0 == strcmp(command,"disk"))
		{
			scanf("%d",&nDisk); 
			GetHandle(&hDisk,nDisk);                   //获取磁盘句柄
			GetPatitionList(hDisk,&dNode);             //获取分区列表
			ShowPatition(dNode->firstPNode);           //显示分区列表
			currentDir = dNode->firstPNode->rootCNode; //重新设置当前目录
			dirFlag = 0;
		}
		else  if(0 == strcmp(command,"sec"))
		{
			scanf("%d",&sector);
			GetBuff(hDisk,buff,sector,&dCount,512);                  //获取扇区信息
			ShowBuff(buff,sector/8388608,(sector*512)%4294967296);   //显示扇区信息
		}
		else if(0 == strcmp(command,"exit"))
		{
			DestroyDNode(dNode);                    //释放所有节点

			if(hDisk == 0xcccccccc)                 //如果读取硬盘句柄失败则异常退出
			{
				exit(0);
			}
			else
			{
				CloseHandle(hDisk);                 //释放硬盘句柄
			}

			return 0;
		}
		else if(0 == strcmp(command,"ls"))
		{
			if(0 == dirFlag)
			{
				ShowPatition(dNode->firstPNode);    //显示分区信息
			}
			else
			{
				ShowDir(currentDir);                //显示当前目录下文件列表
			}
		}
		else if(0 == strcmp(command,"info"))
		{
			if(0 == dirFlag)
			{
				patitionNum = 0;
				scanf("%d",&patitionNum);
				fflush(stdin);
				ShowPatitionInfo(dNode->firstPNode,patitionNum);
			}
			else
			{
				catalogFlag = 0;
				scanf("%s",parameter);
				if (NULL != currentDir->firstChildCNode)
				{
					searchCNode = currentDir->firstChildCNode;
					for(;;)
					{
						if(0 == (strcmp(searchCNode->name,parameter)))
						{
							ShowCatalogInfo(searchCNode);
							break;
						}
						else
						{
							if (NULL == searchCNode->nextBrotherCNode)
							{								
								catalogFlag = -1;
								break;
							}
							else
							{
								searchCNode = searchCNode->nextBrotherCNode;
							}
						}
					}
				}

				if(0 == catalogFlag)
				{
					continue;
				}

				if (NULL != currentDir->firstChildFNode)
				{
					searchFNode = currentDir->firstChildFNode;
					for(;;)
					{
						if(0 == (strcmp(searchFNode->name,parameter)))
						{
							ShowFileInfo(&searchFNode);
							break;
						}
						else
						{
							if (NULL == searchFNode->nextBrotherFNode)
							{
								printf("no match!!please checked and try again!!\n\n >>");
								break;
							}
							else
							{
								searchFNode = searchFNode->nextBrotherFNode;
							}
						}
					}
				}
			}
		}
		else if(0 == strcmp(command,"cd"))
		{
			if (0 == dirFlag)            //如果没有选择分区,则进入相应分区
			{
				patitionNum = 0;
				scanf("%d",&patitionNum);
				fflush(stdin);
				catalogFlag = GetRootNode(hDisk,dNode->firstPNode,&currentDir,patitionNum);
				if(0 != catalogFlag)
				{
					continue;
				}
				dirFlag = 1;             //设置当前位置为目录,接下来cd执行目录操作
			}
			else
			{
				scanf("%s",parameter);
				if (0 != strcmp(parameter,".."))   //回退到上级目录
				{
					catalogFlag = GetCatalogNode(hDisk,&currentDir,parameter);
					if(0 != catalogFlag)
					{
						printf("error catalog!!please try again!!\n\n >>");
						continue;
					}
				}
				else
				{
					if(NULL == currentDir->fatherCNode) //如果没有父目录,则说明
					{                              //分区列表处,目录标志设置为0
						dirFlag = 0;
						printf("\n >>");
						continue;
					}
					else                    //否则将当前目录设置为本目录的父目录
					{
						currentDir = currentDir->fatherCNode;
					}
				}
			}
			GetDir(hDisk,currentDir);       //获取当前目录信息
			printf("\n >>");
		}
		else
		{
			fflush(stdin);
			printf("error input!!please check and try again!!\n >>");
		}
	}
}

/*******************************************************************************
                             结构体初始化函数定义
*******************************************************************************/

/*
函数说明:
	初始化数据节点指针
输入参数:
	数据节点二级指针
输出参数:
	0
  */
int InitFData(FData **fData)
{
	*fData = (FData*)malloc(sizeof(FData));

	if (NULL == fData)
	{
		MessageBox(NULL,"malloc fData failed!","Error",MB_OK);
		exit(1);
	}

	(*fData)->clusterSize = 0;
	(*fData)->baseSec = 0;
	(*fData)->firstSec = 0;
	(*fData)->size = 0;
	(*fData)->nextFData = NULL;

	return 0;
}

/*
函数说明:
	初始化文件节点指针
输入参数:
	文件节点二级指针
输出参数:
	0
  */
int InitFNode(FNode **fNode)
{
	*fNode = (FNode*)malloc(sizeof(FNode));

	if (NULL == fNode)
	{
		MessageBox(NULL,"malloc fNode failed!","Error",MB_OK);
		exit(2);
	}

	(*fNode)->mftSec = 0;
	(*fNode)->clusterSize = 0;
	(*fNode)->baseSec = 0;
	(*fNode)->fatherSec = 0;
	(*fNode)->firstSec = 0;
	(*fNode)->size = 0;
	(*fNode)->attribute = 0;
	memset((*fNode)->name,0,30);
	(*fNode)->fData = NULL;
	(*fNode)->nextBrotherFNode = NULL;
	(*fNode)->fatherCNode = NULL;

	return 0;
}

/*
函数说明:
	初始化目录节点指针
输入参数:
	目录节点二级指针
输出参数:
	0
  */
int InitCNode(CNode **cNode)
{
	*cNode = (CNode*)malloc(sizeof(CNode));

	if (NULL == cNode)
	{
		MessageBox(NULL,"malloc cNode failed!","Error",MB_OK);
		exit(3);
	}

	(*cNode)->mftSec = 0;
	(*cNode)->clusterSize = 0;
	(*cNode)->baseSec = 0;
	(*cNode)->fatherSec = 0;
	(*cNode)->firstSec = 0;
	(*cNode)->attribute = 0;
	memset((*cNode)->name,0,30);
	(*cNode)->firstChildCNode = NULL;
	(*cNode)->firstChildFNode = NULL;
	(*cNode)->nextBrotherCNode = NULL;
	(*cNode)->fatherCNode = NULL;

	return 0;
}

/*
函数说明:
	初始化分区节点指针
输入参数:
	分区节点二级指针
输出参数:
	0
  */
int InitPNode(PNode **pNode)
{
	*pNode = (PNode*)malloc(sizeof(PNode));

	if (NULL == pNode)
	{
		MessageBox(NULL,"malloc pNode failed!","Error",MB_OK);
		exit(4);
	}

	(*pNode)->patitionNum = 0;
	(*pNode)->bootFlag = 0;
	(*pNode)->patitionType = 0;
	(*pNode)->firstSec = 0;
	(*pNode)->patitionSize = 0;
	(*pNode)->ePatitionFlag = 0;
	(*pNode)->rootCNode = NULL;
	(*pNode)->nextPNode = NULL;

	return 0;
}

/*
函数说明:
	初始化硬盘节点指针
输入参数:
	硬盘节点二级指针
输出参数:
	0
  */
int InitDNode(DNode **dNode)
{
	*dNode = (DNode*)malloc(sizeof(DNode));

	if (NULL == dNode)
	{
		MessageBox(NULL,"malloc dNode failed!","Error",MB_OK);
		exit(5);
	}

	(*dNode)->firstPNode = NULL;

	return 0;
}

/*******************************************************************************
                              结构体释放函数定义
*******************************************************************************/

/*
函数说明:
	释放数据节点指针
输入参数:
	数据节点指针
输出参数:
	0
  */
int DestroyFData(FData *fData)
{
	FData *preFData = NULL;
	FData *nextFData = NULL;

	if(NULL == fData)
	{
		return 0;
	}

	preFData = nextFData = fData;
	for (;;)
	{
		if (NULL == preFData->nextFData)
		{
			free(preFData);
			break;
		}
		else
		{
			nextFData = preFData->nextFData;
			free(preFData);
			preFData = nextFData;
		}
	}

	fData = NULL;
	return 0;
}

/*
函数说明:
	释放文件节点指针
输入参数:
	文件节点指针
输出参数:
	0
  */
int DestroyFNode(FNode *fNode)
{
	FNode *preFNode = NULL;
	FNode *nextFNode = NULL;

	if(NULL == fNode)
	{
		return 0;
	}

	preFNode = nextFNode = fNode;
	for (;;)
	{
		if (NULL == preFNode->nextBrotherFNode)
		{
			//先释放数据节点指针再释放文件节点指针
			DestroyFData(preFNode->fData);
			free(preFNode);
			break;
		}
		else
		{
			nextFNode = preFNode->nextBrotherFNode;
			//先释放数据节点指针再释放文件节点指针
			DestroyFData(preFNode->fData);
			free(preFNode);
			preFNode = nextFNode;
		}
	}

	fNode = NULL;
	return 0;
}

/*
函数说明:
	释放目录节点指针
输入参数:
	目录节点指针
输出参数:
	0
  */
int DestroyCNode(CNode *cNode)
{
	CNode *preCNode = NULL;
	CNode *nextCNode = NULL;

	if(NULL == cNode)
	{
		return 0;
	}

	preCNode = nextCNode = cNode;
	for (;;)
	{
		if (NULL == preCNode->nextBrotherCNode)
		{
			//先释放子目录(文件)节点指针再释放本目录节点指针
			DestroyCNode(preCNode->firstChildCNode);
			DestroyFNode(preCNode->firstChildFNode);
			free(preCNode);
			break;
		}
		else
		{
			nextCNode = preCNode->nextBrotherCNode;
			//先释放子目录(文件)节点指针再释放本目录节点指针
			DestroyCNode(preCNode->firstChildCNode);
			DestroyFNode(preCNode->firstChildFNode);
			free(preCNode);
			preCNode = nextCNode;
		}
	}

	cNode = NULL;
	return 0;
}

/*
函数说明:
	释放分区节点指针
输入参数:
	分区节点指针
输出参数:
	0
  */
int DestroyPNode(PNode *pNode)
{
	PNode *prePNode = NULL;
	PNode *nextPNode = NULL;
	
	if(NULL == pNode)
	{
		return 0;
	}

	prePNode = nextPNode = pNode;
	for(;;)
	{
		if (NULL == prePNode->nextPNode)
		{
			//先释放目录节点指针再释放分区节点指针
			DestroyCNode(prePNode->rootCNode);
			free(prePNode);
			break;
		}
		else
		{
			nextPNode = prePNode->nextPNode;
			//先释放目录节点指针再释放分区节点指针
			DestroyCNode(prePNode->rootCNode);
			free(prePNode);
			prePNode = nextPNode;
		}
	}

	pNode = NULL;
	return 0;
}

/*
函数说明:
	释放硬盘节点指针
输入参数:
	硬盘节点指针
输出参数:
	0
  */
int DestroyDNode(DNode *dNode)
{
	if(NULL == dNode)
	{
		return 0;
	}

	//如果有分区节点,则先释放分区节点指针再释放硬盘节点指针
	if (NULL != dNode->firstPNode)
	{
		DestroyPNode(dNode->firstPNode);
	}
	free(dNode);

	dNode = NULL;
	return 0;
}

/*******************************************************************************
                              其他函数定义
*******************************************************************************/

/*
函数说明:
	显示帮助信息
输入参数:
	0
输出参数:
	0
  */
int ShowHelp(void)
{
	printf("********************************************************************************");
	printf("********************************************************************************");
	printf("*****************************   Welcome!!!   ***********************************");
	printf("************Using help for help!!                                   ************");
	printf("************Using disk x to change che disk                         ************");
	printf("************Using sec x to show che sector data                     ************");
	printf("************Using exit to exit the program                          ************");
	printf("************Using ls to display the files in the current directory  ************");
	printf("************Using cd dir_name to entry the directory                ************");
	printf("************Using info + patitionNum/catalogName/fileName to show it's info*****");
	printf("This program just support English and Chinese will be garbled!you can use cd .. ");
	printf("to rollback to the upper directory and you can use cd + the patition number to  ");
	printf("entry the patition you choose!!                                                 ");
	printf("\n >>");
}

/*
函数说明:
	这个函数用于按照给定的磁盘号,返回相应的句柄
输入参数:
	HANDLE *hDisk : 磁盘句柄
	int nDisk    : 磁盘号号
输出参数:
	0
  */
int GetHandle(HANDLE* hDisk,int nDisk)
{
	char szDriverBuffer[128];     //保存相应磁盘路径信息

	memset(szDriverBuffer,0,128); //格式化设备文件名称
	sprintf(szDriverBuffer,"\\\\.\\PhysicalDrive%d",nDisk);//获取设备名称

	*hDisk = CreateFileA(
	szDriverBuffer,               // 设备名称,这里指第一块硬盘，多个硬盘的自己修改就好了
	GENERIC_READ,                 // 指定读访问方式
	FILE_SHARE_READ | FILE_SHARE_WRITE,  // 共享模式为读 | 写,只读的话我总是出错..
	NULL,                         // NULL表示该句柄不能被子程序继承
	OPEN_EXISTING,                // 打开已经存在的文件，文件不存在则函数调用失败
	NULL,                         // 指定文件属性
	NULL); 

	if (*hDisk==INVALID_HANDLE_VALUE)    //如果获取句柄失败,则退出
	{
		MessageBox(NULL,"Error in opening disk!","Error",MB_OK);
		exit(6);
	}

	return 0;
}

/*
函数说明:
	这个函数用于按照给定的扇区号返回相应扇区512字节信息
输入参数:
	HANDLE hDisk : 磁盘句柄
	char buff[]  : 字符串数组
	int n        : 扇区号
	DWORD* DCount: 读取的字节数
输出参数:
	0
  */
int GetBuff(HANDLE hDisk,unsigned char buff[] , int sector,DWORD* dCount,DWORD size)
{
	LARGE_INTEGER li={0}; //记得初始化
	int h_sector = 0;     //32位系统,用于当超出32位寻址范围时保存高地址
	int l_sector = 0;     //32位系统,用于当超出32位寻址范围时保存低地址

	l_sector = sector%8388608; //取低地址部分
	h_sector = sector/8388608; //取高地址部分

	li.LowPart = l_sector * 512; //我32位系统int取四位,FFFFFFFF/200H = 8388607
	li.HighPart = h_sector;      //高位直接传过去


	//如果超出寻址范围则退出,否则将指针移动到相应扇区
	if(-1 == SetFilePointer(hDisk,li.LowPart,&li.HighPart,FILE_BEGIN))
	{
		MessageBox(NULL,"Error sector!The program will exit!!","Error",MB_OK);
		exit(7);
	}

	//读取512字节内容到buff
	ReadFile(hDisk,  //要读的文件句柄 
	buff,            //存放读出内容
	size,            //要读的字节数
	dCount,          //实际读取字节数
	NULL);

	return 0;
}

/*
函数说明:
	这个函数用于按照指定的格式输出一个扇区的512字节的信息,字符ascll码低于32或者
	大于127的都用'.'来代替,其余的输出指定的字符
输入参数:
	char buff[] : 字符串数组
	int addrH   : 数据所在行号的高地址部分
	int addrL   : 数据所在行号的低地址部分
输出参数:
	0
  */
int ShowBuff(char buff[],int addrH, int addrL)
{
	int i = 0;
	int j = 0;

	//system("cls");                            //清屏
	//system("mode con cols=80 lines=37");      //设定命令行窗口大小

	printf("********************************************************************************");
	printf("********************************************************************************");
	printf("\t     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  |\n");
	for(i = 0;i < 32;i++)                     //每行输出16个字节,后面跟着相应的字符
	{
		printf("0X%1X%08X:",addrH,i*16+addrL);
		for(j = 0;j < 16;j++)
		{
			//每四位单独输出,解决小于F时只显示一个字符,对齐的问题
			printf(" %X%X",(buff[j+i*16]&240)>>4,buff[j+i*16]&15);   	                                         
		}
		printf(" | ");
		for(j = 0;j < 16;j++)
		{
			if(buff[j+i*16] > 31 && buff[j+i*16] < 127)
			{
				//输出相应的字符,若对应ascll码低于32或者大于127的都用'.'来代替
				printf("%c",buff[j+i*16]);     
			}
			else
			{
				printf(".");
			}
		}
		printf("\n");
	}
	printf(" >>");

	return 0;
}

/*
函数说明:
	这个函数用于获取分区列表
输入参数:
	cHANDLE hDisk : 硬盘句柄
	DNode **dNode : 硬盘结构体二级指针
输出参数:
	0
  */
int GetPatitionList(HANDLE hDisk,DNode **dNode)
{
	int i = 0;                           //循环参数
	int count = 1;                       //分区计数

	unsigned char buff[513] = {0};       //重新申请保存扇区数据的空间

	int partitionBeginSec = 0;           //分区首扇区号
	int ePartitionBeginSec = 0;          //扩展分区首扇区号
	int dCount = 0;                      //从分区读取的字节数

	float partitionSize = 0;             //分区大小

	DNode *opDNode = NULL;               //在函数体内参与运算的硬盘结构体
	PNode *bkPNode = NULL;               //在函数体内作为备份的分区结构体
	PNode *newPNode = NULL;              //新创建的分区结构体

	opDNode = *dNode;

	if(NULL != opDNode)
	{
		DestroyPNode(opDNode->firstPNode);//释放后重新建立链表
	}

	InitPNode(&newPNode);                 
	opDNode->firstPNode = newPNode;

	GetBuff(hDisk,buff,0,&dCount,512);    //获取第一扇区信息
	for (i = 446; i < 510 ; i = i + 16)   //循环遍历分区表项,输出分区信息
	{
		bkPNode = newPNode;
		if(0 == buff[i+4])                //分区标志为空,说明分区表项结束
		{
			break;
		}

		newPNode->patitionNum = count;    //分区编号
		count++;

		newPNode->bootFlag = buff[i];     //引导标志

		newPNode->patitionType = buff[i+4]; //分区类型
		if (15 == buff[i+4])              //如果分区类型为F,则说明是扩展分区
		{                                 //之后进入扩展分区表
			newPNode->ePatitionFlag = 1;
		}
		else
		{
			newPNode->ePatitionFlag = 0;
		}

		partitionBeginSec = 0;
		partitionBeginSec += buff[i+8];
		partitionBeginSec += buff[i+9]*256;
		partitionBeginSec += buff[i+10]*256*256;
		partitionBeginSec+= buff[i+11]*256*256*256;
		newPNode->firstSec = partitionBeginSec;  //分区首扇区号

		partitionSize = 0;
		partitionSize += buff[i+12];
		partitionSize += buff[i+13]*256;
		partitionSize += buff[i+14]*256*256;
		partitionSize += buff[i+15]*256*256*256;
		partitionSize /=2;
		partitionSize /=1024;
		partitionSize /=1024;
		newPNode->patitionSize = partitionSize;   //分区大小,单位G

		InitPNode(&newPNode);                     //分区信息获取完后,重新申请
		bkPNode->nextPNode = newPNode;            //内存,继续获取下一分区信息

		if(1 == bkPNode->ePatitionFlag)           //如果扩展分区标志为1,则遍历扩
		{                                         //展分区表
			bkPNode->ePatitionFlag = 0;           //这个分区是总览,不计入扩展分区内
			ePartitionBeginSec = partitionBeginSec;   //初始化扩展分区首扇区号    
			for(;;)
			{					
				int j = 446;
				
				bkPNode = newPNode;
				GetBuff(hDisk,buff,partitionBeginSec,&dCount,512);//获取本逻辑分区所在扇区数据
				
				
				newPNode->patitionNum = count;       //分区编号
				count++;

				newPNode->bootFlag = buff[j];        //引导标志

				newPNode->patitionType = buff[j+4];  //分区类型

				newPNode->ePatitionFlag = 1;         //标注为扩展分区

				partitionBeginSec += buff[j+8];      //扩展分区的首扇区号是
				partitionBeginSec += buff[j+9]*256;  //相对偏移
				partitionBeginSec += buff[j+10]*256*256;
				partitionBeginSec+= buff[j+11]*256*256*256;
				newPNode->firstSec = partitionBeginSec; //分区首扇区号

				partitionSize = 0;
				partitionSize += buff[j+12];
				partitionSize += buff[j+13]*256;
				partitionSize += buff[j+14]*256*256;
				partitionSize += buff[j+15]*256*256*256;
				partitionSize /=2;
				partitionSize /=1024;
				partitionSize /=1024;
				newPNode->patitionSize = partitionSize; //分区大小

				InitPNode(&newPNode);             //重新分配新内存,为下一分
				bkPNode->nextPNode = newPNode;    //区做准备

				j = j + 16;
				if(5 == buff[j + 4])              //如果分区标志为5,说明后面还
				{                                 //有扩展分区,然后获得下一逻辑
					partitionBeginSec = 0;        //分区的分区表所在扇区号
					partitionBeginSec += buff[j+8];
					partitionBeginSec += buff[j+9]*256;
					partitionBeginSec += buff[j+10]*256*256;
					partitionBeginSec += buff[j+11]*256*256*256;
					partitionBeginSec = partitionBeginSec + ePartitionBeginSec;
				}
				else
				{
					GetBuff(hDisk,buff,0,&dCount,512);  //扩展分区结束,恢复buff
					break;                              //继续遍历主分区
				}
			}
		}
	}

	bkPNode->nextPNode = NULL;  //因为是先申请内存再遍历分区,所以会多申请一个内
	free(newPNode);             //内存空间,所以要在函数结束的时候释放掉
	return 0;
}

/*
函数说明:
	用于显示磁盘分区情况
输入参数:
	PNode pNode : 第一个分区结构体指针
输出参数:
	0
  */
int ShowPatition(PNode *pNode)
{				
	printf("\t\t引导标志\t分区类型\t起始扇区\t分区大小\n");
	for (;;)  //循环遍历分区表项,输出分区信息
	{
		if(NULL == pNode)
		{
			printf("\n >>");
			return 0;
		}

		if(0 == pNode->ePatitionFlag)
		{
			printf("第%d分区 : \t",pNode->patitionNum);            //显示第几个分区
		}
		else
		{
			printf("--第%d分区 : \t",pNode->patitionNum);          //显示第几个扩展分区
		}

		if (128 == pNode->bootFlag)
		{
			printf("活动分区\t");
		}
		else
		{
			printf("非活动分区\t");
		}

		switch(pNode->patitionType)
		{
		case 0:
			printf("未使用\t\t");
			break;
		case 6:
			printf("FAT16\t\t");
			break;
		case 11:
			printf("FAT32\t\t");
			break;
		case 5:
			printf("扩展分区\t");
			break;
		case 7:
			printf("NTFS分区\t");
			break;
		case 15:
			printf("win扩展分区\t");
			break;
		default:
			printf("其他\t\t");
			break;
		}

		printf("%-11d\t",pNode->firstSec);

		printf("%.2fG\n",pNode->patitionSize);

		pNode = pNode->nextPNode;
	}
}

/*
函数说明:
	用于获取根目录所在的记录
输入参数:
	HANDLE hDisk : 硬盘句柄
	PNode *pNode : 分区结构体
	CNode **returnCNode : 需要返回的目录结构体
	int patitionNum     : 分区编号
输出参数:
	0  代表正常退出
	-1 代表异常退出
  */
int GetRootNode(HANDLE hDisk,PNode *pNode,CNode **returnCNode, int patitionNum)
{
	unsigned char buff[513] = {0};  //用于保存一个扇区的数据
	DWORD dCount = 0;               //返回读取的字节数
	int sector = 0;                 //扇区号
	PNode *prePNode = NULL;         //前一个分区结构体
	PNode *backPNode = NULL;        //备份分区结构体
	CNode *cNode = NULL;            //目录结构体

	prePNode = backPNode = pNode;

	for(;;)                         //循环遍历分区列表
	{
		if(NULL == prePNode->nextPNode)  //最后一个分区
		{
			if(prePNode->patitionNum == patitionNum)
			{
				sector = prePNode->firstSec;
				break;
			}
			else
			{
				printf("error patition Num!!please check and try again!!\n\n >>");
				return -1;
			}			
		}
		else                             //中间分区
		{
			if(prePNode->patitionNum == patitionNum)
			{
				sector = prePNode->firstSec;
				break;
			}
			else
			{
				prePNode = backPNode->nextPNode;
				backPNode = prePNode;
			}
		}
	}

	if(7 != prePNode->patitionType)     //暂时只支持NTFS格式分区
	{
		printf("just suppot NTFS patition!!please try again!!\n\n >>");
		return -1;
	}

	
	InitCNode(&cNode);                  //找到分区后,创建一个根目录节点
	DestroyCNode(prePNode->rootCNode);  //释放掉原来的根目录节点
	prePNode->rootCNode = cNode;        //然后指向新创建的节点
	cNode->fatherSec = sector;
	cNode->baseSec = sector;

	GetBuff(hDisk,buff,sector,&dCount,512);  //获取扇区信息
	cNode->clusterSize = buff[13];
	sector = sector + cNode->clusterSize * (buff[48] + buff[49] * 256 + buff[50] * 256 * 256 + buff[51] * 256 * 256 * 256);
	cNode->firstSec = sector + 10;
	cNode->mftSec = sector;

	*returnCNode = cNode;

	return 0;
}

/*
函数说明:
	用于获取数据节点指针
输入参数:
	HANDLE hDisk  : 硬盘句柄
	FNode **fNode : 文件结构体
输出参数:
	0  代表正常退出
  */
int GetFData(HANDLE hDisk, FNode **fNode)
{
	int i = 0;                      //用于记录当前指针的位置
	int j = 0;                      //用于循环计数
	int size = 0;                   //这个数据区所占扇区大小
	int addr = 0;                   //这个数据区所在首扇区号
	short int temp = 0;          //暂时保存数据
	DWORD dCount = 0;               //实际读取的扇区字节数
	unsigned char buff[1025] = {0}; //保存读取的扇区数据

	FData *newFData = NULL;         //用于申请新的数据节点
	FData *bkFData = NULL;          //保存数据节点

	InitFData(&newFData);           //申请一个数据节点
	DestroyFData((*fNode)->fData);
	(*fNode)->fData = newFData;     //将文件数据节点指向新创建的数据节点
	newFData->baseSec = (*fNode)->baseSec;         //更新数据所在分区首扇区号
	newFData->clusterSize = (*fNode)->clusterSize; //更新数据所在分区簇大小


	
	GetBuff(hDisk,buff,(*fNode)->firstSec,&dCount,1024);  //获取扇区信息
	i = buff[20];
	for(;;)
	{
		if(255 == buff[i])         //对于没有80属性的文件,遍历完所有属性后结束遍历
		{
			break;
		}
		if(128 == buff[i])         //80属性
		{
			if(0 == buff[i+8])     //常驻80属性
			{
				newFData->firstSec = (*fNode)->firstSec;
				newFData->size = buff[i+4] - 24;
				break;
			}

			i = i + buff[i + 32];
			addr = newFData->baseSec;
			for(;;)               //循环遍历计算DataRun
			{				
				size = 0;
				for(j = 0;j < buff[i]%16;j++)
				{
					size = size + buff[i+1+j] * pow(256,j) * newFData->clusterSize;
				}

				if(buff[i + buff[i]%16 + buff[i]/16] > 127)  //负数则求补码后减
				{
					for(j = 0; j < buff[i]/16;j++)
					{
						temp = ~buff[i + buff[i]%16 + 1 + j];
						temp = temp & 255;
						addr = addr - temp * pow(256,j) * newFData->clusterSize;
					}
					addr = addr - 8;
				}
				else
				{
					for(j = 0; j < buff[i]/16;j++)
					{
						 addr = addr + buff[i + buff[i]%16 + 1 + j] * pow(256,j) * newFData->clusterSize;
					}
				}

				newFData->firstSec = addr;
				newFData->size = size;

				bkFData = newFData;                        //保存原来的数据节点
				InitFData(&newFData);                      //申请新的数据节点
				bkFData->nextFData = newFData;             //将原来数据节点的下一个数据节点指向新的数据节点
				newFData->baseSec = (*fNode)->baseSec;
				newFData->clusterSize = (*fNode)->clusterSize;

				i = i + buff[i]/16 + buff[i]%16 + 1;
				if (0 == buff[i])                //80属性结束
				{
					bkFData->nextFData = NULL;   //将保存数据的节点的下一指针置空
					free(newFData);              //释放最后一个没用到的数据节点
					return 0;
				}
			}
		}
		else
		{
			i = i + buff[i + 4] + buff[i + 5]*256;
		}
	}

	return 0;
}

/*
函数说明:
	用于获取一个索引项
输入参数:
	HANDLE hDisk            : 硬盘句柄
	unsigned char buff[]    : 目录项所在扇区
	CNode *fatherCNode      : 当孩子节点为空时要用到的父目录指针
	CNode **lastChildCNode  : 最后一个孩子目录指针,有可能为空
	FNode **lastChildFNode  : 最后一个孩子文件指针,有可能为空
	int i                   : 提供索引项的首地址
输出参数:
	0  代表索引项结束
	1  代表还有索引项
  */
int GetAIndex(HANDLE hDisk, unsigned char buff[], CNode *fatherCNode, CNode **lastChildCNode, FNode **lastChildFNode, int i)
{
	int k = 0;

	int nameSize = 0;

	CNode *newCNode = NULL;
	FNode *newFNode = NULL;

	if(0 == buff[i+6] && 0 == buff[i+7]) //MFT序列号,说明索引项结束
	{
		return 0;
	}

	if (16 == buff[i + 75])              //如果这个索引代表着目录
	{                                    //则申请一个目录节点
		InitCNode(&newCNode);	
		if (NULL == *lastChildCNode)     //如果孩子节点为空,则将父节点的第一
		{                                //个孩子节点指向新创建的节点
			fatherCNode->firstChildCNode = newCNode;
		}
		else                             //否则将最后一个孩子节点在兄弟节点指向
		{                                //新创建的节点
			(*lastChildCNode)->nextBrotherCNode = newCNode;
		}
		*lastChildCNode = newCNode;      //将最后一个孩子指针指向刚创建的节点

		//更新新创建的节点的信息
		newCNode->fatherCNode = fatherCNode;
		newCNode->mftSec = fatherCNode->mftSec;
		newCNode->baseSec = fatherCNode->baseSec;
		newCNode->fatherSec = fatherCNode->firstSec;
		newCNode->clusterSize = fatherCNode->clusterSize;
		newCNode->firstSec = 2 * (buff[i] + buff[i+1] * 256 + buff[i+2] * 256 * 256 + buff[i+3] * 256 * 256 * 256) + newCNode->mftSec;
		i = i + 72;
		newCNode->attribute = buff[i] + buff[i+1] * 256 + buff[i+2] * 256 * 256 + buff[i+3] * 256 * 256 * 256;
		i = i + 10;
		nameSize = buff[i-2];
		for(k = 0;(k < nameSize) && (k < 30);k++)
		{
			newCNode->name[k] = buff[i];
			i = i + 2;
			/*if(0 == buff[i+1])
			{
				newCNode->name[k] = buff[i];
				i = i + 2;
			}
			else
			{
				newCNode->name[2*k] = buff[i];
				newCNode->name[2*k+1] = buff[i+1];
				i = i + 2;
			}*/
		}				
	}
	else               //文件节点与目录节点的处理方法类似
	{
		InitFNode(&newFNode);				
		if (NULL == *lastChildFNode)
		{
			fatherCNode->firstChildFNode = newFNode;
		}
		else
		{
			(*lastChildFNode)->nextBrotherFNode = newFNode;
		}
		*lastChildFNode = newFNode;

		newFNode->fatherCNode = fatherCNode;
		newFNode->mftSec = fatherCNode->mftSec;
		newFNode->baseSec = fatherCNode->baseSec;
		newFNode->fatherSec = fatherCNode->firstSec;
		newFNode->clusterSize = fatherCNode->clusterSize;
		newFNode->firstSec = 2 * (buff[i] + buff[i+1] * 256 + buff[i+2] * 256 * 256 + buff[i+3] * 256 * 256 * 256) + newFNode->mftSec;
		i = i + 64;
		newFNode->size = buff[i] + buff[i+1] * 256 + buff[i+2] * 256 * 256 + buff[i+3] * 256 * 256 * 256;
		i = i + 8;
		newFNode->attribute = buff[i] + buff[i+1] * 256 + buff[i+2] * 256 * 256 + buff[i+3] * 256 * 256 * 256;
		i = i + 10;
		nameSize = buff[i-2];
		for(k = 0;(k < nameSize) && (k < 30); k++)
		{
			newFNode->name[k] = buff[i];
			i = i + 2;
			/*if(0 == buff[i+1])
			{
				newFNode->name[k] = buff[i];
				i = i + 2;
			}
			else
			{
				newFNode->name[2*k] = buff[i];
				i = i + 1;
				newFNode->name[2*k+1] = buff[i];
				i = i + 1;
			}*/
		}
		GetFData(hDisk,&newFNode);
	}

	return 1;
}

/*
函数说明:
	用于获取索引列表
输入参数:
	HANDLE hDisk  :  硬盘句柄
	CNode *cNode  :  目录节点指针
	int addr      :  索引地址
	int size      :  索引大小
输出参数:
	0  代表正常退出
  */
int GetIndexList(HANDLE hDisk,CNode *cNode,int addr,int size)
{
	int i = 0;                      //数据位置
	int j = 0;                      //循环变量
	int temp = 0;                   //保存下一个属性的地址      
	int flag = 0;                   //记录GetAIndex函数的返回值,用于判断是否要结束循环
	int dCount = 0;                 //实际读取的字节数
	unsigned char buff[1025] = {0}; //保存两个扇区的数据

	CNode *lastChildCNode = NULL;   //指向最后一个目录子节点
	FNode *lastChildFNode = NULL;   //指向最后一个文件子节点

	//如果有目录子节点,则将lastChildCNode指向最后一个目录子节点
	if(NULL != cNode->firstChildCNode)
	{
		lastChildCNode = cNode->firstChildCNode;
		for(;;)
		{
			if(NULL != lastChildCNode->nextBrotherCNode)
			{
				lastChildCNode = lastChildCNode->nextBrotherCNode;
			}
			else
			{
				break;
			}
		}
	}

	//如果有文件子节点,则将lastChildFNode指向最后一个文件子节点
	if(NULL != cNode->firstChildFNode)
	{
		lastChildFNode = cNode->firstChildFNode;
		for(;;)
		{
			if(NULL != lastChildFNode->nextBrotherFNode)
			{
				lastChildFNode = lastChildFNode->nextBrotherFNode;
			}
			else
			{
				break;
			}
		}
	}

	GetBuff(hDisk,buff,addr,&dCount,1024);
	i = buff[24] + 24;                     //获得第一个索引项的地址
	for(j = 1;j < size;j++)                //循环读取索引项
	{
		for(;;)
		{
			temp = i + buff[i+8];          //保存下一个索引项的地址
			flag = GetAIndex(hDisk,buff,cNode,&lastChildCNode,&lastChildFNode,i);
			if(0 == flag)
			{
				return 0;
			}

			if (temp > 512)                //下一个索引项在另一个扇区,则记录下
			{         //下一个索引项相对下一个扇区的偏移,然后退出,读取一个扇区
				i = temp - 512;
				break;
			}
			else
			{
				i = temp;
			}
		}
		GetBuff(hDisk,buff,addr+j,&dCount,1024);
	}

	return 0;
}

/*
函数说明:
	用于获取子目录下所有文件和文件夹信息
输入参数:
	HANDLE hDisk  :  硬盘句柄
	CNode *cNode  :  需要获取信息的目录节点指针
输出参数:
	0  代表正常退出
  */
int GetDir(HANDLE hDisk,CNode *cNode)
{
	int i = 0;                      //循环变量
	int j = 0;

	int temp = 0;                   //用于临时保存i的值
	int dCount = 0;                 //保存读取的字节数
	int nameSize = 0;               //用于保存文件名长度
	int addr = 0;                   //用于保存索引分配表地址
	int size = 0;                   //用于保存索引分配表大小

	unsigned char buff[1025] = {0}; //保存读取的数据,索引项可能垮分区,所以每次读取两个分区

	GetBuff(hDisk,buff,cNode->firstSec,&dCount,1024);  //获取扇区信息
	i = buff[20];
	for(;;)
	{
		if (48 == buff[i])   //30H属性
		{
			temp = i + buff[i + 4] + buff[i + 5]*256;
			i = i + 80;
			cNode->attribute = buff[i] + buff[i+1] * 256 + buff[i+2] * 256 * 256 + buff[i+3] * 256 * 256 * 256;
			i = i + 10;
			nameSize = buff[i-2];
			for(j = 0;(j < nameSize)&&(j < 30);j++)
			{
				cNode->name[j] = buff[i];
				i = i + 2;
			}
			i = temp;
		}
		else if(144 == buff[i]) //90属性
		{
			GetAIndex(hDisk,buff,cNode,&cNode->firstChildCNode,&cNode->firstChildFNode,i+64);
			if(0 == buff[i+60]) //小索引目录项只在90属性
			{
				return 0;
			}
			i = i + buff[i + 4] + buff[i + 5]*256;
		}
		else if(160 == buff[i]) //A0属性
		{
			i = i + 72;
			addr = cNode->baseSec;
			for(;;)             //循环遍历索引表
			{
				size = 0;
				for(j = 0;j < buff[i]%16;j++)
				{
					size = size + buff[i+1+j] * pow(256,j) * cNode->clusterSize;
				}

				if(buff[i + buff[i]%16 + buff[i]/16] > 127)  //负数则求补码后减
				{
					for(j = 0; j < buff[i]/16;j++)
					{
						temp = ~buff[i + buff[i]%16 + 1 + j];
						temp = temp & 255;
						addr = addr - temp * pow(256,j) * cNode->clusterSize;
					}
					addr = addr - 8;
				}
				else
				{
					for(j = 0; j < buff[i]/16;j++)
					{
						 addr = addr + buff[i + buff[i]%16 + 1 + j] * pow(256,j) * cNode->clusterSize;
					}
				}

				GetIndexList(hDisk,cNode,addr,size);
				i = i + buff[i]/16 + buff[i]%16 + 1;
				if (0 == buff[i])
				{
					return 0;
				}
			}
		}
		else
		{
			i = i + buff[i + 4] + buff[i + 5]*256;
		}
	}
}

/*
函数说明:
	用于显示目录下的文件信息
输入参数:
	CNode *cNode  :  需要显示信息的目录节点指针
输出参数:
	0  代表正常退出
  */
int ShowDir(CNode *cNode)
{
	FNode *childFNode = NULL;
	CNode *childCNode = NULL;

	//循环显示子目录
	if(NULL != cNode->firstChildCNode)
	{
		childCNode = cNode->firstChildCNode;
		for(;;)
		{
			printf("dir : %s\n",childCNode->name);
			if(NULL == childCNode->nextBrotherCNode)
			{
				break;
			}
			else
			{
				childCNode = childCNode->nextBrotherCNode;
			}
		}
	}

	printf("\n");

	//循环显示本目录下的文件
	if(NULL != cNode->firstChildFNode)
	{
		childFNode = cNode->firstChildFNode;
		for(;;)
		{
			printf("file : %s\n",childFNode->name);
			if(NULL == childFNode->nextBrotherFNode)
			{
				break;
			}
			else
			{
				childFNode = childFNode->nextBrotherFNode;
			}
		}
	}

	printf("\n >>");

	return 0;
}

/*
函数说明:
	用于获取目录节点
输入参数:
	HANDLE hDisk     : 硬盘句柄
	CNode **cNode    : 需要获取信息的目录的父目录指针
	char parameter[] : 需要获取的目录名
输出参数:
	0  代表正常退出
	-1 代表异常退出
  */
int GetCatalogNode(HANDLE hDisk, CNode **cNode, char parameter[])
{
	CNode *bkCNode = NULL;
	CNode *preCNode = NULL;

	bkCNode = *cNode;
	
	//preCNode指向第一个孩子节点,如果没有孩子节点则退出
	if (NULL == (*cNode)->firstChildCNode)
	{
		return -1;
	}
	else
	{
		preCNode = (*cNode)->firstChildCNode;
	}


	//循环查找子目录与所输入的目录相匹配的目录,找到则正常退出,否则返回-1
	for(;;)
	{
		if(0 == (strcmp(preCNode->name,parameter)))
		{
			*cNode = preCNode; 
			return 0;
		}
		else
		{
			*cNode = preCNode;
			if (NULL == (*cNode)->nextBrotherCNode)
			{
				*cNode = bkCNode;
				return -1;
			}
			preCNode = (*cNode)->nextBrotherCNode;			
		}
	}
}

/*
函数说明:
	用于显示分区信息
输入参数:
	PNode *pNode    : 第一个分区节点 
	int patitionNum : 要显示的分区编号
输出参数:
	0  代表正常退出
	-1 代表没有相应分区
  */
int ShowPatitionInfo(PNode *pNode, int patitionNum)
{
	PNode *backPNode = pNode;
	PNode *prePNode = pNode;

	for(;;)                              //循环遍历分区列表
	{
		if(NULL == prePNode->nextPNode)  //最后一个分区
		{
			if(prePNode->patitionNum == patitionNum)
			{
				break;
			}
			else
			{
				printf("error patition Num!!please check and try again!!\n\n >>");
				return -1;
			}			
		}
		else                             //中间分区
		{
			if(prePNode->patitionNum == patitionNum)
			{
				break;
			}
			else
			{
				prePNode = backPNode->nextPNode;
				backPNode = prePNode;
			}
		}
	}

	printf("\n分区编号 : %d\n",prePNode->patitionNum);

	printf("引导类型 : ");
	if (128 == prePNode->bootFlag)
	{
		printf("活动分区\n");
	}
	else
	{
		printf("非活动分区\n");
	}

	printf("分区类型 : ");
	switch(prePNode->patitionType)
	{
	case 0:
		printf("未使用\n");
		break;
	case 6:
		printf("FAT16\n");
		break;
	case 11:
		printf("FAT32\n");
		break;
	case 5:
		printf("扩展分区\n");
		break;
	case 7:
		printf("NTFS分区\n");
		break;
	case 15:
		printf("win扩展分区\n");
		break;
	default:
		printf("其他\n");
		break;
	}


	printf("分区首扇区号 : %-11d\n",prePNode->firstSec);

	printf("分区大小 : %.2fG\n\n >>",prePNode->patitionSize);

	return 0;
}

/*
函数说明:
	用于显示目录信息
输入参数:
	CNode *cNode : 要显示的目录指针
输出参数:
	0  代表正常退出
  */
int ShowCatalogInfo(CNode *cNode)
{
	printf("目录名                             : %s\n",cNode->name);
	printf("父目录名                           : %s\n",cNode->fatherCNode->name);
	printf("目录所在扇区首扇区号               : %d\n",cNode->baseSec);
	printf("本目录所在分区MFT记录首扇区号      : %d\n",cNode->mftSec);
	printf("目录的文件记录所在扇区号           : %d\n",cNode->firstSec);
	printf("目录属性                           : %8x ",cNode->attribute);
	if (cNode->attribute & 1)
	{
		printf("只读文件 ");
	}
	if(cNode->attribute & 2)
	{
		printf("隐藏文件 ");
	}
	if(cNode->attribute & 4)
	{
		printf("系统文件 ");
	}

	printf("\n\n >>");

	return 0;
}

/*
函数说明:
	用于显示文件信息
输入参数:
	FNode *fNode : 要显示的文件指针
输出参数:
	0  代表正常退出
  */
int ShowFileInfo(FNode **fNode)
{
	int i = 0;
	FData *fData = (*fNode)->fData;

	printf("文件名                         : %s\n",(*fNode)->name);
	printf("文件大小                       : %dKB(这个好像有点不对)\n",(*fNode)->size/1024);
	printf("父目录名                       : %s\n",(*fNode)->fatherCNode->name);
	printf("文件所在扇区首扇区号           : %d\n",(*fNode)->baseSec);
	printf("本文件所在分区MFT记录首扇区号  : %d\n",(*fNode)->mftSec);
	printf("文件的文件记录所在扇区号       : %d\n",(*fNode)->firstSec);
	printf("文件属性                       : %0 8x ",(*fNode)->attribute);
	if ((*fNode)->attribute & 1)
	{
		printf("只读文件 ");
	}
	if((*fNode)->attribute & 2)
	{
		printf("隐藏文件 ");
	}
	if((*fNode)->attribute & 4)
	{
		printf("系统文件 ");
	}

	i = 1;
	printf("\n文件数据                       :\n");
	for(;;)
	{
		printf("\t第%d个数据区 --- 首扇区号 : %d\t所占用的扇区数 : %d\n",i,fData->firstSec,fData->size);
		if(NULL != fData->nextFData)
		{
			fData = fData->nextFData;
			i++;
		}
		else
		{
			break;
		}
	}
	printf("\n\n >>");

	return 0;
}
