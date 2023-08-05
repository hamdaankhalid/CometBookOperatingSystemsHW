#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

int getRandomNumber(int min, int max) {
  std::random_device rd;  // Random device to seed the random number engine
  std::mt19937 gen(rd()); // Mersenne Twister 19937 engine (a widely used random
                          // number generator)

  std::uniform_int_distribution<int> dist(min, max);

  return dist(gen);
}

// ********* File Handling with RAII ****

class FileReader {
private:
  std::ifstream file;

public:
  // Constructor: Open the file for reading
  FileReader(const std::string &filename) : file(filename) {
    if (!file) {
      throw std::runtime_error("Error opening the file.");
    }
  };

  // Destructor: Close the file when the object goes out of scope
  ~FileReader() {
    if (file.is_open()) {
      file.close();
    }
  }

  // read file contents line separated as vector
  std::vector<std::string> readStrings() {
    std::vector<std::string> strings;
    std::string line;

    while (std::getline(file, line)) {
      strings.push_back(line);
    }

    return strings;
  }
};

class FileWriter {
private:
  std::ofstream file;

public:
  // Constructor: Open the file for writing
  FileWriter(const std::string &filename) : file(filename) {
    if (!file) {
      throw std::runtime_error("Error opening the file.");
    }
  };

  // Destructor: Close the file when the object goes out of scope
  ~FileWriter() {
    if (file.is_open()) {
      file.close();
    }
  }

  // read file contents line separated as vector
  void writeVector(const std::vector<std::string> &instructions) {
    for (std::string instruction : instructions) {
      file << instruction << std::endl;
    }
  }
};

// ************ Simulator *********************

class MockProc {
public:
  std::string procName;
  int delayTime;
  int totalTime;
  int interrupts;

  std::vector<std::string> createInstructions() const {
    std::vector<std::string> instructions;

    // cpu instructions
    for (int i = 0; i < totalTime; i++) {
      instructions.push_back("cpu");
    }

    // io instructions
    for (int i = 0; i < interrupts; i++) {
      int idx = getRandomNumber(0, totalTime);
      int ioTime = getRandomNumber(0, 10);
      std::ostringstream formatted;
      formatted << "io " << ioTime;
      std::string result = formatted.str();
      instructions[idx] = result;
    }

    return instructions;
  }
};

std::optional<std::pair<int, std::string>>
parseTill(const std::string &str, size_t currIdx, char till) {
  size_t found = str.find(till, currIdx);
  if (found == std::string::npos) {
    return std::nullopt;
  }

  std::string collected = str.substr(currIdx, found - currIdx);

  return std::make_pair(static_cast<int>(found), std::move(collected));
}

std::optional<std::vector<MockProc>> parseMockProcs(std::string filename) {

  FileReader inputFile(filename);

  std::vector<std::string> lines = inputFile.readStrings();

  std::vector<MockProc> procs;
  for (size_t lineCounter = 0; lineCounter < lines.size(); lineCounter++) {

    std::string line = lines[lineCounter];

    MockProc proc;

    // use parseTill
    size_t currIdx = 0;
    std::optional<std::pair<int, std::string>> parseName =
        parseTill(line, currIdx, ',');
    if (!parseName.has_value()) {
      std::cerr << "Failed parsing for proc name on line " << lineCounter
                << std::endl;
      return std::nullopt;
    }

    proc.procName = parseName.value().second;

    currIdx = parseName.value().first + 1;
    std::optional<std::pair<int, std::string>> parseArrival =
        parseTill(line, currIdx, ',');
    if (!parseArrival.has_value()) {
      std::cerr << "Failed parsing for delay time on line " << lineCounter
                << std::endl;
      return std::nullopt;
    }

    try {
      int delayTime = std::stoi(parseArrival.value().second);
      proc.delayTime = delayTime;
    } catch (const std::exception &e) {
      std::cerr << "Failed parsing delay time as int on line " << lineCounter
                << std::endl;
      return std::nullopt;
    }

    currIdx = parseArrival.value().first + 1;
    std::optional<std::pair<int, std::string>> parseTotal =
        parseTill(line, currIdx, ',');
    if (!parseTotal.has_value()) {
      std::cerr << "Failed parsing for total time on line " << lineCounter
                << std::endl;
      return std::nullopt;
    }

    try {
      int totalTime = std::stoi(parseTotal.value().second);
      proc.totalTime = totalTime;
    } catch (const std::exception &e) {
      std::cerr << "Failed parsing for total time as int on line "
                << lineCounter << std::endl;
      return std::nullopt;
    }

    currIdx = parseTotal.value().first + 1;
    std::string interruptStr = line.substr(currIdx);
    try {
      int interrupts = std::stoi(interruptStr);
      proc.interrupts = interrupts;
    } catch (const std::exception &e) {
      std::cerr << "Failed parsing for interrupts as int on line "
                << lineCounter << std::endl;
      return std::nullopt;
    }

    procs.push_back(proc);
  }

  return procs;
}

