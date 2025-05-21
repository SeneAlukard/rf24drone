#ifndef PRINT_SET_HPP
#define PRINT_SET_HPP

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// This function captures output and returns it as a string
std::string captureOutput(const std::function<void()> &func) {
  // Redirect cout to our stringstream
  std::streambuf *oldCoutStreamBuf = std::cout.rdbuf();
  std::ostringstream strCout;
  std::cout.rdbuf(strCout.rdbuf());

  // Call the function
  func();

  // Restore old cout
  std::cout.rdbuf(oldCoutStreamBuf);

  return strCout.str();
}

// Helper to print outputs in a true columnar layout
void printSideBySide(const std::vector<std::string> &outputs) {
  // Parse each output into lines
  std::vector<std::vector<std::string>> outputBlocks;

  for (const auto &output : outputs) {
    std::vector<std::string> lines;
    std::istringstream stream(output);
    std::string line;

    while (std::getline(stream, line)) {
      lines.push_back(line);
    }

    outputBlocks.push_back(lines);
  }

  // Find max width for each column
  std::vector<size_t> columnWidths;
  for (const auto &block : outputBlocks) {
    size_t maxWidth = 0;
    for (const auto &line : block) {
      maxWidth = std::max(maxWidth, line.length());
    }
    columnWidths.push_back(maxWidth + 4); // Add spacing between columns
  }

  // Find max height (number of lines) for all blocks
  size_t maxLines = 0;
  for (const auto &block : outputBlocks) {
    maxLines = std::max(maxLines, block.size());
  }

  // Print line by line
  for (size_t line = 0; line < maxLines; line++) {
    for (size_t col = 0; col < outputBlocks.size(); col++) {
      // Get current line for current block (or empty if we've run out of lines)
      std::string currentLine = "";
      if (line < outputBlocks[col].size()) {
        currentLine = outputBlocks[col][line];
      }

      // Print the current line
      std::cout << currentLine;

      // Add padding up to the max width for this column
      if (col < outputBlocks.size() - 1) { // No padding needed for last column
        size_t padding = columnWidths[col] - currentLine.length();
        for (size_t i = 0; i < padding; i++) {
          std::cout << " ";
        }
      }
    }
    std::cout << std::endl;
  }
}

#endif // PRINT_SET_HPP
