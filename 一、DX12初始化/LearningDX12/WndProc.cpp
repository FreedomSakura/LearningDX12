#include "WndProc.h"

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//��Ϣ����
	switch (message)
	{
		//�����ڱ�����ʱ����ֹ��Ϣѭ��
	case WM_DESTROY:
		PostQuitMessage(0);	//��ֹ��Ϣѭ����������WM_QUIT��Ϣ
		return 0;
	default:
		break;
	}
	//������û�д������Ϣת����Ĭ�ϵĴ��ڹ���
	return DefWindowProc(hWnd, message, wParam, lParam);
}
