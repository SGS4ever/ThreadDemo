// ==================================================
// Author:				XR_G
// File:				Ticket_Main_Linux.cpp
// Last-Modified:		2020-10-10
// Personal-Blog£º		https://SGS4ever.github.io
// Description:			A simple demo of multi-thread programming
//						using C++11 thread class.
//						In this demo, I implement a model of producer-consumer
//						problem, as well considered the priority of three consumers.
// ==================================================
#define _CRT_SECURE_NO_WARNINGS
#include <thread>
#include <iostream>
#include <chrono>
#include <ctime>
#include <mutex>
#include <queue>
#include <string>
#include <condition_variable>
#include <iomanip>
#include <pthread.h>
#include <sched.h>

using namespace std;
// ====================================
// StructName:		TicketInfo
// Description:		a ticket-structure with the time when the ticket is produced
//					and the ID of producer
// ====================================
typedef struct
{
	string producedTime;	// when the ticket is produced
	string producerId;		// the thread ID of prodecer
} TicketInfo;
// ====================================
// FunctionName:	getTime
// Description:		get current system time using chrono and ctime
// Parameter:		none
// Return Value:	string(current time)
// ====================================
string getTime()
{
	// Get current system time
	auto t = chrono::system_clock::to_time_t(chrono::system_clock::now());
	struct tm* tp = localtime(&t);

	char res_char[60];
	sprintf(res_char, "%d-%02d-%02d-%02d.%02d.%02d",
		tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
	return string(res_char);
}
// ====================================
// FunctionName:	GenerateTicket
// Description:		generate a ticket with producedTime and producerId
// Parameter:		int id
// Return Value:	TicketInfo newTicket
// ====================================
TicketInfo GenerateTicket(int id)
{
	TicketInfo newTicket;
	newTicket.producedTime = getTime();
	newTicket.producerId = to_string(id);
	return newTicket;
}
// =========================================
// Functions:
//		- Put()		put a ticket into buffer
//		- Get()		get a ticket from buffer
//		- isFull()	whether the buffer is full
//		- isEmpty()	whether the buffer is empty
// Variables:
//		- buffer	shared buffer to storage tickets
//		- t_mutex	only the thread with the mutex can operate buffer and any related variables
//		- maxSize	buffer is full when the number of ticket equals to maxSize
//		- totalNum	total number of produced ticket
//		- maxNum	when totalNum equals to maxNum, all prodecer thread should stop working
//		- notEmpty	condition_variable, consumer should wait this variable
//		- notFull	condition_variable, prodecer should wait this variable
//		- con_stat	record the distribution situation of consumers
// =========================================
void Put(int id);
void Get(int id);
bool isFull();
bool isEmpty();
queue<TicketInfo> buffer;
mutex t_mutex;
int maxSize = 10;
int totalNum = 0;
int maxNum = 200;
condition_variable_any notEmpty;
condition_variable_any notFull;
int con_stat[10];

// ============================================
// FunctionName:	Put
// Parameter:		int id
// Description:		put one ticket into buffer, only when the buffer is not full
//					and the totalNum < maxNum
// ============================================
void Put(int id)
{
	while (1)					
	{
		lock_guard<mutex> locker(t_mutex);			// wait for the t_mutex
		while (isFull())							// the buffer is full, wait
		{
			cout << "Producer " << id << " wait for consume.Rest: " << buffer.size() << endl;
			notFull.wait(t_mutex);					// this step will temporarily release t_mutex
		}
		if (totalNum < maxNum)						// able to produce
		{
			// loop end, means we can put a ticket into buffer
			TicketInfo newTicket = GenerateTicket(id);
			buffer.push(newTicket);					// put the ticket info buffer
			totalNum++;
			notEmpty.notify_all();					// just unblock one process, not all
			// this_thread::sleep_for(chrono::milliseconds(100));
		}
		else
		{
			break;
		}
	}
	notEmpty.notify_all();		// the producer is terminated, wake up all waiting consumers
	cout << "Producer " << id << " terminated.\n";
}
// ==========================================
// FunctionName:	Get
// Parameter:		int id
// Description:		get a ticket from buffer and print it,
//					only when the buffer is not empty
// ==========================================
void Get(int id)
{
	while (1)
	{
		lock_guard<mutex> locker(t_mutex);
		while (isEmpty() && totalNum < maxNum)	// the buffer is empty & the producer is not terminated
		{
			cout << "Consumer " << id << " wait for produce.Rest: " << buffer.size() << endl;
			notEmpty.wait(t_mutex);
		}
		if (totalNum >= maxNum && isEmpty())	// produced over and buffer is empty
		{
			break;
		}
		TicketInfo getTicket;
		getTicket = buffer.front();
		buffer.pop();
		cout << "================ Process " << id << " get the ticket =================" << endl;
		cout << " ProducedTime:\t\t" << getTicket.producedTime << endl;
		cout << " ProducerId:\t\t" << getTicket.producerId << endl;
		cout << "===========================================================" << endl;
		con_stat[id]++;
		notFull.notify_one();
	}
	cout << "Consumer " << id << " terminated.\n";
}

bool isEmpty()
{
	return (buffer.size() == 0);				// buffer size equals to ZERO, empty
}

bool isFull()
{
	return (buffer.size() == maxSize);			// buffer size equals to maxSize, full
}

void con_statistics()
{
	cout << "================== con_statics ==================" << endl;
	cout << " Consumer 1:\t" << con_stat[1] << "(" << setprecision(3) << fixed << ((double)con_stat[1] / (double)maxNum) << ")" << endl;
	cout << " Consumer 2:\t" << con_stat[2] << "(" << setprecision(3) << fixed << ((double)con_stat[2] / (double)maxNum) << ")" << endl;
	cout << " Consumer 3:\t" << con_stat[3] << "(" << setprecision(3) << fixed << ((double)con_stat[3] / (double)maxNum) << ")" << endl;
	cout << "=================================================" << endl;
}

int main()
{
	maxNum = 1000;
	thread producer1(Put, 1);
	thread producer2(Put, 2);
	thread consumer1(Get, 1);
	thread consumer2(Get, 2);
	thread consumer3(Get, 3);
	// ============== Set Thread Priority (Linux) =====================
	// BlockDescription: set thread priority using pthread_setschedparam
	sched_param sch_params;
	sch_params.sched_priority = 20;						// priority of Consumer1
	if (pthread_setschedparam(consumer1.native_handle(), SCHED_RR, &sch_params))
	{
		cerr << "Unable to set thread scheduling of Consumer1\n";
	}
	sch_params.sched_priority = 40;						// priority of Consumer2
	if (pthread_setschedparam(consumer2.native_handle(), SCHED_RR, &sch_params))
	{
		cerr << "Unable to set thread scheduling of Consumer2\n";
	}
	sch_params.sched_priority = 30;						// priority of Consumer3
	if (pthread_setschedparam(consumer3.native_handle(), SCHED_RR, &sch_params))
	{
		cerr << "Unable to set thread scheduling of Consumer3\n";
	}
	// ============== Block ended ==================================
	producer1.detach();
	producer2.detach();
	consumer1.detach();
	consumer2.detach();
	consumer3.detach();

	this_thread::sleep_for(chrono::seconds(4));
	con_statistics();

	return 0;
}