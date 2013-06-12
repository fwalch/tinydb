#include <iostream>

inline int usageMessage(const char* argv0, int possibilities = -1) {
  std::cerr << "Usage: " << argv0 << " [number of bracket pairs] [Dyck word number]" << std::endl;
  std::cerr << " > Number of bracket pairs must be an integer greater than 0" << std::endl;
  std::cerr << " > Dyck word number must be an integer in the range of [0, number of paths from (0,0) to (2*[number of bracket pairs], 0)[" << std::endl;

  if (possibilities != -1) {
    std::cerr << "   For the given value of bracket pairs, this is the interval [0," << possibilities << "[" << std::endl;
  }
  return 1;
}

inline double factorial(int n) {
  double result = 1.0;
  while (n > 1) {
    result *= n--;
  }
  return result;
}

inline int ballotNumber(double i, double j) {
  return (j+1)/(i+1)*factorial(i+1)/factorial(0.5*(i+j)+1)/factorial(0.5*(i-j));
}

inline int getNumberOfPossibilities(int i, int j, int n) {
  return ballotNumber(2*n-i, j);
}

int main(int argc, const char** argv) {
  if (argc != 3) {
    return usageMessage(argv[0]);
  }
  int numberOfBracketPairs = atoi(argv[1]);
  int wordNumber = atoi(argv[2]);

  if (numberOfBracketPairs < 1 || wordNumber < 0) {
    return usageMessage(argv[0]);
  }

  int possibilities = getNumberOfPossibilities(0, 0, numberOfBracketPairs);

  if (wordNumber >= possibilities) {
    return usageMessage(argv[0], possibilities);
  }

  int y = 1;
  for (int x = 1; x < 2*numberOfBracketPairs; x++) {
    // Walk upwards
    if (wordNumber < (possibilities = getNumberOfPossibilities(x, y, numberOfBracketPairs))) {
      std::cout << "(";
      y++;
    }
    // Walk downwards
    else {
      std::cout << ")";
      wordNumber -= possibilities;
      y--;
    }
  }
  std::cout << ")" << std::endl;

  return 0;
}
