#include <utility>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <optional>
#include <fstream>

class MockProc {
	public:
		std::string procName;
		int arrivalTime;
		int totalTime;
		int interrupts;	
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
		std::optional<std::pair<int, std::string> > parseArrival = parseTill(line, currIdx, ',');
		if (!parseArrival.has_value()) {
			std::cerr << "Failed parsing for arrival time on line " << lineCounter << std::endl;
			return std::nullopt;
		}
	
		try {
			int arrivalTime = std::stoi(parseArrival.value().second);
			proc.arrivalTime = arrivalTime;
		} catch (const std::exception& e) {
			std::cerr << "Failed parsing arrival time as int on line " << lineCounter << std::endl;
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

void createSimulationStory(std::vector<MockProc> procs) {
	for (size_t i = 0; i < procs.size(); i++) {
		MockProc p = procs[i];

		std::cout << p.procName << " " << p.arrivalTime << " " << p.totalTime << " " << p.interrupts << std::endl;
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
		return 1;
	}

	int pipefd[2]; // IPC PIPE for simulator and actual cfs
    char buffer[100];

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
			createSimulationStory(procs.value());
		}
	
		// write an exit command to scheduler
	
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
