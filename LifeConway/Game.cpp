#include "Game.h"

DWORD WINAPI GameThreadFunction(LPVOID lpParam)
{
	Game* game = (Game*)lpParam;
	while (true) {
		if (game->GetGameState() == GAME_STATE_STOP) {
			break;
		}

		game->GameStep();

		// ƒополнительный брейк, что бы поток не засыпал если его отменили.
		if (game->GetInterval() == GAME_STATE_STOP) {
			break;
		}

		Sleep(game->GetInterval());
	}

	game->StateInit();

	return 0;
}

Game::Game(HWND hWnd, int width, int height) {
	windowSize = { 0, 0, width, height };
	gameState = GAME_STATE_INIT;
	hThread = NULL;
	hBitmapDC = NULL;
	hBitmap = NULL;
	interval = 1000;
	xPosStart = 10;
	yPosStart = 10;
	cellSize = 12;
	rowCount = (height - (yPosStart * 2)) / cellSize;
	columnCount = (width - (xPosStart * 2)) / cellSize;

	table = new Citizen * [rowCount];
	for (int r = 0; r < rowCount; r++) {
		table[r] = new Citizen[columnCount];
	}

	this->hWnd = hWnd;
	hWndDC = GetDC(hWnd);
	hBitmapDC = CreateCompatibleDC(hWndDC);
	hBitmap = CreateCompatibleBitmap(hWndDC, width, height);
	SelectObject(hBitmapDC, hBitmap);

	hBrush = CreateSolidBrush(RGB(255, 99, 71));
	hPen = CreatePen(PS_SOLID, 1, RGB(105, 105, 105));
}

Game::~Game() {
	DeleteObject(hBitmap);
	DeleteObject(hBitmapDC);
	DeleteObject(hBrush);
	DeleteObject(hPen);
	ReleaseDC(hWnd, hWndDC);
	for (int r = 0; r < rowCount; r++) {
		delete[] table[r];
	}
	delete[] table;
}

void Game::Start() {
	if (gameState != GAME_STATE_INIT)
		return;

	gameState = GAME_STATE_START;
	DWORD threadId = 1000;
	hThread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size  
		GameThreadFunction,     // thread function name
		this,                   // argument to thread function 
		0,                      // use default creation flags 
		&threadId);   // returns the thread identifier 
}

void Game::Stop() {
	if (gameState != GAME_STATE_START)
		return;

	gameState = GAME_STATE_STOP;
	CloseHandle(hThread);
	hThread = NULL;
}

void Game::Clear() {
	if (gameState != GAME_STATE_INIT)
		return;

	for (int r = 0; r < rowCount; r++) {
		for (int c = 0; c < columnCount; c++) {
			Citizen& citizen = table[r][c];
			citizen.live = false;
			citizen.neighborCount = 0;
		}
	}

	InvalidateRect(hWnd, &windowSize, false);
}

void Game::StateInit() {
	if (gameState == GAME_STATE_STOP)
		gameState = GAME_STATE_INIT;
}

void Game::GameStep() {
	newCitizen.clear();
	deadCitizen.clear();
	for (int r = 0; r < rowCount; r++) {
		for (int c = 0; c < columnCount; c++) {
			Citizen& citizen = table[r][c];
			if (!citizen.live && citizen.neighborCount == 3) {
				newCitizen.push_back({ c, r });
			}
			else if (citizen.live && citizen.neighborCount < 2 || citizen.neighborCount > 3) {
				deadCitizen.push_back({ c, r });
			}
		}
	}

	for (auto it = newCitizen.begin(); it != newCitizen.end(); it++) {
		AddCitizen((*it).y, (*it).x);
	}

	for (auto it = deadCitizen.begin(); it != deadCitizen.end(); it++) {
		RemoveCitizen((*it).y, (*it).x);
	}

	InvalidateRect(hWnd, &windowSize, false);
}

void Game::Draw() {
	POINT pt;

	int xPos = xPosStart;
	int yPos = yPosStart;

	SelectObject(hBitmapDC, hPen);
	FillRect(hBitmapDC, &windowSize, (HBRUSH)GetStockObject(WHITE_BRUSH));

	for (int r = 0; r < rowCount; r++) {
		xPos = xPosStart;
		for (int c = 0; c < columnCount; c++) {
			xPos += cellSize;
			MoveToEx(hBitmapDC, xPos, yPos, &pt);
			LineTo(hBitmapDC, xPos, yPos + cellSize);

			if (table[r][c].live) {
				DrawCell(hBitmapDC, r, c);
			}
		}

		yPos += cellSize;

		MoveToEx(hBitmapDC, xPosStart, yPos, &pt);
		LineTo(hBitmapDC, xPos, yPos);
	}

	// top line
	MoveToEx(hBitmapDC, xPosStart, yPosStart, &pt);
	LineTo(hBitmapDC, xPosStart + (cellSize * columnCount), yPosStart);

	// left line
	MoveToEx(hBitmapDC, xPosStart, yPosStart, &pt);
	LineTo(hBitmapDC, xPosStart, yPosStart + (cellSize * rowCount));

	// use double buffer
	BitBlt(hWndDC,
		0, 0,
		windowSize.right, windowSize.bottom,
		hBitmapDC,
		0, 0,
		SRCCOPY);
}

