#include<mpi.h>
#include<iostream>
#include<time.h>
#include<string>
#include<vector>
#include<random>
#include <thread>
#include <chrono>
#include"MessageType.h"

#define IDENTATION_K 35

struct Fork {

	bool isClean;
	bool hasFork;
};

struct Message {

	int messageType;
	int senderId;
};

int main(int argc, char* argv[]) {

	//initialize random for thinking and eating
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(1, 7);
	
	MPI_Init(NULL, NULL);

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	if (world_size < 3) {
		std::cerr << "There must be at least 3 CPUs!" << std::endl;
		MPI_Finalize();
		exit(1);
	}
	int leftId = (world_rank + world_size - 1) % world_size;
	bool leftRequest = false;

	int rightId = (world_rank + 1) % world_size;
	bool rightRequest = false;

	int neighbours[2] = { leftId, rightId };
	bool requests[2] = { leftRequest, rightRequest };

	Fork leftFork, rightFork;

	//set initial fork distribution: first philosopher has both forks, 
	//last one has none and every other one has his right
	if (world_rank == 0) {

		leftFork.hasFork = true;
		leftFork.isClean = false;

		rightFork.hasFork = true;
		rightFork.isClean = false;

	}
	else if (world_rank == world_size - 1){

		leftFork.hasFork = false;
		leftFork.isClean = false;

		rightFork.hasFork = false;
		rightFork.isClean = false;

	}
	else {

		leftFork.hasFork = false;
		leftFork.isClean = false;

		rightFork.hasFork = true;
		rightFork.isClean = false;

	}

	Fork forks[2] = { leftFork, rightFork };

	std::vector<Message> leftover;

	//start philosophers problem
	while (true) {

		std::cout << std::string(IDENTATION_K * world_rank, ' ');
		printf("Filozof %d misli.", world_rank);
		std::cout << std::endl;
		int sleepSec = dist(rng);

		//sleep for random number of seconds while listening for requests
		for (int i = 0; i < sleepSec; i++) {

			std::this_thread::sleep_for(std::chrono::seconds(1));
			
			for (int j = 0; j < 2; j++) {
				int flag;
				MPI_Status status;
				MPI_Iprobe(neighbours[j], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

				if (flag == 1) {

					int request;

					MPI_Recv(&request, 1, MPI_INT, neighbours[j], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

					std::cout << std::string(IDENTATION_K * world_rank, ' ');
					printf("Filozof %d prima zahtjev od %d.", world_rank, neighbours[j]);
					std::cout << std::endl;
					std::this_thread::sleep_for(std::chrono::seconds(1));

					int response = MessageType::FORK_RESPONSE;

					MPI_Send(&response, 1, MPI_INT, neighbours[j], 0, MPI_COMM_WORLD);

					std::cout << std::string(IDENTATION_K * world_rank, ' ');
					printf("Filozof %d daje vilicu %d.", world_rank, neighbours[j]);
					std::cout << std::endl;
					std::this_thread::sleep_for(std::chrono::seconds(1));

					forks[j].hasFork = false;
					forks[j].isClean = true;
				}
			}
		}

		
		std::cout << std::string(IDENTATION_K * world_rank, ' ');
		printf("Filozof %d je gladan.", world_rank);
		std::cout << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		
		//philosopher is getting both forks
		while (true) {

			if (forks[0].hasFork && forks[1].hasFork) {
				break;
			}


			//philosopher wants to eat, so he sends requests to right and left ( if he does not have cosidered fork )
			int request = MessageType::FORK_REQUEST;

			for (int i = 0; i < 2; i++) {

				if (!forks[i].hasFork) {

					std::cout << std::string(IDENTATION_K * world_rank, ' ');
					printf("Filozof %d traži vilicu od %d.", world_rank, neighbours[i]);
					std::cout << std::endl;
					MPI_Send(&request, 1, MPI_INT, neighbours[i], 0, MPI_COMM_WORLD);
					std::this_thread::sleep_for(std::chrono::seconds(1));

				}
				else{
				
					continue;

				}

				//philosopher recieves messages as long as he doesn't have wanted fork
				while (!forks[i].hasFork) {

					for(int j = 0; j < 2; j++){
						int flag;
						MPI_Status status;
						MPI_Iprobe(neighbours[j], MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);

						if (flag == 1) {
							int buf;

							MPI_Recv(&buf, 1, MPI_INT, neighbours[j], 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

							//if response -> get the fork!
							if (buf == MessageType::FORK_RESPONSE) {

								std::cout << std::string(IDENTATION_K * world_rank, ' ');
								printf("Filozof %d prima vilicu od %d.", world_rank, neighbours[j]);
								std::cout << std::endl;
								std::this_thread::sleep_for(std::chrono::seconds(1));

								forks[j].hasFork = true;
								forks[j].isClean = true;
							}
							else {
								std::cout << std::string(IDENTATION_K * world_rank, ' ');
								printf("Filozof %d traži vilicu od %d.",  neighbours[j], world_rank);
								std::cout << std::endl;

								//if philosopher has dirty fork he must give it to other philosopher if asked
								if (forks[j].hasFork && !forks[j].isClean) {

									forks[j].hasFork = false;
									forks[j].isClean = true;
									int response = MessageType::FORK_RESPONSE;

									MPI_Send(&response, 1, MPI_INT, neighbours[j], 0, MPI_COMM_WORLD);

									std::cout << std::string(IDENTATION_K * world_rank, ' ');
									printf("Filozof %d daje vilicu %d.", world_rank, neighbours[j]);
									std::cout << std::endl;
									std::this_thread::sleep_for(std::chrono::seconds(1));
								}
								//if not then he stores request for later
								else {
									Message temp;
									temp.messageType = buf;
									temp.senderId = neighbours[j];
									leftover.push_back(temp);
								}
							}

						}
					}


				}

			}



		}
		//philosopher has both forks and starts to eat for a random no of sec
		int eatSec = dist(rng);
		std::cout << std::string(IDENTATION_K * world_rank, ' ');
		printf("Filozof %d jede.", world_rank);
		std::cout << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(eatSec));
		std::cout << std::string(IDENTATION_K * world_rank, ' ');
		printf("Filozof %d gotov.", world_rank);
		std::cout << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(1));

		forks[0].isClean = false;
		forks[1].isClean = false;

		//respond to memorized requests
		int mess = MessageType::FORK_RESPONSE;

		for (auto it = leftover.begin(); it != leftover.end(); it++) {

			if (it->messageType == MessageType::FORK_RESPONSE) {

				std::cerr << "Can't be response!" << std::endl;
				exit(1);

			}

			MPI_Send(&mess, 1, MPI_INT, it->senderId, 0, MPI_COMM_WORLD);

			std::cout << std::string(IDENTATION_K * world_rank, ' ');
			printf("Filozof %d daje vilicu %d.", world_rank, it->senderId);
			std::cout << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));

			if (it->senderId == neighbours[0]) {
				forks[0].isClean = true;
				forks[0].hasFork = false;
			}
			else if (it->senderId == neighbours[1]) {
				forks[1].isClean = true;
				forks[1].hasFork = false;

			}
			else {
				std::cerr << "Impossible sender id!!" << std::endl;
				exit(2);
			}

		}

	}


	MPI_Finalize();
	return 0;
}