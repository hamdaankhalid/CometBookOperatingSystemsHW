#include <utility>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <optional>
#include <fstream>
#include <random>
#include <sstream>
#include <chrono>
#include <thread>
#include <sys/wait.h>

int getRandomNumber(int min, int max) {
    std::random_device rd;  // Random device to seed the random number engine
    std::mt19937 gen(rd()); // Mersenne Twister 19937 engine (a widely used random number generator)

    // Create a uniform distribution for integers in the range [min, max]
    std::uniform_int_distribution<int> dist(min, max);

    // Generate a random number using the distribution and return it
    return dist(gen);
}

class MockProc {
	public:
		std::string procName;
		int delayTime;
		int totalTime;
		int interrupts;	
	
	std::vector<std::string> createInstructions() {
		std::vector<std::string> instructions;
		
		// cpu instructions
		for (int i = 0; i < totalTime; i++) {
			instructions.push_back("noOp");
		}
		
		// io instructions
		for (int i = 0; i < interrupts; i++) {
			int idx = getRandomNumber(0, totalTime);
			int ioTime = getRandomNumber(0, 200);
			std::ostringstream formatted;
			formatted << "io "	<< ioTime; 
			std::string result = formatted.str();
			instructions[idx] = result;
		}

		return instructions;
	}
};


std::optional<std::pair<int, std::string>> parseTill(const std::string& str, size_t currIdx, char till) {
	size_t found = str.find(till, currIdx);	
	if (found == std::string::npos) {
		return std::nullopt;
	}

	std::string collected = str.substr(currIdx, found - currIdx);

	return std::make_pair(found, std::move(collected));
}

std::optional<std::vector<MockProc> > parseMockProcs(std::string filename) {

	std::ifstream inputFile;
	
	inputFile.open(filename);

	if (!inputFile.is_open()) {
        std::cout << "Error opening the file." << std::endl;
        return std::nullopt;
    }

    std::string line;
	std::string fileContents;
	int lineCounter = 0;	
	std::vector<MockProc> procs;
    while (std::getline(inputFile, line)) {
		lineCounter++;
	
		MockProc proc;

		// use parseTill
		size_t currIdx = 0;
		std::optional<std::pair<int, std::string> > parseName = parseTill(line, currIdx, ',');
		if (!parseName.has_value()) {
			std::cerr << "Failed parsing for proc name on line " << lineCounter << std::endl;
			return std::nullopt;
		}

		proc.procName = parseName.value().second;

		currIdx = parseName.value().first+1;
		std::optional<std::pair<int, std::string>> parseArrival = parseTill(line, currIdx, ',');
		if (!parseArrival.has_value()) {
			std::cerr << "Failed parsing for delay time on line " << lineCounter << std::endl;
			return std::nullopt;
		}
	
		try {
			int delayTime = std::stoi(parseArrival.value().second);
			proc.delayTime = delayTime;
		} catch (const std::exception& e) {
			std::cerr << "Failed parsing delay time as int on line " << lineCounter << std::endl;
			return std::nullopt;
		}
		
		currIdx = parseArrival.value().first+1;
		std::optional<std::pair<int, std::string> > parseTotal = parseTill(line, currIdx, ',');
		if (!parseTotal.has_value()) {
			std::cerr << "Failed parsing for total time on line " << lineCounter << std::endl;
			return std::nullopt;
		}

		try {
			int totalTime = std::stoi(parseTotal.value().second);
			proc.totalTime = totalTime;
		} catch (const std::exception& e) {
			std::cerr << "Failed parsing for total time as int on line " << lineCounter << std::endl;
			return std::nullopt;
		}
		
		currIdx = parseTotal.value().first+1;
		std::string interruptStr = line.substr(currIdx);
		try {
			int interrupts = std::stoi(interruptStr);
			proc.interrupts = interrupts;
		} catch (const std::exception& e) {
			std::cerr << "Failed parsing for interrupts as int on line " << lineCounter << std::endl;
			return std::nullopt;
		}	

		procs.push_back(proc);
    }

    inputFile.close();
	return procs;
}

/*
 * based on the process list create a list of commands that the scheduler will interact with
 * commands include:
 * init procName: Simulates a user launching a process
 * cpuUse procName: Simulates the process using the cpu
 * ioInterrupt procName: Simulates the process getting IO interrupted
 * ioReturnFromInterrupt procName: Simulates the process returning from IO interrupt
 * end procName: Simulates the process finishing execution
 *
 * Maybe the scheduler listens to a user, user sends processes with details over time "instructions"
 * The scheduler will then send us stats about the simulations end results.
 * So think of something like the user submitting a process to run to the shell, and then the kernel
 * running it, for each cpu instruction we will do  no-op loop, for each io network, we will create
 * a callback background timer that wakes up and tells the schduler about io return.
 * */