void Game::DrawCell(HDC hdc, int row, int col) {
	RECT rect = {
		xPosStart + (col * cellSize),
		yPosStart + (row * cellSize),
		xPosStart + (col * cellSize) + cellSize,
		yPosStart + (row * cellSize) + cellSize };
	FillRect(hdc, &rect, hBrush);
}

void Game::ChangeCell(int row, int column, bool add) {
	if (row > 0 && row < rowCount && column > 0 && column < columnCount) {
		if (add || !table[row][column].live)
			AddCitizen(row, column);
		else
			RemoveCitizen(row, column);

		InvalidateRect(hWnd, &windowSize, false);
	}
}

void Game::AddCitizen(int row, int column) {
	Citizen& citizen = table[row][column];
	if (citizen.live)
		return;

	citizen.live = true;
	// ”величивать кол-во жителей текущей €чейки не нужно, т.к. это делают другие €чейки при рождении.

	// left
	if (column > 0) {
		table[row][column - 1].neighborCount++;
	}

	// left-top
	if (column > 0 && row > 0) {
		table[row - 1][column - 1].neighborCount++;
	}

	// top
	if (row > 0) {
		table[row - 1][column].neighborCount++;
	}

	// top-right
	if (row > 0 && column + 1 < columnCount) {
		table[row - 1][column + 1].neighborCount++;
	}

	// right
	if (column + 1 < columnCount) {
		table[row][column + 1].neighborCount++;
	}

	// right-bottom
	if (row + 1 < rowCount && column + 1 < columnCount) {
		table[row + 1][column + 1].neighborCount++;
	}

	// bottom
	if (row + 1 < rowCount) {
		table[row + 1][column].neighborCount++;
	}

	// bottom-left
	if (column > 0 && row + 1 < rowCount) {
		table[row + 1][column - 1].neighborCount++;
	}
}

void Game::RemoveCitizen(int row, int column) {
	Citizen& citizen = table[row][column];
	if (!citizen.live)
		return;

	citizen.live = false;
	// ”меньшать кол-во жителей у текущей клетки не нужно т.к это делают другие клетки при погибании.

	// left
	if (column > 0) {
		table[row][column - 1].neighborCount--;
	}

	// left-top
	if (column > 0 && row > 0) {
		table[row - 1][column - 1].neighborCount--;
	}

	// top
	if (row > 0) {
		table[row - 1][column].neighborCount--;
	}

	// top-right
	if (row > 0 && column + 1 < columnCount) {
		table[row - 1][column + 1].neighborCount--;
	}

	// right
	if (column + 1 < columnCount) {
		table[row][column + 1].neighborCount--;
	}

	// right-bottom
	if (row + 1 < rowCount && column + 1 < columnCount) {
		table[row + 1][column + 1].neighborCount--;
	}

	// bottom
	if (row + 1 < rowCount) {
		table[row + 1][column].neighborCount--;
	}

	// bottom-left
	if (column > 0 && row + 1 < rowCount) {
		table[row + 1][column - 1].neighborCount--;
	}
}

bool Game::FindCell(int x, int y, POINT& point) {
	int xPos = xPosStart;
	int yPos = yPosStart;

	for (int r = 0; r < rowCount; r++) {
		xPos = xPosStart;
		int nextYPos = yPos + cellSize;
		for (int c = 0; c < columnCount; c++) {
			int nextXPos = xPos + cellSize;
			if (x >= xPos && x < nextXPos && y >= yPos && y < nextYPos) {
				point.x = c;
				point.y = r;
				return true;
			}
			xPos = nextXPos;
		}
		yPos = nextYPos;
	}

	return false;
}

int Game::GetGameState() {
	return gameState;
}

unsigned int Game::GetInterval() {
	return interval;
}

int Game::GetRowCount() {
	return rowCount;
}

int Game::GetColumnCount() {
	return columnCount;
}

void Game::SetSpeed(int speed) {
	interval = 1000 / speed;
}