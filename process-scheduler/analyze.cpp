#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ########################### 24 bit BMP Image Implementation ################################
// Use packing for forcing no padding on structs for BMP file format compliance

struct __attribute__((packed)) BmpHeader {
  char signature[2] = {'B', 'M'};
  std::uint32_t fileSize;
  std::uint32_t reserved = 0; // unused
  std::uint32_t dataOffset =
      54; // Update this value to reflect the correct offset.
};

struct __attribute__((packed)) BmpInfoHeader {
  std::uint32_t size = 40;
  std::int32_t width;  // Use int32_t to handle negative width (if needed).
  std::int32_t height; // Use int32_t to handle negative height (if needed).
  std::uint16_t planes = 1;
  std::uint16_t bitsPerPixel = 24; // RGB pixel-based data
  std::uint32_t compression = 0;
  std::uint32_t imageSize = 0;
  std::int32_t horizontalResolution; // Use int32_t to handle negative
                                     // resolution (if needed).
  std::int32_t verticalResolution; // Use int32_t to handle negative resolution
                                   // (if needed).
  std::uint32_t colorsUsed = 0;
  std::uint32_t importantColors = 0;
};

struct __attribute__((packed)) Pixel {
  std::uint8_t blue;
  std::uint8_t green;
  std::uint8_t red;
};

class BmpFile {
private:
  BmpHeader header;
  BmpInfoHeader infoHeader;
  std::vector<Pixel> bmp;

public:
  BmpFile(std::vector<Pixel> bitmap, std::int32_t width, std::int32_t height)
      : bmp(std::move(bitmap)) {
    infoHeader.width = width;
    infoHeader.height = height;

    // Calculate filesize
    std::size_t sizeOfBmpArr = sizeof(Pixel) * bmp.size();
    header.fileSize = sizeof(BmpHeader) + sizeof(BmpInfoHeader) + sizeOfBmpArr;
  }

  void serializeToFile(const std::string &filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
      std::cerr << "Error opening file for writing." << std::endl;
      return;
    }

    outFile.write(reinterpret_cast<const char *>(&this->header),
                  sizeof(BmpHeader));
    outFile.write(reinterpret_cast<const char *>(&this->infoHeader),
                  sizeof(BmpInfoHeader));
    outFile.write(reinterpret_cast<const char *>(this->bmp.data()),
                  this->bmp.size() * sizeof(Pixel));

    outFile.close();
  }
};

// ################### Utils #############################

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

uint8_t getRandomNumber(uint8_t min, uint8_t max) {
  std::random_device rd;  // Random device to seed the random number engine
  std::mt19937 gen(rd()); // Mersenne Twister 19937 engine (a widely used random
                          // number generator)

  std::uniform_int_distribution<uint8_t> dist(min, max);

  return dist(gen);
}

void printColor(int r, int g, int b, const std::string &text) {
  std::cout << "\033[38;2;" << r << ";" << g << ";" << b << "m" << text
            << "\033[0m";
}

// ######################## Log Processing and Utils #######################

struct Event {
  std::string
      eventType; // I honestly dont think this not being an enum is an issue
  std::string procName;
};

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

  // Graph creation by making a matrix for an image, and creating a BMP image on disk
  unsigned int rows = 600;
  unsigned int cols = 2000;

  Pixel white;
  white.red = 255;
  white.green = 255;
  white.blue = 255;

  std::vector<std::vector<Pixel>> image(rows, std::vector<Pixel>(cols, white));

  std::unordered_map<std::string, Pixel> procColors;

  // size of each block the log represents
  int heightBlock = 80;
  int widthBlock = 3;

  std::pair<int, int> lastLeftTop =
      std::make_pair(200, 20); // initial padding showing y, x

  for (const Event &ev : events) {
    // based on event type and proc name make a rectangular block of pixels and send that
    std::unordered_map<std::string, Pixel>::iterator it =
        procColors.find(ev.procName);

    if (it == procColors.end()) {
      Pixel rndm;
      rndm.red = getRandomNumber(0, 255);
      rndm.green = getRandomNumber(0, 255);
      rndm.blue = getRandomNumber(0, 255);

      procColors[ev.procName] = rndm;
    }
    Pixel colorForProc = procColors[ev.procName];

    // draw the block
    for (int i = lastLeftTop.first; i < lastLeftTop.first + heightBlock; i++) {
      for (int j = lastLeftTop.second; j < lastLeftTop.second + widthBlock;
           j++) {
        image[i][j] = colorForProc;
      }
    }

    lastLeftTop.second += widthBlock;
  }

  // flatten out image matrix
  std::vector<Pixel> flatBmp;
  for (std::vector<Pixel> row : image) {
    for (Pixel px : row) {
      flatBmp.push_back(px);
    }
  }

  // now that we have the image we can try to create bmp file
  BmpFile visualFile(flatBmp, cols, rows);

  visualFile.serializeToFile("scheduler_visual.bmp");

  std::cout << " ------------- Finished log processing --------------"
            << std::endl;

  std::cout << "************* Scheduler Execution Report *******************"
            << std::endl;

  std::cout << "Graph File: scheduler_visual.bmp" << std::endl;

  std::cout << "########## Color Legend ##########" << std::endl;

  std::unordered_map<std::string, Pixel>::iterator it;

  // Iterate over the map and print values
  for (it = procColors.begin(); it != procColors.end(); ++it) {
    std::cout << "PROC: " << it->first << " ";
    printColor(static_cast<std::uint8_t>(it->second.red),
               static_cast<std::uint8_t>(it->second.green),
               static_cast<std::uint8_t>(it->second.blue), "COLOR");
    std::cout << "\n";
  }

  std::cout << "#########################" << std::endl;
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
