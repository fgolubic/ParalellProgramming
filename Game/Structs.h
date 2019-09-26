#pragma once
#include<mpi.h>
#include"MessageTypeE.h"
#include<cstring>
#include<string>
#include<exception>
#include<vector>
#include<iostream>

#define MAX_BRANCH_PATH 5
#define MAX_DATA_BYTES 256
#define NUM_SPACES 15

typedef unsigned char BYTE;

struct Message {
	MessageType type;
	BYTE data[MAX_DATA_BYTES];
	int dataSize;

	Message *ping() {
		type = PING;
		memset(data, 0, sizeof(data));
		dataSize = 0;
		return this;
	}

	Message *request() {
		type = REQUEST;
		memset(data, 0, sizeof(data));
		dataSize = 0;
		return this;
	}

	Message *sleep() {
		type = SLEEP;
		memset(data, 0, sizeof(data));
		dataSize = 0;
		return this;
	}

	Message *exit() {
		type = EXIT;
		memset(data, 0, sizeof(data));
		dataSize = 0;
		return this;
	}

	Message *task(void *dataPtr, int size) {
		type = TASK;
		memcpy(data, dataPtr, size);
		dataSize = size;
		return this;
	}

	Message *solution(void *dataPtr, int size) {
		type = SOLUTION;
		memcpy(data, dataPtr, size);
		dataSize = size;
		return this;
	}

	int send(int destination) {
		return MPI_Send(this, sizeof(Message), MPI_BYTE, destination, 0, MPI_COMM_WORLD);
	}

	int receive(int source, MPI_Status &status) {
		return MPI_Recv(this, sizeof(Message), MPI_BYTE, source, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}

	int broadcast(int root) {
		return MPI_Bcast(this, sizeof(Message), MPI_BYTE, root, MPI_COMM_WORLD);
	}

	const char* messageType() {

		switch (type) {

			case PING:
				return "ping";

			case REQUEST:
				return "request";

			case SLEEP:
				return "sleep";

			case EXIT:
				return "exit";

			case TASK:
				return "task";

			case SOLUTION:
				return "solution";

			default:
				break;

		}
		return "Unknown message";
	}
};

struct GameException : public std::exception {
	std::string message;

	GameException(std::string msg) : message(msg) {}

	virtual const char* errMsg() const throw() {
		return message.c_str();
	}

	virtual ~GameException() throw() {}
};

struct Column {
	unsigned int col[6];
	int len;

	Column() {  
		for (int i = 0; i < 6; i++) {
			col[i] = 0;
		}
		len = 0; 
	}
	void set(int pos, int player);
	int get(int pos);
	int maxPos();
};

struct Board {
	static const int rows = 6, columns = 7;
	Column cols[columns];

	Board() {}
	void set(int xpos, int ypos, int player);
	int get(int xpos, int ypos);
	bool checkWin(int xpos, int ypos);
	bool play(int xpos, int player);
	void draw();
	static bool isValidPos(int xpos, int ypos);
};

struct State {
	int pos[MAX_BRANCH_PATH];
	int len;

	State() : len(0) {}

	const bool operator <(const State &key) const {

		if (len != key.len) {
			return len < key.len;
		}
		else {
			for (int i = 0; i < len; ++i)
				if (pos[i] != key.pos[i])
					return pos[i] < key.pos[i];
			return false;
		}
		
	}

	bool operator ==(const State &key) {

		if (len != key.len) {

			return false;

		}
		else {

			for (int i = 0; i < len; i++)
				if (pos[i] != key.pos[i])
					return false;

			return true;

		}
			
	}

	State& operator = (const State &key) {
		len = key.len;
		memcpy(pos, key.pos, sizeof(pos));
		return *this;
	}

	int operator[] (int i) {
		return pos[i];
	}

	void insert(int x) {
		pos[len++] = x;
	}
};

struct Task {
	Board board;
	int nextPlayer;
	State key;

	Task() {}
	Task(Board board, int nextPlayer, State key)
		: board(board), nextPlayer(nextPlayer), key(key) {}

};

struct Solution {
	State state;
	float value;

	Solution() {}
	Solution(State state, float value) : state(state), value(value) {}
};

