#include"Structs.h"

void Column::set(int pos, int player) {
	col[pos] = player;
	len++;
}

int Column::get(int pos) {
	return col[pos];
}

int Column::maxPos() {
	return len - 1;
}


void Board::set(int xpos, int ypos, int player) {
	if (!isValidPos(xpos, ypos))
		throw GameException("Invalid position!");
	cols[xpos].set(ypos, player);
}

int Board::get(int xpos, int ypos) {
	return cols[xpos].get(ypos);
}

//put move on the board
bool Board::play(int xpos, int player) {
	int ypos = cols[xpos].maxPos() + 1;
	
	set(xpos, ypos, player);
	return checkWin(xpos, ypos);
}

//check if there is a win in the game on a current board state
bool Board::checkWin(int xpos, int ypos) {
	/**
		| 2 | 4 | 7 |
		| 1 | * | 6 |   ---> designated directions considering current position *
		| 0 | 3 | 5 |
	*/
	const int xy[8][2] = { {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1} };
	const int pairs[4][2] = { {0, 7}, {1, 6}, {2, 5}, {3, 4} };
	
	int init = get(xpos, ypos);
	int cnt[8] = { 0,0,0,0,0,0,0,0 };

	if (init == 0)
		return false;

	for (int k = 0; k < 8; k++) {
		int x = xpos, y = ypos;
		while (isValidPos(x, y) && get(x, y) == init) {
			x += xy[k][0];
			y += xy[k][1];
			cnt[k]++;
		}
	}
	
	//iterate through pairs of directions
	for (int i = 0; i < 4; i++)
		if (cnt[pairs[i][0]] + cnt[pairs[i][1]] >= 5)
			return true;
	
	return false;
}
//print current game board
void Board::draw() {
	printf("%*s", NUM_SPACES, "");
	printf("  + - - - - - - - +\n");
	for (int j = 5; j >= 0; j--) {
		printf("%*s", NUM_SPACES, "");
		printf("%d |", j);
		for (int i = 0; i < 7; i++) {
			int v = get(i, j);
			printf("%s", v == 0 ? " *" : (v == 1 ? " P" : " C"));
			fflush(stdout);
		}
		printf(" |\n");
	}
	printf("%*s", NUM_SPACES, "");
	printf("  + - - - - - - - +\n");
	printf("%*s", NUM_SPACES, ""); 
	printf("    0 1 2 3 4 5 6 \n\n");
	fflush(stdout);
}
//check if given position is on the board
bool Board::isValidPos(int xpos, int ypos) {
	return (xpos >= 0 && xpos < columns && ypos >= 0 && ypos < rows);
}