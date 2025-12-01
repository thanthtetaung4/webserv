# include <fstream>
# include <iostream>
# include <string>

int main() {
    std::cout << "====================== Reading From File ../test.txt ======================" << std::endl;
    std::ifstream inFile("/home/taung/webserv/../test.txt");
    std::string line;
    std::string path = "/home/taung/webserv/../test.txt";

    if (inFile.is_open()) {
        if (path.find("..") == std::string::npos) {
            while (std::getline(inFile, line)) {
                std::cout << line << std::endl;
              }
        } else {
            std::cout << "bad path" << std::endl;
        }
    }
}