void createSimulationStory(std::vector<MockProc> procs, int writePipe) {
	// Create an instruction list and send it over the buffer based on the procs arrival time
	std::unordered_map<std::string, std::vector<std::string> > procInstructions;

	for (size_t i = 0; i < procs.size(); i++) {
		MockProc p = procs[i];
		std::vector<std::string> instructions = p.createInstructions();
		
		// make a process instruction file and send the child process instruction for new proc to schedule and run
		std::ostringstream formatted;
		formatted << "proc_" << p.procName; 
		std::string filename = formatted.str();
		
		// create a file and add instructions as each line
		std::ofstream outputFile(filename);
		
		for (std::string instruction : instructions) {
			outputFile << instruction << std::endl;
		}
		
		outputFile.close();
		
		std::ostringstream initProcFormatted;
		initProcFormatted << "proc " << p.procName << " " << filename; 
		std::string initProcMessage = initProcFormatted.str();

		ssize_t bytesWritten = write(writePipe, initProcMessage.c_str(), initProcMessage.length());
        if (bytesWritten < 0) {
            std::cerr << "Simulator failed to write message to CFS Scheduler Child Proc" << std::endl;
        } else {
			std::cout << "Wrote message to child " << initProcMessage << std::endl;
		}

	    std::this_thread::sleep_for(std::chrono::seconds(p.delayTime));
	}

	std::string simulatorDoneMessage = "end";
	ssize_t bytesWritten = write(writePipe, simulatorDoneMessage.c_str(), simulatorDoneMessage.length());
	if (bytesWritten < 0) {
		std::cerr << "Simulator failed to notify CFS scheduler child of simulation end" << std::endl; 
	}
}

class CompletelyFairScheduler {
	private:
		int readPipe;
		char buffer[100];
	
	public:
	
		CompletelyFairScheduler(int readPipeDesc) : 
			readPipe(readPipeDesc) {}

		void runScheduler() {
			bool notFinished = true;
			while (notFinished) {
				ssize_t bytesRead = read(this->readPipe, this->buffer, sizeof(this->buffer) - 1);
				if (bytesRead > 0) {
					std::string recvdSignal(this->buffer, bytesRead);

					std::cout << "Child received: " << recvdSignal << std::endl;

					if (recvdSignal.compare("end")) {
						printf("CFS recvd end of proc enqueueing signal\n");
						notFinished = false;
					}
				} else {
					std::cerr << "Child failed to read from the pipe." << std::endl;
				}
			}

			// run a cleanup such as waiting on CFS to finish existing jobs
		}
};

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
		return 1;
	}

	int pipefd[2]; // IPC PIPE for simulator and actual cfs

	// Create the pipe
    if (pipe(pipefd) == -1) {
        std::cerr << "IPC Pipe creation failed." << std::endl;
        return 1;
    }

    pid_t pid = fork();

	if (pid == -1) {
		std::cerr << "Fork failed" << std::endl;
	} else if (pid == 0) {
		// Child process (CFS Scheduler long running process)
	
		close(pipefd[1]); // close the write end

		// run the CFS scheduler listening to the read end for commands
		std::cout << "CFS Scheduler Started" << std::endl;
		
		CompletelyFairScheduler cfs(pipefd[0]);
		cfs.runScheduler();

		close(pipefd[0]); // close the read end at the end
	} else {
		// Parent process (Simulator)
		close(pipefd[0]); // close the read end

		// run the simulator explicitly using the write end	
		// parent process the simulator that creates a simulation
		// and sends events to the scheduler		
		std::optional<std::vector<MockProc>> procs = parseMockProcs(argv[1]);
		if (!procs.has_value()) {
			std::cerr << "failed to process simulation file" << std::endl;
		} else {
			// process and send proc simulations
			createSimulationStory(procs.value(), pipefd[1]);
		}
		
		// close our write end at the end
		close(pipefd[1]);

		int status;
        if (waitpid(pid, &status, 0) == -1) {
            std::cerr << "Error waiting for the child process" << std::endl;
            return 1;
        }

        std::cout << "Child process (CFS Scheduler) has terminated. Simulator Exiting." << std::endl;
	}

	return 0;
}
