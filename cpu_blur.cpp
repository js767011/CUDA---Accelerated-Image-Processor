#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>

//use a flat 1D vector to store the 2D image data
  //MUST be 1D since CUDA global memory is strictly 1D, anything else wont port easily
struct Image {
  int width; //store width of image
  int height; //store height of image
  int max_color; //specific to PPM format
  //in c++, a char is 1 byte (8bits), making it unsigned restricts the value to range form 0-255
    //this works perfectly with standard rgb pixels, which are made of 3 values ranging from 0-255
  //need a vector since trying to just create a standard raw array would immediately trigger a stack overflow
  std::vector<unsigned char> data;
};

int main() {
  std::string inputFile = "input.ppm";
  std::string outputFile = "output_blurred.ppm";

  //ios::binary -> tells OS to read the file as raw bytes
  //ios::ate -> stands for "at the end"; basically tells OS to open file and jump to the last byte of the file
  std::ifstream fileInfo(inputFile, std::ios::binary | std::ios::ate);
  if (fileInfo) {
    //std::streamsize -> c++ type designed to be large enough to hold huge file sizes (usually 64 bit int)
    //tellg() -> "tell get"; gets the byte position of the reading cursor, this trick tells the size of the file
    std::streamsize size = fileInfo.tellg();
    //1024 bytes in kB, 1024 kB's in MB. Use .0 to force floating-pt division to avoid rounding down the MB
    std::cout << "Input file size: " << size / (1024.0 * 1024.0) << "MB (" << size << " bytes)" << std::endl;
    fileInfo.close();
  } else {
    std::cerr << "Warning: Could not open " << inputFile << " to determine file size." << std::endl;
  }


  std::cout << "Reading image..." << std::endl;
  //Image img = readPPM(inputFile);

  return 0;
}