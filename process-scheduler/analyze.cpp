#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

struct Event {
  std::string
      eventType; // I honestly dont think this not being an enum is an issue
  std::string procName;
};

bool subStringSearch(const std::string &searchOn, const std::string &find) {
  if (find.size() > searchOn.size()) {
    return false;
  }

  // otherwise start checking if a substring matfches as we iterate
  int idx = 0;

  while (static_cast<unsigned long>(idx) < (searchOn.size() - find.size())) {
    std::string check = searchOn.substr(idx, find.size());
    if (check == find) {
      return true;
    }
    idx++;
  }

  return false;
}

//  a procname is always a "p" followed by an integer
//  given a line identify the proc name and return it
std::string getProcName(const std::string &line) {
  // The idea of using regex has come rather late to me i could have
  // saved myself so much time of parsign if I just used regex here instead
  std::regex procNameRegex("(p)(\\d+)");
  std::smatch matches;
  if (std::regex_search(line, matches, procNameRegex)) {
    return matches[0];
  } else {
    return "";
  }
}

/*
 * Given a file go through the log contents and create
 * a visual report of what proccesses were run over the lif when
 * */
void printReport(const std::string &logfilename) {
  // For each log that is either OS or HHardware create an event
  // each event should show s if a process was recieved by the OS
  // if a proccess was run on hardware
  std::vector<Event> events;

  // since this is a short lived script process I am ignorinng RAII
  std::ifstream file(logfilename);

  if (!file) {
    throw std::runtime_error(
        "Error opening the logfile for report generation.");
  }

  std::vector<std::string> fileContents;
  std::string fline;

  while (std::getline(file, fline)) {
    fileContents.push_back(fline);
  }

  file.close();

  for (const std::string &line : fileContents) {
    // based on the line we need to make a type of an event
    std::cout << "processing ...: " << line << std::endl;

    int indexOfClosingBracket = line.find("]");
    std::string logType = line.substr(1, indexOfClosingBracket - 1);

    std::string procName = getProcName(line);

    Event event;
    event.eventType = "";
    event.procName = procName;

    // now if it is an OS log or hardware log we are going to make a different
    // event
    if (logType == "OS") {
      if (subStringSearch(line, "Creating")) {
        event.eventType = "Created";
      } else if (subStringSearch(line, "completion")) {
        event.eventType = "Finished";
      } else if (subStringSearch(line, "idling")) {
        event.eventType = "Idling";
        event.procName = "Scheduler";
      }
    } else if (logType == "HARDWARE") {
      if (subStringSearch(line, "CPU")) {
        event.eventType = "CPU";
      } else if (subStringSearch(line, "IO")) {
        event.eventType = "IO";
      }
    }

    // monkey brain
    if (event.eventType != "" && event.procName != "") {
      events.push_back(event);
    }
  }

  std::cout << " ------------- Finsihed log processing --------------"
            << std::endl;

  std::cout << "************* Scheduler Execution Report *******************"
            << std::endl;

  for (const Event &ev : events) {
    std::cout << "{ " << ev.eventType << ", " << ev.procName << " }"
              << std::endl;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <filename>" << std::endl;
    return 1;
  }

  std::string logfile = argv[1];

  printReport(logfile);

  return 0;
}
