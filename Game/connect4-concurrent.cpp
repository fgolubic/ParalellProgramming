#include<cstdio>
#include<deque>
#include<map>
#include<time.h>
#include<vector>
#include<thread>
#include<chrono>
#include<iostream>
#include"Structs.h"

#define HUMAN 1
#define CPU 2
#define OTHER(p) (p == CPU ? HUMAN : CPU)

#define BRANCH_DEPTH 2
#define TASK_DEPTH 6

/**
	Create tasks. Number of tasks is defined by branch depth. I.e. BRANCH_DEPTH = 2, number of tasks is 7^2. 
	Tasks are put in task queue.
*/
void generateTasks( int player, int currMove, Board board, State tempPosition, std::deque<Task> &taskQueue, int depth){

	if (currMove > -1) { // for non init call

		if (board.play(currMove, player)) // game isOver 
			return;
		else
			tempPosition.insert(currMove);
	}

	if (depth < BRANCH_DEPTH)  {

		for (int move = 0; move < 7; move++) {

			if (board.cols[move].maxPos() < board.rows - 1) {// possible currMove

				generateTasks( OTHER(player), move, board, tempPosition, taskQueue, depth + 1 );

			}
		}

	}
	
	taskQueue.push_back(Task(board, OTHER(player), tempPosition));
	return;
}

/**
	Calculate value of a state.
*/
float calcValueOfState( int player, int currMove, Board board, int depth) {

	if (currMove > -1) {

		if (board.play(currMove, player)) { // game isOver

			return (player == CPU ? 1 : -1);

		}
	}
	if (depth < TASK_DEPTH) {

		int moveCount = 0;
		float sum = 0;
		float value;

		for (int move = 0; move < 7; move++)

			if (board.cols[move].maxPos() < board.rows - 1) { // possible move

				value = calcValueOfState( OTHER(player), move, board, depth + 1);

				if ((-1 == value && player == HUMAN) || (1 == value && player == CPU))
					return value;

				sum += value;
				moveCount++;
			}
		return sum / moveCount;
	}

	return 0.0;
}

/**
	Function calculates value for a move and returns that value. If player wins value is -1, if CPU wins value is 1.
	Check it for each move. Last move is taken from memorized map.
*/
float calcValueOfMove(int player, int currMove, Board board, State tempPosition, std::map<State, float> &taskRes , int depth) {

	if (board.play(currMove, player))  // game isOver
		return (player == CPU ? 1 : -1);

	tempPosition.insert(currMove);

	if (depth < BRANCH_DEPTH){

		int moveCount = 0;
		float sum = 0;
		float value;

		for (int move = 0; move < 7; move++) {

			if (board.cols[move].maxPos() < board.rows - 1) { // possible currMove

				value = calcValueOfMove( OTHER(player), move, board, tempPosition, taskRes, depth + 1);

				if( ( -1 == value && player == HUMAN) || ( 1 == value && player == CPU) )
					return value;

				sum += value;
				moveCount++;
			}
		}
		return sum / moveCount;
	}

	return taskRes[tempPosition];
}

