#pragma once

/**
	M master, W worker

	PING       M -> W   broadcasted
	REQUEST    W -> M
	SLEEP      M -> W
	EXIT       M -> W
	TASK       M -> W
	SOLUTION   W -> M
*/
enum MessageType {
	PING, 
	REQUEST, 
	SLEEP, 
	EXIT, 
	TASK, 
	SOLUTION
};