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

Image readPPM(const std::string& filename) {
  //need the std::ios::binary -> tells OS not to mess with line-ending bytes like \n or \r 
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Error: Could not open " << filename << std::endl;
    exit(1);
  }

  //First line of the PPM file should have P6 indicating the PPM with raw binary info
    //extract out the header and validate the file
  std::string magic;
  file >> magic;
  if (magic != "P6") {
    std::cerr << "Error: Not a valid P6 PPM file." << std::endl;
    exit(1);
  }

  Image img;
  //extract out the width, height, and max color of the PPM
  file >> img.width >> img.height >> img.max_color;

  //skip the whitespace/newline characters after the header
  file.ignore(256, '\n');

  //allocate teh space for the vector via resize() : width * height * 3 (for the Red, Green, and Blue channels)
  img.data.resize(img.width * img.height * 3);

  //read the block of raw binary pixel data directly into the vector
    //img.data -> accesses the struct vector, img.data.data -> strips away the c++ safety wrappers and returns a raw mem ptr to the first byte of the array (an unsigned char*)
    //the file.read() needs a mem address to dump the bytes (MUST be a char*), and the number of bytes to dump (via img.data.size())
    //since the vector is of type unsigned char, cant pass in an unsigned char* -> need to use reinterpret_cast<char*>
      //reinterpret_cast<char*> -> DOES NOT CHANGE ANY BITS, just takes the unsigned char* mem address and tells compile to pretend its a char*
  file.read(reinterpret_cast<char*>(img.data.data()), img.data.size());

  file.close();

  return img;
}

void writePPM(const std::string& filename, const Image& img) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Error: Could not open " << filename << " for writing." << std::endl;
    exit(1);
  }

  //write the strict PPM header
  file << "P6\n" << img.width << " " << img.height << "\n" << img.max_color << "\n";
  
  //write the raw binary pixel data
  file.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
  file.close();
}

//https://stackoverflow.com/questions/1696113/how-do-i-gaussian-blur-an-image-without-using-any-in-built-gaussian-functions
//always pass by ref to not copy the entire input, make it const to ensure nothing in the input is changed
Image applyGaussianBlur(const Image& input) {
  //invoke the copy constructor on the input to duplicate the dimensions and pre-allocate data
  Image output = input; 

  //perform a standard 3x3 Gaussian convolution matrix (kernel)
    //this is the 3x3 "glass square" that will move across the pixels and actually cause the changes - the blur
    //do integer math instead of using decimals, it is historically faster
    //the sum of these weights is 16
  const int kernel[3][3] = {
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1}
  };
  const int kernelWeight = 16;

  //start at 1 and end at width-1/height-1 to safely ignore the outermost edge pixels (if we started at 0, then the "glass square" would be hanging off the edge -> seg fault)
  //handling edges requires bounds-checking which slows down the baseline
  //the y is the current row, and the x is the current column
  for (int y = 1; y < input.height - 1; ++y) {
    for (int x = 1; x < input.width - 1; ++x) {
      
      //these are the "mixing buckets" for doing the calculations, and are set to 0 for each pixel
        //the pixels in the vector are unsigned char's -> they max out at 255
        //if a pure white pixel (255) is multiplied by 4 (1020), that cant be stored in an unsigned char -> overflow problem
        //making the buckets ints solves this issue
      int sumR = 0, sumG = 0, sumB = 0;

      //these inner loops represent the "glass square" -> for each pixel (x, y), we check the row above (-1), the current row (0), and the row below (+1); and the same for columns
      for (int ky = -1; ky <= 1; ++ky) {
        for (int kx = -1; kx <= 1; ++kx) {
          
          //RAM is one massive continuous line of bytes -> no way to store 2D grids
            //thus, the grid needs to be flattened into a 1D memory address
            //(y + ky) -> the row of the neighbor we are looking at
            //(x + kx) -> the column of the neighbor
            //Row * Width + Column -> the formula for flattening 2D memory; *3 because every pixel takes 3 bytes in Ram (R, G, B)
          int pixelIndex = ((y + ky) * input.width + (x + kx)) * 3;

          //need to look up the correct multiplier in the 3x3 array
            //note: arrays do not have negative indexes, loop uses -1, 0, and 1
              //adding 1 to them, can shift them to 0, 1, and 2 which matches the kernel[3][3] array
          int weight = kernel[ky + 1][kx + 1];

          //reach into 1D vector at pixelIndex calculated, grab Red byte, multiply it by glass weight, and dump into red bucket
            //same for blue and green
          sumR += input.data[pixelIndex] * weight;
          sumG += input.data[pixelIndex + 1] * weight;
          sumB += input.data[pixelIndex + 2] * weight;
        }
      }

      //inner loops are finished, buckets are full of mixed colors
        //need to figure out where to paint the new pixel on the output
        //use same flattening math, but just use the base (y, x) center coordinates
      int outputIndex = (y * input.width + x) * 3;
      output.data[outputIndex]     = sumR / kernelWeight;
      output.data[outputIndex + 1] = sumG / kernelWeight;
      output.data[outputIndex + 2] = sumB / kernelWeight;
    }
  }
  return output;
}

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
  Image img = readPPM(inputFile);
  std::cout << "Image loaded: " << img.width << " x " << img.height << " pixels." << std::endl;

  std::cout << "Applying Gaussian Blur..." << std::endl;

  auto start = std::chrono::high_resolution_clock::now();

  //to ensure that the image is truly blurred, going to run the algo 10 times
  Image blurredImg = img;
  for (int i = 0; i < 30; ++i) {
    blurredImg = applyGaussianBlur(blurredImg);
    std::cout << "Gaussian Blur applied " << i+1 << " times" << std::endl;
  }

  auto end = std::chrono::high_resolution_clock::now();

  //calculate the difference in ms
  std::chrono::duration<double, std::milli> duration = end - start;

  std::cout << "CPU compute time: " << duration.count() << " ms" << std::endl;

  std::cout << "Saving image..." << std::endl;
  writePPM(outputFile, blurredImg);
  std::cout << "Done!" << std::endl;

  return 0;
}