/**
	Function that calculates move of CPU after player took a move.
*/
int calcCPUMove(Board board, int N) {
	Solution solution;
	std::deque<Task> taskQueue;
	std::map<State, float> taskRes;
	clock_t starttime, endtime;
	const int workersNum = N - 1;
	Message msg;
	MPI_Status status;
	int k = 0;
	int stoppedWorkers = 0;
	Task task;

	starttime = clock();

	// generate tasks
	generateTasks( HUMAN, -1, board, State(), taskQueue,  0);

	if (1 == N) {
		for (auto task : taskQueue)
			taskRes[task.key] = calcValueOfState( OTHER(task.nextPlayer), -1, task.board, 0);
	}
	else if ( 1 < N ){

		// ping sleeping workers
		msg.ping()->broadcast(0);

		// process all tasks
		do {

			msg.receive(MPI_ANY_SOURCE, status);

			if (msg.type == SOLUTION) {
				memcpy(&solution, msg.data, msg.dataSize);
				taskRes[solution.state] = solution.value;
			}

			if (!taskQueue.empty()) {
				Task task = taskQueue.front(); 
				taskQueue.pop_front();
				msg.task(&task, sizeof(task))->send(status.MPI_SOURCE);
			}
			else {
				stoppedWorkers++;
				msg.sleep()->send(status.MPI_SOURCE);
			}
		} while (!taskQueue.empty() || stoppedWorkers < workersNum);
	}
	else {
		std::cout << "ERROR" << std::endl;
		fflush(stdout);
		throw GameException("Fatal error: N<1");
	}

	// collect task results and calculate best solution
	float bestSolution = -2;
	float tempSolution;
	int bestMove = -1;

	for (int move = 0; move < 7; move++) {

		if (board.cols[move].maxPos() < board.rows - 1) {  // if possible currMove

			tempSolution = calcValueOfMove( CPU, move, board, State(), taskRes, 1);

			if (tempSolution > bestSolution) {

				bestSolution = tempSolution;
				bestMove = move;

			}
		}
	}
	printf("Best computer move %d with value %.4f\n", bestMove, bestSolution);

	endtime = clock();
	printf("CPU time: %.2f\n", (endtime - starttime) / (float)(CLOCKS_PER_SEC));

	return bestMove;
}
/**
	Get action from player's input.
*/
int getAction() {
	int move;
	std::cout << "Take action. Possible moves: 0-6 --> ";
	fflush(stdout);
	scanf_s("%d", &move);
	std::cout << std::endl << "Player move ---> " << move << std::endl;
	std::cout << "-----------------------" << std::endl;
	return move;
}



/*MAIN: start  master and workers.*/
int main(int argc, char* argv[]) {
	int N = 1, k = 0, name_len;
	char processor_name[16] = "AMD";

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &N);
	MPI_Comm_rank(MPI_COMM_WORLD, &k);
	MPI_Get_processor_name(processor_name, &name_len);

	printf("Started at %s\n", processor_name);
	fflush(stdout);

	if (k == 0) { //master
		int move;
		int winner = 0;
		bool isOver;
		Board board;

		std::this_thread::sleep_for(std::chrono::seconds(1));

		printf("%*s", NUM_SPACES, "");
		std::cout << "  -----------------\n";
		printf("%*s", NUM_SPACES, "");
		std::cout << "      Connect 4\n";
		printf("%*s", NUM_SPACES, ""); 
		std::cout<<"  -----------------\n";

		 do{
			board.draw();

			// HUMAN's currMove
			while (true) {
				try {
					move = getAction();
					isOver = board.play(move, HUMAN);
					board.draw();

					if (isOver)
						winner = HUMAN;

					break;
				}
				catch (GameException exception) {
					printf("---> %s\n----------------------\n", exception.errMsg());
				}
			}

			if (winner)
				break;

			// CPU's current move
			move = calcCPUMove(board, N);
			printf("Computer move --> %d\n", move);
			isOver = board.play(move, CPU);

			if (isOver)
				winner = CPU;

			std::this_thread::sleep_for(std::chrono::seconds(1));

		}while (!winner);

		printf("\n----------------------\n%s wins!\n\n", winner == CPU ? "Computer" : "Player");
		board.draw();

		// stop all workers
		Message exitMessage;
		exitMessage.exit()->broadcast(0);
		std::cout << "Game finished." << std::endl;

	}
	else { //worker

		Message msg;
		MPI_Status status;
		Task task;
		Solution solution;

		do { // until the game is done

			msg.broadcast(0);  // wait for master command

			if (EXIT == msg.type) {
				std::cout<<"Received an EXIT."<<std::endl;
				break;
			}

			msg.request()->send(0);    // initial REQUEST message

			while (true) {  // while there are tasks

				msg.receive(0, status);
				if (SLEEP == msg.type) {
					break;
				}
				else if (TASK == msg.type) {
					memcpy(&task, msg.data, msg.dataSize);

					fflush(stdout);
					solution.state = task.key;
					solution.value = calcValueOfState( OTHER(task.nextPlayer), -1, task.board, 0);

					msg.solution(&solution, sizeof(solution))->send(0);
				}
				else {
					printf("Unknown message type --> %d", msg.type);
				}
			}
		} while (true);

	}
		

	MPI_Finalize();

	return 0;
}