/*
 * based on the mock process list create a list of commands that the scheduler
 * will interact with commands include: init procName: Simulates a user
 * launching a process cpuUse procName: Simulates the process using the cpu
 * ioInterrupt procName: Simulates the process getting IO interrupted
 * ioReturnFromInterrupt procName: Simulates the process returning from IO
 * interrupt end procName: Simulates the process finishing execution
 *
 * the scheduler listens to a user, user sends processes with details over
 * time "instructions" The scheduler will then send us stats about the
 * simulations end results. So think of something like the user submitting a
 * process to run to the shell, and then the kernel running it, for each cpu
 * instruction we will do  fake instruction, for each io network, we will create a
 * callback background timer that wakes up and tells the scheduler about io
 * return.
 * */
void createSimulationStory(std::vector<MockProc> procs, int writePipe) {
  // Create an instruction list and send it over the buffer based on the procs
  // arrival time
  std::unordered_map<std::string, std::vector<std::string>> procInstructions;

  for (size_t i = 0; i < procs.size(); i++) {
    MockProc p = procs[i];
    std::vector<std::string> instructions = p.createInstructions();

    // make a process instruction file and send the child process instruction
    // for new proc to schedule and run
    std::ostringstream formatted;
    formatted << "proc_" << p.procName;
    std::string filename = formatted.str();

    // create a file and add instructions as each line
    FileWriter outputFile(filename);
    outputFile.writeVector(instructions);

    std::ostringstream initProcFormatted;
    initProcFormatted << "proc " << p.procName << " " << filename;
    std::string initProcMessage = initProcFormatted.str();

    std::cout << "[SIMULATOR] perform a delay on delivery " << p.delayTime
              << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(p.delayTime));

    ssize_t bytesWritten =
        write(writePipe, initProcMessage.c_str(), initProcMessage.length());
    if (bytesWritten < 0) {
      std::cerr
          << "Simulator failed to write message to CFS Scheduler Child Proc"
          << std::endl;
    } else {
      std::cout << "[SIMULATOR] Wrote message to child " << initProcMessage
                << std::endl;
    }
  }

  std::string simulatorDoneMessage = "end";
  ssize_t bytesWritten = write(writePipe, simulatorDoneMessage.c_str(),
                               simulatorDoneMessage.length());
  if (bytesWritten < 0) {
    std::cerr
        << "Simulator failed to notify CFS scheduler child of simulation end"
        << std::endl;
  }
}

// ****************** CFS Scheduler ***********************

struct ProcRunResult {
  int ranFor;
  std::optional<int> ioEvent;

  // overload for std::cout
  friend std::ostream &operator<<(std::ostream &os, const ProcRunResult &res) {
    int ioEvent = res.ioEvent.has_value() ? res.ioEvent.value() : -1;
    os << "ranFor: " << res.ranFor << ", ioEvent: " << ioEvent;
    return os;
  }
};

/*
 * CFS scheduler object will store these schduler objects and track it to
 * completion
 * */
class SchedulerProccess {
private:
  std::string procName;
  double rawRuntime = 0.0;
  double vruntime = 0.0;
  int weight = 1024; // default nice set to 0
  std::vector<std::string> instructions;
  int instructionCounter = 0;

  // shared readonly memory that is only initialized once and shared by all
  // objects
  static constexpr int weights[40] = {
      /* -20 */ 88761, 71755, 56483, 46273, 36291,
      /* -15 */ 29154, 23254, 18705, 14949, 11916,
      /* -10 */ 9548,  7620,  6100,  4904,  3906,
      /* -5 */ 3121,   2501,  1991,  1586,  1277,
      /* 0 */ 1024,    820,   655,   526,   423,
      /* 5 */ 335,     272,   215,   172,   137,
      /* 10 */ 110,    87,    70,    56,    45,
      /* 15 */ 36,     29,    23,    18,    15,
  };

public:
  SchedulerProccess(const std::string &filename, const std::string &procName) {
    // read instructions into vector from file
    FileReader filereader(filename);
    this->instructions = filereader.readStrings();
    this->procName = procName;
  }

  // Delete default constructor so we are always ever making this class with the
  // instructions loaded
  SchedulerProccess() = delete;

  std::string getProcName() { return this->procName; }

