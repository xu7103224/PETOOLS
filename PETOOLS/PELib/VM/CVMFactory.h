// VMPackLib


#ifndef __CVMFACTORY__
#define __CVMFACTORY__
#include "InterpretHandler.h"
#include "VCommand.h"
#include "CLink.h"
#include "CVirtualMemory.h"
#include"Structs.h"
#include"CTree.h"
#include<vector>
#include<list>
using namespace std;

#pragma   warning(disable:4311)//关闭类型转换警告
#pragma   warning(disable:4312)//关闭类型转换警告

//vstartvm
//Jcc:																		;当流程改变时跳到这里
//mov		ebx,esi															;把伪代码base_addr做key
#define		JCCOFFSET			23
//movzx   eax, byte ptr [esi] 不需要更换密钥时直接跳到这里
#define		JMPOFFSET			25

//DCheckESP跳到vstartvm的偏移
#define		STACKOFFSET			8


// 处理VM的一些事项,填充Handler,生成字节码
class CVMFactory
{

private:
	CVirtualMemory *m_JumpTable;				//JMP表（handler地址序列，根据idx跳到此处jmp 此处地址）
	CVirtualMemory *m_CodeEngine;				//VM指令引擎(handler指令代码)
	CVirtualMemory *m_EnterStub;				//重新进入vm的汇编代码
	CVirtualMemory *m_VMEnterStubCode;			//重新进入的vmcode
	CVirtualMemory *m_VMCode;					//通常的vmcode（翻译后的虚拟机代码）

private:
	CInterpretHandler InterpretHandler;
public:

	CVMFactory();

	~CVMFactory();
private:
	list<VCodeNode*> vmlist;//主要代码链表
	void ClearList();
public:
	// 通过汇编语句所在的虚拟地址找出VM语句所在的地址
	DWORD	GetVMAddrFromVirtualAddress(DWORD VirtualAddress);
	// 编译从代码中进入虚拟机的汇编代码片段
	void	CompileEnterStubCode(char* baseaddr,DWORD VirtualAddress,int len);
	// 编译代码为字节码
	BOOL	BuildVMCode(char* baseaddr,AVL<CodeNode,ulong> *AVLTree,list<AddrNode*> *AddrNodes,char* ErrText);
	// 转换为VM结构并加入链表(非编译为字节码)
	BOOL	TranslateVM(CodeNode* code);
	// 修改链表中的一些数据(如跳转)
	void	FixVCodeList(list<AddrNode*> *AddrNodes);

	//添加handler到链表中
	void	AddVMHandler(CodeNode *codenode,CVirtualMemory* sect);
	//非CALL
	void	AddVMHandlerEx(VCodeNode* vcodenode,CodeNode *codenode,CVirtualMemory* sect);
	void	AddRealCodeToVM(CodeNode *codenode,CVirtualMemory* sect);
	// 辅助VM指令
	//idx = 第几个操作数
	void	AddDPushIMM(int idx,CodeNode *codenode,CVirtualMemory* sect);
	void	AddDPushREG(int idx,CodeNode *codenode,CVirtualMemory* sect);
	void	AddDPushMem(int idx,CodeNode *codenode,CVirtualMemory* sect);

	void	AddDPopReg(int idx,CodeNode *codenode,CVirtualMemory* sect);
	void	AddDFree(int idx,CodeNode *codenode,CVirtualMemory* sect);
	void	AddDSaveEsp(int idx,CodeNode *codenode,CVirtualMemory* sect);
	void	AddDRestoreEsp(int idx,CodeNode *codenode,CVirtualMemory* sect);

	// 进入VM后执行的第一条恢复指令
	void	AddBeginHanlder(list<VCodeNode*>* vlist,CVirtualMemory* sect);
	// 编译
	// vcodenode = VM结构
	// sect = 所在的内存段
	// CodeNode *codenode用于修正VMAddress地址
	void	CompileVMCode(VCodeNode* vcodenode,CVirtualMemory* sect);

	// 编译未声明的指令
	void	CompileUndeclared(t_disasm *disasm);
	// 编译重新进入虚拟机的汇编代码片段
	DWORD	CompileReEnterStubCode(t_disasm *disasm);
	// 编译退出并跳转到真实环境执行的代码
	void	CompileToRealCode(t_disasm *disasm);
	// 编译重新进入虚拟机的VM代码片段
	void	CompileVMEnterStubCode();
public:
	// 获得虚拟内存类
	void	SetupVirtualMemory(CVirtualMemory *JumpTable,
								CVirtualMemory *CodeEngine,
								CVirtualMemory *EnterStub,
								CVirtualMemory *VMEnterStub,
								CVirtualMemory *VMCode);
	// 初始化VM环境.
	// 1.初始化寄存器位置
	// 2.初始化核心宏代码(将代码拷贝到结构体中).
	// 3.将Handler写入对应新节的内存地址.
	// 4.得到Handler所在的虚拟地址后,修复相互间的跳转.设置汇编代码中固定的一些值.
	void	InitVM();
private:
	// 初始化核心宏和Handler
	BOOL	InitCoreMacro();
	// 初始化手动Handler
	BOOL    InitCoreManualHandler();
	// 初始化VM描述表的Handler
	BOOL	InitCoreVMTableHandler();
	// 编译生成的Handler代码并添加到Handler
	BOOL	CompileHandler(char* handlername,char* asmtext);
	// 放置Handler
	void	PlaceHandler();
	// 修复StartVM
	void	FixStartVM();
	// 修复DCheckESP
	void	FixCheckESP();
private://service
	// 编译代码
	BOOL	CompileCode(DWORD VirtualAddr,char* asmtext,char *code,int *len);
	// 通过名称获得Handler结构
	VHandler* GetHandler(char* vmname);
	// 添加一个Handler名称到Handler索引表
	BOOL	AddVHandlerName(char* vmname);
	//如果是V(自动生成的)指令,返回结构,否则返回NULL
	VMTable	*SearchForVMTable(char* vmname);
	//从已经加载的指令中搜索,有的话则返回VMTABLE描述表
	VMTable* GetVMTableForAlready(char* vmname);
	// 获得func_stub指向的地址(jmp xxxx)
	HandlerFunc GetFunctionRVA(HandlerFunc Func);
	// 获得函数长度
	int GetFunctionSize(HandlerFunc FuncName);
private:
	// 通过VM指令名称获得索引
	int GetIdxFromVMName(char* vmname);
private:
	// 伪指令数组，顺序存放编译的handler序列
	//VHandler	m_vhandler[HANDLERMAXLEN];
	//int			m_curvhandidx;
	list<VHandler> m_vhandler;
private:
	// Handler所对应的随机序列,顺序存放handler名以及对应随机idx与m_HandlerIdx存储的序列对应
	//VM_RandomTable	m_RandomTable[HANDLERMAXLEN];
	vector<VM_RandomTable> m_RandomTable;
	// 当前用到的序号
	//int			m_curtableidx;
	// 初始化一个Handler乱序表,然后每添加一个Handler就从中获取一个索引
	//int			m_HandlerIdx[HANDLERMAXLEN];
	//vector<int> m_HandlerIdx;
private:
	// Handler地址跳转表,jmp跳这个表的地址，即handler地址执行，随机
	//DWORD		m_HandlerTable[HANDLERMAXLEN];
	vector<DWORD> m_HandlerTable;

};// END CLASS DEFINITION CVMFactory

#endif // __CVMFACTORY__
 