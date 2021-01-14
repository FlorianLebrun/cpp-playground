
#include <cstdlib>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <string>

extern"C" __declspec(dllimport) char* wConvertRtf2Html(const char* buffer, int size);
extern"C" __declspec(dllimport) char* wConvertRtf2InlineHtml(const char* buffer, int size);
extern"C" __declspec(dllimport) char* wConvertRtf2Text(const char* buffer, int size);

int main()
{
   try
   {
      std::string inputFile = "Immediate Annuity.2.rtf";

      std::ifstream file_in(inputFile);
      std::string str_in(std::istreambuf_iterator<char>(file_in.rdbuf()), std::istreambuf_iterator<char>());

      char* result = 0;

      result = wConvertRtf2Html(str_in.c_str(), str_in.size());
      std::ofstream(inputFile + ".html") << result;
      free(result);

      result = wConvertRtf2InlineHtml(str_in.c_str(), str_in.size());
      std::ofstream(inputFile + ".inline.html") << result;
      free(result);

      result = wConvertRtf2Text(str_in.c_str(), str_in.size());
      std::ofstream(inputFile + ".txt") << result;
      std::cout << result;
      free(result);

      return 0;
   }
   catch (std::exception & e)
   {
      std::cerr << "Error: " << e.what() << std::endl;
   }
   catch (...)
   {
      std::cerr << "Something really bad happened!";
   }
}