  void setNiceness(int howNice) {
    // Range check just for the f of it
    if (howNice < -20 || howNice > 20) {
      std::cerr
          << "Nice value for setNiceness must be between -20, and 20 inclusive."
          << std::endl;
      return;
    }

    // transform -20 - 20 inlusive range into 0 - 40, we can do that if we just
    // add 20 to any incoming niceness
    int scaledNice = howNice + 20;
    this->weight = SchedulerProccess::weights[scaledNice];
  }

  int getWeight() const { return this->weight; }

  double getVruntime() const { return this->vruntime; }

  // weightSum is the sum of all the processes weights
  double timeSlice(int schedLatency, int minGranularity, int weightSum) const {
    double tSl = (this->weight / weightSum) * schedLatency;
    return tSl < minGranularity ? minGranularity : tSl;
  }

  void incrVruntime(double runTimeIncr) {
    this->rawRuntime += runTimeIncr;
    int weight0 = SchedulerProccess::weights[20];
    this->vruntime =
        this->vruntime + (weight0 / this->weight) * this->rawRuntime;
  }

  std::optional<ProcRunResult> runWithCap(int allocatedTimeSlice) {
    int timeSlCounter = 0;
    while (timeSlCounter < allocatedTimeSlice &&
           this->instructionCounter <
               static_cast<int>(this->instructions.size())) {
      std::string instruction = this->instructions[this->instructionCounter];

      if (instruction == "cpu") {
        // exactly this is a no op there's gonna be nothing here
        // but we are still calling print to do a lil log that shows how the
        // scheduler is working
        std::cout << "[HARDWARE] CPU Instruction for " << this->procName
                  << " Program Instruction Counter " << this->instructionCounter << std::endl;
      } else {
        // this is an io event to be simulated
        // update the rawRuntime and do an early return
        // get the the numero that this should be an io event for
        int spaceIdx = instruction.find(" ");
        std::string eventTimeUnfmt =
            instruction.substr(spaceIdx + 1, instruction.size() - spaceIdx);
        int ioTime = std::stoi(eventTimeUnfmt);

        std::cout << "[HARDWARE] IO event occurred for " << this->procName
                  << " for " << ioTime << " time " << std::endl;
        this->incrVruntime(timeSlCounter); // avoid 0 indexing?
        this->instructionCounter++;
        return ProcRunResult{timeSlCounter, ioTime};
      }

      this->instructionCounter++;
      timeSlCounter++;
    }

    if (this->instructionCounter >=
        static_cast<int>(this->instructions.size())) {
      return std::nullopt;
    }

    this->incrVruntime(timeSlCounter);

    return ProcRunResult{allocatedTimeSlice, std::nullopt};
  }
};

/*
 * Used to order schduler process in red black trees by their vruntime, and
 * */
struct SchedulerProccessComparator {
  bool operator()(const std::shared_ptr<SchedulerProccess> obj1,
                  const std::shared_ptr<SchedulerProccess> obj2) const {
    return obj1->getVruntime() < obj2->getVruntime();
  }
};

/*
 * Linux CompletelyFairScheduler implementation that runs a schduler listening
 * on pipe if it recvs data on the pipe to enqueue a process it enqueues it
 * internally. On a separate thread we handle the running of the enqueued
 * processes. When a process is enqueued we load its instructions into memory.
 * When it is a processes turn if we get a cpu we just do a fake instruction,
 * if it is an IO event we put the process in a vector and run a timer
 * to evict it when it the timer goes off.
 * */
class CompletelyFairScheduler {
private:
  // IPC Communication related members
  int readPipe;
  char buffer[100];

  // Scheduler Algorithm members
  double schedLatency = 45;
  double minGranularity = 30;
  std::map<std::shared_ptr<SchedulerProccess>, bool,
           SchedulerProccessComparator>
      runningProcs;
  std::unordered_map<std::string, std::shared_ptr<SchedulerProccess>> inIoProcs;

  // Concurrency members
  std::thread schedulerThread;
  bool isThreadRunning = false;
  std::mutex procRbtMu;
  std::mutex ioProcsMu;

public:
  CompletelyFairScheduler(int readPipeDesc) : readPipe(readPipeDesc) {}

