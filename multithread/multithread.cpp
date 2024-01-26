#include <iostream>
#include <queue>
#include <ctime>
#include <thread>
#include <mutex>
#include <string>
#include <chrono>
#include <signal.h>

using namespace std;

volatile sig_atomic_t ctrl_c_pressed = 0;

int readVariable(string variableType) {
	string variableName = (variableType == "devices") ? "приборов" : "групп";
	if (variableType == "capacity") {
		cout << "Введите ёмкость: ";
	}
	else {
		cout << "Введите количество " + variableName + ": ";
	}
	int result;
	cin >> result;
	if ((variableType == "devices" && result <= 2) || (variableType == "groups" && result <= 3)) {
		cout << "Нужно больше " + variableName + "!\n";
		return readVariable(variableType);
	}
	return result;
}

void requestsProcessor(priority_queue<int, vector<int>>& requests, mutex& globalMutex, int groupNumber, int deviceNumber) {
	do {
		if (requests.empty() && ctrl_c_pressed) {
			break;
		}
		this_thread::sleep_for(chrono::milliseconds(200));
		//Строка выше необходима для корректного вывода (иначе выводы потоков накладываются друг на друга)
		globalMutex.lock();
		if (requests.empty()) {
			globalMutex.unlock();
		}
		else {
			requests.pop();
			double sleepTime = rand() % 10000;
			cout << "Прибор номер " + to_string(deviceNumber) + " группы номер " + to_string(groupNumber) + " достал и обработал заявку, теперь можно и поспать (" + to_string(sleepTime / 1000) + " секунд).\n";
			globalMutex.unlock();
			this_thread::sleep_for(chrono::milliseconds(int(sleepTime)));
			cout << "Прибор номер " + to_string(deviceNumber) + " группы номер " + to_string(groupNumber) + " проснулся!\n";
		}

	} while (true);
	cout << "Прибор номер " + to_string(deviceNumber) + " группы номер " + to_string(groupNumber) + " завершил свою работу навсегда.\n";
}

void requestsGenerator(priority_queue<int, vector<int>>& requests, mutex& globalMutex, int groupsAmount, int storageCapacity) {
	while (!ctrl_c_pressed) {
		this_thread::sleep_for(chrono::milliseconds(rand() % 1000));
		globalMutex.lock();
		if (requests.size() < storageCapacity) {
			requests.push(rand() % 2);
			cout << "Добавлена заявка!\nОбщее число заявок: " + to_string(requests.size()) << endl;
		}
		else {
			cout << "Места больше нет, ждём.\n";
		}
		globalMutex.unlock();
	}
	cout << "Вы завершили генерацию заявок.\n";
}

void signal_handler(int signal_number) {
	if (signal_number == SIGINT) {
		ctrl_c_pressed = 1;
	}
}

int main()
{
	signal(SIGINT, signal_handler);
	srand(time(0));
	setlocale(LC_ALL, "Russian");
	int storageCapacity, deviceAmount, groupsAmount;
	priority_queue<int, vector<int>> requests;
	vector<thread> allThreads;
	mutex globalMutex;
	storageCapacity = readVariable("capacity");
	deviceAmount = readVariable("devices");
	groupsAmount = readVariable("groups");
	thread generatorThread(requestsGenerator, ref(requests), ref(globalMutex), groupsAmount, storageCapacity);
	for (int i = 0; i < groupsAmount; ++i) {
		for (int j = 0; j < deviceAmount; ++j) {
			allThreads.push_back(thread(requestsProcessor, ref(requests), ref(globalMutex), i, j));
		}
	}
	for (int i = 0; i < groupsAmount; ++i) {
		for (int j = 0; j < deviceAmount; ++j) {
			allThreads[i * deviceAmount + j].join();
		}
	}
	generatorThread.join();
	cout << "Программа завершена!";
}