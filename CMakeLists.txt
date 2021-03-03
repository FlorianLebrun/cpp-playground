cmake_minimum_required(VERSION 3.16)

project(Playground
  LANGUAGES CXX CSharp
  VERSION 1.0.0
)

include(.tools/index.cmake)

#add_subdirectory(./BloomFilter)
add_subdirectory(./TestWithGoogleTest/GoogleTest)
add_subdirectory(./TestWithGoogleTest/Test)
#add_subdirectory(./Diff)
#add_subdirectory(./rtf2html)
#add_subdirectory(./CallStackAnalyzer)
#add_subdirectory(./MarkdownParser)
#add_subdirectory(./UrlParser)
#add_subdirectory(./SyncPattern)
#add_subdirectory(./HttpClient)
#add_subdirectory(./LinearOptimizer)
#add_subdirectory(./HttpServer_CS)
#add_subdirectory(./TypeinfoMapper)
#add_subdirectory(./StringFormat)