  void startScheduler() {
    this->schedulerThread = std::thread([this]() {
      this->isThreadRunning = true;

      int idleFor = 0;
      while (true) {
        if (this->runningProcs.empty()) {
          if (idleFor > 60) {
            std::cout
                << "[OS] Scheduler idled for 60 seconds before deciding to "
                   "kill itself."
                << std::endl;
            return;
          }

          std::this_thread::sleep_for(std::chrono::seconds(1));

          std::cout << "[OS] Scheduler idling " << idleFor << " 'th' time."
                    << std::endl;

          idleFor++;
          continue;
        }

        // reset our idleFor counter
        idleFor = 0;

        std::shared_ptr<SchedulerProccess> procToRun = nullptr;

        int weightsSum = 0;

        // create a block to force lock to be released via RAII
        {
          procToRun = this->runningProcs.begin()->first;
          std::lock_guard<std::mutex> lock(this->procRbtMu);
		  // poppping off the least vRuntime proccess
          this->runningProcs.erase(procToRun);
          for (auto &proc : this->runningProcs) {
            weightsSum += proc.first->getWeight();
          }
          weightsSum += procToRun->getWeight();
        }

        int runFor = procToRun->timeSlice(this->schedLatency,
                                          this->minGranularity, weightsSum);

        std::cout << "[OS] Choosing to run " << procToRun->getProcName()
                  << " Proc for " << runFor << std::endl;

        std::optional<ProcRunResult> result = procToRun->runWithCap(runFor);

        if (!result.has_value()) {
          std::cout << "[OS] Proc " << procToRun->getProcName()
                    << " reported completion to scheduler." << std::endl;
          continue;
        }

        int ranFor = result.value().ranFor;
        procToRun->incrVruntime(ranFor);

        std::optional<int> ioEvent = result.value().ioEvent;

        if (ioEvent.has_value()) {
          std::lock_guard<std::mutex> lock(this->ioProcsMu);
          this->inIoProcs[procToRun->getProcName()] = procToRun;

          std::thread bgTimer = std::thread([this, procToRun, ioEvent]() {
            std::this_thread::sleep_for(std::chrono::seconds(ioEvent.value()));
            std::lock_guard<std::mutex> ioLock(this->ioProcsMu);
            this->inIoProcs.erase(procToRun->getProcName());
            std::lock_guard<std::mutex> procLock(this->procRbtMu);
            this->runningProcs[procToRun] = true;
          });

          bgTimer.detach();
        } else {
          std::lock_guard<std::mutex> lock(this->procRbtMu);
          this->runningProcs[procToRun] = true;
        }
      }

      this->isThreadRunning = false;
    });
  }

  void listen() {
    bool notFinished = true;
    while (notFinished) {
      ssize_t bytesRead =
          read(this->readPipe, this->buffer, sizeof(this->buffer) - 1);
      if (bytesRead > 0) {
        std::string recvdSignal(this->buffer, bytesRead);

        if (recvdSignal == "end") {
          printf("[OS] Scheduler CFS recvd end of proc enqueueing signal\n");
          notFinished = false;
        } else {
          // read the proc file and load instructions into memory for the
          std::string procName = recvdSignal.substr(5, 2);
          std::string filename = recvdSignal.substr(8, recvdSignal.size() - 8);

          // use blocks to release locks via RAII
          {
            std::lock_guard<std::mutex> lock(this->procRbtMu);
            std::shared_ptr<SchedulerProccess> proc =
                std::make_shared<SchedulerProccess>(filename, procName);

            // For every 3rd process let's change its niceness :)
            if (std::stoi(procName.substr(1, 1)) % 3 == 0) {
              int howNice = getRandomNumber(-20, 20);
              proc->setNiceness(howNice);
              std::cout << "[SIMULATOR] set the niceness of " << procName
                        << " to " << howNice << std::endl;
            }

            this->runningProcs[proc] = true;
            std::cout << "[OS] Creating Proc Entry for " << procName
                      << " from filename " << filename << std::endl;
          }
        }

      } else {
        std::cerr << "[OS] Child failed to read from the pipe." << std::endl;
      }
    }

    // run a cleanup such as waiting on CFS to finish existing jobs
    if (this->isThreadRunning) {
      std::cout << "[OS] User Simulator sent end of process requests signal. "
                   "Waiting for "
                   "background process to cleanup."
                << std::endl;
      this->schedulerThread.join();
    }
  }
};

// ********* Driver **********************

// Kick off a child and parent proc, read simulation instructions from user
// input story send instrcutions to child as if the parent is a user and the
// child is the kernel
int main(int argc, char *argv[]) {
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
    std::cout << "[OS] CFS Scheduler Started" << std::endl;

    CompletelyFairScheduler cfs(pipefd[0]);
    // non blocking
    cfs.startScheduler();
    // blocking
    cfs.listen();

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

    std::cout << "[SIMULATOR] Child process (CFS Scheduler) has terminated. "
                 "Simulator Exiting."
              << std::endl;
  }

  return 0;
}
