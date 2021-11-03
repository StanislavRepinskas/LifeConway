#pragma once
#include <windows.h>
#include <list>

DWORD WINAPI GameThreadFunction(LPVOID lpParam);

#define GAME_STATE_INIT 0
#define GAME_STATE_START 1
#define GAME_STATE_STOP 2

class Citizen {
public:
	bool live;
	unsigned char neighborCount;

	Citizen() {
		live = false;
		neighborCount = 0;
	}
};

class Game {
private:
	HANDLE hThread;
	HDC hWndDC;
	HDC hBitmapDC;
	HBITMAP hBitmap;
	Citizen** table;
	HBRUSH hBrush;
	HPEN hPen;
	unsigned int interval;
	int gameState;
	int rowCount;
	int columnCount;
	int cellSize;
	int xPosStart;
	int yPosStart;
	HWND hWnd;
	std::list<POINT> newCitizen;
	std::list<POINT> deadCitizen;
	RECT windowSize;

	void DrawCell(HDC hdc, int row, int col);
	void AddCitizen(int row, int column);
	void RemoveCitizen(int row, int column);

public:	
	Game(HWND hWnd, int width, int height);
	~Game();
	void Start();
	void Stop();
	void Clear();
	void StateInit();
	void Draw();
	void ChangeCell(int row, int column, bool add = false);
	void GameStep();
	bool FindCell(int x, int y, POINT& point);

	int GetGameState();
	unsigned int GetInterval();
	int GetRowCount();
	int GetColumnCount();
	void SetSpeed(int speed);
};
