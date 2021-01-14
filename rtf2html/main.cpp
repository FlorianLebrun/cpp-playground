
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
   bool onlybody=true;

   std::istream *p_file_in = new std::ifstream("D:/rtf/Immediate Annuity.2.rtf");
   std::ostream *p_file_out = 1?new std::ofstream("D:/rtf/conv.html"):&std::cout;   

   std::istream &file_in=*p_file_in;
   std::ostream &file_out=*p_file_out;
   file_in.exceptions(std::ios::failbit);
   file_out.exceptions(std::ios::failbit);
   std::istreambuf_iterator<char> f_buf_in(file_in.rdbuf());
   std::istreambuf_iterator<char> f_buf_in_end;
   std::string str_in;
   for (; f_buf_in != f_buf_in_end ; ++f_buf_in)
      str_in.push_back(*f_buf_in);

   //auto result = wConvertRtf2Html(str_in.c_str(), str_in.size());
   //auto result = wConvertRtf2InlineHtml(str_in.c_str(), str_in.size());
   auto result = wConvertRtf2Text(str_in.c_str(), str_in.size());
   file_out << result;
   std::cout << result;
   free(result);

   delete p_file_in;
   delete p_file_out;
   return 0;
   }
   catch (std::exception &e)
   {
      std::cerr<<"Error: "<<e.what()<<std::endl;
   }
   catch (...)
   {
      std::cerr<<"Something really bad happened!";
   }
}

